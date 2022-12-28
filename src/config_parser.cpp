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

#include "config_parser.h"
#include<iostream>
#include<regex>
#include "reference_parser.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const map<string, ConfigParser::ModuleType> ConfigParser::MODULE_TYPES = {
    { "har", ModuleType::HAR },
    { "entry", ModuleType::ENTRY },
    { "feature", ModuleType::FEATURE },
    { "shared", ModuleType::SHARED }
};

const map<string, string> ConfigParser::JSON_STRING_IDS = {
    { "icon", "^\\$media:" },
    { "label", "^\\$string:" },
    { "description", "^\\$string:" },
    { "theme", "^\\$theme:" },
    { "reason", "^\\$string:" },
    { "startWindowIcon", "^\\$media:" },
    { "startWindowBackground", "^\\$color:"},
    { "resource", "^\\$[a-z]+:" },
    { "extra", "^\\$[a-z]+:" }
};

const map<string, string> ConfigParser::JSON_ARRAY_IDS = {
    { "landscapeLayouts", "^\\$layout:" },
    { "portraitLayouts", "^\\$layout:" }
};

bool ConfigParser::useModule_ = false;

ConfigParser::ConfigParser()
    : filePath_(""), packageName_(""), moduleName_(""), moduleType_(ModuleType::NONE)
{
}

ConfigParser::ConfigParser(const string &filePath)
    : filePath_(filePath), packageName_(""), moduleName_(""), moduleType_(ModuleType::NONE)
{
}

ConfigParser::~ConfigParser()
{
}

