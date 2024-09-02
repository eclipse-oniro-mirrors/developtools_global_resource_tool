/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "rawfile_resfile_packer.h"

#include "compression_parser.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;

RawFileResFilePacker::RawFileResFilePacker(const PackageParser &packageParser, const std::string &moduleName)
    : packageParser_(packageParser), moduleName_(moduleName), threadPool_(ThreadPool(THREAD_POOL_SIZE))
{
    threadPool_.Start();
}

RawFileResFilePacker::~RawFileResFilePacker()
{
    threadPool_.Stop();
}

std::future<uint32_t> RawFileResFilePacker::CopyRawFileOrResFileAsync(const std::vector<std::string> &inputs)
{
    auto func = [this](const std::vector<std::string> &inputs) { return this->CopyRawFileOrResFile(inputs); };
    std::future<uint32_t> res = threadPool_.Enqueue(func, inputs);
    return res;
}

uint32_t RawFileResFilePacker::CopyRawFileOrResFile(const vector<string> &inputs)
{
    for (const auto &input : inputs) {
        string rawfilePath = FileEntry::FilePath(input).Append(RAW_FILE_DIR).GetPath();
        if (CopyRawFileOrResFile(rawfilePath, RAW_FILE_DIR) == RESTOOL_ERROR) {
            cerr << "Error: copy raw file failed." << NEW_LINE_PATH << rawfilePath << endl;
            return RESTOOL_ERROR;
        }
        string resfilePath = FileEntry::FilePath(input).Append(RES_FILE_DIR).GetPath();
        if (CopyRawFileOrResFile(resfilePath, RES_FILE_DIR) == RESTOOL_ERROR) {
            cerr << "Error: copy res file failed." << NEW_LINE_PATH << resfilePath << endl;
            return RESTOOL_ERROR;
        }
    }
    for (auto &res : copyResults_) {
        uint32_t ret = res.get();
        if (ret != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t RawFileResFilePacker::CopyRawFileOrResFile(const string &filePath, const string &fileType)
{
    if (!ResourceUtil::FileExist(filePath)) {
        return RESTOOL_SUCCESS;
    }

    if (!FileEntry::IsDirectory(filePath)) {
        cerr << "Error: '" << filePath << "' not directory." << endl;
        return RESTOOL_ERROR;
    }

    string dst = FileEntry::FilePath(packageParser_.GetOutput()).Append(RESOURCES_DIR).Append(fileType).GetPath();
    if (CopyRawFileOrResFileImpl(filePath, dst) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t RawFileResFilePacker::CopyRawFileOrResFileImpl(const string &src, const string &dst)
{
    if (!ResourceUtil::CreateDirs(dst)) {
        cerr << "Error: copy rawfile of resfile, create dirs failed." << NEW_LINE_PATH << dst << endl;
        return RESTOOL_ERROR;
    }

    FileEntry f(src);
    if (!f.Init()) {
        return RESTOOL_ERROR;
    }
    for (const auto &entry : f.GetChilds()) {
        string filename = entry->GetFilePath().GetFilename();
        if (ResourceUtil::IsIgnoreFile(filename, entry->IsFile())) {
            continue;
        }

        string subPath = FileEntry::FilePath(dst).Append(filename).GetPath();
        if (!entry->IsFile()) {
            if (CopyRawFileOrResFileImpl(entry->GetFilePath().GetPath(), subPath) != RESTOOL_SUCCESS) {
                return RESTOOL_ERROR;
            }
            continue;
        }

        if (!g_resourceSet.emplace(subPath).second) {
            cerr << "Warning: '" << entry->GetFilePath().GetPath() << "' is defined repeatedly." << endl;
            continue;
        }
        string path = entry->GetFilePath().GetPath();
        auto copyFunc = [this](const string path, string subPath) { return this->CopySingleFile(path, subPath); };
        std::future<uint32_t> res = threadPool_.Enqueue(copyFunc, path, subPath);
        copyResults_.push_back(std::move(res));
    }
    return RESTOOL_SUCCESS;
}

uint32_t RawFileResFilePacker::CopySingleFile(const std::string &path, std::string &subPath)
{
    if (moduleName_ == "har" || CompressionParser::GetCompressionParser()->GetDefaultCompress()) {
        if (!ResourceUtil::CopyFileInner(path, subPath)) {
            cerr << "Error: copy rawfile or resfile failed." << NEW_LINE_PATH << path << endl;
            return RESTOOL_ERROR;
        }
        return RESTOOL_SUCCESS;
    }
    if (!CompressionParser::GetCompressionParser()->CopyAndTranscode(path, subPath, true)) {
        cerr << "Error: copy rawfile or resfile, CopyAndTranscode failed." << NEW_LINE_PATH << endl;
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
} // namespace Restool
} // namespace Global
} // namespace OHOS
