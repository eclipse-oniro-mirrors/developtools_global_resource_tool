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

#include "increment_manager.h"
#include<algorithm>
#include<iostream>
#include "file_entry.h"
#include "id_worker.h"
#include "key_parser.h"
#include "module_combine.h"
#include "resource_module_inc.h"
#include "restool_errors.h"
#include "xml_key_node.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const string IncrementManager::ID_JSON_FILE = "ids.json";
IncrementManager::~IncrementManager()
{
}

uint32_t IncrementManager::Init(const string &cachePath, const vector<std::string> &folder,
    const string &outputPath, const string &moduleName)
{
    cachePath_ = cachePath;
    folder_ = folder;
    outputPath_ = outputPath;
    moduleName_ = moduleName;

    if (!InitIdJson()) {
        return RESTOOL_ERROR;
    }

    if (!ClearSolidXml()) {
        return RESTOOL_ERROR;
    }

    vector<IncrementList::FileIncrement> dels;
    if (!InitList(dels)) {
        return RESTOOL_ERROR;
    }

    DeleteRawFile(dels);
    if (!ScanModules(dels)) {
        return RESTOOL_ERROR;
    }
    enalbe_= true;
    return RESTOOL_SUCCESS;
}

// below private
bool IncrementManager::InitIdJson()
{
    string idJsonPath = FileEntry::FilePath(cachePath_).Append(ID_JSON_FILE).GetPath();
    if (!ResourceUtil::FileExist(idJsonPath)) {
        return true;
    }
    if (!LoadIdJson()) {
        return false;
    }
    firstIncrement_ = false;
    return true;
}

bool IncrementManager::ScanModules(const vector<IncrementList::FileIncrement> &dels)
{
    vector<ModuleInfo> moduleInfos;
    for (const auto &folder : folder_) {
        ModuleInfo moduleInfo;
        moduleInfo.rootPath = folder;
        for_each(dels.begin(), dels.end(), [&moduleInfo](const auto &iter) {
            if (moduleInfo.rootPath != iter.rootPath) {
                return;
            }
            moduleInfo.fileIncrements.push_back(iter);
        });
        moduleInfos.push_back(moduleInfo);
    }

    for (auto &iter : moduleInfos) {
        string pathHash = ResourceUtil::GenerateHash(iter.rootPath);
        string moduleCachePath = FileEntry::FilePath(cachePath_).Append(pathHash).GetPath();
        ResourceModuleInc resourceModuleInc(iter.rootPath, moduleCachePath, moduleName_, folder_);
        if (FirstIncrement()) {
            if (resourceModuleInc.ResourceModule::ScanResource() != RESTOOL_SUCCESS) {
                return false;
            }
        } else {
            if (resourceModuleInc.ScanResource(iter.fileIncrements) != RESTOOL_SUCCESS) {
                return false;
            }
        }

        PushScanDir(resourceModuleInc.GetScanDirectorys());
        if (ResourceModule::MergeResourceItem(items_, resourceModuleInc.GetOwner()) != RESTOOL_SUCCESS) {
            return false;
        }
        if (resourceModuleInc.SaveIndex() != RESTOOL_SUCCESS) {
            return false;
        }

        ModuleCombine moduleCombine(moduleCachePath, outputPath_);
        if (!moduleCombine.Combine()) {
            return false;
        }
    }
    FlushId();
    SaveIdJson();
    return true;
}

bool IncrementManager::InitList(vector<IncrementList::FileIncrement> &dels) const
{
    string listPath = FileEntry::FilePath(cachePath_).Append(IncrementList::RESTOOL_LIST_FILE).GetPath();
    if (!ResourceUtil::FileExist(listPath)) {
        return true;
    }

    IncrementList incrementList(listPath, folder_);
    if (!incrementList.Parse(dels)) {
        return false;
    }
    return true;
}

void IncrementManager::FlushId()
{
    for_each(items_.begin(), items_.end(), [](const auto &iter) {
        auto &idWorker = IdWorker::GetInstance();
        ResType resType = iter.second.begin()->GetResType();
        string name = ResourceUtil::GetIdName(iter.second.begin()->GetName(), resType);
        idWorker.GenerateId(resType, name);
    });
}

