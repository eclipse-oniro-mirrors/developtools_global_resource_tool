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

#ifndef OHOS_RESTOOL_INCREMENT_INDEX_H
#define OHOS_RESTOOL_INCREMENT_INDEX_H

#include "resource_item.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
class IncrementIndex {
public:
    IncrementIndex(const std::string &indexPath, const std::vector<std::string> &folder);
    virtual ~IncrementIndex() {};
    bool Save(const std::map<int32_t, std::vector<ResourceItem>> &items) const;
    bool Load(std::map<int32_t, std::vector<ResourceItem>> &items) const;
    void SetSkipPaths(const std::vector<std::string> &skipPaths);

    static const std::string INDEX_FILE;
private:
    bool LoadIndex(const Json::Value &indexInfo, std::map<int32_t, std::vector<ResourceItem>> &items) const;
    bool ParseResourceItem(const Json::Value &item, const std::string &filePath, ResourceItem &resourceItem) const;
    bool GetResourceItemProp(const Json::Value &item, const std::string &key, std::string &value) const;
    bool PushResourceItem(const ResourceItem &resourceItem, int32_t id,
        std::map<int32_t, std::vector<ResourceItem>> &items) const;
    bool IsIgnore(const std::string &filePath) const;
    const std::string &indexPath_;
    const std::vector<std::string> &folder_;
    std::vector<std::string> skipPaths_;
};
}
}
}
#endif