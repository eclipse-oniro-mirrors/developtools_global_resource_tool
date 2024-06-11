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

#include <algorithm>
#include <iostream>
#include <mutex>
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
static shared_ptr<CompressionParser> compressionParseMgr = nullptr;
static once_flag compressionParserMgrFlag;

constexpr int OPT_TYPE_ONE = 1;
constexpr int OPT_TYPE_TWO = 2;
constexpr int OPT_TYPE_THREE = 3;
constexpr int OPT_SIZE_ONE = 1;
constexpr int OPT_SIZE_TWO = 2;
const map<TranscodeError, string> ERRORCODEMAP = {
    { TranscodeError::SUCCESS, "SUCCESS" },
    { TranscodeError::IMAGE_RESOLUTION_NOT_MATCH, "IMAGE_RESOLUTION_NOT_MATCH" },
    { TranscodeError::ANIMATED_IMAGE_SKIP, "ANIMATED_IMAGE_SKIP" },
    { TranscodeError::MALLOC_FAILED, "MALLOC_FAILED" },
    { TranscodeError::ENCODE_ASTC_FAILED, "ENCODE_ASTC_FAILED" },
    { TranscodeError::SUPER_COMPRESS_FAILED, "SUPER_COMPRESS_FAILED" },
    { TranscodeError::LOAD_COMPRESS_FAILED, "LOAD_COMPRESS_FAILED" },
};

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
#ifdef __WIN32
    if (handle_) {
        FreeLibrary(handle_);
        handle_ = nullptr;
    }
#else
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
    }
