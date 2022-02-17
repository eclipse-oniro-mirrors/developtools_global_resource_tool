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

#include "resource_module_inc.h"
#include<filesystem>
#include<iostream>
#include "factory_resource_compiler.h"
#include "increment_index.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
ResourceModuleInc::ResourceModuleInc(const string &modulePath, const string &moduleOutput,
    const string &moduleName, const vector<string> &folder)
    : ResourceModule(modulePath, moduleOutput, moduleName), folder_(folder)
{
}

uint32_t ResourceModuleInc::ScanResource(const vector<IncrementList::FileIncrement> &fileIncrements)
{
    vector<string> skips;
    for (const auto &fileIncrement : fileIncrements) {
        skips.push_back(fileIncrement.filePath);
        if (fileIncrement.dirType == ResType::ELEMENT) {
            continue;
        }
        string filePathDel = filesystem::path(moduleOutput_).append(RESOURCES_DIR)
            .append(fileIncrement.limitKey).append(fileIncrement.fileCluster).append(fileIncrement.filename).string();
        if (ResourceUtil::NeedConverToSolidXml(fileIncrement.dirType) &&
            filesystem::path(filePathDel).extension().string() == ".xml") {
            filePathDel = filesystem::path(filePathDel).replace_extension(".sxml").string();
        }
        if (!filesystem::remove(filePathDel)) {
            cerr << "Error: delete '" << filePathDel << "' fail" << endl;
            return RESTOOL_ERROR;
        }
    }

    string indexPath = filesystem::path(moduleOutput_).append(IncrementIndex::INDEX_FILE).string();
    IncrementIndex moduleIndex(indexPath, folder_);
    moduleIndex.SetSkipPaths(skips);
    if (!moduleIndex.Load(owner_)) {
        return RESTOOL_ERROR;
    }

    for (const auto &fileIncrement : fileIncrements) {
        unique_ptr<IResourceCompiler> resourceCompiler =
            FactoryResourceCompiler::CreateCompiler(fileIncrement.dirType, moduleOutput_);
        resourceCompiler->SetModuleName(moduleName_);
        if (resourceCompiler->Compile(fileIncrement) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }

        if (MergeResourceItem(owner_, resourceCompiler->GetResult(), true) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourceModuleInc::SaveIndex() const
{
    string indexPath = filesystem::path(moduleOutput_).append(IncrementIndex::INDEX_FILE).string();
    IncrementIndex moduleIndex(indexPath, folder_);
    if (!moduleIndex.Save(owner_)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}