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

#ifndef OHOS_RESTOOL_CONFIG_PARSER_H
#define OHOS_RESTOOL_CONFIG_PARSER_H

#include<functional>
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ConfigParser {
public:
    enum class ModuleType {
        NONE = 0,
        HAR = 1,
        ENTRY = 2,
        FEATURE = 3,
    };

    ConfigParser();
    explicit ConfigParser(const std::string &filePath);
    virtual ~ConfigParser();
    uint32_t Init();
    const std::string &GetPackageName() const;
    const std::string &GetModuleName() const;
    ModuleType GetModuleType() const;
    uint32_t ParseRefence();
    uint32_t Save(const std::string &filePath) const;
    static void SetUseModule()
    {
        useModule_ = true;
    };
    static std::string GetConfigName()
    {
        return useModule_ ? MODULE_JSON : CONFIG_JSON;
    };
private:
    bool ParseModule(Json::Value &moduleNode);
    bool ParseDistro(Json::Value &distroNode);
    bool ParseRefImpl(Json::Value &parent, const std::string &key, Json::Value &node);
    bool ParseJsonArrayRef(Json::Value &parent, const std::string &key, Json::Value &node);
    bool ParseJsonStringRef(Json::Value &parent, const std::string &key, Json::Value &node);
    bool GetRefIdFromString(std::string &value, bool &update, const std::string &match) const;
    bool ParseModuleType(const std::string &type);
    std::string filePath_;
    std::string packageName_;
    std::string moduleName_;
    ModuleType moduleType_;
    Json::Value rootNode_;
    static const std::map<std::string, ModuleType> MODULE_TYPES;
    static const std::map<std::string, std::string> JSON_STRING_IDS;
    static const std::map<std::string, std::string> JSON_ARRAY_IDS;
    static bool useModule_;
};
}
}
}
#endif