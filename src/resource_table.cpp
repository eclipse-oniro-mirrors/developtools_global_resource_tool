/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include <cJSON.h>
#include <cstdint>
#include "cmd/cmd_parser.h"
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
    auto &parser = CmdParser::GetInstance();
    auto &packageParser = parser.GetPackageParser();
    if (!packageParser.GetIdDefinedOutput().empty()) {
        idDefinedPath_ = FileEntry::FilePath(packageParser.GetIdDefinedOutput()).Append(ID_DEFINED_FILE).GetPath();
    }
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

    if (!idDefinedPath_.empty()) {
        if (CreateIdDefined(allResource) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourceTable::CreateResourceTable(const map<int64_t, vector<shared_ptr<ResourceItem>>> &items)
{
    map<string, vector<TableData>> configs;
    map<int64_t, vector<ResourceItem>> allResource;
    for (const auto &item : items) {
        vector<ResourceItem> resourceItems;
        for (const auto &resourceItemPtr : item.second) {
            if (resourceItemPtr->GetResType() == ResType::ID) {
                break;
            }
            TableData tableData;
            tableData.id = item.first;
            tableData.resourceItem = *resourceItemPtr;
            resourceItems.push_back(*resourceItemPtr);
            configs[resourceItemPtr->GetLimitKey()].push_back(tableData);
        }
        allResource.emplace(item.first, resourceItems);
    }

    if (SaveToResouorceIndex(configs) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (!idDefinedPath_.empty()) {
        if (CreateIdDefined(allResource) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourceTable::LoadResTable(const string path, map<int64_t, vector<ResourceItem>> &resInfos)
{
    ifstream in(path, ios::binary);
    if (!in.is_open()) {
        PrintError(GetError(ERR_CODE_OPEN_FILE_ERROR).FormatCause(path.c_str(), strerror(errno)));
        return RESTOOL_ERROR;
    }

    in.seekg(0, ios::end);
    int64_t length = in.tellg();
    if (length <= 0) {
        in.close();
        PrintError(GetError(ERR_CODE_READ_FILE_ERROR).FormatCause(path.c_str(), "file is empty"));
        return RESTOOL_ERROR;
    }
    in.seekg(0, ios::beg);
    uint32_t errorCode = LoadResTable(in, resInfos);
    in.close();
    return errorCode;
}

uint32_t ResourceTable::LoadResTable(basic_istream<char> &in, map<int64_t, vector<ResourceItem>> &resInfos)
{
    if (!in) {
        std::string msg = "file stream bad, state code: " + std::to_string(in.rdstate());
        PrintError(GetError(ERR_CODE_READ_FILE_ERROR).FormatCause(in.rdstate(), msg.c_str()));
        return RESTOOL_ERROR;
    }
    in.seekg(0, ios::end);
    int64_t length = in.tellg();
    in.seekg(0, ios::beg);
    uint64_t pos = 0;
    IndexHeader indexHeader;
    if (!ReadFileHeader(in, indexHeader, pos, static_cast<uint64_t>(length))) {
        return RESTOOL_ERROR;
    }

    map<int64_t, vector<KeyParam>> limitKeys;
    if (!ReadLimitKeys(in, limitKeys, indexHeader.limitKeyConfigSize, pos, static_cast<uint64_t>(length))) {
        return RESTOOL_ERROR;
    }

    map<int64_t, pair<int64_t, int64_t>> datas;
    if (!ReadIdTables(in, datas, indexHeader.limitKeyConfigSize, pos, static_cast<uint64_t>(length))) {
        return RESTOOL_ERROR;
    }

    while (in.tellg() < length) {
        RecordItem record;
        if (!ReadDataRecordPrepare(in, record, pos, static_cast<uint64_t>(length)) ||
            !ReadDataRecordStart(in, record, limitKeys, datas, resInfos)) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourceTable::CreateIdDefined(const map<int64_t, vector<ResourceItem>> &allResource) const
{
    cJSON *root = cJSON_CreateObject();
    cJSON *recordArray = cJSON_CreateArray();
    if (recordArray == nullptr) {
        PrintError(GetError(ERR_CODE_UNDEFINED_ERROR).FormatCause("failed to create cJSON object for record array")
            .SetPosition(idDefinedPath_));
        cJSON_Delete(root);
        return RESTOOL_ERROR;
    }
    cJSON_AddItemToObject(root, "record", recordArray);
    for (const auto &pairPtr : allResource) {
        if (pairPtr.second.empty()) {
            PrintError(GetError(ERR_CODE_UNDEFINED_ERROR).FormatCause("resource item vector is empty")
                .SetPosition(idDefinedPath_));
            cJSON_Delete(root);
            return RESTOOL_ERROR;
        }
        cJSON *jsonItem = cJSON_CreateObject();
        if (jsonItem == nullptr) {
            PrintError(GetError(ERR_CODE_UNDEFINED_ERROR).FormatCause("failed to create cJSON object for resource item")
                .SetPosition(idDefinedPath_));
            cJSON_Delete(root);
            return RESTOOL_ERROR;
        }
        ResourceItem item = pairPtr.second.front();
        ResType resType = item.GetResType();
        string type = ResourceUtil::ResTypeToString(resType);
        string name = item.GetName();
        int64_t id = pairPtr.first;
        if (type.empty()) {
            string errMsg = "name = ";
            errMsg.append(name);
            errMsg.append("invalid restype, type must in ").append(ResourceUtil::GetAllRestypeString());
            PrintError(GetError(ERR_CODE_UNDEFINED_ERROR).FormatCause(errMsg.c_str()).SetPosition(idDefinedPath_));
            cJSON_Delete(jsonItem);
            cJSON_Delete(root);
            return RESTOOL_ERROR;
        }
        cJSON_AddStringToObject(jsonItem, "type", type.c_str());
        cJSON_AddStringToObject(jsonItem, "name", ResourceUtil::GetIdName(name, resType).c_str());
        cJSON_AddStringToObject(jsonItem, "id", ResourceUtil::DecToHexStr(id).c_str());
        cJSON_AddItemToArray(recordArray, jsonItem);
    }
    if (!ResourceUtil::SaveToJsonFile(idDefinedPath_, root)) {
        cJSON_Delete(root);
        return RESTOOL_ERROR;
    }
    cJSON_Delete(root);
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
        PrintError(GetError(ERR_CODE_OPEN_FILE_ERROR).FormatCause(indexFilePath_.c_str(), strerror(errno)));
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
        PrintError(GetError(ERR_CODE_UNDEFINED_ERROR).FormatCause("memcpy error when init index header"));
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
            PrintError(GetError(ERR_CODE_INVALID_LIMIT_KEY).FormatCause(config.first.c_str()));
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
            PrintError(GetError(ERR_CODE_INVALID_LIMIT_KEY).FormatCause(config.first.c_str()));
            return false;
        }

        for (const auto &tableData : config.second) {
            if (idSet->second.data.find(tableData.id) == idSet->second.data.end()) {
                string resType = ResourceUtil::ResTypeToString(tableData.resourceItem.GetResType());
                string name = tableData.resourceItem.GetName();
                PrintError(GetError(ERR_CODE_RESOURCE_ID_NOT_DEFINED).FormatCause(name.c_str(), resType.c_str()));
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

bool ResourceTable::ReadFileHeader(basic_istream<char> &in, IndexHeader &indexHeader, uint64_t &pos, uint64_t length)
{
    pos += sizeof(indexHeader);
    if (pos > length) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("header length error"));
        return false;
    }
    in.read(reinterpret_cast<char *>(indexHeader.version), VERSION_MAX_LEN);
    in.read(reinterpret_cast<char *>(&indexHeader.fileSize), INT_TO_BYTES);
    in.read(reinterpret_cast<char *>(&indexHeader.limitKeyConfigSize), INT_TO_BYTES);
    return true;
}

bool ResourceTable::ReadLimitKeys(basic_istream<char> &in, map<int64_t, vector<KeyParam>> &limitKeys,
                                  uint32_t count, uint64_t &pos, uint64_t length)
{
    for (uint32_t i = 0; i< count; i++) {
        pos = pos + TAG_LEN + INT_TO_BYTES + INT_TO_BYTES;
        if (pos > length) {
            PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("KEYS length error"));
            return false;
        }
        LimitKeyConfig limitKey;
        in.read(reinterpret_cast<char *>(limitKey.keyTag), TAG_LEN);
        string keyTag(reinterpret_cast<const char *>(limitKey.keyTag), TAG_LEN);
        if (keyTag != "KEYS") {
            PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX)
                .FormatCause(string("invalid key tag = " + keyTag).c_str()));
            return false;
        }
        in.read(reinterpret_cast<char *>(&limitKey.offset), INT_TO_BYTES);
        in.read(reinterpret_cast<char *>(&limitKey.keyCount), INT_TO_BYTES);

        vector<KeyParam> keyParams;
        for (uint32_t j = 0; j < limitKey.keyCount; j++) {
            pos = pos + INT_TO_BYTES + INT_TO_BYTES;
            if (pos > length) {
                PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("keyParams length error"));
                return false;
            }
            KeyParam keyParam;
            in.read(reinterpret_cast<char *>(&keyParam.keyType), INT_TO_BYTES);
            in.read(reinterpret_cast<char *>(&keyParam.value), INT_TO_BYTES);
            keyParams.push_back(keyParam);
        }
        limitKeys[limitKey.offset] = keyParams;
    }
    return true;
}

bool ResourceTable::ReadIdTables(basic_istream<char> &in, std::map<int64_t, std::pair<int64_t, int64_t>> &datas,
                                 uint32_t count, uint64_t &pos, uint64_t length)
{
    for (uint32_t i = 0; i< count; i++) {
        pos = pos + TAG_LEN + INT_TO_BYTES;
        if (pos > length) {
            PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("IDSS length error"));
            return false;
        }
        IdSet idss;
        int64_t offset = in.tellg();
        in.read(reinterpret_cast<char *>(idss.idTag), TAG_LEN);
        string idTag(reinterpret_cast<const char *>(idss.idTag), TAG_LEN);
        if (idTag != "IDSS") {
            PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX)
                .FormatCause(string("invalid id tag = " + idTag).c_str()));
            return false;
        }
        in.read(reinterpret_cast<char *>(&idss.idCount), INT_TO_BYTES);

        for (uint32_t j = 0; j < idss.idCount; j++) {
            pos = pos + INT_TO_BYTES + INT_TO_BYTES;
            if (pos > length) {
                PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("id data length error"));
                return false;
            }
            IdData data;
            in.read(reinterpret_cast<char *>(&data.id), INT_TO_BYTES);
            in.read(reinterpret_cast<char *>(&data.dataOffset), INT_TO_BYTES);
            datas[data.dataOffset] = make_pair(data.id, offset);
        }
    }
    return true;
}

