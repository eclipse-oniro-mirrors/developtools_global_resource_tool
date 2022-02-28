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

#ifndef OHOS_RESTOOL_SOLID_XML_COMPILER_H
#define OHOS_RESTOOL_SOLID_XML_COMPILER_H

#include "generic_compiler.h"
#include "libxml/parser.h"

namespace OHOS {
namespace Global {
namespace Restool {
class SolidXmlCompiler : public GenericCompiler {
public:
    SolidXmlCompiler(ResType type, const std::string &output);
    virtual ~SolidXmlCompiler();
protected:
    uint32_t CompileSingleFile(const FileInfo &fileInfo) override;
private:
    bool ParseXml(const FileInfo &fileInfo);
    bool ParseNodeId(const FileInfo &fileInfo, const xmlNodePtr &node, std::vector<std::string> &ids);
};
}
}
}
#endif