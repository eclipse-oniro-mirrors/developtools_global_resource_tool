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

#include "file_manager.h"
#include<algorithm>
#include<iostream>
#include "factory_resource_compiler.h"
#include "file_entry.h"
#include "key_parser.h"
#include "reference_parser.h"
#include "resource_directory.h"
#include "resource_util.h"
#include "restool_errors.h"

#include "resource_module.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
uint32_t FileManager::ScanModules(const vector<string> &inputs, const string &output)
{
    vector<string> sxmlFolders;
    for (auto input : inputs) {
        map<ResType, vector<DirectoryInfo>> resTypeOfDirs;
        if (ScanModule(input, output, resTypeOfDirs) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
        FilterRefSolidXml(output, sxmlFolders, resTypeOfDirs);
    }
    return ParseReference(output, sxmlFolders);
}

uint32_t FileManager::ScanIncrement(const string &output)
{
    auto &incrementManager = IncrementManager::GetInstance();
    items_ = incrementManager.GetResourceItems();
    vector<string> sxmlFolders;
    FilterRefSolidXml(output, sxmlFolders, incrementManager.GetScanDirs());
    return ParseReference(output, sxmlFolders);
}

uint32_t FileManager::MergeResourceItem(const map<int32_t, vector<ResourceItem>> &resourceInfos)
{
    return ResourceModule::MergeResourceItem(items_, resourceInfos);
}

// below private founction
uint32_t FileManager::ScanModule(const string &input, const string &output,
    map<ResType, vector<DirectoryInfo>> &resTypeOfDirs)
{
    ResourceModule resourceModule(input, output, moduleName_);
    if (resourceModule.ScanResource() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    resTypeOfDirs = resourceModule.GetScanDirectorys();
    MergeResourceItem(resourceModule.GetOwner());
    return RESTOOL_SUCCESS;
}

uint32_t FileManager::ParseReference(const string &output, const vector<string> &sxmlFolders)
{
    ReferenceParser referenceParser;
    if (referenceParser.ParseRefInSolidXml(sxmlFolders) != RESTOOL_SUCCESS ||
        referenceParser.ParseRefInElement(items_) != RESTOOL_SUCCESS ||
        referenceParser.ParseRefInProfile(output) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

bool FileManager::NeedParseReferenceInSolidXml(ResType resType) const
{
    if (resType == ResType::LAYOUT || resType == ResType::ANIMATION || resType == ResType::GRAPHIC) {
        return true;
    }
    return false;
}

void FileManager::FilterRefSolidXml(const string &output, vector<string> &outputPaths,
    const map<ResType, vector<DirectoryInfo>> &resTypeOfDirs) const
{
    for (const auto &iter : resTypeOfDirs) {
        if (!NeedParseReferenceInSolidXml(iter.first)) {
            continue;
        }
        for (const auto &resourceDir : iter.second) {
            string outputPath = FileEntry::FilePath(output).Append(RESOURCES_DIR).Append(resourceDir.limitKey)
                .Append(resourceDir.fileCluster).GetPath();
            if (find(outputPaths.begin(), outputPaths.end(), outputPath) == outputPaths.end()) {
                outputPaths.push_back(outputPath);
            }
        }
    }
}
}
}
}
