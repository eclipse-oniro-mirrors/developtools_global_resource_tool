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

#include "reference_parser.h"
#include <iostream>
#include <regex>
#include "file_entry.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const map<string, ResType> ReferenceParser::ID_REFS = {
    { "^\\$id:", ResType::ID },
    { "^\\$boolean:", ResType::BOOLEAN },
    { "^\\$color:", ResType::COLOR },
    { "^\\$float:", ResType::FLOAT },
    { "^\\$media:", ResType::MEDIA },
    { "^\\$profile:", ResType::PROF },
    { "^\\$integer:", ResType::INTEGER },
    { "^\\$string:", ResType::STRING },
    { "^\\$pattern:", ResType::PATTERN },
    { "^\\$plural:", ResType::PLURAL },
    { "^\\$theme:", ResType::THEME }
};

const map<string, ResType> ReferenceParser::ID_OHOS_REFS = {
    { "^\\$ohos:id:", ResType::ID },
    { "^\\$ohos:boolean:", ResType::BOOLEAN },
    { "^\\$ohos:color:", ResType::COLOR },
    { "^\\$ohos:float:", ResType::FLOAT },
    { "^\\$ohos:media:", ResType::MEDIA },
    { "^\\$ohos:profile:", ResType::PROF },
    { "^\\$ohos:integer:", ResType::INTEGER },
    { "^\\$ohos:string:", ResType::STRING },
    { "^\\$ohos:pattern:", ResType::PATTERN },
    { "^\\$ohos:plural:", ResType::PLURAL },
    { "^\\$ohos:theme:", ResType::THEME }
};

ReferenceParser::ReferenceParser() : idWorker_(IdWorker::GetInstance())
{
}

ReferenceParser::~ReferenceParser()
{
}

