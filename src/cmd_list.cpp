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

#include "cmd_list.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
uint32_t CmdList::Init(const string &filePath, function<uint32_t(int c, const string &argValue)> callback)
{
    Json::Value root;
    if (!ResourceUtil::OpenJsonFile(filePath, root)) {
        return RESTOOL_ERROR;
    }

    if (!callback) {
        return RESTOOL_SUCCESS;
    }

    if (GetString(root["configPath"], 'j', callback) != RESTOOL_SUCCESS ||
        GetString(root["packageName"], 'p', callback) != RESTOOL_SUCCESS ||
        GetString(root["output"], 'o', callback) != RESTOOL_SUCCESS ||
        GetString(root["startId"], 'e', callback) != RESTOOL_SUCCESS ||
        GetString(root["entryCompiledResource"], 'd', callback) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GetModuleNames(root["moduleNames"], callback) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GetArray(root["ResourceTable"], 'r', callback) != RESTOOL_SUCCESS ||
        GetString(root["applicationResource"], 'i', callback) != RESTOOL_SUCCESS ||
        GetArray(root["moduleResources"], 'i', callback) != RESTOOL_SUCCESS ||
        GetArray(root["dependencies"], 'i', callback) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    callback('f', "");
    return RESTOOL_SUCCESS;
}

// below private
uint32_t CmdList::GetString(const Json::Value &node, int c, HandleBack callback)
{
    if (!node.isString()) {
        return RESTOOL_SUCCESS;
    }

    if (callback(c, node.asString()) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t CmdList::GetArray(const Json::Value &node, int c, HandleBack callback)
{
    if (!node.isArray()) {
        return RESTOOL_SUCCESS;
    }

    for (Json::ArrayIndex i = 0; i < node.size(); i++) {
        if (!node[i].isString()) {
            return RESTOOL_ERROR;
        }
        if (callback(c, node[i].asString()) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t CmdList::GetModuleNames(const Json::Value &node, HandleBack callback)
{
    string moduleNames;
    if (node.isString()) {
        return GetString(node, 'm', callback);
    }
    if (GetArray(node, 'm', [&moduleNames](int c, const string &argValue) {
        if (!moduleNames.empty()) {
            moduleNames.append(",");
        }
        moduleNames.append(argValue);
        return RESTOOL_SUCCESS;
    }) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (!moduleNames.empty() && callback('m', moduleNames) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}
