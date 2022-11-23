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

#include "resource_module.h"
#include<algorithm>
#include<iostream>
#include "factory_resource_compiler.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const vector<ResType> ResourceModule::SCAN_SEQ = {
    ResType::ELEMENT,
    ResType::MEDIA,
    ResType::PROF,
    ResType::ANIMATION,
    ResType::GRAPHIC,
    ResType::LAYOUT,
};
ResourceModule::ResourceModule(const string &modulePath, const string &moduleOutput, const string &moduleName)
    : modulePath_(modulePath), moduleOutput_(moduleOutput), moduleName_(moduleName)
{
}

uint32_t ResourceModule::ScanResource()
{
    if (!ResourceUtil::FileExist(modulePath_)) {
        return RESTOOL_SUCCESS;
    }

    ResourceDirectory directory;
    if (!directory.ScanResources(modulePath_, [this](const DirectoryInfo &info) -> bool {
            scanDirs_[info.dirType].push_back(info);
            return true;
        })) {
        return RESTOOL_ERROR;
    }

    for (const auto &type : SCAN_SEQ) {
        const auto &item = scanDirs_.find(type);
        if (item == scanDirs_.end() || item->second.empty()) {
            continue;
        }

        unique_ptr<IResourceCompiler> resourceCompiler =
            FactoryResourceCompiler::CreateCompiler(type, moduleOutput_);
        resourceCompiler->SetModuleName(moduleName_);
        resourceCompiler->SetPreviewMode(previewMode_);
        if (resourceCompiler->Compile(item->second) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
        Push(resourceCompiler->GetResult());
    }
    return RESTOOL_SUCCESS;
}

const map<int32_t, vector<ResourceItem>> &ResourceModule::GetOwner() const
{
    return owner_;
}

const map<ResType, vector<DirectoryInfo>> &ResourceModule::GetScanDirectorys() const
{
    return scanDirs_;
}

uint32_t ResourceModule::MergeResourceItem(map<int32_t, vector<ResourceItem>> &alls,
    const map<int32_t, vector<ResourceItem>> &other, bool tipError)
{
    for (const auto &iter : other) {
        auto result = alls.emplace(iter.first, iter.second);
        if (result.second) {
            continue;
        }

        for (const auto &resourceItem : iter.second) {
            auto ret = find_if(result.first->second.begin(), result.first->second.end(), [&resourceItem](auto &iter) {
                return resourceItem.GetLimitKey() == iter.GetLimitKey();
            });
            if (ret == result.first->second.end()) {
                result.first->second.push_back(resourceItem);
                continue;
            }
            if (tipError) {
                cerr << "Error: '"<< resourceItem.GetName() <<"' conflict, first declared.";
                cerr << NEW_LINE_PATH << ret->GetFilePath() << endl;
                cerr << "but declared again." <<NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
                return RESTOOL_ERROR;
            }
            cout << "Warning: '"<< resourceItem.GetName() <<"' conflict, first declared.";
            cout << NEW_LINE_PATH << ret->GetFilePath() << endl;
            cout << "but declared again." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        }
    }
    return RESTOOL_SUCCESS;
}
// below private
void ResourceModule::Push(const map<int32_t, std::vector<ResourceItem>> &other)
{
    if (previewMode_) {
        return;
    }

    for (const auto &iter : other) {
        owner_.emplace(iter.first, iter.second);
    }
}
}
}
}
