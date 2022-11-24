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

#ifndef OHOS_RESTOOL_RESOURCE_PACK_H
#define OHOS_RESTOOL_RESOURCE_PACK_H

#include "cmd_parser.h"
#include "config_parser.h"
#include "resource_util.h"
#include "resource_item.h"
#include "resource_data.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ResourcePack {
public:
    explicit ResourcePack(const PackageParser &packageParser);
    virtual ~ResourcePack() = default;
    uint32_t Package();

private:
    uint32_t Init();
    uint32_t InitModule();
    void InitHeaderCreater();
    uint32_t InitOutput() const;
    uint32_t GenerateHeader() const;
    uint32_t InitConfigJson();
    uint32_t GenerateTextHeader(const std::string &headerPath) const;
    uint32_t GenerateCplusHeader(const std::string &headerPath) const;
    uint32_t GenerateJsHeader(const std::string &headerPath) const;
    uint32_t CopyRawFile(const std::vector<std::string> &inputs) const;
    uint32_t CopyRawFileImpl(const std::string &src, const std::string &dst) const;
    uint32_t GenerateConfigJson();
    uint32_t ScanResources(const std::vector<std::string> &inputs, const std::string &output);
    uint32_t PackNormal();
    uint32_t PackPreview();
    uint32_t PackAppend();
    uint32_t PackCombine();
    uint32_t HandleFeature();
    uint32_t FindResourceItems(const std::map<int32_t, std::vector<ResourceItem>> &resInfoLocal,
                               std::vector<ResourceItem> &items, int32_t id) const;
    uint32_t HandleLabel(std::vector<ResourceItem> &items, ConfigParser &config) const;
    uint32_t HandleIcon(std::vector<ResourceItem> &items, ConfigParser &config) const;
    void SaveResourceItem(const ResourceItem &resourceItem, int32_t nextId) const;
    bool CopyIcon(std::string &dataPath, const std::string &idName, std::string &fileName) const;
    PackageParser packageParser_;
    std::string moduleName_;
    using HeaderCreater = std::function<uint32_t(const std::string&)>;
    std::map<std::string, HeaderCreater> headerCreaters_;
    ConfigParser configJson_;
};
}
}
}
#endif