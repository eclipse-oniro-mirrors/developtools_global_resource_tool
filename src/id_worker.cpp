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

#include "id_worker.h"
#include<iostream>
#include<regex>
#include "cmd_parser.h"
#include "file_entry.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const int32_t IdWorker::START_SYS_ID = 0x07800000;
uint32_t IdWorker::Init(ResourceIdCluster type, int32_t startId)
{
    type_ = type;
    if (InitIdDefined() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (type == ResourceIdCluster::RES_ID_APP) {
        appId_ = startId;
        maxId_ = GetMaxId(startId);
    }
    return RESTOOL_SUCCESS;
}

int32_t IdWorker::GenerateId(ResType resType, const string &name)
{
    if (type_ == ResourceIdCluster::RES_ID_APP) {
        return GenerateAppId(resType, name);
    }
    return GenerateSysId(resType, name);
}

vector<IdWorker::ResourceId> IdWorker::GetHeaderId() const
{
    map<ResType, vector<ResourceId>> idClassify;
    for (const auto &it : ids_) {
        ResourceId resourceId;
        resourceId.id = it.second;
        resourceId.type = ResourceUtil::ResTypeToString(it.first.first);
        resourceId.name = it.first.second;
        idClassify[it.first.first].push_back(resourceId);
    }

    vector<ResourceId> ids;
    for (const auto &item : idClassify) {
        ids.insert(ids.end(), item.second.begin(), item.second.end());
    }
    return ids;
}

int32_t IdWorker::GetId(ResType resType, const string &name) const
{
    auto result = ids_.find(make_pair(resType, name));
    if (result == ids_.end()) {
        return -1;
    }
    return result->second;
}

int32_t IdWorker::GetSystemId(ResType resType, const string &name) const
{
    auto result = definedIds_.find(make_pair(resType, name));
    if (result == definedIds_.end()) {
        return -1;
    }
    return result->second.id;
}

bool IdWorker::IsValidName(const string &name) const
{
    if (!regex_match(name, regex("[a-zA-z0-9_]+"))) {
        cerr << "Error: '" << name << "' only contain [a-zA-z0-9_]." << endl;
        return false;
    }
    if (type_ != ResourceIdCluster::RES_ID_SYS) {
        return true;
    }
    return IsValidSystemName(name);
}

bool IdWorker::PushCache(ResType resType, const string &name, int32_t id)
{
    auto result = cacheIds_.emplace(make_pair(resType, name), id);
    if (!result.second) {
        return false;
    }
    if (appId_ == id) {
        appId_ = id + 1;
        return true;
    }

    if (id < appId_) {
        return false;
    }

    for (int32_t i = appId_; i < id; i++) {
        delIds_.push_back(i);
    }
    appId_ = id + 1;
    return true;
}

void IdWorker::PushDelId(int32_t id)
{
    delIds_.push_back(id);
}

int32_t IdWorker::GenerateAppId(ResType resType, const string &name)
{
    auto result = ids_.find(make_pair(resType, name));
    if (result != ids_.end()) {
        return result->second;
    }

    result = cacheIds_.find(make_pair(resType, name));
    if (result != cacheIds_.end()) {
        ids_.emplace(make_pair(resType, name), result->second);
        return result->second;
    }

    if (appId_ > maxId_) {
        cerr << "Error: id count exceed " << appId_ << ">" << maxId_ << endl;
        return -1;
    }
    int32_t id = 0;
    if (!delIds_.empty()) {
        id = delIds_.front();
        delIds_.erase(delIds_.begin());
    } else {
        id = appId_;
        appId_++;
    }
    ids_.emplace(make_pair(resType, name), id);
    return id;
}

int32_t IdWorker::GenerateSysId(ResType resType, const string &name)
{
    auto result = ids_.find(make_pair(resType, name));
    if (result != ids_.end()) {
        return result->second;
    }

    auto defined = definedIds_.find(make_pair(resType, name));
    if (defined != definedIds_.end()) {
        ids_.emplace(make_pair(resType, name), defined->second.id);
        return defined->second.id;
    }
    return -1;
}

uint32_t IdWorker::InitIdDefined()
{
    InitParser();
    CmdParser<PackageParser> &parser = CmdParser<PackageParser>::GetInstance();
    PackageParser &packageParser = parser.GetCmdParser();
    string idDefinedPath;
    if (type_ == ResourceIdCluster::RES_ID_SYS) {
        for (const auto &inputPath : packageParser.GetInputs()) {
            idDefinedPath = FileEntry::FilePath(inputPath).Append(RESOURCES_DIR)
                .Append("base").Append("element").Append(ID_DEFINED_FILE).GetPath();
            if (InitIdDefined(idDefinedPath) != RESTOOL_SUCCESS) {
                return RESTOOL_ERROR;
            }
        }
        return RESTOOL_SUCCESS;
    }

    idDefinedPath = FileEntry::FilePath(packageParser.GetRestoolPath()).GetParent().Append(ID_DEFINED_FILE).GetPath();
    if (InitIdDefined(idDefinedPath) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t IdWorker::InitIdDefined(const std::string &filePath)
{
    if (!ResourceUtil::FileExist(filePath)) {
        return RESTOOL_SUCCESS;
    }

    Json::Value root;
    if (!ResourceUtil::OpenJsonFile(filePath, root)) {
        return RESTOOL_ERROR;
    }

    int32_t startSysId = GetStartId(root);
    if (startSysId < 0) {
        return RESTOOL_ERROR;
    }
    
    auto record = root["record"];
    if (record.empty()) {
        cerr << "Error: id_defined.json record empty." << endl;
        return RESTOOL_ERROR;
    }
    if (!record.isArray()) {
        cerr << "Error: id_defined.json record not array." << endl;
        return RESTOOL_ERROR;
    }

    for (Json::ArrayIndex index = 0; index < record.size(); index++) {
        auto arrayItem = record[index];
        ResourceId resourceId;
        resourceId.seq = index;
        resourceId.id = startSysId;
        if (!arrayItem.isObject()) {
            return RESTOOL_ERROR;
        }
        for (const auto &handle : handles_) {
            if (!handle.second(arrayItem[handle.first], resourceId)) {
                return RESTOOL_ERROR;
            }
        }
        if (!PushResourceId(resourceId)) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

void IdWorker::InitParser()
{
    using namespace placeholders;
    handles_.emplace("type", bind(&IdWorker::ParseType, this, _1, _2));
    handles_.emplace("name", bind(&IdWorker::ParseName, this, _1, _2));
    handles_.emplace("order", bind(&IdWorker::ParseOrder, this, _1, _2));
}

bool IdWorker::ParseType(const Json::Value &type, ResourceId &resourceId)
{
    if (type.empty()) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " type empty." << endl;
        return false;
    }
    if (!type.isString()) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " type not string." << endl;
        return false;
    }
    if (ResourceUtil::GetResTypeFromString(type.asString()) == ResType::INVALID_RES_TYPE) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " type '";
        cerr << type.asString() << "' invalid." << endl;
        return false;
    }
    resourceId.type = type.asString();
    return true;
}

bool IdWorker::ParseName(const Json::Value &name, ResourceId &resourceId)
{
    if (name.empty()) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " name empty." << endl;
        return false;
    }
    if (!name.isString()) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " name not string." << endl;
        return false;
    }
    resourceId.name = name.asString();
    if ((resourceId.id & START_SYS_ID) == START_SYS_ID && !IsValidSystemName(resourceId.name)) {
        cerr << "Error: id_defined.json."<< endl;
        return false;
    }
    return true;
}