#endif
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
    cJSON *filtersNode = cJSON_GetObjectItem(compressionNode, "filters");
    if (!ParseContext(contextNode) || !ParseFilters(filtersNode)) {
        cerr << NEW_LINE_PATH << filePath_ << endl;
        return RESTOOL_ERROR;
    }
    if (!LoadImageTranscoder()) {
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

bool CompressionParser::ParseFilters(const cJSON *filtersNode)
{
    if (!filtersNode) {
        cerr << "Error: if image transcoding is supported, the 'filters' node cannot be empty.";
        return false;
    }
    if (!cJSON_IsArray(filtersNode)) {
        cerr << "Error: 'filters' must be array.";
        return false;
    }
    if (cJSON_GetArraySize(filtersNode) == 0) {
        cerr << "Error: 'filters' value cannot be empty.";
        return false;
    }
    for (cJSON *item = filtersNode->child; item; item = item->next) {
        if (!cJSON_IsObject(item)) {
            cerr << "Error: 'filters' value type must be object.";
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
        shared_ptr<CompressFilter> compressFilter = make_shared<CompressFilter>();
        compressFilter->method = "\"method\":" + ParseJsonStr(methodNode);
        cJSON *pathNode = cJSON_GetObjectItem(item, "path");
        compressFilter->path = ParsePath(pathNode);
        cJSON *excludePathNode = cJSON_GetObjectItem(item, "exclude_path");
        compressFilter->excludePath = ParsePath(excludePathNode);
        cJSON *rulesNode = cJSON_GetObjectItem(item, "rules_orignal");
        compressFilter->rules = ParseRules(rulesNode);
        cJSON *expandRulesNode = cJSON_GetObjectItem(item, "rules_union");
        compressFilter->expandRules = ParseRules(expandRulesNode);
        compressFilters_.emplace_back(compressFilter);
    }
    return true;
}

vector<string> CompressionParser::ParseRules(const cJSON *rulesNode)
{
    vector<string> res;
    if (!rulesNode || !cJSON_IsObject(rulesNode)) {
        cerr << "Error: ParseRules fail.";
        return res;
    }
    for (cJSON *item = rulesNode->child; item; item = item->next) {
        if (!item || !cJSON_IsArray(item)) {
            continue;
        }
        string name(item->string);
        res.emplace_back("\"" + name + "\":" + ParseJsonStr(item));
    }
    return res;
}

vector<string> CompressionParser::ParsePath(const cJSON *pathNode)
{
    vector<string> res;
    if (!pathNode || !cJSON_IsArray(pathNode)) {
        cerr << "Error: ParsePath fail.";
        return res;
    }
    for (cJSON *item = pathNode->child; item; item = item->next) {
        if (!item || !cJSON_IsString(item)) {
            continue;
        }
        res.emplace_back(item->valuestring);
    }
    return res;
}

string CompressionParser::ParseJsonStr(const cJSON *node)
{
    if (!node) {
        return "";
    }
    char *jsonString = cJSON_Print(node);
    string res(jsonString);
    free(jsonString);
    return res;
}

bool CompressionParser::LoadImageTranscoder()
{
#ifdef __WIN32
    if (!handle_) {
        handle_ = LoadLibrary(TEXT(extensionPath_.c_str()));
        if (!handle_) {
            cerr << "Error: open '" << extensionPath_.c_str() << "' fail." << endl;
            cerr << "Error: LoadLibrary failed with error: " << GetLastError() << endl;
            return false;
        }
    }
#else
    if (!handle_) {
        handle_ = dlopen(extensionPath_.c_str(), RTLD_LAZY);
        if (!handle_) {
            cerr << "Error: open '" << extensionPath_.c_str() << "' fail." << endl;
            cerr << "Error: dlopen failed with error: " << dlerror() << endl;
            return false;
        }
    }
#endif
    return true;
}

bool CompressionParser::SetTranscodeOptions(const string &optionJson)
{
    if (!handle_) {
        cerr << "Error: handle_ is nullptr."<< endl;
        return false;
    }
#ifdef __WIN32
    ISetTranscodeOptions iSetTranscodeOptions = (ISetTranscodeOptions)GetProcAddress(handle_, "SetTranscodeOptions");
#else
    ISetTranscodeOptions iSetTranscodeOptions = (ISetTranscodeOptions)dlsym(handle_, "SetTranscodeOptions");
#endif
    if (!iSetTranscodeOptions) {
        cerr << "Error: Failed to get the 'SetTranscodeOptions'."<< endl;
        return false;
    }
    bool ret = (*iSetTranscodeOptions)(optionJson);
    if (!ret) {
        cerr << "Error: SetTranscodeOptions failed."<< endl;
        return false;
    }
    return true;
}

TranscodeError CompressionParser::TranscodeImages(const string &imagePath, string &outputPath, TranscodeResult &result)
{
    if (!handle_) {
        cerr << "Error: handle_ is nullptr."<< endl;
        return TranscodeError::LOAD_COMPRESS_FAILED;
    }
#ifdef __WIN32
    ITranscodeImages iTranscodeImages = (ITranscodeImages)GetProcAddress(handle_, "Transcode");
#else
    ITranscodeImages iTranscodeImages = (ITranscodeImages)dlsym(handle_, "Transcode");
#endif
    if (!iTranscodeImages) {
        cerr << "Error: Failed to get the 'Transcode'."<< endl;
        return TranscodeError::LOAD_COMPRESS_FAILED;
    }
    TranscodeError ret = (*iTranscodeImages)(imagePath, outputPath, result);
    if (ret != TranscodeError::SUCCESS) {
        auto iter = ERRORCODEMAP.find(ret);
        if (iter != ERRORCODEMAP.end()) {
            cerr << "Error: TranscodeImages failed, error message: " << iter->second << endl;
        } else {
            cerr << "Error: TranscodeImages failed." << endl;
        }
        return ret;
    }
    return TranscodeError::SUCCESS;
}

bool CompressionParser::CheckPath(const string &src, const vector<string> &paths)
{
    return any_of(paths.begin(), paths.end(), [src](const auto &iter) {
        return iter == src;
    });
}

bool CompressionParser::IsInPath(const string &src, const shared_ptr<CompressFilter> &compressFilter)
{
    return CheckPath(src, compressFilter->path);
}

bool CompressionParser::IsInExcludePath(const string &src, const shared_ptr<CompressFilter> &compressFilter)
{
    return CheckPath(src, compressFilter->excludePath);
}
string CompressionParser::GetOptionsString(const shared_ptr<CompressFilter> &compressFilter, int type)
{
    if (compressFilter->rules.size() == 0 || compressFilter->expandRules.size() == 0) {
        return "{" + compressFilter->method + "}";
    }
    string res = "{" + compressFilter->method + "}";
    switch (type) {
        case OPT_TYPE_ONE:
            if (compressFilter->rules.size() == OPT_SIZE_ONE) {
                res.append(compressFilter->rules[0]).append("}");
            } else {
                res.append(compressFilter->rules[0]).append(",").append(compressFilter->rules[1]).append("}");
            }
            break;
        case OPT_TYPE_TWO:
            if (compressFilter->rules.size() == OPT_SIZE_ONE) {
                res.append(compressFilter->rules[0]).append("}");
            } else if (compressFilter->expandRules.size() == OPT_SIZE_TWO) {
                res.append(compressFilter->rules[0]).append(",").append(compressFilter->expandRules[1]).append("}");
            }
            break;
        case OPT_TYPE_THREE:
            if (compressFilter->expandRules.size() == OPT_SIZE_ONE) {
                res.append(compressFilter->expandRules[0]).append("}");
            } else if (compressFilter->rules.size() == OPT_SIZE_TWO) {
                res.append(compressFilter->expandRules[0]).append(",").append(compressFilter->rules[1]).append("}");
            }
            break;
        default:
            break;
    }
    return res;
}

bool CompressionParser::CopyAndTranscode(const string &src, string &dst)
{
    if (!mediaSwitch_) {
        return ResourceUtil::CopyFileInner(src, dst);
    }

    auto index = dst.find_last_of(SEPARATOR_FILE);
    if (index == string::npos) {
        cerr << "Error: invalid output path." << NEW_LINE_PATH << dst << endl;
        return false;
    }
    string output = dst.substr(0, index);
    TranscodeResult result = {0, 0, 0, 0, 0};
    for (const auto &compressFilter : compressFilters_) {
        if (!IsInPath(src, compressFilter)) {
            continue;
        }
        if (IsInExcludePath(src, compressFilter)) {
            if (!SetTranscodeOptions(GetOptionsString(compressFilter, OPT_TYPE_ONE)) ||
                TranscodeImages(src, output, result) != TranscodeError::SUCCESS) {
                continue;
            }
            dst = output;
            return true;
        } else {
            if ((!SetTranscodeOptions(GetOptionsString(compressFilter, OPT_TYPE_THREE)) ||
                TranscodeImages(src, output, result) != TranscodeError::SUCCESS) &&
                (!SetTranscodeOptions(GetOptionsString(compressFilter, OPT_TYPE_TWO)) ||
                TranscodeImages(src, output, result) != TranscodeError::SUCCESS)) {
                continue;
            }
            dst = output;
            return true;
        }
    }
    return ResourceUtil::CopyFileInner(src, dst);
}
}
}
}
