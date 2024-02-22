/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RESTOOL_ID_DEFINED_PARSER_H
#define OHOS_RESTOOL_ID_DEFINED_PARSER_H

#include <string>
#include <cJSON.h>
#include "cmd_parser.h"

namespace OHOS {
namespace Global {
namespace Restool {
class IdDefinedParser {
public:
    IdDefinedParser(const PackageParser &packageParser, const ResourceIdCluster &type);
    virtual ~IdDefinedParser();
    std::map<std::pair<ResType, std::string>, ResourceId> &GetSysDefinedIds()
    {
        return sysDefinedIds_;
    }
    std::map<std::pair<ResType, std::string>, ResourceId> &GetAppDefinedIds()
    {
        return appDefinedIds_;
    }
    uint32_t Init();
private:
    uint32_t Init(const std::string &filePath, bool isSystem);
    using ParseFunction = std::function<bool(const cJSON *, ResourceId&)>;
    void InitParser();
    uint32_t IdDefinedToResourceIds(const cJSON *record, bool isSystem, const int32_t strtSysId = 0);
    bool ParseType(const cJSON *type, ResourceId &resourceId);
    bool ParseName(const cJSON *name, ResourceId &resourceId);
    bool ParseOrder(const cJSON *order, ResourceId &resourceId);
    bool ParseId(const cJSON *id, ResourceId &resourceId);
    bool PushResourceId(const ResourceId &resourceId, bool isSystem);
    int32_t GetStartId() const;
    std::map<std::pair<ResType, std::string>, ResourceId> sysDefinedIds_;
    std::map<std::pair<ResType, std::string>, ResourceId> appDefinedIds_;
    std::map<int32_t, ResourceId> idDefineds_;
    std::map<std::string, ParseFunction> handles_;
    const PackageParser packageParser_;
    static const int32_t START_SYS_ID;
    ResourceIdCluster type_;
    cJSON *root_;
};
}
}
}
#endif