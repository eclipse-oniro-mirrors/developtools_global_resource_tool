/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_RESTOOL_CMD_LIST_H
#define OHOS_RESTOOL_CMD_LIST_H

#include<functional>
#include<string>
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
class CmdList {
public:
    CmdList() {};
    virtual ~CmdList() {};
    using HandleBack = std::function<uint32_t(int c, const std::string &argValue)>;
    uint32_t Init(const std::string &filePath, HandleBack callback);
private:
    uint32_t GetString(const Json::Value &node, int c, HandleBack callback);
    uint32_t GetArray(const Json::Value &node, int c, HandleBack callback);
    uint32_t GetModuleNames(const Json::Value &node, HandleBack callback);
};
}
}
}
#endif