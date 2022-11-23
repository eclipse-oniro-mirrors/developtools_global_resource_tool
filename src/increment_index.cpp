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

#include "increment_index.h"
#include<algorithm>
#include<iostream>
#include "key_parser.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const string IncrementIndex::INDEX_FILE = "index.json";
IncrementIndex::IncrementIndex(const string &indexPath, const vector<string> &folder)
    : indexPath_(indexPath), folder_(folder)
{
}

bool IncrementIndex::Save(const map<int32_t, vector<ResourceItem>> &items) const
{
    Json::Value root;
    root["header"] = Json::Value(Json::ValueType::objectValue);
    root["header"]["folder"] = Json::Value(Json::ValueType::arrayValue);
    for (const auto &folder : folder_) {
        root["header"]["folder"].append(folder);
    }

    root["index"] = Json::Value(Json::ValueType::objectValue);
    for (const auto &item : items) {
        string id = to_string(item.first);
        root["index"][id] = Json::Value(Json::ValueType::objectValue);
        for (const auto &resourceItem : item.second) {
            root["index"][id][resourceItem.GetFilePath()] = Json::Value(Json::ValueType::objectValue);
            auto &record = root["index"][id][resourceItem.GetFilePath()];
            string data(reinterpret_cast<const char *>(resourceItem.GetData()), resourceItem.GetDataLength());
            record["data"] = data;
            record["name"] = resourceItem.GetName();
            record["limitkey"] = resourceItem.GetLimitKey();
            record["type"] = ResourceUtil::ResTypeToString(resourceItem.GetResType());
        }
    }

    if (!ResourceUtil::SaveToJsonFile(indexPath_, root)) {
        return false;
    }
    return true;
}

bool IncrementIndex::Load(map<int32_t, vector<ResourceItem>> &items) const
{
    Json::Value indexJson;
    if (!ResourceUtil::OpenJsonFile(indexPath_, indexJson)) {
        return false;
    }

    auto headerInfo = indexJson["header"];
    if (headerInfo.empty() || !headerInfo.isObject()) {
        cerr << "Error: header info." << NEW_LINE_PATH << indexPath_ << endl;
        return true;
    }
    auto folderInfo = headerInfo["folder"];
    if (folderInfo.empty() || !folderInfo.isArray()) {
        cerr << "Error: folder info." << NEW_LINE_PATH << indexPath_ << endl;
        return false;
    }
    if (folder_.size() != folderInfo.size()) {
        cerr << "Error: add/delete dependency, don't support increment compile." << endl;
        return false;
    }

    for (size_t i = 0; i < folder_.size(); i++) {
        if (folder_[i] != folderInfo[static_cast<int>(i)].asString()) {
            cerr << "Error: dependency change, don't support increment compile." << endl;
            return true;
        }
    }

    if (!LoadIndex(indexJson["index"], items)) {
        return false;
    }
    return true;
}

void IncrementIndex::SetSkipPaths(const vector<string> &skipPaths)
{
    skipPaths_ = skipPaths;
}

// below private
bool IncrementIndex::LoadIndex(const Json::Value &indexInfo, map<int32_t, vector<ResourceItem>> &items) const
{
    if (indexInfo.empty() || !indexInfo.isObject()) {
        cerr << "Error: index info." << NEW_LINE_PATH << indexPath_ << endl;
        return false;
    }

    for (const auto &idMember : indexInfo.getMemberNames()) {
        int32_t id = strtol(idMember.c_str(), nullptr, 10);
        if (id < 0) {
            cerr << "Error: '" << idMember << "' not integer string." << NEW_LINE_PATH << indexPath_ << endl;
            return false;
        }
        if (items.count(id) != 0) {
            cerr << "Error: '" << idMember << "' duplicated." << NEW_LINE_PATH << indexPath_ << endl;
            return false;
        }
        if (!indexInfo[idMember].isObject()) {
            cerr << "Error: '" << idMember << "' not object." << NEW_LINE_PATH << indexPath_ << endl;
            return false;
        }
        for (const auto &pathMember : indexInfo[idMember].getMemberNames()) {
            if (IsIgnore(pathMember)) {
                continue;
            }
            auto item = indexInfo[idMember][pathMember];
            if (!item.isObject()) {
                cerr << "Error: [" << idMember << "][" << pathMember<<"] not object.";
                cerr << NEW_LINE_PATH << indexPath_ << endl;
                return false;
            }
            ResourceItem resourceItem;
            if (!ParseResourceItem(item, pathMember, resourceItem)) {
                cerr << "Error: [" << idMember << "][" << pathMember<<"]." << NEW_LINE_PATH << indexPath_ << endl;
                return false;
            }
            if (!PushResourceItem(resourceItem, id, items)) {
                return false;
            }
        }
    }
    return true;
}

