/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "resource_table.h"
#include "cmd_parser.h"
#include "file_entry.h"
#include "file_manager.h"
#include "resource_util.h"
#include "securec.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
ResourceTable::ResourceTable()
{
    auto &parser = CmdParser<PackageParser>::GetInstance();
    auto &packageParser = parser.GetCmdParser();
    indexFilePath_ = FileEntry::FilePath(packageParser.GetOutput()).Append(RESOURCE_INDEX_FILE).GetPath();
}

ResourceTable::~ResourceTable()
{
}

uint32_t ResourceTable::CreateResourceTable()
{
    FileManager &fileManager = FileManager::GetInstance();
    auto &allResource = fileManager.GetResources();
    map<string, vector<TableData>> configs;
    for (const auto &item : allResource) {
        for (const auto &resourceItem : item.second) {
            if (resourceItem.GetResType() == ResType::ID) {
                break;
            }
            TableData tableData;
            tableData.id = item.first;
            tableData.resourceItem = resourceItem;
            configs[resourceItem.GetLimitKey()].push_back(tableData);
        }
    }

    if (SaveToResouorceIndex(configs) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourceTable::CreateResourceTable(const map<int32_t, vector<shared_ptr<ResourceItem>>> &items)
{
    map<string, vector<TableData>> configs;
    for (const auto &item : items) {
        for (const auto &resourceItemPtr : item.second) {
            if (resourceItemPtr->GetResType() == ResType::ID) {
                break;
            }
            TableData tableData;
            tableData.id = item.first;
            tableData.resourceItem = *resourceItemPtr;
            configs[resourceItemPtr->GetLimitKey()].push_back(tableData);
        }
    }

    if (SaveToResouorceIndex(configs) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

// below private
uint32_t ResourceTable::SaveToResouorceIndex(const map<string, vector<TableData>> &configs) const
{
    uint32_t pos = 0;
    IndexHeader indexHeader;
    if (!InitIndexHeader(indexHeader, configs.size())) {
        return RESTOOL_ERROR;
    }
    pos += sizeof(IndexHeader);

    map<string, LimitKeyConfig> limitKeyConfigs;
    map<string, IdSet> idSets;
    if (!Prepare(configs, limitKeyConfigs, idSets, pos)) {
        return RESTOOL_ERROR;
    }

    ofstream out(indexFilePath_, ofstream::out | ofstream::binary);
    if (!out.is_open()) {
        cerr << "Error: open failed '" << indexFilePath_ << ", reason: " << strerror(errno) << endl;
        return RESTOOL_ERROR;
    }

    ostringstream outStreamData;
    if (!SaveRecordItem(configs, outStreamData, idSets, pos)) {
        return RESTOOL_ERROR;
    }

    ostringstream outStreamHeader;
    indexHeader.fileSize = pos;
    SaveHeader(indexHeader, outStreamHeader);
    SaveLimitKeyConfigs(limitKeyConfigs, outStreamHeader);
    SaveIdSets(idSets, outStreamHeader);
    out << outStreamHeader.str();
    out << outStreamData.str();
    return RESTOOL_SUCCESS;
}

bool ResourceTable::InitIndexHeader(IndexHeader &indexHeader, uint32_t count) const
{
    if (memcpy_s(indexHeader.version, VERSION_MAX_LEN, RESTOOL_VERSION, VERSION_MAX_LEN) != EOK) {
        cerr << "Error: InitIndexHeader memcpy_s fail." << endl;
        return false;
    }
    indexHeader.limitKeyConfigSize = count;
    return true;
}

bool ResourceTable::Prepare(const map<string, vector<TableData>> &configs,
                            map<string, LimitKeyConfig> &limitKeyConfigs,
                            map<string, IdSet> &idSets, uint32_t &pos) const
{
    for (const auto &config : configs) {
        LimitKeyConfig limitKeyConfig;
        const auto &keyParams = config.second.at(0).resourceItem.GetKeyParam();
        limitKeyConfig.keyCount = keyParams.size();
        pos += sizeof(limitKeyConfig.keyTag) + sizeof(limitKeyConfig.offset) + sizeof(limitKeyConfig.keyCount);
        for (const auto &keyParam : keyParams) {
            limitKeyConfig.data.push_back(static_cast<int32_t>(keyParam.keyType));
            limitKeyConfig.data.push_back(static_cast<int32_t>(keyParam.value));
            pos += sizeof(KeyParam);
        }
        limitKeyConfigs.emplace(config.first, limitKeyConfig);
    }

    for (const auto &config : configs) {
        auto limitKeyConfig = limitKeyConfigs.find(config.first);
        if (limitKeyConfig == limitKeyConfigs.end()) {
            cerr << "Error: limit key config don't find '" << config.first << "'" << endl;
            return false;
        }
        limitKeyConfig->second.offset = pos;
    
        IdSet idSet;
        idSet.idCount = config.second.size();
        pos += sizeof(idSet.idTag) + sizeof(idSet.idCount);
        for (const auto &tableData : config.second) {
            idSet.data.emplace(tableData.id, 0);
            pos += sizeof(uint32_t) + sizeof(uint32_t);
        }
        idSets.emplace(config.first, idSet);
    }
    return true;
}

bool ResourceTable::SaveRecordItem(const map<string, vector<TableData>> &configs,
                                   ostringstream &out, map<string, IdSet> &idSets, uint32_t &pos) const
{
    for (const auto &config : configs) {
        auto idSet = idSets.find(config.first);
        if (idSet == idSets.end()) {
            cerr << "Error: id set don't find '" << config.first << "'" << endl;
            return false;
        }

        for (const auto &tableData : config.second) {
            if (idSet->second.data.find(tableData.id) == idSet->second.data.end()) {
                cerr << "Error: resource table don't find id '" << tableData.id << "'" << endl;
                return false;
            }
            idSet->second.data[tableData.id] = pos;
            RecordItem recordItem;
            recordItem.id = tableData.id;
            recordItem.resType = static_cast<int32_t>(tableData.resourceItem.GetResType());
            vector<string> contents;
            string value(reinterpret_cast<const char *>(tableData.resourceItem.GetData()),
                tableData.resourceItem.GetDataLength());
            contents.push_back(value);
            string name = ResourceUtil::GetIdName(tableData.resourceItem.GetName(),
                tableData.resourceItem.GetResType());
            contents.push_back(name);
            string data = ResourceUtil::ComposeStrings(contents, true);
            recordItem.size = sizeof(RecordItem) + data.length() - sizeof(uint32_t);
            pos += recordItem.size + sizeof(uint32_t);

            out.write(reinterpret_cast<const char *>(&recordItem.size), sizeof(uint32_t));
            out.write(reinterpret_cast<const char *>(&recordItem.resType), sizeof(uint32_t));
            out.write(reinterpret_cast<const char *>(&recordItem.id), sizeof(uint32_t));
            out.write(reinterpret_cast<const char *>(data.c_str()), data.length());
        }
    }
    return true;
}

void ResourceTable::SaveHeader(const IndexHeader &indexHeader, ostringstream &out) const
{
    out.write(reinterpret_cast<const char *>(&indexHeader.version), VERSION_MAX_LEN);
    out.write(reinterpret_cast<const char *>(&indexHeader.fileSize), sizeof(uint32_t));
    out.write(reinterpret_cast<const char *>(&indexHeader.limitKeyConfigSize), sizeof(uint32_t));
}

void ResourceTable::SaveLimitKeyConfigs(const map<string, LimitKeyConfig> &limitKeyConfigs, ostringstream &out) const
{
    for (const auto &iter : limitKeyConfigs) {
        out.write(reinterpret_cast<const char *>(iter.second.keyTag), TAG_LEN);
        out.write(reinterpret_cast<const char *>(&iter.second.offset), sizeof(uint32_t));
        out.write(reinterpret_cast<const char *>(&iter.second.keyCount), sizeof(uint32_t));
        for (const auto &value : iter.second.data) {
            out.write(reinterpret_cast<const char *>(&value), sizeof(int32_t));
        }
    }
}

void ResourceTable::SaveIdSets(const map<string, IdSet> &idSets, ostringstream &out) const
{
    for (const auto &iter : idSets) {
        out.write(reinterpret_cast<const char *>(iter.second.idTag), TAG_LEN);
        out.write(reinterpret_cast<const char *>(&iter.second.idCount), sizeof(uint32_t));
        for (const auto &keyValue : iter.second.data) {
            out.write(reinterpret_cast<const char *>(&keyValue.first), sizeof(uint32_t));
            out.write(reinterpret_cast<const char *>(&keyValue.second), sizeof(uint32_t));
        }
    }
}
}
}
}
