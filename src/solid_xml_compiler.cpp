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

#include "solid_xml_compiler.h"
#include<iostream>
#include<regex>
#include "resource_util.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
SolidXmlCompiler::SolidXmlCompiler(ResType type, const string &output)
    : GenericCompiler(type, output)
{
}

SolidXmlCompiler::~SolidXmlCompiler()
{
}

uint32_t SolidXmlCompiler::CompileSingleFile(const FileInfo &fileInfo)
{
    if (!IsXmlFile(fileInfo)) {
        cerr << "Error: '" << fileInfo.filePath << "' should be xml file." << endl;
        return RESTOOL_ERROR;
    }

    if (HasConvertedToSolidXml(fileInfo)) {
        return RESTOOL_SUCCESS;
    }

    if (!PostFile(fileInfo)) {
        return RESTOOL_ERROR;
    }

    if (!ParseXml(fileInfo)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

bool SolidXmlCompiler::ParseXml(const FileInfo &fileInfo)
{
    xmlKeepBlanksDefault(0);
    xmlDocPtr doc = xmlParseFile(fileInfo.filePath.c_str());
    if (doc == nullptr) {
        return false;
    }

    vector<string> ids;
    bool result = ParseNodeId(fileInfo, xmlDocGetRootElement(doc), ids);
    xmlFreeDoc(doc);
    if (!result) {
        return false;
    }

    for (const auto &id : ids) {
        ResourceItem resourceItem(id, fileInfo.keyParams, ResType::ID);
        resourceItem.SetFilePath(fileInfo.filePath);
        resourceItem.SetLimitKey(fileInfo.limitKey);
        if (!MergeResourceItem(resourceItem)) {
            return false;
        }
    }
    return true;
}

bool SolidXmlCompiler::ParseNodeId(const FileInfo &fileInfo, const xmlNodePtr &node, vector<string> &ids)
{
    if (node == nullptr) {
        return true;
    }

    if (node->type == XML_COMMENT_NODE) {
        return ParseNodeId(fileInfo, node->next, ids);
    }

    string idValue;
    xmlChar * xmlValue = xmlGetProp(node, BAD_CAST "id");
    if (xmlValue != nullptr) {
        idValue = string(reinterpret_cast<const char *>(xmlValue));
        xmlFree(xmlValue);
    }

    regex ref("^\\$\\+id:");
    smatch result;
    if (regex_search(idValue, result, ref)) {
        string name = idValue.substr(result[0].str().size());
        if (find(ids.begin(), ids.end(), name) != ids.end()) {
            cerr << "Error: '" << idValue << "' duplicated in " << fileInfo.filePath << endl;
            return false;
        }
        ids.push_back(name);
    }

    if (!ParseNodeId(fileInfo, node->children, ids)) {
        return false;
    }

    if (!ParseNodeId(fileInfo, node->next, ids)) {
        return false;
    }
    return true;
}
}
}
}
