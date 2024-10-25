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

#include "config_parser.h"
#include <iostream>
#include <regex>
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
    { "extra", "^\\$[a-z]+:" },
    { "fileContextMenu", "^\\$profile:" },
    { "orientation", "^\\$string:" }
};

const map<string, string> ConfigParser::JSON_ARRAY_IDS = {
    { "landscapeLayouts", "^\\$layout:" },
    { "portraitLayouts", "^\\$layout:" }
};

bool ConfigParser::useModule_ = false;

ConfigParser::ConfigParser()
    : filePath_(""), packageName_(""), moduleName_(""), moduleType_(ModuleType::NONE),
    abilityIconId_(0), abilityLabelId_(0), root_(nullptr)
{
}

ConfigParser::ConfigParser(const string &filePath)
    : filePath_(filePath), packageName_(""), moduleName_(""), moduleType_(ModuleType::NONE),
    abilityIconId_(0), abilityLabelId_(0), root_(nullptr)
{
}

ConfigParser::~ConfigParser()
{
    if (root_) {
        cJSON_Delete(root_);
    }
}

uint32_t ConfigParser::Init()
{
    if (!ResourceUtil::OpenJsonFile(filePath_, &root_)) {
        return RESTOOL_ERROR;
    }

    if (!root_ || !cJSON_IsObject(root_)) {
        cerr << "Error: JSON file parsing failed, please check the JSON file." << NEW_LINE_PATH << filePath_ << endl;
        return RESTOOL_ERROR;
    }

    cJSON *moduleNode = cJSON_GetObjectItem(root_, "module");
    if (!ParseModule(moduleNode)) {
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

int64_t ConfigParser::GetAbilityIconId() const
{
    return abilityIconId_;
}

int64_t ConfigParser::GetAbilityLabelId() const
{
    return abilityLabelId_;
}

ConfigParser::ModuleType ConfigParser::GetModuleType() const
{
    return moduleType_;
}

uint32_t ConfigParser::ParseRefence()
{
    if (ParseRefImpl(root_, "", root_)) {
        return RESTOOL_SUCCESS;
    }
    return RESTOOL_ERROR;
}

uint32_t ConfigParser::Save(const string &filePath) const
{
    if (ResourceUtil::SaveToJsonFile(filePath, root_)) {
        return RESTOOL_SUCCESS;
    }
    return RESTOOL_ERROR;
}

bool ConfigParser::SetAppIcon(string &icon, int64_t id)
{
    cJSON *appNode = cJSON_GetObjectItem(root_, "app");
    if (!appNode || !cJSON_IsObject(appNode)) {
        cerr << "Error: 'app' not object" << endl;
        return false;
    }
    cJSON_AddStringToObject(appNode, "icon", icon.c_str());
    cJSON_AddNumberToObject(appNode, "iconId", id);
    return true;
}

bool ConfigParser::SetAppLabel(string &label, int64_t id)
{
    cJSON *appNode = cJSON_GetObjectItem(root_, "app");
    if (!appNode || !cJSON_IsObject(appNode)) {
        cerr << "Error: 'app' not object" << endl;
        return false;
    }
    cJSON_AddStringToObject(appNode, "label", label.c_str());
    cJSON_AddNumberToObject(appNode, "labelId", id);
    return true;
}

// below private
bool ConfigParser::ParseModule(cJSON *moduleNode)
{
    if (!moduleNode || !cJSON_IsObject(moduleNode)) {
        cerr << "Error: 'module' not object." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (cJSON_GetArraySize(moduleNode) == 0) {
        cerr << "Error: 'module' empty." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }

    if (!useModule_) {
        cJSON *packageNode = cJSON_GetObjectItem(moduleNode, "package");
        if (packageNode && cJSON_IsString(packageNode)) {
            packageName_ = packageNode->valuestring;
        }
        cJSON *distroNode = cJSON_GetObjectItem(moduleNode, "distro");
        if (!ParseDistro(distroNode)) {
            return false;
        }
        return ParseAbilitiesForDepend(moduleNode);
    }
    cJSON *nameNode = cJSON_GetObjectItem(moduleNode, "name");
    if (nameNode && cJSON_IsString(nameNode)) {
        moduleName_ = nameNode->valuestring;
    }

    if (moduleName_.empty()) {
        cerr << "Error: 'name' don't found in 'module'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    cJSON *typeNode = cJSON_GetObjectItem(moduleNode, "type");
    if (typeNode && cJSON_IsString(typeNode) && !ParseModuleType(typeNode->valuestring)) {
        return false;
    }
    return true;
}

bool ConfigParser::ParseAbilitiesForDepend(cJSON *moduleNode)
{
    if (!IsDependEntry()) {
        return true;
    }
    cJSON *mainAbilityNode = cJSON_GetObjectItem(moduleNode, "mainAbility");
    if (mainAbilityNode && cJSON_IsString(mainAbilityNode)) {
        mainAbility_ = mainAbilityNode->valuestring;
        if (mainAbility_[0] == '.') {
            mainAbility_ = packageName_ + mainAbility_;
        }
        return ParseAbilities(cJSON_GetObjectItem(moduleNode, "abilities"));
    }
    return true;
}

bool ConfigParser::ParseDistro(cJSON *distroNode)
{
    if (!distroNode || !cJSON_IsObject(distroNode)) {
        cerr << "Error: 'distro' not object." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (cJSON_GetArraySize(distroNode) == 0) {
        cerr << "Error: 'distro' empty." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }

    cJSON *moduleNameNode = cJSON_GetObjectItem(distroNode, "moduleName");
    if (moduleNameNode && cJSON_IsString(moduleNameNode)) {
        moduleName_ = moduleNameNode->valuestring;
    }

    if (moduleName_.empty()) {
        cerr << "Error: 'moduleName' don't found in 'distro'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }

    cJSON *moduleTypeNode = cJSON_GetObjectItem(distroNode, "moduleType");
    if (moduleTypeNode && cJSON_IsString(moduleTypeNode) && !ParseModuleType(moduleTypeNode->valuestring)) {
        return false;
    }
    return true;
}

bool ConfigParser::ParseAbilities(const cJSON *abilities)
{
    if (!abilities || !cJSON_IsArray(abilities)) {
        cerr << "Error: abilites not array." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (cJSON_GetArraySize(abilities) == 0) {
        return true;
    }
    bool isMainAbility = false;
    for (cJSON *ability = abilities->child; ability; ability = ability->next) {
        if (!ParseAbilitiy(ability, isMainAbility)) {
            cerr << "Error: ParseAbilitiy fail." << endl;
            return false;
        }
        if (isMainAbility) {
            break;
        }
    }
    return true;
}

bool ConfigParser::ParseAbilitiy(const cJSON *ability, bool &isMainAbility)
{
    if (!ability || !cJSON_IsObject(ability)) {
        cerr << "Error: ability not object." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    if (cJSON_GetArraySize(ability) == 0) {
        return true;
    }
    cJSON *nameNode = cJSON_GetObjectItem(ability, "name");
    if (!nameNode || !cJSON_IsString(nameNode)) {
        return false;
    }
    string name = nameNode->valuestring;
    if (name[0] == '.') {
        name = packageName_ + name;
    }
    if (mainAbility_ != name && !IsMainAbility(cJSON_GetObjectItem(ability, "skills"))) {
        return true;
    }
    cJSON *iconIdNode = cJSON_GetObjectItem(ability, "iconId");
    if (iconIdNode && ResourceUtil::IsIntValue(iconIdNode)) {
        abilityIconId_ = iconIdNode->valueint;
    }
    if (abilityIconId_ <= 0) {
        cerr << "Error: iconId don't found in 'ability'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    cJSON *labelIdNode = cJSON_GetObjectItem(ability, "labelId");
    if (labelIdNode && ResourceUtil::IsIntValue(labelIdNode)) {
        abilityLabelId_ = labelIdNode->valueint;
    }
    if (abilityLabelId_ <= 0) {
        cerr << "Error: labelId don't found in 'ability'." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    isMainAbility = true;
    return true;
}

bool ConfigParser::IsMainAbility(const cJSON *skills)
{
    if (!skills || !cJSON_IsArray(skills)) {
        return false;
    }
    for (cJSON *skill = skills->child; skill; skill = skill->next) {
        if (!cJSON_IsObject(skill)) {
            return false;
        }
        if (IsHomeAction(cJSON_GetObjectItem(skill, "actions"))) {
            return true;
        }
    }
    return false;
}

bool ConfigParser::IsHomeAction(const cJSON *actions)
{
    if (!actions || !cJSON_IsArray(actions)) {
        return false;
    }
    for (cJSON *action = actions->child; action; action = action->next) {
        if (!cJSON_IsObject(action)) {
            return false;
        }
        if (strcmp(action->valuestring, "action.system.home") == 0) {
            return true;
        }
    }
    return false;
}

bool ConfigParser::ParseRefImpl(cJSON *parent, const string &key, cJSON *node)
{
    if (cJSON_IsArray(node)) {
        const auto &result = JSON_ARRAY_IDS.find(key);
        if (result != JSON_ARRAY_IDS.end()) {
            return ParseJsonArrayRef(parent, key, node);
        }
        cJSON *arrayItem = node->child;
        while (arrayItem) {
            if (!ParseRefImpl(node, "", arrayItem)) {
                return false;
            }
            arrayItem = arrayItem->next;
        }
    } else if (cJSON_IsObject(node)) {
        cJSON *child = node->child;
        while (child) {
            if (!ParseRefImpl(node, child->string, child)) {
                return false;
            }
            child = child->next;
        }
    } else if (!key.empty() && cJSON_IsString(node)) {
        return ParseJsonStringRef(parent, key, node);
    }
    return true;
}

bool ConfigParser::ParseJsonArrayRef(cJSON *parent, const string &key, cJSON *node)
{
    if (!node || !cJSON_IsArray(node)) {
        cerr << "Error: '"<< key << "'node not array." << NEW_LINE_PATH << filePath_ << endl;
        return false;
    }
    cJSON *array = cJSON_CreateArray();
    for (cJSON *item = node->child; item; item = item->next) {
        if (!cJSON_IsString(item)) {
            cerr << "Error: '" << key << "' invalid value." << NEW_LINE_PATH << filePath_ << endl;
            cJSON_Delete(array);
            return false;
        }
        string value = item->valuestring;
        bool update = false;
        if (!GetRefIdFromString(value, update, JSON_ARRAY_IDS.at(key))) {
            cerr << "Error: '" << key << "' value " << value << " invalid." << NEW_LINE_PATH << filePath_ << endl;
            cJSON_Delete(array);
            return false;
        }
        if (update) {
            cJSON_AddItemToArray(array, cJSON_CreateNumber(atoll(value.c_str())));
        }
    }
    cJSON_AddItemToObject(parent, (key + "Id").c_str(), array);
    return true;
}

bool ConfigParser::ParseJsonStringRef(cJSON *parent, const string &key, cJSON *node)
{
    const auto &result = JSON_STRING_IDS.find(key);
    if (result == JSON_STRING_IDS.end()) {
        return true;
    }
    if (!node || !cJSON_IsString(node)) {
        cerr << "Error: '" << key << "' invalid value." << endl;
        return false;
    }
    string value = node->valuestring;
    bool update = false;
    if (!GetRefIdFromString(value, update, JSON_STRING_IDS.at(key))) {
        cerr << "Error: '" << key << "' value " << value << " invalid value." << NEW_LINE_PATH << filePath_ << endl;
        cerr << SOLUTIONS << endl;
        cerr << SOLUTIONS_ARROW << "Please check the module.json5/config.json file in the src/main directory of the "
             << GetModuleName() << "' module." << endl;
        return false;
    }
    if (update) {
        cJSON_AddItemToObject(parent, (key + "Id").c_str(), cJSON_CreateNumber(atoll(value.c_str())));
        AddCheckNode(key, static_cast<uint32_t>(atoll(value.c_str())));
    }
    return true;
}

void ConfigParser::AddCheckNode(const string &key, uint32_t id)
{
    if (g_keyNodeIndexs.find(key) != g_keyNodeIndexs.end()) {
        auto result = jsonCheckIds_.find(key);
        if (result == jsonCheckIds_.end()) {
            set<uint32_t> set;
            set.emplace(id);
            jsonCheckIds_.emplace(key, set);
        } else {
            result->second.emplace(id);
        }
        auto layerIconIds = ReferenceParser::GetLayerIconIds();
        if (layerIconIds.find(id) != layerIconIds.end()) {
            auto ids = layerIconIds[id];
            jsonCheckIds_[key].insert(ids.begin(), ids.end());
        }
    }
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
