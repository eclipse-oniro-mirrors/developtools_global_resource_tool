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
#include <algorithm>
#include <iostream>
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
    for (auto input : inputs) {
        map<ResType, vector<DirectoryInfo>> resTypeOfDirs;
        if (ScanModule(input, output, resTypeOfDirs) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return ParseReference(output);
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

uint32_t FileManager::ParseReference(const string &output)
{
    ReferenceParser referenceParser;
    if (referenceParser.ParseRefInResources(items_, output) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}
