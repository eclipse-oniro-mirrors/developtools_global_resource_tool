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

#ifndef OHOS_RESTOOL_JSON_COMPILER_H
#define OHOS_RESTOOL_JSON_COMPILER_H

#include <functional>
#include "i_resource_compiler.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
class JsonCompiler : public IResourceCompiler {
public:
    JsonCompiler(ResType type, const std::string &output);
    virtual ~JsonCompiler();
protected:
    uint32_t CompileSingleFile(const FileInfo &fileInfo) override;
private:
    void InitParser();
    bool ParseJsonArrayLevel(const Json::Value &arrayNode, const FileInfo &fileInfo);
    bool ParseJsonObjectLevel(const Json::Value &objectNode, const FileInfo &fileInfo);

    using HandleResource = std::function<bool(const Json::Value&, ResourceItem&)>;
    bool HandleString(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandleInteger(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandleBoolean(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandleColor(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandleFloat(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandleStringArray(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandleIntegerArray(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandleTheme(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandlePattern(const Json::Value &objectNode, ResourceItem &resourceItem) const;
    bool HandlePlural(const Json::Value &objectNode, ResourceItem &resourceItem) const;

    bool PushString(const std::string &value, ResourceItem &resourceItem) const;
    bool CheckJsonStringValue(const Json::Value &valueNode, const ResourceItem &resourceItem) const;
    bool CheckJsonIntegerValue(const Json::Value &valueNode, const ResourceItem &resourceItem) const;
    using HandleValue = std::function<bool(const Json::Value&, const ResourceItem&, std::vector<std::string>&)>;
    bool ParseValueArray(const Json::Value &objectNode, ResourceItem &resourceItem,
                         const std::vector<std::string> &extra, HandleValue callback) const;
    bool ParseParent(const Json::Value &objectNode, const ResourceItem &resourceItem,
                     std::vector<std::string> &extra) const;
    bool ParseAttribute(const Json::Value &arrayItem, const ResourceItem &resourceItem,
                        std::vector<std::string> &values) const;
    bool CheckPluralValue(const Json::Value &arrayItem, const ResourceItem &resourceItem) const;
    bool CheckColorValue(const char *s) const;
    std::map<ResType, HandleResource> handles_;
    static const std::string TAG_NAME;
    static const std::string TAG_VALUE;
    static const std::string TAG_PARENT;
    static const std::string TAG_QUANTITY;
    static const std::vector<std::string> QUANTITY_ATTRS;
};
}
}
}
#endif