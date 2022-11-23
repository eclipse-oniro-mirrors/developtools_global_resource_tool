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
#include<iostream>
#include<regex>
#include "file_entry.h"
#include "restool_errors.h"
#include "xml_key_node.h"

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
    { "^\\$layout:", ResType::LAYOUT },
    { "^\\$pattern:", ResType::PATTERN },
    { "^\\$plural:", ResType::PLURAL },
    { "^\\$graphic:", ResType::GRAPHIC },
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
    { "^\\$ohos:layout:", ResType::LAYOUT },
    { "^\\$ohos:pattern:", ResType::PATTERN },
    { "^\\$ohos:plural:", ResType::PLURAL },
    { "^\\$ohos:graphic:", ResType::GRAPHIC },
    { "^\\$ohos:theme:", ResType::THEME }
};

ReferenceParser::ReferenceParser() : idWorker_(IdWorker::GetInstance())
{
}

ReferenceParser::~ReferenceParser()
{
}

uint32_t ReferenceParser::ParseRefInSolidXml(const vector<string> &solidXmlFolders) const
{
    for (const auto &solidXmlFolder : solidXmlFolders) {
        string filePath = FileEntry::FilePath(solidXmlFolder)
            .Append(XmlKeyNode::KEY_TO_FILE_NAME.at(XmlKeyNode::KeyType::CONSTANT)).GetPath();
        if (!ResourceUtil::FileExist(filePath)) {
            continue;
        }

        XmlKeyNode xmlKeyNode;
        if (!xmlKeyNode.LoadFromFile(filePath, [this](auto &key) -> bool {
            return ParseRefString(key);
            })) {
            return RESTOOL_ERROR;
        }

        if (!xmlKeyNode.SaveToFile(filePath)) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ReferenceParser::ParseRefInElement(map<int32_t, vector<ResourceItem>> &items) const
{
    for (auto &iter : items) {
        for (auto &resourceItem : iter.second) {
            if (IsNotElement(resourceItem.GetResType())) {
                break;
            }
            if (!ParseRefResourceItem(resourceItem)) {
                return RESTOOL_ERROR;
            }
        }
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

uint32_t ReferenceParser::ParseRefInProfile(const string &output) const
{
    string profileFolder = FileEntry::FilePath(output).Append(RESOURCES_DIR).Append("base").Append("profile").GetPath();
    if (!ResourceUtil::FileExist(profileFolder)) {
        return RESTOOL_SUCCESS;
    }

    FileEntry f(profileFolder);
    for (const auto &entry : f.GetChilds()) {
        if (!entry->IsFile()) {
            cerr << "Error: '" << entry->GetFilePath().GetPath() << "' is directory." << endl;
            return false;
        }

        if (entry->GetFilePath().GetExtension() != ".json") {
            continue;
        }

        if (ParseRefInJson(entry->GetFilePath().GetPath()) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ReferenceParser::ParseRefInJson(const string &filePath) const
{
    return ParseRefInJson(filePath, filePath);
}

uint32_t ReferenceParser::ParseRefInJson(const string &from, const string &to) const
{
    Json::Value root;
    if (!ResourceUtil::OpenJsonFile(from, root)) {
        return RESTOOL_ERROR;
    }

    bool needSave = false;
    if (!ParseRefJsonImpl(root, needSave)) {
        return RESTOOL_ERROR;
    }

    if (!needSave) {
        return RESTOOL_SUCCESS;
    }

    if (!ResourceUtil::CreateDirs(FileEntry::FilePath(to).GetParent().GetPath())) {
        return RESTOOL_ERROR;
    }

    if (!ResourceUtil::SaveToJsonFile(to, root)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ReferenceParser::ParseRefInResourceItem(ResourceItem &resourceItem) const
{
    if (!ParseRefResourceItem(resourceItem)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

bool ReferenceParser::ParseRefResourceItem(ResourceItem &resourceItem) const
{
    ResType resType = resourceItem.GetResType();
    string data;
    bool update = false;
    if (IsStringOfResourceItem(resType)) {
        data = string(reinterpret_cast<const char *>(resourceItem.GetData()), resourceItem.GetDataLength());
        if (!ParseRefString(data, update)) {
            cerr << "Error: " << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
            return false;
        }
        if (!update) {
            return true;
        }
    } else if (IsArrayOfResourceItem(resType)) {
        if (!ParseRefResourceItemData(resourceItem, data, update)) {
            return false;
        }
        if (!update) {
            return true;
        }
    }
    if (update && !resourceItem.SetData(reinterpret_cast<const int8_t *>(data.c_str()), data.length())) {
        cerr << "Error: set data fail. name = '" << resourceItem.GetName() << "' data = '" << data << "'.";
        cerr << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
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

bool ReferenceParser::IsNotElement(ResType resType) const
{
    auto result = find_if(g_contentClusterMap.begin(), g_contentClusterMap.end(), [resType](const auto &iter) {
        return resType == iter.second;
    });
    if (result == g_contentClusterMap.end()) {
        return true;
    }
    return false;
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
                key = key = "$" + ResourceUtil::ResTypeToString(ref.second) + ":" + to_string(id);
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