bool ResourceTable::ReadDataRecordPrepare(basic_istream<char> &in, RecordItem &record, uint64_t &pos, uint64_t length)
{
    pos = pos + INT_TO_BYTES;
    if (pos > length) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("data record length error"));
        return false;
    }
    in.read(reinterpret_cast<char *>(&record.size), INT_TO_BYTES);
    pos = pos + record.size;
    if (pos > length) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("record.size length error"));
        return false;
    }
    in.read(reinterpret_cast<char *>(&record.resType), INT_TO_BYTES);
    in.read(reinterpret_cast<char *>(&record.id), INT_TO_BYTES);
    return true;
}

bool ResourceTable::ReadDataRecordStart(basic_istream<char> &in, RecordItem &record,
                                        const map<int64_t, vector<KeyParam>> &limitKeys,
                                        const map<int64_t, pair<int64_t, int64_t>> &datas,
                                        map<int64_t, vector<ResourceItem>> &resInfos)
{
    int64_t offset = in.tellg();
    offset = offset - INT_TO_BYTES - INT_TO_BYTES - INT_TO_BYTES;
    uint16_t value_size = 0;
    in.read(reinterpret_cast<char *>(&value_size), sizeof(uint16_t));
    if (value_size + sizeof(uint16_t) > record.size) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("value size error"));
        return false;
    }
    int8_t values[value_size];
    in.read(reinterpret_cast<char *>(&values), value_size);

    uint16_t name_size = 0;
    in.read(reinterpret_cast<char *>(&name_size), sizeof(uint16_t));
    if (value_size + sizeof(uint16_t) + name_size + sizeof(uint16_t) > record.size) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("name size error"));
        return false;
    }
    int8_t name[name_size];
    in.read(reinterpret_cast<char *>(name), name_size);
    string filename(reinterpret_cast<char *>(name));

    auto idTableOffset = datas.find(offset);
    if (idTableOffset == datas.end()) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("invalid id offset"));
        return false;
    }

    if (idTableOffset->second.first != record.id) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("invalid id"));
        return false;
    }

    if (limitKeys.find(idTableOffset->second.second) == limitKeys.end()) {
        PrintError(GetError(ERR_CODE_INVALID_RESOURCE_INDEX).FormatCause("invalid limit key offset"));
        return false;
    }

    const vector<KeyParam> &keyparams = limitKeys.find(datas.find(offset)->second.second)->second;
    ResourceItem resourceitem(filename, keyparams, g_resTypeMap.find(record.resType)->second);
    resourceitem.SetLimitKey(ResourceUtil::PaserKeyParam(keyparams));
    resourceitem.SetData(values, value_size);
    resourceitem.MarkCoverable();
    resInfos[record.id].push_back(resourceitem);
    return true;
}

}
}
}
