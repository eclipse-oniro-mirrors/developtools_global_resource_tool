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

#ifndef OHOS_RESTOOL_REFERENCE_PARSER_H
#define OHOS_RESTOOL_REFERENCE_PARSER_H

#include "id_worker.h"
#include "resource_item.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ReferenceParser {
public:
    ReferenceParser();
    virtual ~ReferenceParser();
    uint32_t ParseRefInSolidXml(const std::vector<std::string> &solidXmlFolders) const;
    uint32_t ParseRefInElement(std::map<int32_t, std::vector<ResourceItem>> &items) const;
    uint32_t ParseRefInString(std::string &value, bool &update) const;
    uint32_t ParseRefInProfile(const std::string &output) const;
    uint32_t ParseRefInJson(const std::string &filePath) const;
    uint32_t ParseRefInJson(const std::string &from, const std::string &to) const;
    uint32_t ParseRefInResourceItem(ResourceItem &resourceItem) const;
private:
    bool ParseRefResourceItem(ResourceItem &resourceItem) const;
    bool ParseRefResourceItemData(const ResourceItem &resourceItem, std::string &data, bool &update) const;
    bool IsStringOfResourceItem(ResType resType) const;
    bool IsArrayOfResourceItem(ResType resType) const;
    bool IsNotElement(ResType resType) const;
    bool ParseRefString(std::string &key) const;
    bool ParseRefString(std::string &key, bool &update) const;
    bool ParseRefImpl(std::string &key, const std::map<std::string, ResType> &refs, bool isSystem) const;
    bool ParseRefJsonImpl(Json::Value &root, bool &needSave) const;
    const IdWorker &idWorker_;
    static const std::map<std::string, ResType> ID_REFS;
    static const std::map<std::string, ResType> ID_OHOS_REFS;
};
}
}
}
#endif