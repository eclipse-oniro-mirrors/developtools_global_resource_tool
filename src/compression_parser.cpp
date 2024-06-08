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

#include "compression_parser.h"
#include <iostream>
#include <mutex>
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
static shared_ptr<CompressionParser> compressionParseMgr = nullptr;
static once_flag compressionParserMgrFlag;
 
CompressionParser::CompressionParser()
    : filePath_(""), extensionPath_(""), mediaSwitch_(false), root_(nullptr)
{
}

CompressionParser::CompressionParser(const string &filePath)
    : filePath_(filePath), extensionPath_(""), mediaSwitch_(false), root_(nullptr)
{
}

CompressionParser::~CompressionParser()
{
    if (root_) {
        cJSON_Delete(root_);
    }
}

shared_ptr<CompressionParser> CompressionParser::GetCompressionParser(const string &filePath)
{
    call_once(compressionParserMgrFlag, [&] {
        compressionParseMgr = make_shared<CompressionParser>(filePath);
    });
    return compressionParseMgr;
}

shared_ptr<CompressionParser> CompressionParser::GetCompressionParser()
{
    if (!compressionParseMgr) {
        compressionParseMgr = make_shared<CompressionParser>();
    }
    return compressionParseMgr;
}

uint32_t CompressionParser::Init()
{
    if (!ResourceUtil::OpenJsonFile(filePath_, &root_)) {
        return RESTOOL_ERROR;
    }
    if (!root_ || !cJSON_IsObject(root_)) {
        cerr << "Error: JSON file parsing failed, please check the JSON file." << NEW_LINE_PATH << filePath_ << endl;
        return RESTOOL_ERROR;
    }
    cJSON *compressionNode = cJSON_GetObjectItem(root_, "compression");
    if (!ParseCompression(compressionNode)) {
        return RESTOOL_ERROR;
    }
    if (!mediaSwitch_) {
        return RESTOOL_SUCCESS;
    }
    cJSON *contextNode = cJSON_GetObjectItem(root_, "context");
    cJSON *filterNode = cJSON_GetObjectItem(compressionNode, "filter");
    if (!ParseContext(contextNode) || !ParseFilter(filterNode)) {
        cerr << NEW_LINE_PATH << filePath_ << endl;
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

bool CompressionParser::ParseCompression(const cJSON *compressionNode)
{
    if (!compressionNode) {
        cerr << "Warning: get 'compression' node is empty, the compiled images are not transcoded.";
        cerr << NEW_LINE_PATH << filePath_ << endl;
        return true;
    }
    if (!cJSON_IsObject(compressionNode)) {
        cerr << "Error: 'compression' must be object." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    cJSON *mediaNode = cJSON_GetObjectItem(compressionNode, "media");
    if (!mediaNode) {
        cerr << "Warning: get 'media' node is empty, the compiled images are not transcoded.";
        cerr << NEW_LINE_PATH << filePath_ << endl;
        return true;
    }
    if (!cJSON_IsObject(mediaNode)) {
        cerr << "Error: 'media' must be object." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    cJSON *enableNode = cJSON_GetObjectItem(mediaNode, "enable");
    if (!enableNode) {
        cerr << "Warning: get 'enable' node is empty, the compiled images are not transcoded.";
        cerr << NEW_LINE_PATH << filePath_ << endl;
        return true;
    }
    if (!cJSON_IsBool(enableNode)) {
        cerr << "Error: 'enable' must be bool." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    mediaSwitch_ = cJSON_IsTrue(enableNode);
    return true;
}

bool CompressionParser::ParseContext(const cJSON *contextNode)
{
    if (!contextNode) {
        cerr << "Error: if image transcoding is supported, the 'context' node cannot be empty.";
        return false;
    }
    if (!cJSON_IsObject(contextNode)) {
        cerr << "Error: 'context' must be object.";
        return false;
    }
    cJSON *extensionPathNode = cJSON_GetObjectItem(contextNode, "extensionPath");
    if (!extensionPathNode) {
        cerr << "Error: if image transcoding is supported, the 'extensionPath' node cannot be empty.";
        return false;
    }
    if (!cJSON_IsString(extensionPathNode)) {
        cerr << "Error: 'extensionPath' must be string.";
        return false;
    }
    extensionPath_ = extensionPathNode->valuestring;
    if (extensionPath_.empty()) {
        cerr << "Error: 'extensionPath' value cannot be empty.";
        return false;
    }
    return true;
}

bool CompressionParser::ParseFilter(const cJSON *filterNode)
{
    if (!filterNode) {
        cerr << "Error: if image transcoding is supported, the 'filter' node cannot be empty.";
        return false;
    }
    if (!cJSON_IsArray(filterNode)) {
        cerr << "Error: 'filter' must be array.";
        return false;
    }
    if (cJSON_GetArraySize(filterNode) == 0) {
        cerr << "Error: 'filter' value cannot be empty.";
        return false;
    }
    for (cJSON *item = filterNode->child; item; item = item->next) {
        if (!cJSON_IsObject(item)) {
            cerr << "Error: 'filter' value type must be object.";
            return false;
        }
        cJSON *methodNode = cJSON_GetObjectItem(item, "method");
        if (!methodNode) {
            cerr << "Error: if image transcoding is supported, the 'method' node cannot be empty.";
            return false;
        }
        if (!cJSON_IsObject(methodNode)) {
            cerr << "Error: 'method' must be object.";
            return false;
        }
    }
    return true;
}
}
}
}
