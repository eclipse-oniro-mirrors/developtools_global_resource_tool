/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RESTOOL_COMPRESSION_PARSER_H
#define OHOS_RESTOOL_COMPRESSION_PARSER_H

#include <cJSON.h>
#ifdef __WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {

enum class TranscodeError {
    SUCCESS = 0,
    IMAGE_ERROR,
    IMAGE_RESOLUTION_NOT_MATCH,
    ANIMATED_IMAGE_SKIP,
    MALLOC_FAILED,
    ENCODE_ASTC_FAILED,
    SUPER_COMPRESS_FAILED,
    LOAD_COMPRESS_FAILED
};

struct TranscodeResult {
    int64_t timeCostMs;
    size_t originSize;
    int32_t size;
    int32_t width;
    int32_t height;
};

typedef TranscodeError (*ITranscodeImages) (const std::string &imagePath,
    std::string &outputPath, TranscodeResult &result);
typedef bool (*ISetTranscodeOptions) (const std::string &optionJson);

class CompressionParser {
public:
    static std::shared_ptr<CompressionParser> GetCompressionParser(const std::string &filePath);
    static std::shared_ptr<CompressionParser> GetCompressionParser();
    CompressionParser();
    explicit CompressionParser(const std::string &filePath);
    virtual ~CompressionParser();
    uint32_t Init();
    bool CopyAndTranscode(const std::string &src, std::string &dst);
private:
    bool ParseContext(const cJSON *contextNode);
    bool ParseCompression(const cJSON *compressionNode);
    bool ParseFilters(const cJSON *filtersNode);
    bool LoadImageTranscoder();
    bool SetTranscodeOptions(const std::string &optionJson);
    TranscodeError TranscodeImages(const std::string &imagePath, std::string &outputPath, TranscodeResult &result);
    std::vector<std::string> ParsePath(const cJSON *pathNode);
    std::vector<std::string> ParseRules(const cJSON *rulesNode);
    std::string ParseJsonStr(const cJSON *node);
    bool CheckPath(const std::string &src, const std::vector<std::string> &paths);
    bool IsInPath(const std::string &src, const std::shared_ptr<CompressFilter> &compressFilter);
    bool IsInExcludePath(const std::string &src, const std::shared_ptr<CompressFilter> &compressFilter);
    std::string GetOptionsString(const std::shared_ptr<CompressFilter> &compressFilter, int type);
    std::string filePath_;
    std::string extensionPath_;
    std::vector<std::shared_ptr<CompressFilter>> compressFilters_;
    bool mediaSwitch_;
    cJSON *root_;
#ifdef __WIN32
    HMODULE handle_ = nullptr;
#else
    void *handle_ = nullptr;
#endif
};
}
}
}
#endif