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

#include "json_compiler.h"
#include <iostream>
#include <regex>
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const string JsonCompiler::TAG_NAME = "name";
const string JsonCompiler::TAG_VALUE = "value";
const string JsonCompiler::TAG_PARENT = "parent";
const string JsonCompiler::TAG_QUANTITY = "quantity";
const vector<string> JsonCompiler::QUANTITY_ATTRS = { "zero", "one", "two", "few", "many", "other" };

JsonCompiler::JsonCompiler(ResType type, const string &output)
    : IResourceCompiler(type, output)
{
    InitParser();
}

JsonCompiler::~JsonCompiler()
{
}

uint32_t JsonCompiler::CompileSingleFile(const FileInfo &fileInfo)
{
    if (fileInfo.limitKey == "base" &&
        fileInfo.fileCluster == "element" &&
        fileInfo.filename == ID_DEFINED_FILE) {
        return RESTOOL_SUCCESS;
    }

    Json::Value root;
    if (!ResourceUtil::OpenJsonFile(fileInfo.filePath, root)) {
        return RESTOOL_ERROR;
    }

    if (!root.isObject()) {
        cerr << "Error: root node must object." << NEW_LINE_PATH << fileInfo.filePath << endl;
        return RESTOOL_ERROR;
    }

    if (root.getMemberNames().size() != 1) {
        cerr << "Error: root node must only one member." << NEW_LINE_PATH << fileInfo.filePath << endl;
        return RESTOOL_ERROR;
    }

    string tag = root.getMemberNames()[0];
    auto ret = g_contentClusterMap.find(tag);
    if (ret == g_contentClusterMap.end()) {
        cerr << "Error: invalid tag name '" << tag << "'." << NEW_LINE_PATH << fileInfo.filePath << endl;
        return RESTOOL_ERROR;
    }
    
    FileInfo copy = fileInfo;
    copy.fileType = ret->second;
    if (!ParseJsonArrayLevel(root[tag], copy)) {
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
}

bool JsonCompiler::ParseJsonArrayLevel(const Json::Value &arrayNode, const FileInfo &fileInfo)
{
    if (!arrayNode.isArray()) {
        cerr << "Error: '" << ResourceUtil::ResTypeToString(fileInfo.fileType) << "' must be array.";
        cerr << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }

    if (arrayNode.empty()) {
        cerr << "Error: '" << ResourceUtil::ResTypeToString(fileInfo.fileType) << "' empty.";
        cerr << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }

    for (Json::ArrayIndex index = 0; index < arrayNode.size(); index++) {
        if (!arrayNode[index].isObject()) {
            cerr << "Error: the seq=" << index << " item must be object." << NEW_LINE_PATH << fileInfo.filePath << endl;
            return false;
        }
        if (!ParseJsonObjectLevel(arrayNode[index], fileInfo)) {
            return false;
        }
    }
    return true;
}

bool JsonCompiler::ParseJsonObjectLevel(const Json::Value &objectNode, const FileInfo &fileInfo)
{
    auto nameNode = objectNode[TAG_NAME];
    if (nameNode.empty()) {
        cerr << "Error: name empty." << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }

    if (!nameNode.isString()) {
        cerr << "Error: name must string." << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }

    ResourceItem resourceItem(nameNode.asString(), fileInfo.keyParams, fileInfo.fileType);
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

bool JsonCompiler::HandleString(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    Json::Value valueNode = objectNode[TAG_VALUE];
    if (!CheckJsonStringValue(valueNode, resourceItem)) {
        return false;
    }
    return PushString(valueNode.asString(), resourceItem);
}

bool JsonCompiler::HandleInteger(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    Json::Value valueNode = objectNode[TAG_VALUE];
    if (!CheckJsonIntegerValue(valueNode, resourceItem)) {
        return false;
    }
    return PushString(valueNode.asString(), resourceItem);
}

bool JsonCompiler::HandleBoolean(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    Json::Value valueNode = objectNode[TAG_VALUE];
    if (valueNode.isString()) {
        regex ref("^\\$(ohos:)?boolean:.*");
        if (!regex_match(valueNode.asString(), ref)) {
            cerr << "Error: '" << valueNode.asString() << "' only refer '$boolean:xxx'.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
    } else if (!valueNode.isBool()) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not boolean.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return PushString(valueNode.asString(), resourceItem);
}

bool JsonCompiler::HandleColor(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    return HandleString(objectNode, resourceItem);
}

bool JsonCompiler::HandleFloat(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    return HandleString(objectNode, resourceItem);
}

bool JsonCompiler::HandleStringArray(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    return ParseValueArray(objectNode, resourceItem, extra,
        [this](const Json::Value &arrayItem, const ResourceItem &resourceItem, vector<string> &values) -> bool {
            if (!arrayItem.isObject()) {
                cerr << "Error: '" << resourceItem.GetName() << "' value array item not object.";
                cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
                return false;
            }
            auto value = arrayItem[TAG_VALUE];
            if (!CheckJsonStringValue(value, resourceItem)) {
                return false;
            }
            values.push_back(value.asString());
            return true;
    });
}

bool JsonCompiler::HandleIntegerArray(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    return ParseValueArray(objectNode, resourceItem, extra,
        [this](const Json::Value &arrayItem, const ResourceItem &resourceItem, vector<string> &values) -> bool {
            if (!CheckJsonIntegerValue(arrayItem, resourceItem)) {
                return false;
            }
            values.push_back(arrayItem.asString());
            return true;
    });
}

bool JsonCompiler::HandleTheme(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    if (!ParseParent(objectNode, resourceItem, extra)) {
        return false;
    }
    return ParseValueArray(objectNode, resourceItem, extra,
        [this](const Json::Value &arrayItem, const ResourceItem &resourceItem, vector<string> &values) {
            return ParseAttribute(arrayItem, resourceItem, values);
        });
}

bool JsonCompiler::HandlePattern(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    return HandleTheme(objectNode, resourceItem);
}

bool JsonCompiler::HandlePlural(const Json::Value &objectNode, ResourceItem &resourceItem) const
{
    vector<string> extra;
    vector<string> attrs;
    bool result = ParseValueArray(objectNode, resourceItem, extra,
        [&attrs, this](const Json::Value &arrayItem, const ResourceItem &resourceItem, vector<string> &values) {
            if (!CheckPluralValue(arrayItem, resourceItem)) {
                return false;
            }
            string quantityValue = arrayItem[TAG_QUANTITY].asString();
            if (find(attrs.begin(), attrs.end(), quantityValue) != attrs.end()) {
                cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity '" << quantityValue;
                cerr << "' duplicated." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
                return false;
            }
            attrs.push_back(quantityValue);
            values.push_back(quantityValue);
            values.push_back(arrayItem[TAG_VALUE].asString());
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

bool JsonCompiler::PushString(const string &value, ResourceItem &resourceItem) const
{
    if (!resourceItem.SetData(reinterpret_cast<const int8_t *>(value.c_str()), value.length())) {
        cerr << "Error: resourceItem setdata fail,'" << resourceItem.GetName() << "'.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::CheckJsonStringValue(const Json::Value &valueNode, const ResourceItem &resourceItem) const
{
    if (!valueNode.isString()) {
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

    string value = valueNode.asString();
    ResType type = resourceItem.GetResType();
    if (type ==  ResType::COLOR && !CheckColorValue(value.c_str())) {
        string error = "invaild color value '" + value + \
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

bool JsonCompiler::CheckJsonIntegerValue(const Json::Value &valueNode, const ResourceItem &resourceItem) const
{
    if (valueNode.isString()) {
        regex ref("^\\$(ohos:)?integer:.*");
        if (!regex_match(valueNode.asString(), ref)) {
            cerr << "Error: '" << valueNode.asString() << "', only refer '$integer:xxx'.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
    } else if (!valueNode.isInt()) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not integer.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool JsonCompiler::ParseValueArray(const Json::Value &objectNode, ResourceItem &resourceItem,
                                   const vector<string> &extra, HandleValue callback) const
{
    Json::Value arrayNode = objectNode[TAG_VALUE];
    if (!arrayNode.isArray()) {
        cerr << "Error: '" << resourceItem.GetName() << "' value not array.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    if (arrayNode.empty()) {
        cerr << "Error: '" << resourceItem.GetName() << "' value empty.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    vector<string> contents;
    if (!extra.empty()) {
        contents.assign(extra.begin(), extra.end());
    }
    for (Json::ArrayIndex index = 0; index < arrayNode.size(); index++) {
        vector<string> values;
        if (!callback(arrayNode[index], resourceItem, values)) {
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

bool JsonCompiler::ParseParent(const Json::Value &objectNode, const ResourceItem &resourceItem,
                               vector<string> &extra) const
{
    auto parent = objectNode[TAG_PARENT];
    string type = ResourceUtil::ResTypeToString(resourceItem.GetResType());
    if (!parent.isNull()) {
        if (!parent.isString()) {
            cerr << "Error: " << type << " '" << resourceItem.GetName() << "' parent not string.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
        if (parent.empty()) {
            cerr << "Error: " << type << " '"<< resourceItem.GetName() << "' parent empty.";
            cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
        string parentValue = parent.asString();
        if (regex_match(parentValue, regex("^ohos:" + type + ":.+"))) {
            parentValue = "$" + parentValue;
        } else {
            parentValue = "$" + type + ":" + parentValue;
        }
        extra.push_back(parentValue);
    }
    return true;
}

bool JsonCompiler::ParseAttribute(const Json::Value &arrayItem, const ResourceItem &resourceItem,
                                  vector<string> &values) const
{
    string type = ResourceUtil::ResTypeToString(resourceItem.GetResType());
    if (!arrayItem.isObject()) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute not object.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    auto name = arrayItem[TAG_NAME];
    if (name.empty()) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute name empty.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!name.isString()) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute name not string.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    values.push_back(name.asString());

    auto value = arrayItem[TAG_VALUE];
    if (value.isNull()) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute '" << name.asString();
        cerr << "' value empty." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!value.isString()) {
        cerr << "Error: " << type << " '" << resourceItem.GetName() << "' attribute '" << name.asString();
        cerr << "' value not string." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    values.push_back(value.asString());
    return true;
}

bool JsonCompiler::CheckPluralValue(const Json::Value &arrayItem, const ResourceItem &resourceItem) const
{
    if (!arrayItem.isObject()) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' array item not object.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    auto quantity = arrayItem[TAG_QUANTITY];
    if (quantity.empty()) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity empty.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!quantity.isString()) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity not string.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    string quantityValue = quantity.asString();
    if (find(QUANTITY_ATTRS.begin(), QUANTITY_ATTRS.end(), quantityValue) == QUANTITY_ATTRS.end()) {
        string buffer(" ");
        for_each(QUANTITY_ATTRS.begin(), QUANTITY_ATTRS.end(), [&buffer](auto iter) {
                buffer.append(iter).append(" ");
            });
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity '" << quantityValue;
        cerr << "' not in [" << buffer << "]." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    auto value = arrayItem[TAG_VALUE];
    if (value.isNull()) {
        cerr << "Error: Plural '" << resourceItem.GetName() << "' quantity '" << quantityValue;
        cerr << "' value empty." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    if (!value.isString()) {
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
