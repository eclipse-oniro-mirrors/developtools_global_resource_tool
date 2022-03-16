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

#include "module_combine.h"
#include<algorithm>
#include<iostream>
#include "file_entry.h"
#include "key_manager.h"
#include "resource_util.h"
#include "solid_xml.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
ModuleCombine::ModuleCombine(const string &modulePath, const string &outputPath)
    : modulePath_(modulePath), outputPath_(outputPath)
{
}

bool ModuleCombine::Combine()
{
    string resourceDir = FileEntry::FilePath(modulePath_).Append(RESOURCES_DIR).GetPath();
    ResourceDirectory resourceDirectory;
    if (!resourceDirectory.ScanResources(resourceDir, [this](const DirectoryInfo &info) -> bool {
        return CombineDirectory(info);
    })) {
        return false;
    }
    return true;
}

// below private
bool ModuleCombine::CombineDirectory(const DirectoryInfo &directoryInfo)
{
    string outputFolder = FileEntry::FilePath(outputPath_).Append(RESOURCES_DIR)
        .Append(directoryInfo.limitKey).Append(directoryInfo.fileCluster).GetPath();
    if (!ResourceUtil::CreateDirs(outputFolder)) {
        return false;
    }

    map<string, string> sxmlPaths;
    FileEntry f(directoryInfo.dirPath);
    if (!f.Init()) {
        return false;
    }
    for (const auto &entry : f.GetChilds()) {
        string src = entry->GetFilePath().GetPath();
        if (!entry->IsFile()) {
            cerr << "Error: " << src << " is directory." << endl;
            return false;
        }

        string filename = entry->GetFilePath().GetFilename();
        string dst = FileEntry::FilePath(outputFolder).Append(filename).GetPath();
        if (ResourceUtil::FileExist(dst)) {
            continue;
        }

        if (entry->GetFilePath().GetExtension() == ".sxml") {
            sxmlPaths.emplace(src, dst);
            continue;
        }

        auto result = find_if (XmlKeyNode::KEY_TO_FILE_NAME.begin(), XmlKeyNode::KEY_TO_FILE_NAME.end(),
            [&filename](const auto &iter) {
                return filename == iter.second;
            });
        if (result != XmlKeyNode::KEY_TO_FILE_NAME.end()) {
            continue;
        }

        if (!ResourceUtil::CopyFleInner(src, dst)) {
            return true;
        }
    }
    return CombineSolidXml(directoryInfo.dirPath, outputFolder, sxmlPaths);
}

bool ModuleCombine::CombineSolidXml(const string &src, const string &dst, const map<string, string> &sxmlPaths)
{
    if (sxmlPaths.empty()) {
        return true;
    }

    KeyManager oldKeyManager;
    if (!oldKeyManager.LoadKey(src)) {
        return false;
    }

    KeyManager newKeyManager;
    if (!newKeyManager.LoadKey(dst)) {
        return false;
    }

    for (const auto &sxmlPath : sxmlPaths) {
        SolidXml solidXml(sxmlPath.first, oldKeyManager.GetKeys());
        if (!solidXml.FlushNodeKeys(sxmlPath.second, newKeyManager.GetKeys())) {
            return false;
        }
    }

    if (!newKeyManager.SaveKey(dst)) {
        cerr << "Error: GenerateToOther " << modulePath_ << " fail." << endl;
        return false;
    }
    return true;
}
}
}
}
