/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RESTOOL_ID_WORKER_H
#define OHOS_RESTOOL_ID_WORKER_H

#include <vector>
#include "id_defined_parser.h"
#include "resource_data.h"
#include "resource_util.h"
#include "singleton.h"

namespace OHOS {
namespace Global {
namespace Restool {
class IdWorker : public Singleton<IdWorker> {
public:
    uint32_t Init(ResourceIdCluster &type, int32_t start = 0x01000000);
    int32_t GenerateId(ResType resType, const std::string &name);
    std::vector<ResourceId> GetHeaderId() const;
    int32_t GetId(ResType resType, const std::string &name) const;
    int32_t GetSystemId(ResType resType, const std::string &name) const;
    bool PushCache(ResType resType, const std::string &name, int32_t id);
    void PushDelId(int32_t id);

private:
    int32_t GenerateAppId(ResType resType, const std::string &name);
    int32_t GenerateSysId(ResType resType, const std::string &name);
    int32_t GetMaxId(int32_t startId) const;
    int32_t GetCurId();
    int32_t appId_;
    int32_t maxId_;
    ResourceIdCluster type_;
    std::map<std::pair<ResType, std::string>, int32_t> ids_;
    std::map<std::pair<ResType, std::string>, ResourceId> sysDefinedIds_;
    std::map<std::pair<ResType, std::string>, ResourceId> appDefinedIds_;
    std::vector<int32_t> delIds_;
    std::map<std::pair<ResType, std::string>, int32_t> cacheIds_;
};
}
}
}
#endif