bool IncrementManager::SaveIdJson() const
{
    Json::Value root;
    for (const auto &iter : items_) {
        Json::Value node;
        ResType resType = iter.second.begin()->GetResType();
        node["name"] = ResourceUtil::GetIdName(iter.second.begin()->GetName(), resType);
        node["type"] = ResourceUtil::ResTypeToString(resType);
        root[to_string(iter.first)] = node;
    }

    string idJsonPath = FileEntry::FilePath(cachePath_).Append(ID_JSON_FILE).GetPath();
    if (!ResourceUtil::SaveToJsonFile(idJsonPath, root)) {
        return false;
    }
    return true;
}

bool IncrementManager::LoadIdJson()
{
    Json::Value root;
    string idJsonPath = FileEntry::FilePath(cachePath_).Append(ID_JSON_FILE).GetPath();
    if (!ResourceUtil::OpenJsonFile(idJsonPath, root)) {
        return false;
    }

    if (!root.isObject()) {
        cerr << "Error: invalid, not object." << NEW_LINE_PATH << idJsonPath << endl;
        return false;
    }

    auto &idWorker = IdWorker::GetInstance();
    for (const auto &member : root.getMemberNames()) {
        int32_t id = strtol(member.c_str(), nullptr, 10);
        if (id < 0) {
            cerr << "Error: invalid '" << member << "'." << NEW_LINE_PATH << idJsonPath  << endl;
            return false;
        }

        const auto &node = root[member];
        if (!node.isObject()) {
            cerr << "Error: '" << member << "' not object." << NEW_LINE_PATH << idJsonPath  << endl;
            return false;
        }
        if (!node["name"].isString()) {
            cerr << "Error: '" << member << "' name not string." << NEW_LINE_PATH << idJsonPath << endl;
            return false;
        }
        string name = node["name"].asString();

        if (!node["type"].isString()) {
            cerr << "Error: '" << member << "' type not string." << NEW_LINE_PATH << idJsonPath << endl;
            return false;
        }
        ResType resType = ResourceUtil::GetResTypeFromString(node["type"].asString());
        if (resType == ResType::INVALID_RES_TYPE) {
            cerr << "Error: '" << member << "' '" << node["type"] << "' invalid.";
            cerr << NEW_LINE_PATH << idJsonPath << endl;
            return false;
        }
        if (!idWorker.PushCache(resType, name, id)) {
            return false;
        }
    }
    return true;
}

void IncrementManager::PushScanDir(const map<ResType, vector<DirectoryInfo>> &scanDirs)
{
    for (const auto &iter : scanDirs) {
        for (const auto &directoryInfo : iter.second) {
            scanDirs_[iter.first].push_back(directoryInfo);
        }
    }
}

void IncrementManager::DeleteRawFile(vector<IncrementList::FileIncrement> &dels) const
{
    for (auto it = dels.begin(); it != dels.end();) {
        if (it->dirType != ResType::INVALID_RES_TYPE) {
            it++;
            continue;
        }
        string rawFilePath = FileEntry::FilePath(outputPath_).Append(it->relativePath).GetPath();
        if (remove(rawFilePath.c_str()) != 0) {
            cout << "Warning: remove fail '" << rawFilePath << "'" << endl;
        }
        it = dels.erase(it);
    }
}

bool IncrementManager::ClearSolidXml() const
{
    string resourceDir = FileEntry::FilePath(outputPath_).Append(RESOURCES_DIR).GetPath();
    if (!ResourceUtil::FileExist(resourceDir)) {
        return true;
    }

    ResourceDirectory resourceDirectory;
    if (!resourceDirectory.ScanResources(resourceDir, [](const DirectoryInfo &info) {
        if (!ResourceUtil::NeedConverToSolidXml(info.dirType)) {
            return true;
        }

        FileEntry f(info.dirPath);
        if (!f.Init()) {
            return false;
        }

        for (const auto &entry : f.GetChilds()) {
            if (!entry->IsFile()) {
                cerr << "Error: '" << entry->GetFilePath().GetPath() << "' is directroy." << endl;
                return false;
            }

            string extension = entry->GetFilePath().GetExtension();
            if (extension != ".sxml" && extension != ".key" && extension != ".json") {
                continue;
            }

            if (remove(entry->GetFilePath().GetPath().c_str()) != 0) {
                cerr << "Error: remove failed '" << entry->GetFilePath().GetPath();
                cerr << "', reason: " << strerror(errno) << endl;
                return false;
            }
        }
        return true;
    })) {
        return false;
    }
    return true;
}
}
}
}
