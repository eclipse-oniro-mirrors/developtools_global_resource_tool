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

#ifndef OHOS_RESTOOL_XML_CONVERTER_H
#define OHOS_RESTOOL_XML_CONVERTER_H

#include<memory>
#include<vector>
#include<string>
#include "key_manager.h"

namespace OHOS {
namespace Global {
namespace Restool {
class XmlConverter {
public:
    XmlConverter(const std::vector<std::string> &xmlPaths, const std::string &outputPath);
    virtual ~XmlConverter();
    bool GenerateSolidXml();
    bool GenerateKey();
private:
    bool LoadKeys(const std::string &folderPath);
    const std::vector<std::string> &xmlPaths_;
    const std::string &outputPath_;
    KeyManager keyManager;
};
}
}
}
#endif