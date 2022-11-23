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

#include "solid_xml.h"
#include <iostream>
#include <regex>
#include "securec.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const uint8_t SolidXml::SOLID_XML_MAGIC[] = "SolidXml";
const uint32_t SolidXml::SOLID_XML_MAGIC_LENGTH =
    (sizeof(SolidXml::SOLID_XML_MAGIC) / sizeof(uint32_t) + 1) * sizeof(uint32_t);

SolidXml::SolidXml(const string &xmlPath, map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &keys)
    : xmlPath_(xmlPath), keys_(keys)
{
}

SolidXml::~SolidXml()
{
}

bool SolidXml::GenerateSolidXml(const string &filePath)
{
    xmlKeepBlanksDefault(0);
    xmlDocPtr doc = xmlParseFile(xmlPath_.c_str());
    if (doc == nullptr) {
        return false;
    }

    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    if (rootNode == nullptr) {
        xmlFreeDoc(doc);
        return false;
    }
    auto root = make_shared<XmlNode>();
    nodes_.push_back(root);
    Compile(rootNode, root);
    xmlFreeDoc(doc);
    return SaveToFile(filePath);
}

bool SolidXml::FlushNodeKeys(const string &filePath, map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &newKeys)
{
    if (!LoadFromFile(xmlPath_)) {
        cerr << "Error: load " << xmlPath_ << " fail." << endl;
        return false;
    }

    bool firstNode = true;
    for (auto &node : nodes_) {
        if (!node->FlushIndex(keys_, newKeys)) {
            cerr << "Error: flush node key fail, '" << xmlPath_ << "'" << endl;
            return false;
        }
        if (firstNode && !FlushXmlnsKey(newKeys)) {
            return false;
        }
        firstNode = false;
    }

    for (auto &attr : attributes_) {
        if (!attr->FlushIndex(keys_, newKeys)) {
            cerr << "Error: flush attibutes key fail, '" << xmlPath_ << "'" << endl;
            return false;
        }
    }
    return SaveToFile(filePath);
}

// below private
void SolidXml::Compile(const xmlNodePtr nodePtr, shared_ptr<XmlNode> &node)
{
    if (nodePtr->type == XML_COMMENT_NODE) {
        if (nodePtr->next) {
            Compile(nodePtr->next, node);
        }
        return;
    }

    string name(reinterpret_cast<const char *>(nodePtr->name));
    node->SetName(keys_[XmlKeyNode::KeyType::NODE]->PushKey(name));
    CompileNameSpace(nodePtr, node);
    CompileAttr(nodePtr->properties, node);

    if (nodePtr->children) {
        if (nodePtr->children->type == XML_TEXT_NODE) {
            string content(reinterpret_cast<const char *>(nodePtr->children->content));
            node->SetValue(keys_[XmlKeyNode::KeyType::CONTENT]->PushKey(content));
        } else {
            auto child = make_shared<XmlNode>();
            nodes_.push_back(child);
            node->SetChild(nodes_.size() - 1);
            Compile(nodePtr->children, child);
        }
    }

    if (nodePtr->next) {
        auto brother = make_shared<XmlNode>();
        nodes_.push_back(brother);
        node->SetBrother(nodes_.size() - 1);
        Compile(nodePtr->next, brother);
    }
}

void SolidXml::CompileAttr(const xmlAttrPtr attrPtr, shared_ptr<XmlNode> &node)
{
    if (attrPtr == nullptr) {
        return;
    }

    xmlChar *xmlValue = xmlNodeListGetString(attrPtr->doc, attrPtr->children, 1);
    string value(reinterpret_cast<const char *>(xmlValue));
    PretreatmentAttr(value);
    xmlFree(xmlValue);
    string name(reinterpret_cast<const char *>(attrPtr->name));
    auto attr = make_shared<Node>();
    attributes_.push_back(attr);
    attr->SetName(keys_[XmlKeyNode::KeyType::ATTRIBUTE]->PushKey(name));
    attr->SetValue(keys_[XmlKeyNode::KeyType::CONSTANT]->PushKey(value));
    if (attrPtr->ns != nullptr && attrPtr->ns->prefix != nullptr) {
        string nameSpace(reinterpret_cast<const char *>(attrPtr->ns->prefix));
        attr->SetNameSpace(keys_[XmlKeyNode::KeyType::NODE]->PushKey(nameSpace));
    }
    node->AddAttribute(attributes_.size() - 1);
    CompileAttr(attrPtr->next, node);
}

