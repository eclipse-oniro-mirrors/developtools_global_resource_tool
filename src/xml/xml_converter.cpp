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

#include "xml_converter.h"
#include "file_entry.h"
#include "resource_util.h"
#include "solid_xml.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
XmlConverter::XmlConverter(const vector<string> &xmlPaths, const string &outputPath)
    : xmlPaths_(xmlPaths), outputPath_(outputPath)
{
}

XmlConverter::~XmlConverter()
{
}

bool XmlConverter::GenerateSolidXml()
{
    if (!LoadKeys(outputPath_)) {
        return false;
    }
    for (const auto &xmlPath : xmlPaths_) {
        SolidXml solidXml(xmlPath, keyManager.GetKeys());
        string filename = FileEntry::FilePath(xmlPath).GetFilename();
        string filePath = FileEntry::FilePath(outputPath_).Append(filename).ReplaceExtension(".sxml").GetPath();
        if (!solidXml.GenerateSolidXml(filePath)) {
            return false;
        }
    }
    return true;
}

bool XmlConverter::GenerateKey()
{
    return keyManager.SaveKey(outputPath_);
}

bool XmlConverter::LoadKeys(const std::string &folderPath)
{
    return keyManager.LoadKey(folderPath);
}
}
}
}
