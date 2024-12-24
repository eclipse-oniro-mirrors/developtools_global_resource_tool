/*
 * Copyright (c) 2024 - 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RESTOOL_DUMP_PARSER_H
#define OHOS_RESTOOL_DUMP_PARSER_H

#include "cmd_parser.h"
#include <memory>
#include <string>

namespace OHOS {
namespace Global {
namespace Restool {
class DumpParser : public ICmdParser {
public:
    virtual ~DumpParser() = default;
    uint32_t Parse(int argc, char *argv[]) override;
    uint32_t ExecCommand() override;
    const std::string &GetInputPath() const;

private:
    std::string inputPath_;
    std::string type_;
};
} // namespace Restool
} // namespace Global
} // namespace OHOS
#endif
