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

#include "key_manager.h"
#include "file_entry.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
KeyManager::KeyManager()
{
    keys_.emplace(XmlKeyNode::KeyType::NODE, make_shared<XmlKeyNode>());
    keys_.emplace(XmlKeyNode::KeyType::ATTRIBUTE, make_shared<XmlKeyNode>());
    keys_.emplace(XmlKeyNode::KeyType::CONSTANT, make_shared<XmlKeyNode>());
    keys_.emplace(XmlKeyNode::KeyType::CONTENT, make_shared<XmlKeyNode>());
}

bool KeyManager::LoadKey(const string &keysPath)
{
    for (auto &key : keys_) {
        string keyPath = FileEntry::FilePath(keysPath).Append(XmlKeyNode::KEY_TO_FILE_NAME.at(key.first)).GetPath();
        if (!ResourceUtil::FileExist(keyPath)) {
            continue;
        }
        if (!key.second->LoadFromFile(keyPath)) {
            return false;
        }
    }
    return true;
}

bool KeyManager::SaveKey(const string &keysPath)
{
    for (const auto &iter : keys_) {
        string keyPath = FileEntry::FilePath(keysPath).Append(XmlKeyNode::KEY_TO_FILE_NAME.at(iter.first)).GetPath();
        if (!iter.second->SaveToFile(keyPath)) {
            return false;
        }
    }
    return true;
}
}
}
}