void SolidXml::CompileNameSpace(const xmlNodePtr nodePtr, shared_ptr<XmlNode> &node)
{
    if (nodePtr->ns && nodePtr->ns->prefix) {
        string nameSpace(reinterpret_cast<const char *>(nodePtr->ns->prefix));
        node->SetNameSpace(keys_[XmlKeyNode::KeyType::NODE]->PushKey(nameSpace));
    }

    auto nsDef = nodePtr->nsDef;
    while (nsDef) {
        string nameSpace;
        string href;
        if (nsDef->prefix) {
            nameSpace = string(reinterpret_cast<const char *>(nsDef->prefix));
        }
        if (nsDef->href) {
            href = string(reinterpret_cast<const char *>(nsDef->href));
        }
        int32_t nameSpaceIndex = keys_[XmlKeyNode::KeyType::NODE]->PushKey(nameSpace);
        int32_t herfIndex = keys_[XmlKeyNode::KeyType::NODE]->PushKey(href);
        AddNampeSpaceDef(nameSpaceIndex, herfIndex);
        nsDef = nsDef->next;
    }
}

void SolidXml::Node::RawData(ofstream &out) const
{
    out.write(reinterpret_cast<const char *>(&nameSpace_), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(&name_), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(&value_), sizeof(int32_t));
}

void SolidXml::XmlNode::RawData(ofstream &out) const
{
    SolidXml::Node::RawData(out);
    out.write(reinterpret_cast<const char *>(&child_), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(&brother_), sizeof(int32_t));
    int32_t attrStart = attributes_.size();
    int32_t attrCount = attributes_.size();
    if (!attributes_.empty()) {
        attrStart = attributes_.at(0);
    }
    out.write(reinterpret_cast<const char *>(&attrStart), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(&attrCount), sizeof(int32_t));
}

bool SolidXml::Node::LoadFrom(ifstream &in)
{
    CHECK_IO(in.read(reinterpret_cast<char *>(&nameSpace_), sizeof(int32_t)))
    CHECK_IO(in.read(reinterpret_cast<char *>(&name_), sizeof(int32_t)));
    CHECK_IO(in.read(reinterpret_cast<char *>(&value_), sizeof(int32_t)));
    return true;
}

bool SolidXml::XmlNode::LoadFrom(ifstream &in)
{
    if (!SolidXml::Node::LoadFrom(in)) {
        return false;
    }
    CHECK_IO(in.read(reinterpret_cast<char *>(&child_), sizeof(int32_t)));
    CHECK_IO(in.read(reinterpret_cast<char *>(&brother_), sizeof(int32_t)));
    int32_t attrStart = 0;
    int32_t attrCount = 0;
    CHECK_IO(in.read(reinterpret_cast<char *>(&attrStart), sizeof(int32_t)));
    CHECK_IO(in.read(reinterpret_cast<char *>(&attrCount), sizeof(int32_t)));
    for (int32_t i = attrStart; i < attrStart + attrCount; i++) {
        attributes_.push_back(i);
    }
    return true;
}

bool SolidXml::Node::FlushIndex(const map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &oldKeys,
    map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &newKeys)
{
    if (!ChangeToNewKey(oldKeys, newKeys, XmlKeyNode::KeyType::NODE, nameSpace_) ||
        !ChangeToNewKey(oldKeys, newKeys, XmlKeyNode::KeyType::ATTRIBUTE, name_) ||
        !ChangeToNewKey(oldKeys, newKeys, XmlKeyNode::KeyType::CONSTANT, value_)) {
        return false;
    }
    return true;
}

bool SolidXml::XmlNode::FlushIndex(const map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &oldKeys,
    map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &newKeys)
{
    if (!ChangeToNewKey(oldKeys, newKeys, XmlKeyNode::KeyType::NODE, nameSpace_) ||
        !ChangeToNewKey(oldKeys, newKeys, XmlKeyNode::KeyType::NODE, name_) ||
        !ChangeToNewKey(oldKeys, newKeys, XmlKeyNode::KeyType::CONTENT, value_)) {
        return false;
    }
    return true;
}

void SolidXml::AddNampeSpaceDef(int32_t nameSpace, int32_t href)
{
    nameSpaces_.push_back(nameSpace);
    hrefs_.push_back(href);
}

