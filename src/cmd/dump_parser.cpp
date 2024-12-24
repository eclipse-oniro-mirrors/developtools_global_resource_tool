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

#include "cmd/dump_parser.h"
#include "resource_util.h"
#include "resource_dumper.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
uint32_t DumpParser::Parse(int argc, char *argv[])
{
    // argv[0] is restool path, argv[1] is 'dump' subcommand.
    int currentIndex = 2;
    if (currentIndex >= argc) {
        cerr << "Error: missing input Hap path." << endl;
        return RESTOOL_ERROR;
    }
    const set<string> supportedDumpType = ResourceDumperFactory::GetSupportDumpType();
    if (supportedDumpType.count(argv[currentIndex]) != 0) {
        type_ = argv[currentIndex];
        currentIndex++;
    }
    if (currentIndex >= argc) {
        cerr << "Error: missing input Hap path." << endl;
        return RESTOOL_ERROR;
    }
    inputPath_ = ResourceUtil::RealPath(argv[currentIndex]);
    if (inputPath_.empty()) {
        cerr << "Error: invalid input path: '" << argv[currentIndex] << "'" << endl;
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

const string& DumpParser::GetInputPath() const
{
    return inputPath_;
}

uint32_t DumpParser::ExecCommand()
{
    auto resourceDump = ResourceDumperFactory::CreateResourceDumper(type_);
    if (resourceDump) {
        return resourceDump->Dump(*this);
    }
    cerr << "Error: not support dump type '" << type_ << "'" << endl;
    return RESTOOL_ERROR;
}
} // namespace Restool
} // namespace Global
} // namespace OHOS