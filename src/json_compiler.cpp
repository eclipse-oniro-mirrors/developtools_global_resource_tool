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

#include "json_compiler.h"
#include <iostream>
#include <limits>
#include <regex>
#include "restool_errors.h"
#include "translatable_parser.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const string TAG_NAME = "name";
const string TAG_VALUE = "value";
const string TAG_PARENT = "parent";
const string TAG_QUANTITY = "quantity";
const vector<string> QUANTITY_ATTRS = { "zero", "one", "two", "few", "many", "other" };
const vector<string> TRANSLATION_TYPE = { "string", "strarray", "plural" };

JsonCompiler::JsonCompiler(ResType type, const string &output)
    : IResourceCompiler(type, output), isBaseString_(false), root_(nullptr)
{
    InitParser();
}

JsonCompiler::~JsonCompiler()
{
    if (root_) {
        cJSON_Delete(root_);
    }
}

uint32_t JsonCompiler::CompileSingleFile(const FileInfo &fileInfo)
{
    if (fileInfo.limitKey == "base" &&
        fileInfo.fileCluster == "element" &&
        fileInfo.filename == ID_DEFINED_FILE) {
        return RESTOOL_SUCCESS;
    }

    if (!ResourceUtil::OpenJsonFile(fileInfo.filePath, &root_)) {
        return RESTOOL_ERROR;
    }
    if (!root_ || !cJSON_IsObject(root_)) {
        cerr << "Error: JSON file parsing failed, please check the JSON file.";
        cerr << NEW_LINE_PATH << fileInfo.filePath << endl;
        return RESTOOL_ERROR;
    }
    cJSON *item = root_->child;
    if (cJSON_GetArraySize(root_) != 1) {
        cerr << "Error: node of a JSON file can only have one member, please check the JSON file.";
        cerr << NEW_LINE_PATH << fileInfo.filePath << endl;
        return RESTOOL_ERROR;
    }

    string tag = item->string;
    auto ret = g_contentClusterMap.find(tag);
    if (ret == g_contentClusterMap.end()) {
        cerr << "Error: invalid tag name '" << tag << "', please check the JSON file.";
        cerr << NEW_LINE_PATH << fileInfo.filePath << endl;
        return RESTOOL_ERROR;
    }
    isBaseString_ = (fileInfo.limitKey == "base" &&
        find(TRANSLATION_TYPE.begin(), TRANSLATION_TYPE.end(), tag) != TRANSLATION_TYPE.end());
    FileInfo copy = fileInfo;
    copy.fileType = ret->second;
    if (!ParseJsonArrayLevel(item, copy)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

// below private
void JsonCompiler::InitParser()
{
    using namespace placeholders;
    handles_.emplace(ResType::STRING, bind(&JsonCompiler::HandleString, this, _1, _2));
    handles_.emplace(ResType::INTEGER, bind(&JsonCompiler::HandleInteger, this, _1, _2));
    handles_.emplace(ResType::BOOLEAN, bind(&JsonCompiler::HandleBoolean, this, _1, _2));
    handles_.emplace(ResType::COLOR, bind(&JsonCompiler::HandleColor, this, _1, _2));
    handles_.emplace(ResType::FLOAT, bind(&JsonCompiler::HandleFloat, this, _1, _2));
    handles_.emplace(ResType::STRARRAY, bind(&JsonCompiler::HandleStringArray, this, _1, _2));
    handles_.emplace(ResType::INTARRAY, bind(&JsonCompiler::HandleIntegerArray, this, _1, _2));
    handles_.emplace(ResType::THEME, bind(&JsonCompiler::HandleTheme, this, _1, _2));
    handles_.emplace(ResType::PATTERN, bind(&JsonCompiler::HandlePattern, this, _1, _2));
    handles_.emplace(ResType::PLURAL, bind(&JsonCompiler::HandlePlural, this, _1, _2));
    handles_.emplace(ResType::SYMBOL, bind(&JsonCompiler::HandleSymbol, this, _1, _2));
}

bool JsonCompiler::ParseJsonArrayLevel(const cJSON *arrayNode, const FileInfo &fileInfo)
{
    if (!arrayNode || !cJSON_IsArray(arrayNode)) {
        cerr << "Error: '" << ResourceUtil::ResTypeToString(fileInfo.fileType) << "' must be array.";
        cerr << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }

    if (cJSON_GetArraySize(arrayNode) == 0) {
        cerr << "Error: '" << ResourceUtil::ResTypeToString(fileInfo.fileType) << "' empty.";
        cerr << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }
    int32_t index = -1;
    for (cJSON *item = arrayNode->child; item; item = item->next) {
        index++;
        if (!item || !cJSON_IsObject(item)) {
            cerr << "Error: the seq=" << index << " item must be object." << NEW_LINE_PATH << fileInfo.filePath << endl;
            return false;
        }
        if (!ParseJsonObjectLevel(item, fileInfo)) {
            return false;
        }
    }
    return true;
}

bool JsonCompiler::ParseJsonObjectLevel(cJSON *objectNode, const FileInfo &fileInfo)
{
    cJSON *nameNode = cJSON_GetObjectItem(objectNode, TAG_NAME.c_str());
    if (!nameNode) {
        cerr << "Error: name empty." << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }

    if (!cJSON_IsString(nameNode)) {
        cerr << "Error: name must string." << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }

    if (isBaseString_ && !TranslatableParse::ParseTranslatable(objectNode, fileInfo, nameNode->valuestring)) {
        return false;
    }
    ResourceItem resourceItem(nameNode->valuestring, fileInfo.keyParams, fileInfo.fileType);
    resourceItem.SetFilePath(fileInfo.filePath);
    resourceItem.SetLimitKey(fileInfo.limitKey);
    auto ret = handles_.find(fileInfo.fileType);
    if (ret == handles_.end()) {
        cerr << "Error: json parser don't support " << ResourceUtil::ResTypeToString(fileInfo.fileType) << endl;
        return false;
    }

    if (!ret->second(objectNode, resourceItem)) {
        return false;
    }

    return MergeResourceItem(resourceItem);
}

bool JsonCompiler::HandleString(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    cJSON *valueNode = cJSON_GetObjectItem(objectNode, TAG_VALUE.c_str());
    if (!CheckJsonStringValue(valueNode, resourceItem)) {
        return false;
    }
    return PushString(valueNode->valuestring, resourceItem);
}

bool JsonCompiler::HandleInteger(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    cJSON *valueNode = cJSON_GetObjectItem(objectNode, TAG_VALUE.c_str());
    if (!CheckJsonIntegerValue(valueNode, resourceItem)) {
        return false;
    }
    if (cJSON_IsString(valueNode)) {
        return PushString(valueNode->valuestring, resourceItem);
    } else if (cJSON_IsNumber(valueNode)) {
        return PushString(to_string(valueNode->valueint), resourceItem);
    } else {
        return false;
    }
}

bool JsonCompiler::HandleBoolean(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    cJSON *valueNode = cJSON_GetObjectItem(objectNode, TAG_VALUE.c_str());
    if (cJSON_IsString(valueNode)) {
        regex ref("^\\$(ohos:)?boolean:.*");
        if (!regex_match(valueNode->valuestring, ref)) {
            cerr << "Error: '" << valueNode->valuestring << "' only refer '$boolean:xxx'.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
        return PushString(valueNode->valuestring, resourceItem);
    }
    if (!cJSON_IsBool(valueNode)) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not boolean.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return PushString(cJSON_IsTrue(valueNode) == 1 ? "true" : "false", resourceItem);
}

bool JsonCompiler::HandleColor(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    return HandleString(objectNode, resourceItem);
}

bool JsonCompiler::HandleFloat(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    return HandleString(objectNode, resourceItem);
}

bool JsonCompiler::HandleStringArray(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    return ParseValueArray(objectNode, resourceItem, extra,
        [this](const cJSON *arrayItem, const ResourceItem &resourceItem, vector<string> &values) -> bool {
            if (!cJSON_IsObject(arrayItem)) {
                cerr << "Error: '" << resourceItem.GetName() << "' value array item not object.";
                cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
                return false;
            }
            cJSON *valueNode = cJSON_GetObjectItem(arrayItem, TAG_VALUE.c_str());
            if (!CheckJsonStringValue(valueNode, resourceItem)) {
                return false;
            }
            values.push_back(valueNode->valuestring);
            return true;
    });
}

bool JsonCompiler::HandleIntegerArray(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    return ParseValueArray(objectNode, resourceItem, extra,
        [this](const cJSON *arrayItem, const ResourceItem &resourceItem, vector<string> &values) -> bool {
            if (!CheckJsonIntegerValue(arrayItem, resourceItem)) {
                return false;
            }
            if (cJSON_IsString(arrayItem)) {
                values.push_back(arrayItem->valuestring);
            } else {
                values.push_back(to_string(arrayItem->valueint));
            }
            return true;
    });
}

bool JsonCompiler::HandleTheme(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    if (!ParseParent(objectNode, resourceItem, extra)) {
        return false;
    }
    return ParseValueArray(objectNode, resourceItem, extra,
        [this](const cJSON *arrayItem, const ResourceItem &resourceItem, vector<string> &values) {
            return ParseAttribute(arrayItem, resourceItem, values);
        });
}

bool JsonCompiler::HandlePattern(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    return HandleTheme(objectNode, resourceItem);
}

bool JsonCompiler::HandlePlural(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    vector<string> attrs;
    bool result = ParseValueArray(objectNode, resourceItem, extra,
        [&attrs, this](const cJSON *arrayItem, const ResourceItem &resourceItem, vector<string> &values) {
            if (!CheckPluralValue(arrayItem, resourceItem)) {
                return false;
            }
            cJSON *quantityNode = cJSON_GetObjectItem(arrayItem, TAG_QUANTITY.c_str());
            if (!quantityNode || !cJSON_IsString(quantityNode)) {
                return false;
            }
            string quantityValue = quantityNode->valuestring;
            if (find(attrs.begin(), attrs.end(), quantityValue) != attrs.end()) {
                cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity '" << quantityValue;
                cerr << "' duplicated." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
                return false;
            }
            attrs.push_back(quantityValue);
            values.push_back(quantityValue);
            cJSON *valueNode = cJSON_GetObjectItem(arrayItem, TAG_VALUE.c_str());
            if (!valueNode || !cJSON_IsString(valueNode)) {
                return false;
            }
            values.push_back(valueNode->valuestring);
            return true;
        });
    if (!result) {
        return false;
    }
    if (find(attrs.begin(), attrs.end(), "other") == attrs.end()) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity must contains 'other'.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::HandleSymbol(const cJSON *objectNode, ResourceItem &resourceItem) const
{
    cJSON *valueNode = cJSON_GetObjectItem(objectNode, TAG_VALUE.c_str());
    if (!CheckJsonSymbolValue(valueNode, resourceItem)) {
        return false;
    }
    return PushString(valueNode->valuestring, resourceItem);
}

bool JsonCompiler::PushString(const string &value, ResourceItem &resourceItem) const
{
    if (!resourceItem.SetData(reinterpret_cast<const int8_t *>(value.c_str()), value.length())) {
        cerr << "Error: resourceItem setdata fail,'" << resourceItem.GetName() << "'.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::CheckJsonStringValue(const cJSON *valueNode, const ResourceItem &resourceItem) const
{
    if (!valueNode || !cJSON_IsString(valueNode)) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not string.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    const map<ResType, string> REFS = {
        { ResType::STRING, "\\$(ohos:)?string:" },
        { ResType::STRARRAY, "\\$(ohos:)?string:" },
        { ResType::COLOR, "\\$(ohos:)?color:" },
        { ResType::FLOAT, "\\$(ohos:)?float:" }
    };

    string value = valueNode->valuestring;
    ResType type = resourceItem.GetResType();
    if (type ==  ResType::COLOR && !CheckColorValue(value.c_str())) {
        string error = "invalid color value '" + value + \
                        "', only support refer '$color:xxx' or '#rgb','#argb','#rrggbb','#aarrggbb'.";
        cerr << "Error: " << error << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    regex ref("^\\$.+:");
    smatch result;
    if (regex_search(value, result, ref) && !regex_match(result[0].str(), regex(REFS.at(type)))) {
        cerr << "Error: '" << value << "', only refer '"<< REFS.at(type) << "xxx'.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::CheckJsonIntegerValue(const cJSON *valueNode, const ResourceItem &resourceItem) const
{
    if (!valueNode) {
        cerr << "Error: '" << resourceItem.GetName() << "' value is empty";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (cJSON_IsString(valueNode)) {
        regex ref("^\\$(ohos:)?integer:.*");
        if (!regex_match(valueNode->valuestring, ref)) {
            cerr << "Error: '" << valueNode->valuestring << "', only refer '$integer:xxx'.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
    } else if (!ResourceUtil::IsIntValue(valueNode)) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not integer.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::CheckJsonSymbolValue(const cJSON *valueNode, const ResourceItem &resourceItem) const
{
    if (!valueNode || !cJSON_IsString(valueNode)) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not string.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    string unicodeStr = valueNode->valuestring;
    if (regex_match(unicodeStr, regex("^\\$(ohos:)?symbol:.*"))) {
        return true;
    }
    int unicode = strtol(unicodeStr.c_str(), nullptr, 16);
    if (!ResourceUtil::isUnicodeInPlane15or16(unicode)) {
        cerr << "Error: '" << resourceItem.GetName() << "' value must in 0xF0000 ~ 0xFFFFF or 0x100000 ~ 0x10FFFF.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::ParseValueArray(const cJSON *objectNode, ResourceItem &resourceItem,
                                   const vector<string> &extra, HandleValue callback) const
{
    cJSON *arrayNode = cJSON_GetObjectItem(objectNode, TAG_VALUE.c_str());
    if (!cJSON_IsArray(arrayNode)) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not array.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    if (cJSON_GetArraySize(arrayNode) == 0) {
        cerr << "Error: '" << resourceItem.GetName() << "' value empty.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    vector<string> contents;
    if (!extra.empty()) {
        contents.assign(extra.begin(), extra.end());
    }
    for (cJSON *item = arrayNode->child; item; item = item->next) {
        vector<string> values;
        if (!callback(item, resourceItem, values)) {
            return false;
        }
        contents.insert(contents.end(), values.begin(), values.end());
    }

    string data = ResourceUtil::ComposeStrings(contents);
    if (data.empty()) {
        cerr << "Error: '" << resourceItem.GetName() << "' array too large.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return PushString(data, resourceItem);
}

bool JsonCompiler::ParseParent(const cJSON *objectNode, const ResourceItem &resourceItem,
                               vector<string> &extra) const
{
    cJSON *parentNode = cJSON_GetObjectItem(objectNode, TAG_PARENT.c_str());
    string type = ResourceUtil::ResTypeToString(resourceItem.GetResType());
    if (parentNode) {
        if (!cJSON_IsString(parentNode)) {
            cerr << "Error: " << type << " '" << resourceItem.GetName() << "' parent not string.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
        string parentValue = parentNode->valuestring;
        if (parentValue.empty()) {
            cerr << "Error: " << type << " '"<< resourceItem.GetName() << "' parent empty.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
        if (regex_match(parentValue, regex("^ohos:" + type + ":.+"))) {
            parentValue = "$" + parentValue;
        } else {
            parentValue = "$" + type + ":" + parentValue;
        }
        extra.push_back(parentValue);
    }
    return true;
}

bool JsonCompiler::ParseAttribute(const cJSON *arrayItem, const ResourceItem &resourceItem,
                                  vector<string> &values) const
{
    string type = ResourceUtil::ResTypeToString(resourceItem.GetResType());
    if (!arrayItem || !cJSON_IsObject(arrayItem)) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute not object.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    cJSON *nameNode = cJSON_GetObjectItem(arrayItem, TAG_NAME.c_str());
    if (!nameNode) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute name empty.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!cJSON_IsString(nameNode)) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute name not string.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    values.push_back(nameNode->valuestring);

    cJSON *valueNode = cJSON_GetObjectItem(arrayItem, TAG_VALUE.c_str());
    if (!valueNode) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute '" << nameNode->valuestring;
        cerr << "' value empty." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!cJSON_IsString(valueNode)) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute '" << nameNode->valuestring;
        cerr << "' value not string." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    values.push_back(valueNode->valuestring);
    return true;
}

bool JsonCompiler::CheckPluralValue(const cJSON *arrayItem, const ResourceItem &resourceItem) const
{
    if (!arrayItem || !cJSON_IsObject(arrayItem)) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' array item not object.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    cJSON *quantityNode = cJSON_GetObjectItem(arrayItem, TAG_QUANTITY.c_str());
    if (!quantityNode) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity empty.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!cJSON_IsString(quantityNode)) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity not string.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    string quantityValue = quantityNode->valuestring;
    if (find(QUANTITY_ATTRS.begin(), QUANTITY_ATTRS.end(), quantityValue) == QUANTITY_ATTRS.end()) {
        string buffer(" ");
        for_each(QUANTITY_ATTRS.begin(), QUANTITY_ATTRS.end(), [&buffer](auto iter) {
                buffer.append(iter).append(" ");
            });
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity '" << quantityValue;
        cerr << "' not in [" << buffer << "]." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    cJSON *valueNode = cJSON_GetObjectItem(arrayItem, TAG_VALUE.c_str());
    if (!valueNode) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity '" << quantityValue;
        cerr << "' value empty." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!cJSON_IsString(valueNode)) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity '" << quantityValue;
        cerr << "' value not string." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::CheckColorValue(const char *s) const
{
    if (s == nullptr) {
        return false;
    }
    // color regex
    string regColor = "^#([A-Fa-f0-9]{3}|[A-Fa-f0-9]{4}|[A-Fa-f0-9]{6}|[A-Fa-f0-9]{8})$";
    if (regex_match(s, regex("^\\$.*")) || regex_match(s, regex(regColor))) {
        return true;
    }
    return false;
}
}
}
}