uint32_t ConfigParser::Init()
{
    if (!ResourceUtil::OpenJsonFile(filePath_, rootNode_)) {
        return RESTOOL_ERROR;
    }

    if (!rootNode_.isObject()) {
        cerr << "Error: root node not obeject." << NEW_LINE_PATH << filePath_ << endl;
        return RESTOOL_ERROR;
    }

    if (!ParseModule(rootNode_["module"])) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

const string &ConfigParser::GetPackageName() const
{
    return packageName_;
}

const string &ConfigParser::GetModuleName() const
{
    return moduleName_;
}

int32_t ConfigParser::GetAbilityIconId() const
{
    return abilityIconId_;
}

int32_t ConfigParser::GetAbilityLabelId() const
{
    return abilityLabelId_;
}

ConfigParser::ModuleType ConfigParser::GetModuleType() const
{
    return moduleType_;
}

uint32_t ConfigParser::ParseRefence()
{
    if (ParseRefImpl(rootNode_, "", rootNode_)) {
        return RESTOOL_SUCCESS;
    }
    return RESTOOL_ERROR;
}

uint32_t ConfigParser::Save(const string &filePath) const
{
    if (ResourceUtil::SaveToJsonFile(filePath, rootNode_)) {
        return RESTOOL_SUCCESS;
    }
    return RESTOOL_ERROR;
}

bool ConfigParser::SetAppIcon(string &icon, int32_t id)
{
    if (!rootNode_["app"].isObject()) {
        cerr << "Error: 'app' not object" << endl;
        return false;
    }
    rootNode_["app"]["icon"] = icon;
    rootNode_["app"]["iconId"] = id;
    return true;
}

bool ConfigParser::SetAppLabel(string &label, int32_t id)
{
    if (!rootNode_["app"].isObject()) {
        cerr << "Error: 'app' not object" << endl;
        return false;
    }
    rootNode_["app"]["label"] = label;
    rootNode_["app"]["labelId"] = id;
    return true;
}

// below private
bool ConfigParser::ParseModule(Json::Value &moduleNode)
{
    if (!moduleNode.isObject()) {
        cerr << "Error: 'module' not object." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (moduleNode.empty()) {
        cerr << "Error: 'module' empty." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }

    if (!useModule_) {
        if (moduleNode.isMember("package") && moduleNode["package"].isString()) {
            packageName_ = moduleNode["package"].asString();
        }
        if (!ParseDistro(moduleNode["distro"])) {
            return false;
        }
        return ParseAbilitiesForDepend(moduleNode);
    }

    if (moduleNode["name"].isString()) {
        moduleName_ = moduleNode["name"].asString();
    }

    if (moduleName_.empty()) {
        cerr << "Error: 'name' don't found in 'module'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }

    if (moduleNode["type"].isString() && !ParseModuleType(moduleNode["type"].asString())) {
        return false;
    }
    return true;
}

bool ConfigParser::ParseAbilitiesForDepend(Json::Value &moduleNode)
{
    if (!IsDependEntry()) {
        return true;
    }
    if (moduleNode["mainAbility"].isString()) {
        mainAbility_ = moduleNode["mainAbility"].asString();
        if (mainAbility_[0] == '.') {
            mainAbility_ = packageName_ + mainAbility_;
        }
        return ParseAbilities(moduleNode["abilities"]);
    }
    return true;
}

bool ConfigParser::ParseDistro(Json::Value &distroNode)
{
    if (!distroNode.isObject()) {
        cerr << "Error: 'distro' not object." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (distroNode.empty()) {
        cerr << "Error: 'distro' empty." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }

    if (distroNode["moduleName"].isString()) {
        moduleName_ = distroNode["moduleName"].asString();
    }

    if (moduleName_.empty()) {
        cerr << "Error: 'moduleName' don't found in 'distro'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }

    if (distroNode["moduleType"].isString() && !ParseModuleType(distroNode["moduleType"].asString())) {
        return false;
    }
    return true;
}

bool ConfigParser::ParseAbilities(const Json::Value &abilites)
{
    if (abilites.empty()) {
        return true;
    }
    if (!abilites.isArray()) {
        cerr << "Error: abilites not array." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    bool isMainAbility = false;
    for (Json::ArrayIndex i = 0; i < abilites.size(); i++) {
        if (!ParseAbilitiy(abilites[i], isMainAbility)) {
            cerr << "Error: ParseAbilitiy fail." << endl;
            return false;
        }
        if (isMainAbility) {
            break;
        }
    }
    return true;
}

bool ConfigParser::ParseAbilitiy(const Json::Value &ability, bool &isMainAbility)
{
    if (ability.empty()) {
        return true;
    }
    if (!ability["name"].isString()) {
        return false;
    }
    string name = ability["name"].asString();
    if (name[0] == '.') {
        name = packageName_ + name;
    }
    if (mainAbility_ != name && !IsMainAbility(ability["skills"])) {
        return true;
    }
    if (ability["iconId"].isInt()) {
        abilityIconId_ = ability["iconId"].asInt();
    }
    if (abilityIconId_ <= 0) {
        cerr << "Error: iconId don't found in 'ability'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (ability["labelId"].isInt()) {
        abilityLabelId_ = ability["labelId"].asInt();
    }
    if (abilityLabelId_ <= 0) {
        cerr << "Error: labelId don't found in 'ability'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    isMainAbility = true;
    return true;
}

bool ConfigParser::IsMainAbility(const Json::Value &skills)
{
    if (!skills.isArray()) {
        return false;
    }
    for (Json::ArrayIndex i = 0; i < skills.size(); i++) {
        if (!skills[i].isObject()) {
            return false;
        }
        if (IsHomeAction(skills[i]["actions"])) {
            return true;
        }
    }
    return false;
}

bool ConfigParser::IsHomeAction(const Json::Value &actions)
{
    if (!actions.isArray()) {
        return false;
    }
    for (Json::ArrayIndex i = 0; i < actions.size(); i++) {
        if (!actions[i].isObject()) {
            return false;
        }
        if (actions[i].asString() == "action.system.home") {
            return true;
        }
    }
    return false;
}

bool ConfigParser::ParseRefImpl(Json::Value &parent, const std::string &key, Json::Value &node)
{
    if (node.isArray()) {
        const auto &result = JSON_ARRAY_IDS.find(key);
        if (result != JSON_ARRAY_IDS.end()) {
            return ParseJsonArrayRef(parent, key, node);
        }
        for (Json::ArrayIndex index = 0; index < node.size(); index++) {
            if (!ParseRefImpl(node, "", node[index])) {
                return false;
            }
        }
    } else if (node.isObject()) {
        const auto members = node.getMemberNames();
        for (const auto &member : members) {
            if (!ParseRefImpl(node, member, node[member])) {
                return false;
            }
        }
    } else if (!key.empty() && node.isString()) {
        return ParseJsonStringRef(parent, key, node);
    }
    return true;
}

bool ConfigParser::ParseJsonArrayRef(Json::Value &parent, const string &key, Json::Value &node)
{
    Json::ArrayIndex size = node.size();
    Json::Value array(Json::arrayValue);
    for (Json::ArrayIndex index = 0; index < size; index++) {
        if (!node[index].isString()) {
            cerr << "Error: '" << key << "' invalid value." << NEW_LINE_PATH << filePath_ << endl;
            return false;
        }
        string value = node[index].asString();
        bool update = false;
        if (!GetRefIdFromString(value, update, JSON_ARRAY_IDS.at(key))) {
            cerr << "Error: '" << key << "' value " << node[index] << " invalid." << NEW_LINE_PATH << filePath_ << endl;
            return false;
        }
        if (update) {
            array.append(atoi(value.c_str()));
        }
    }
    parent[key + "Id"] = array;
    return true;
}

bool ConfigParser::ParseJsonStringRef(Json::Value &parent, const string &key, Json::Value &node)
{
    const auto &result = JSON_STRING_IDS.find(key);
    if (result == JSON_STRING_IDS.end()) {
        return true;
    }
    string value = node.asString();
    bool update = false;
    if (!GetRefIdFromString(value, update, JSON_STRING_IDS.at(key))) {
        cerr << "Error: '" << key << "' value " << node << " invalid value." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (update) {
        parent[key + "Id"] = atoi(value.c_str());
    }
    return true;
}

bool ConfigParser::GetRefIdFromString(string &value, bool &update, const string &match) const
{
    ReferenceParser refParser;
    string error = "Error: '" + value + "' must start with '" + match.substr(match.find("\\") + 1) + "'";
    if (refParser.ParseRefInString(value, update) != RESTOOL_SUCCESS) {
        return false;
    }
    if (!update) {
        return true;
    }
    smatch result;
    if (regex_search(value, result, regex(match))) {
        value = value.substr(result[0].str().length());
        return true;
    }
    cerr << error << endl;
    return false;
}

bool ConfigParser::ParseModuleType(const string &type)
{
    const auto &result = MODULE_TYPES.find(type);
    if (result == MODULE_TYPES.end()) {
        cerr << "Error: moduleType='" << type << "' invalid value." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    moduleType_ = result->second;
    return true;
}
}
}
}
