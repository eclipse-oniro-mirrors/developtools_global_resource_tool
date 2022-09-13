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

#ifndef OHOS_RESTOOL_RESOURCE_COMPILER_H
#define OHOS_RESTOOL_RESOURCE_COMPILER_H

#include<vector>
#include "increment_manager.h"
#include "resource_data.h"
#include "resource_item.h"

namespace OHOS {
namespace Global {
namespace Restool {
class IResourceCompiler {
public:
    IResourceCompiler(ResType type, const std::string &output);
    virtual ~IResourceCompiler();
    uint32_t Compile(const std::vector<DirectoryInfo> &directoryInfos);
    const std::map<int32_t, std::vector<ResourceItem>> &GetResult() const;
    uint32_t Compile(const FileInfo &fileInfo);
    void SetModuleName(const std::string &moduleName);
    void SetPreviewMode(bool enable)
    {
        previewMode_ = enable;
    };
    uint32_t CompileForAppend(const FileInfo &fileInfo);
    const std::map<std::pair<ResType, std::string>, std::vector<ResourceItem>> &GetResourceItems() const;

protected:
    virtual uint32_t CompileSingleFile(const FileInfo &fileInfo);
    bool MergeResourceItem(const ResourceItem &resourceItem);
    bool IsXmlFile(const FileInfo &fileInfo) const;
    bool HasConvertedToSolidXml(const FileInfo &fileInfo) const;
    bool NeedIfConvertToSolidXml() const;
    std::string GetOutputFolder(const DirectoryInfo &directoryInfo) const;
    ResType type_;
    std::string output_;
    std::string moduleName_;
    bool previewMode_ = false;

    // id, resource items
    std::map<int32_t, std::vector<ResourceItem>> resourceInfos_;
    // ResType, name, resourceItems
    std::map<std::pair<ResType, std::string>, std::vector<ResourceItem>> nameInfos_;

private:
    uint32_t ConvertToSolidXml(const std::map<std::string, std::vector<FileInfo>> &setsByDirectory);
    uint32_t PostCommit();
    void ListXmlFile(const std::vector<FileInfo> &fileInfo, std::vector<std::string> &xmlPaths) const;
};
}
}
}
#endif