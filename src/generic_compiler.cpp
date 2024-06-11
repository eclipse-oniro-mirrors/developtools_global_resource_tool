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
#include <iostream>
#include "file_entry.h"
#include "resource_util.h"
#include "restool_errors.h"
#include "compression_parser.h"

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

    string output = "";
    if (!CopyMediaFile(fileInfo, output)) {
        return RESTOOL_ERROR;
    }

    if (!PostMediaFile(fileInfo, output)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

bool GenericCompiler::PostMediaFile(const FileInfo &fileInfo, const std::string &output)
{
    ResourceItem resourceItem(fileInfo.filename, fileInfo.keyParams, type_);
    resourceItem.SetFilePath(fileInfo.filePath);
    resourceItem.SetLimitKey(fileInfo.limitKey);

    auto index = output.find_last_of(SEPARATOR_FILE);
    if (index == string::npos) {
        cerr << "Error: invalid output path." << NEW_LINE_PATH << output << endl;
        return false;
    }
    string data = output.substr(index + 1);
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
    return ResourceUtil::FileExist(GetOutputFilePath(fileInfo));
}

bool GenericCompiler::CopyMediaFile(const FileInfo &fileInfo, std::string &output)
{
    string outputFolder = GetOutputFolder(fileInfo);
    if (!ResourceUtil::CreateDirs(outputFolder)) {
        return false;
    }
    output = GetOutputFilePath(fileInfo);
    return CompressionParser::GetCompressionParser()->CopyAndTranscode(fileInfo.filePath, output);
}
}
}
}