bool IncrementIndex::ParseResourceItem(const Json::Value &item, const string &filePath,
    ResourceItem &resourceItem) const
{
    string name;
    string type;
    string limitKey;
    string data;
    if (!GetResourceItemProp(item, "name", name) || !GetResourceItemProp(item, "type", type) ||
        !GetResourceItemProp(item, "limitkey", limitKey) || !GetResourceItemProp(item, "data", data)) {
        cerr << "Error: 'name' 'type' 'limitkey' 'data' invalid." << endl;
        return false;
    }
    ResType resType = ResourceUtil::GetResTypeFromString(type);
    if (resType == ResType::INVALID_RES_TYPE) {
        cerr << "Error: invaid ResType '" << type << "'." << NEW_LINE_PATH << indexPath_ << endl;
        return false;
    }
    vector<KeyParam> keyParams;
    if (!KeyParser::Parse(limitKey, keyParams)) {
        return false;
    }
    ResourceItem temp(name, keyParams, resType);
    if (resType != ResType::ID && !temp.SetData(reinterpret_cast<const int8_t *>(data.c_str()), data.length())) {
        cerr << "Error: resource set data fail." << endl;
        return false;
    }
    temp.SetFilePath(filePath);
    temp.SetLimitKey(limitKey);
    resourceItem = temp;
    return true;
}

bool IncrementIndex::GetResourceItemProp(const Json::Value &item, const string &key, string &value) const
{
    if (item[key].empty() || !item[key].isString()) {
        return false;
    }
    value = item[key].asString();
    return true;
}

bool IncrementIndex::PushResourceItem(const ResourceItem &resourceItem, int32_t id,
    map<int32_t, vector<ResourceItem>> &items) const
{
    if (items.find(id) == items.end()) {
        items[id].push_back(resourceItem);
        return true;
    }

    const auto &first = items[id].begin();
    if (resourceItem.GetName() != first->GetName()) {
        cerr << "Error: '" << resourceItem.GetName() << "', expect '" << first->GetName() << "'.";
        cerr << NEW_LINE_PATH << indexPath_ <<endl;
        return false;
    }
    if (resourceItem.GetResType() != first->GetResType()) {
        cerr << "Error: '" << ResourceUtil::ResTypeToString(resourceItem.GetResType());
        cerr << "', expect '" << ResourceUtil::ResTypeToString(first->GetResType()) << "'.";
        cerr << NEW_LINE_PATH << indexPath_ << endl;
        return false;
    }
    auto result = find_if(items[id].begin(), items[id].end(), [&resourceItem](const auto &iter) {
        return resourceItem.GetLimitKey() == iter.GetLimitKey();
    });
    if (result != items[id].end()) {
        cerr << "Error: '" << resourceItem.GetLimitKey() << "' conflict." << NEW_LINE_PATH << indexPath_ << endl;
        return false;
    }
    items[id].push_back(resourceItem);
    return true;
}

bool IncrementIndex::IsIgnore(const string &filePath) const
{
    if (skipPaths_.empty()) {
        return false;
    }

    if (find(skipPaths_.begin(), skipPaths_.end(), filePath) != skipPaths_.end()) {
        return true;
    }
    return false;
}
}
}
}