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

#ifndef OHOS_RESTOOL_SOLID_XML_H
#define OHOS_RESTOOL_SOLID_XML_H

#include<fstream>
#include<memory>
#include<string>
#include<vector>
#include "libxml/parser.h"
#include "xml_key_node.h"

namespace OHOS {
namespace Global {
namespace Restool {
class SolidXml {
public:
    explicit SolidXml(const std::string &xmlPath, std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &keys);
    virtual ~SolidXml();
    bool GenerateSolidXml(const std::string &filePath);
    bool FlushNodeKeys(const std::string &filePath,
        std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &newKeys);
private:
    class Node {
    public:
        Node(): name_(-1), value_(-1), nameSpace_(-1) {};
        virtual ~Node() {};
        virtual void RawData(std::ofstream &out) const;
        virtual bool LoadFrom(std::ifstream &in);
        virtual bool FlushIndex(const std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &oldKeys,
            std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &newKeys);
        void SetName(int32_t index)
        {
            name_ = index;
        };
        void SetValue(int32_t index)
        {
            value_ = index;
        };
        void SetNameSpace(int32_t index)
        {
            nameSpace_ = index;
        };
    protected:
        int32_t name_;
        int32_t value_;
        int32_t nameSpace_;
    };

    class XmlNode : public Node {
    public:
        XmlNode() : child_(-1), brother_(-1) {};
        virtual ~XmlNode() {};
        void RawData(std::ofstream &out) const override;
        bool LoadFrom(std::ifstream &in) override;
        bool FlushIndex(const std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &oldKeys,
            std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &newKeys) override;
        void SetChild(int32_t index)
        {
            child_ = index;
        };
        void SetBrother(int32_t index)
        {
            brother_ = index;
        };
        void AddAttribute(int32_t index)
        {
            attributes_.push_back(index);
        };
    private:
        int32_t child_;
        int32_t brother_;
        std::vector<int32_t> attributes_;
    };

    void Compile(const xmlNodePtr nodePtr, std::shared_ptr<XmlNode> &node);
    void CompileAttr(const xmlAttrPtr attrPtr, std::shared_ptr<XmlNode> &node);
    void CompileNameSpace(const xmlNodePtr nodePtr, std::shared_ptr<XmlNode> &node);
    void AddNampeSpaceDef(int32_t nameSpace, int32_t href);
    bool SaveToFile(const std::string &filePath) const;
    void PretreatmentAttr(std::string &value) const;
    bool LoadFromFile(const std::string &sxmlPath);
    static bool ChangeToNewKey(const std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &oldKeys,
        std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &newKeys,
        XmlKeyNode::KeyType keyType, int32_t &keyId);
    bool FlushXmlnsKey(std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &newKeys);
    const std::string &xmlPath_;
    std::map<XmlKeyNode::KeyType, std::shared_ptr<XmlKeyNode>> &keys_;
    std::vector<std::shared_ptr<Node>> attributes_;
    std::vector<std::shared_ptr<XmlNode>> nodes_;
    std::vector<int32_t> nameSpaces_;
    std::vector<int32_t> hrefs_;

    static const uint8_t SOLID_XML_MAGIC[];
    static const uint32_t SOLID_XML_MAGIC_LENGTH;
    #define CHECK_IO(stream) \
        if (!stream) { \
            return false; \
        }
};
}
}
}
#endif