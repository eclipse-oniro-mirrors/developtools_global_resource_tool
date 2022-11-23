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

#include "generic_compiler.h"
#include<iostream>
#include "file_entry.h"
#include "resource_util.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
GenericCompiler::GenericCompiler(ResType type, const string &output)
    : IResourceCompiler(type, output)
{
}
GenericCompiler::~GenericCompiler()
{
}

uint32_t GenericCompiler::CompileSingleFile(const FileInfo &fileInfo)
{
    if (IsIgnore(fileInfo)) {
        return RESTOOL_SUCCESS;
    }

    if (!CopyFile(fileInfo)) {
        return RESTOOL_ERROR;
    }

    if (!PostFile(fileInfo)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

bool GenericCompiler::PostFile(const FileInfo &fileInfo)
{
    ResourceItem resourceItem(fileInfo.filename, fileInfo.keyParams, type_);
    resourceItem.SetFilePath(fileInfo.filePath);
    resourceItem.SetLimitKey(fileInfo.limitKey);

    string data = fileInfo.filename;
    if (IsConvertToSolidXml(fileInfo)) {
        data = FileEntry::FilePath(data).ReplaceExtension(".sxml").GetPath();
    }
    data = moduleName_ + SEPARATOR + RESOURCES_DIR + SEPARATOR + \
        fileInfo.limitKey + SEPARATOR + fileInfo.fileCluster + SEPARATOR + data;
    if (!resourceItem.SetData(reinterpret_cast<const int8_t *>(data.c_str()), data.length())) {
        cerr << "Error: resource item set data fail, data: " << data << NEW_LINE_PATH << fileInfo.filePath << endl;
        return false;
    }
    return MergeResourceItem(resourceItem);
}

string GenericCompiler::GetOutputFilePath(const FileInfo &fileInfo) const
{
    string outputFolder = GetOutputFolder(fileInfo);
    string outputFilePath = FileEntry::FilePath(outputFolder).Append(fileInfo.filename).GetPath();
    return outputFilePath;
}

bool GenericCompiler::IsIgnore(const FileInfo &fileInfo) const
{
    if (IsConvertToSolidXml(fileInfo)) {
        if (HasConvertedToSolidXml(fileInfo)) {
            return true;
        }
        return false;
    }
    return ResourceUtil::FileExist(GetOutputFilePath(fileInfo));
}

bool GenericCompiler::CopyFile(const FileInfo &fileInfo) const
{
    if (previewMode_) {
        return true;
    }
    if (IsConvertToSolidXml(fileInfo)) {
        return true;
    }

    string outputFolder = GetOutputFolder(fileInfo);
    if (!ResourceUtil::CreateDirs(outputFolder)) {
        return false;
    }
    return ResourceUtil::CopyFleInner(fileInfo.filePath, GetOutputFilePath(fileInfo));
}

bool GenericCompiler::IsConvertToSolidXml(const FileInfo &fileInfo) const
{
    if (NeedIfConvertToSolidXml() && IsXmlFile(fileInfo)) {
        return true;
    }
    return false;
}
}
}
}