bool IdWorker::ParseOrder(const Json::Value &order, ResourceId &resourceId)
{
    if (order.empty()) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " order empty." << endl;
        return false;
    }
    if (!order.isInt()) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " order not int." << endl;
        return false;
    }
    if (order.asInt() != resourceId.seq) {
        cerr << "Error: id_defined.json seq=" << resourceId.seq << " order value ";
        cerr << order.asInt() << " vs expect " << resourceId.seq << endl;
        return false;
    }
    resourceId.id = resourceId.id + order.asInt();
    return true;
}

bool IdWorker::PushResourceId(const ResourceId &resourceId)
{
    ResType resType = ResourceUtil::GetResTypeFromString(resourceId.type);
    auto result = definedIds_.emplace(make_pair(resType, resourceId.name), resourceId);
    if (!result.second) {
        cerr << "Error: '" << resourceId.type << "' '" << resourceId.name << "' duplicated." << endl;
        return false;
    }
    return true;
}

bool IdWorker::IsValidSystemName(const string &name) const
{
    if (regex_match(name, regex("^ohos.+"))) {
        return true;
    }
    cerr << "Error: '" << name << "' must start with 'ohos'" << endl;
    return false;
}

int32_t IdWorker::GetStartId(const Json::Value &root) const
{
    auto startId = root["startId"];
    if (startId.empty()) {
        cerr << "Error: id_defined.json 'startId' empty." << endl;
        return -1;
    }

    if (!startId.isString()) {
        cerr << "Error: id_defined.json 'startId' not string." << endl;
        return -1;
    }

    int32_t id = strtol(startId.asString().c_str(), nullptr, 16);
    return id;
}

int32_t IdWorker::GetMaxId(int32_t startId) const
{
    int32_t flag = 1;
    while ((flag & startId) == 0) {
        flag = flag << 1;
    }
    return (startId + flag - 1);
}
}
}
}