uint32_t ReferenceParser::ParseRefInResources(map<int32_t, vector<ResourceItem>> &items, const string &output)
{
    for (auto &iter : items) {
        for (auto &resourceItem : iter.second) {
            if (IsElementRef(resourceItem) && ParseRefInResourceItem(resourceItem) != RESTOOL_SUCCESS) {
                return RESTOOL_ERROR;
            }
            if ((IsMediaRef(resourceItem) || IsProfileRef(resourceItem)) &&
                ParseRefInJsonFile(resourceItem, output) != RESTOOL_SUCCESS) {
                return RESTOOL_ERROR;
            }
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ReferenceParser::ParseRefInResourceItem(ResourceItem &resourceItem) const
{
    ResType resType = resourceItem.GetResType();
    string data;
    bool update = false;
    if (IsStringOfResourceItem(resType)) {
        data = string(reinterpret_cast<const char *>(resourceItem.GetData()), resourceItem.GetDataLength());
        if (!ParseRefString(data, update)) {
            cerr << "Error: " << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return RESTOOL_ERROR;
        }
        if (!update) {
            return RESTOOL_SUCCESS;
        }
    } else if (IsArrayOfResourceItem(resType)) {
        if (!ParseRefResourceItemData(resourceItem, data, update)) {
            return RESTOOL_ERROR;
        }
        if (!update) {
            return RESTOOL_SUCCESS;
        }
    }
    if (update && !resourceItem.SetData(reinterpret_cast<const int8_t *>(data.c_str()), data.length())) {
        cerr << "Error: set data fail. name = '" << resourceItem.GetName() << "' data = '" << data << "'.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ReferenceParser::ParseRefInJsonFile(ResourceItem &resourceItem, const string &output, const bool isIncrement)
{
    string jsonPath;
    if (resourceItem.GetResType() == ResType::MEDIA) {
        jsonPath = FileEntry::FilePath(output).Append(RESOURCES_DIR)
            .Append(resourceItem.GetLimitKey()).Append("media").Append(resourceItem.GetName()).GetPath();
    } else {
        jsonPath = FileEntry::FilePath(output).Append(RESOURCES_DIR)
            .Append("base").Append("profile").Append(resourceItem.GetName()).GetPath();
    }
    if (!ParseRefJson(resourceItem.GetFilePath(), jsonPath)) {
        return RESTOOL_ERROR;
    }

    if (isIncrement && ResourceUtil::FileExist(jsonPath)) {
        resourceItem.SetData(reinterpret_cast<const int8_t *>(jsonPath.c_str()), jsonPath.length());
    }
    return RESTOOL_SUCCESS;
}

uint32_t ReferenceParser::ParseRefInString(string &value, bool &update) const
{
    if (ParseRefString(value, update)) {
        return RESTOOL_SUCCESS;
    }
    return RESTOOL_ERROR;
}

bool ReferenceParser::ParseRefJson(const string &from, const string &to) const
{
    Json::Value root;
    if (!ResourceUtil::OpenJsonFile(from, root)) {
        return false;
    }

    bool needSave = false;
    if (!ParseRefJsonImpl(root, needSave)) {
        return false;
    }

    if (!needSave) {
        return true;
    }

    if (!ResourceUtil::CreateDirs(FileEntry::FilePath(to).GetParent().GetPath())) {
        return false;
    }

    if (!ResourceUtil::SaveToJsonFile(to, root)) {
        return false;
    }
    return true;
}

bool ReferenceParser::ParseRefResourceItemData(const ResourceItem &resourceItem, string &data, bool &update) const
{
    data = string(reinterpret_cast<const char *>(resourceItem.GetData()), resourceItem.GetDataLength());
    vector<string> contents = ResourceUtil::DecomposeStrings(data);
    if (contents.empty()) {
        cerr << "Error: DecomposeStrings fail. name = '" << resourceItem.GetName() << "' data = '" << data << "'.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    for (auto &content : contents) {
        bool flag = false;
        if (!ParseRefString(content, flag)) {
            cerr << "Error: " << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
        update = (update || flag);
    }

    if (!update) {
        return true;
    }

    data = ResourceUtil::ComposeStrings(contents);
    if (data.empty()) {
        cerr << "Error: ComposeStrings fail. name = '" << resourceItem.GetName();
        cerr << "'  contents size is " << contents.size() << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }
    return true;
}

bool ReferenceParser::IsStringOfResourceItem(ResType resType) const
{
    if (resType == ResType::STRING ||
        resType == ResType::INTEGER ||
        resType == ResType::BOOLEAN ||
        resType == ResType::COLOR ||
        resType == ResType::FLOAT) {
        return true;
    }
    return false;
}

bool ReferenceParser::IsArrayOfResourceItem(ResType resType) const
{
    if (resType == ResType::STRARRAY ||
        resType == ResType::INTARRAY ||
        resType == ResType::PLURAL ||
        resType == ResType::THEME ||
        resType == ResType::PATTERN) {
        return true;
    }
    return false;
}

bool ReferenceParser::IsElementRef(const ResourceItem &resourceItem) const
{
    ResType resType = resourceItem.GetResType();
    auto result = find_if(g_contentClusterMap.begin(), g_contentClusterMap.end(), [resType](const auto &iter) {
        return resType == iter.second;
    });
    if (result == g_contentClusterMap.end()) {
        return false;
    }
    return true;
}

bool ReferenceParser::IsMediaRef(const ResourceItem &resourceItem) const
{
    return resourceItem.GetResType() == ResType::MEDIA &&
                FileEntry::FilePath(resourceItem.GetFilePath()).GetExtension() == JSON_EXTENSION;
}

bool ReferenceParser::IsProfileRef(const ResourceItem &resourceItem) const
{
    return resourceItem.GetResType() == ResType::PROF && resourceItem.GetLimitKey() == "base" &&
                FileEntry::FilePath(resourceItem.GetFilePath()).GetExtension() == JSON_EXTENSION;
}

bool ReferenceParser::ParseRefString(string &key) const
{
    bool update = false;
    return ParseRefString(key, update);
}

bool ReferenceParser::ParseRefString(std::string &key, bool &update) const
{
    update = false;
    if (regex_match(key, regex("^\\$ohos:[a-z]+:.+"))) {
        update = true;
        return ParseRefImpl(key, ID_OHOS_REFS, true);
    } else if (regex_match(key, regex("^\\$[a-z]+:.+"))) {
        update = true;
        return ParseRefImpl(key, ID_REFS, false);
    }
    return true;
}

bool ReferenceParser::ParseRefImpl(string &key, const map<string, ResType> &refs, bool isSystem) const
{
    for (const auto &ref : refs) {
        smatch result;
        if (regex_search(key, result, regex(ref.first))) {
            string name = key.substr(result[0].str().length());
            int32_t id = idWorker_.GetId(ref.second, name);
            if (isSystem) {
                id = idWorker_.GetSystemId(ref.second, name);
            }
            if (id < 0) {
                cerr << "Error: ref '" << key << "' don't be defined." << endl;
                return false;
            }

            key = to_string(id);
            if (ref.second != ResType::ID) {
                key = "$" + ResourceUtil::ResTypeToString(ref.second) + ":" + to_string(id);
            }
            return true;
        }
    }
    cerr << "Error: reference '" << key << "' invalid." << endl;
    return false;
}

bool ReferenceParser::ParseRefJsonImpl(Json::Value &node, bool &needSave) const
{
    if (node.isObject()) {
        for (const auto &member : node.getMemberNames()) {
            if (!ParseRefJsonImpl(node[member], needSave)) {
                return false;
            }
        }
    } else if (node.isArray()) {
        for (Json::ArrayIndex i = 0; i < node.size(); i++) {
            if (!ParseRefJsonImpl(node[i], needSave)) {
                return false;
            }
        }
    } else if (node.isString()) {
        string value = node.asString();
        bool update = false;
        if (!ParseRefString(value, update)) {
            return false;
        }
        if (update) {
            needSave = update;
        }
        node = value;
    }
    return true;
}
}
}
}
