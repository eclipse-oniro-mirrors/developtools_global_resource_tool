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
uint32_t CmdList::Init(const string &filePath, HandleBack callback)
{
    Json::Value root;
    if (!ResourceUtil::OpenJsonFile(filePath, root)) {
        return RESTOOL_ERROR;
    }

    if (!callback) {
        return RESTOOL_SUCCESS;
    }

    InitFileListCommand(root, callback);

    for (auto iter : fileListHandles_) {
        if (iter() != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }

    callback(Option::FORCEWRITE, "");
    return RESTOOL_SUCCESS;
}

// below private
void CmdList::InitFileListCommand(Json::Value &root, HandleBack callback)
{
    using namespace placeholders;
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["configPath"], Option::JSON, callback));
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["packageName"], Option::PACKAGENAME, callback));
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["output"], Option::OUTPUTPATH, callback));
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["startId"], Option::STARTID, callback));
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["entryCompiledResource"],
        Option::DEPENDENTRY, callback));
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["ids"], Option::IDS, callback));
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["definedIds"], Option::DEFINED_IDS, callback));
    fileListHandles_.push_back(bind(&CmdList::GetString, this, root["applicationResource"],
        Option::INPUTPATH, callback));
    fileListHandles_.push_back(bind(&CmdList::GetArray, this, root["ResourceTable"], Option::RESHEADER, callback));
    fileListHandles_.push_back(bind(&CmdList::GetArray, this, root["moduleResources"], Option::INPUTPATH, callback));
    fileListHandles_.push_back(bind(&CmdList::GetArray, this, root["dependencies"], Option::INPUTPATH, callback));
    fileListHandles_.push_back(bind(&CmdList::GetModuleNames, this, root["moduleNames"], Option::MODULES, callback));
    fileListHandles_.push_back(bind(&CmdList::GetBool, this, root["iconCheck"], Option::ICON_CHECK, callback));
}

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

uint32_t CmdList::GetModuleNames(const Json::Value &node, int c, HandleBack callback)
{
    string moduleNames;
    if (node.isString()) {
        return GetString(node, c, callback);
    }
    if (GetArray(node, c, [&moduleNames](int c, const string &argValue) {
        if (!moduleNames.empty()) {
            moduleNames.append(",");
        }
        moduleNames.append(argValue);
        return RESTOOL_SUCCESS;
    }) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (!moduleNames.empty() && callback(c, moduleNames) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t CmdList::GetBool(const Json::Value &node, int c, HandleBack callback)
{
    if (node.type() != Json::booleanValue) {
        return RESTOOL_SUCCESS;
    }

    if (node.asBool() && callback(c, "") != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}
