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

#include <cstring>
#include "cmd/package_parser.h"
#include "cmd/dump_parser.h"
#include "resource_pack.h"
#include "compression_parser.h"

using namespace std;
using namespace OHOS::Global::Restool;

constexpr int PARAM_MIN_NUM = 2;

int main(int argc, char *argv[])
{
    if (argv == nullptr) {
        cerr << "Error: argv null" << endl;
        return RESTOOL_ERROR;
    }
    if (argc < PARAM_MIN_NUM) {
        cerr << "Error: At least 1 parameters are required, but no parameter is passed in." << endl;
        return RESTOOL_ERROR;
    }
    if (strcmp(argv[1], "dump") == 0) {
        auto &dumpParser = CmdParser<DumpParser>::GetInstance();
        if (dumpParser.Parse(argc, argv) != RESTOOL_SUCCESS) {
            dumpParser.ShowUseage();
            return RESTOOL_ERROR;
        }
        return dumpParser.ExecCommand();
    }
    auto &packParser = CmdParser<PackageParser>::GetInstance();
    if (packParser.Parse(argc, argv) != RESTOOL_SUCCESS) {
        packParser.ShowUseage();
        return RESTOOL_ERROR;
    }

    return packParser.ExecCommand();
}
