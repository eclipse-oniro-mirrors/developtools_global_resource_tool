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

#include "xml_key_node.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const map<XmlKeyNode::KeyType, string> XmlKeyNode::KEY_TO_FILE_NAME = {
    {KeyType::NODE, "nodes.key" },
    {KeyType::ATTRIBUTE, "attributes.key" },
    {KeyType::CONSTANT, "constants.key" },
    {KeyType::CONTENT, "contents.key" }
};
XmlKeyNode::XmlKeyNode() : keyId_(0)
{
    PushKey("");
}

XmlKeyNode::~XmlKeyNode()
{
}

int32_t XmlKeyNode::PushKey(const string &name)
{
    auto result = keyMap_.emplace(name, keyId_);
    if (result.second) {
        return keyId_++;
    }
    return result.first->second;
}

bool XmlKeyNode::SaveToFile(const string &filePath) const
{
    ofstream out(filePath, ofstream::out | ofstream::binary);
    if (!out.is_open()) {
        cerr << "Error: open failed '" << filePath << "', reason: " << strerror(errno) << endl;
        return false;
    }

    vector<pair<string, int32_t>> sets(keyMap_.begin(), keyMap_.end());
    sort(sets.begin(), sets.end(), [](const auto &a, const auto &b) {
        return a.second < b.second;
    });

    char null = 0;
    for (const auto &iter : sets) {
        out.write(reinterpret_cast<const char *>(iter.first.c_str()), iter.first.length());
        out.write(&null, sizeof(char));
    }
    return true;
}

bool XmlKeyNode::LoadFromFile(const string &filePath)
{
    return LoadFromFile(filePath, nullptr);
}

bool XmlKeyNode::LoadFromFile(const std::string &filePath, RefParser parser)
{
    ifstream in(filePath, ifstream::in | ifstream::binary);
    if (!in.is_open()) {
        cerr << "Error: open failed '" << filePath << "', reason: " << strerror(errno) << endl;
        return false;
    }

    string inputLine;
    getline(in, inputLine);
    if (inputLine.empty()) {
        return true;
    }

    string::size_type offset = 0;
    while (offset < inputLine.length()) {
        string item(inputLine.c_str() + offset);
        offset = offset + item.length() + sizeof(char);
        if (parser && !parser(item)) {
            return false;
        }
        PushKey(item);
    }
    return true;
}

bool XmlKeyNode::GetKeyValue(int32_t keyId, std::string &value) const
{
    auto result = find_if(keyMap_.begin(), keyMap_.end(), [&keyId](const auto &iter) {
        return keyId == iter.second;
    });
    if (result == keyMap_.end()) {
        return false;
    }
    value = result->first;
    return true;
}
}
}
}