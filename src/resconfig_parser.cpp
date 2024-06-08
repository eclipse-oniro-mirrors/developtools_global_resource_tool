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

#include "resconfig_parser.h"
#include <iostream>
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
ResConfigParser::ResConfigParser() : root_(nullptr) {}
ResConfigParser::~ResConfigParser()
{
    if (root_) {
        cJSON_Delete(root_);
    }
}
uint32_t ResConfigParser::Init(const string &filePath, HandleBack callback)
{
    if (!ResourceUtil::OpenJsonFile(filePath, &root_)) {
        return RESTOOL_ERROR;
    }
    if (!root_ || !cJSON_IsObject(root_)) {
        cerr << "Error: JSON file parsing failed, please check the JSON file." << NEW_LINE_PATH << filePath << endl;
        return RESTOOL_ERROR;
    }
    if (!callback) {
        return RESTOOL_SUCCESS;
    }

    InitFileListCommand(callback);

    for (cJSON *item = root_->child; item; item = item->next) {
        auto handler = fileListHandles_.find(item->string);
        if (handler == fileListHandles_.end()) {
            cout << "Warning: unsupport " << item->string << endl;
            continue;
        }
        if (handler->second(item) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }

    callback(Option::FORCEWRITE, "");
    return RESTOOL_SUCCESS;
}

// below private
void ResConfigParser::InitFileListCommand(HandleBack callback)
{
    using namespace placeholders;
    fileListHandles_.emplace("configPath", bind(&ResConfigParser::GetString, this, _1,
        Option::JSON, callback));
    fileListHandles_.emplace("packageName", bind(&ResConfigParser::GetString, this, _1,
        Option::PACKAGENAME, callback));
    fileListHandles_.emplace("output", bind(&ResConfigParser::GetString, this, _1,
        Option::OUTPUTPATH, callback));
    fileListHandles_.emplace("startId", bind(&ResConfigParser::GetString, this, _1,
        Option::STARTID, callback));
    fileListHandles_.emplace("entryCompiledResource", bind(&ResConfigParser::GetString, this, _1,
        Option::DEPENDENTRY, callback));
    fileListHandles_.emplace("ids", bind(&ResConfigParser::GetString, this, _1,
        Option::IDS, callback));
    fileListHandles_.emplace("definedIds", bind(&ResConfigParser::GetString, this, _1,
        Option::DEFINED_IDS, callback));
    fileListHandles_.emplace("applicationResource", bind(&ResConfigParser::GetString, this, _1,
        Option::INPUTPATH, callback));
    fileListHandles_.emplace("ResourceTable", bind(&ResConfigParser::GetArray, this, _1,
        Option::RESHEADER, callback));
    fileListHandles_.emplace("moduleResources", bind(&ResConfigParser::GetArray, this, _1,
        Option::INPUTPATH, callback));
    fileListHandles_.emplace("dependencies", bind(&ResConfigParser::GetArray, this, _1,
        Option::INPUTPATH, callback));
    fileListHandles_.emplace("moduleNames", bind(&ResConfigParser::GetModuleNames, this, _1,
        Option::MODULES, callback));
    fileListHandles_.emplace("iconCheck", bind(&ResConfigParser::GetBool, this, _1,
        Option::ICON_CHECK, callback));
    fileListHandles_.emplace("definedSysIds", bind(&ResConfigParser::GetString, this, _1,
        Option::DEFINED_SYSIDS, callback));
    fileListHandles_.emplace("compression", bind(&ResConfigParser::GetString, this, _1,
        Option::COMPRESSED_CONFIG, callback));
}

uint32_t ResConfigParser::GetString(const cJSON *node, int c, HandleBack callback)
{
    if (!node || !cJSON_IsString(node)) {
        cerr << "Error: GetString node not string. Option = " << c << endl;
        return RESTOOL_ERROR;
    }

    if (callback(c, node->valuestring) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResConfigParser::GetArray(const cJSON *node, int c, HandleBack callback)
{
    if (!node || !cJSON_IsArray(node)) {
        cerr << "Error: GetArray node not array. Option = " << c << endl;
        return RESTOOL_ERROR;
    }

    for (cJSON *item = node->child; item; item = item->next) {
        if (!cJSON_IsString(item)) {
            return RESTOOL_ERROR;
        }
        if (callback(c, item->valuestring) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResConfigParser::GetModuleNames(const cJSON *node, int c, HandleBack callback)
{
    if (!node) {
        cerr << "Error: GetModuleNames node is null. Option = " << c << endl;
        return RESTOOL_ERROR;
    }
    if (cJSON_IsString(node)) {
        return GetString(node, c, callback);
    }
    string moduleNames;
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

uint32_t ResConfigParser::GetBool(const cJSON *node, int c, HandleBack callback)
{
    if (!node || !cJSON_IsBool(node)) {
        cerr << "Error: GetBool node not bool. Option = " << c << endl;
        return RESTOOL_ERROR;
    }

    if (cJSON_IsTrue(node) == 1 && callback(c, "") != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}
