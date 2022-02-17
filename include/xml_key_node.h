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

#ifndef OHOS_RESTOOL_XML_KEY_NODE_H
#define OHOS_RESTOOL_XML_KEY_NODE_H

#include<functional>
#include<map>
#include<stdint.h>
#include<string>

namespace OHOS {
namespace Global {
namespace Restool {
class XmlKeyNode {
public:
    XmlKeyNode();
    virtual ~XmlKeyNode();
    int32_t PushKey(const std::string &name);
    bool SaveToFile(const std::string &filePath) const;
    bool LoadFromFile(const std::string &filePath);
    using RefParser = std::function<bool(std::string&)>;
    bool LoadFromFile(const std::string &filePath, RefParser parser);
    bool GetKeyValue(int32_t keyId, std::string &value) const;

    enum class KeyType {
        NODE = 0,
        ATTRIBUTE,
        CONSTANT,
        CONTENT,
        END
    };
    static const std::map<KeyType, std::string> KEY_TO_FILE_NAME;
private:
    int32_t keyId_;
    std::map<std::string, int32_t> keyMap_;
};
}
}
}
#endif