bool SolidXml::SaveToFile(const std::string &filePath) const
{
    ofstream out(filePath, ofstream::out | ofstream::binary);
    if (!out.is_open()) {
        cerr << "Error: open failed '" << filePath << "', reason: " << strerror(errno) << endl;
        return false;
    }

    // write solid xml header
    uint8_t magic[SOLID_XML_MAGIC_LENGTH] = {0};
    if (memcpy_s(magic, SOLID_XML_MAGIC_LENGTH, SOLID_XML_MAGIC, sizeof(SOLID_XML_MAGIC)) != EOK) {
        cerr << "Error: SolidXml::SaveToFile memcpy_s fail." << endl;
        return false;
    }
    uint32_t version = 1;
    uint32_t numOfNodes = nodes_.size();
    uint32_t numOfAttributes = attributes_.size();
    uint32_t numOfNameSpaces = nameSpaces_.size();
    out.write(reinterpret_cast<const char *>(magic), SOLID_XML_MAGIC_LENGTH);
    out.write(reinterpret_cast<const char *>(&version), sizeof(uint32_t));
    out.write(reinterpret_cast<const char *>(&numOfNodes), sizeof(uint32_t));
    out.write(reinterpret_cast<const char *>(&numOfAttributes), sizeof(uint32_t));
    out.write(reinterpret_cast<const char *>(&numOfNameSpaces), sizeof(uint32_t));

    // write node
    for (const auto &node : nodes_) {
        node->RawData(out);
    }

    // write attribute
    for (const auto &attribute : attributes_) {
        attribute->RawData(out);
    }

    // write namespace
    for (const auto &nameSpace : nameSpaces_) {
        out.write(reinterpret_cast<const char *>(&nameSpace), sizeof(int32_t));
    }

    // write href
    for (const auto &href : hrefs_) {
        out.write(reinterpret_cast<const char *>(&href), sizeof(int32_t));
    }
    return true;
}

void SolidXml::PretreatmentAttr(string &value) const
{
    regex ref("^\\$\\+id:");
    smatch result;
    if (!regex_search(value, result, ref)) {
        return;
    }
    value.replace(0, result[0].str().length(), "$id:");
}

bool SolidXml::LoadFromFile(const string &sxmlPath)
{
    ifstream in(sxmlPath, ifstream::in | ifstream::binary);
    if (!in.is_open()) {
        cerr << "Error: open failed '" << sxmlPath << "', reason: " << strerror(errno) << endl;
        return false;
    }

    char header[SOLID_XML_MAGIC_LENGTH];
    CHECK_IO(in.read(header, SOLID_XML_MAGIC_LENGTH));

    uint32_t version = 0;
    CHECK_IO(in.read(reinterpret_cast<char *>(&version), sizeof(uint32_t)));

    uint32_t numOfNodes = 0;
    CHECK_IO(in.read(reinterpret_cast<char *>(&numOfNodes), sizeof(uint32_t)));

    uint32_t numOfAttributes = 0;
    CHECK_IO(in.read(reinterpret_cast<char *>(&numOfAttributes), sizeof(uint32_t)));

    uint32_t numOfNameSpaces = 0;
    CHECK_IO(in.read(reinterpret_cast<char *>(&numOfNameSpaces), sizeof(uint32_t)));

    for (uint32_t i = 0; i < numOfNodes; i++) {
        auto xmlNode = make_shared<XmlNode>();
        if (!xmlNode->LoadFrom(in)) {
            return false;
        }
        nodes_.push_back(xmlNode);
    }

    for (uint32_t i = 0; i < numOfAttributes; i++) {
        auto attr = make_shared<Node>();
        if (!attr->LoadFrom(in)) {
            return false;
        }
        attributes_.push_back(attr);
    }

    for (uint32_t i = 0; i < numOfNameSpaces; i++) {
        int32_t nameSpace = -1;
        CHECK_IO(in.read(reinterpret_cast<char *>(&nameSpace), sizeof(int32_t)));
        nameSpaces_.push_back(nameSpace);
    }

    for (uint32_t i = 0; i < numOfNameSpaces; i++) {
        int32_t href = -1;
        CHECK_IO(in.read(reinterpret_cast<char *>(&href), sizeof(int32_t)));
        hrefs_.push_back(href);
    }
    return true;
}

bool SolidXml::ChangeToNewKey(const map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &oldKeys,
    map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &newKeys,
    XmlKeyNode::KeyType keyType, int32_t &keyId)
{
    if (keyId <= 0) {
        return true;
    }

    string value;
    if (!oldKeys.at(keyType)->GetKeyValue(keyId, value)) {
        return false;
    }

    keyId = newKeys[keyType]->PushKey(value);
    return true;
}

bool SolidXml::FlushXmlnsKey(map<XmlKeyNode::KeyType, shared_ptr<XmlKeyNode>> &newKeys)
{
    if (nameSpaces_.size() != hrefs_.size()) {
        return false;
    }

    for (uint32_t i = 0; i < nameSpaces_.size(); i++) {
        if (!ChangeToNewKey(keys_, newKeys, XmlKeyNode::KeyType::NODE, nameSpaces_[i]) ||
            !ChangeToNewKey(keys_, newKeys, XmlKeyNode::KeyType::NODE, hrefs_[i])) {
            cerr << "Error: flush namespace key fail, '" << xmlPath_ << "'" << endl;
            return false;
        }
    }
    return true;
}
}
}
}
