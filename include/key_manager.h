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

#ifndef OHOS_RESTOOL_KEY_MANAGER_H
#define OHOS_RESTOOL_KEY_MANAGER_H

#include<memory>
#include "xml_key_node.h"

namespace OHOS {
namespace Global {
namespace Restool {
class KeyManager {
public:
    KeyManager();
    virtual ~KeyManager() {};
    bool LoadKey(const std::string &keysPath);
    bool SaveKey(const std::string &keysPath);
    std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &GetKeys()
    {
        return keys_;
    };
private:
    std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> keys_;
};
}
}
}
#endif