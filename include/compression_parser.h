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
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
class CompressionParser {
public:
    static std::shared_ptr<CompressionParser> GetCompressionParser(const std::string &filePath);
    static std::shared_ptr<CompressionParser> GetCompressionParser();
    CompressionParser();
    explicit CompressionParser(const std::string &filePath);
    virtual ~CompressionParser();
    uint32_t Init();
private:
    bool ParseContext(const cJSON *contextNode);
    bool ParseCompression(const cJSON *compressionNode);
    bool ParseFilter(const cJSON *filterNode);
    std::string filePath_;
    std::string extensionPath_;
    bool mediaSwitch_;
    cJSON *root_;
};
}
}
}
#endif