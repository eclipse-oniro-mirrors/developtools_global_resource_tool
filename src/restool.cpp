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

#include "cmd_parser.h"
#include "task_handle.h"

using namespace std;
using namespace OHOS::Global::Restool;
namespace {
uint32_t ProccssHap(PackageParser &packageParser)
{
    TaskHandle taskHandle;
    return taskHandle.HandlePackage(packageParser);
}

uint32_t PackCmdHandle(PackageParser &packageParser)
{
    return ProccssHap(packageParser);
}
}

int main(int argc, char *argv[])
{
    if (argv == nullptr) {
        cerr << "Error: argv null" << endl;
        return RESTOOL_ERROR;
    }
    auto &parser = CmdParser<PackageParser>::GetInstance();
    if (parser.Parse(argc, argv) != RESTOOL_SUCCESS) {
        parser.ShowUseage();
        return RESTOOL_ERROR;
    }

    auto &packageParser = parser.GetCmdParser();
    if (PackCmdHandle(packageParser) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    cout << "Info: restool resources compile success." << endl;
    return RESTOOL_SUCCESS;
}
