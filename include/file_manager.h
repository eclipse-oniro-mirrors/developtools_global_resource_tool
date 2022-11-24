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

#ifndef OHOS_RESTOOL_FILE_MANAGER_H
#define OHOS_RESTOOL_FILE_MANAGER_H

#include<vector>
#include "resource_data.h"
#include "resource_item.h"
#include "increment_manager.h"
#include "singleton.h"

namespace OHOS {
namespace Global {
namespace Restool {
class FileManager : public Singleton<FileManager> {
public:
    uint32_t ScanModules(const std::vector<std::string> &inputs, const std::string &output);
    const std::map<int32_t, std::vector<ResourceItem>> &GetResources() const
    {
        return items_;
    };
    void SetModuleName(const std::string &moduleName)
    {
        moduleName_ = moduleName;
    };
    uint32_t ScanIncrement(const std::string &output);
    uint32_t MergeResourceItem(const std::map<int32_t, std::vector<ResourceItem>> &resourceInfos);

private:
    uint32_t ScanModule(const std::string &input, const std::string &output,
        std::map<ResType, std::vector<DirectoryInfo>> &resTypeOfDirs);
    uint32_t ParseReference(const std::string &output, const std::vector<std::string> &sxmlFolders);
    bool NeedParseReferenceInSolidXml(ResType resType) const;
    void FilterRefSolidXml(const std::string &output, std::vector<std::string> &outputPaths,
        const std::map<ResType, std::vector<DirectoryInfo>> &resTypeOfDirs) const;

    // id, resource items
    std::map<int32_t, std::vector<ResourceItem>> items_;
    std::string moduleName_;
};
}
}
}
#endif
