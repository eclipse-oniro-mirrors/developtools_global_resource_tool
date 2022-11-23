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
#include<iostream>
#include "factory_resource_compiler.h"
#include "file_entry.h"
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
        string filePathDel = FileEntry::FilePath(moduleOutput_).Append(RESOURCES_DIR)
            .Append(fileIncrement.limitKey).Append(fileIncrement.fileCluster).Append(fileIncrement.filename).GetPath();
        if (ResourceUtil::NeedConverToSolidXml(fileIncrement.dirType) &&
            FileEntry::FilePath(filePathDel).GetExtension() == ".xml") {
            filePathDel = FileEntry::FilePath(filePathDel).ReplaceExtension(".sxml").GetPath();
        }
        if (remove(filePathDel.c_str()) != 0) {
            cerr << "Error: remove failed '" << filePathDel << "', reason: " << strerror(errno) << endl;
            return RESTOOL_ERROR;
        }
    }

    string indexPath = FileEntry::FilePath(moduleOutput_).Append(IncrementIndex::INDEX_FILE).GetPath();
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
    string indexPath = FileEntry::FilePath(moduleOutput_).Append(IncrementIndex::INDEX_FILE).GetPath();
    IncrementIndex moduleIndex(indexPath, folder_);
    if (!moduleIndex.Save(owner_)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}