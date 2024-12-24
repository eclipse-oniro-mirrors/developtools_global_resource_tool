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

#ifndef OHOS_RESTOOL_CMD_PARSER_H
#define OHOS_RESTOOL_CMD_PARSER_H

#include <cstdint>
#include <memory>
#include <iostream>
#include <string>
#include <type_traits>
#include "singleton.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {

class ICmdParser {
public:
    virtual uint32_t Parse(int argc, char *argv[]) = 0;
    virtual uint32_t ExecCommand() = 0;
};

template<class T>
class CmdParser : public Singleton<CmdParser<T>> {
    static_assert(std::is_base_of_v<ICmdParser, T>, "Template T must inherit ICmdParser.");
public:
    T &GetCmdParser();
    uint32_t Parse(int argc, char *argv[]);
    uint32_t ExecCommand();
    static void ShowUseage();

private:
    T cmdParser_;
};

template<class T>
void CmdParser<T>::ShowUseage()
{
    std::cout << "This is an OHOS Packaging Tool.\n";
    std::cout << "Usage:\n";
    std::cout << TOOL_NAME << " [arguments] Package the OHOS resources.\n";
    std::cout << "[arguments]:\n";
    std::cout << "    -i/--inputPath      input resource path, can add more.\n";
    std::cout << "    -p/--packageName    package name.\n";
    std::cout << "    -o/--outputPath     output path.\n";
    std::cout << "    -r/--resHeader      resource header file path(like ./ResourceTable.js, ./ResrouceTable.h).\n";
    std::cout << "    -f/--forceWrite     if output path exists,force delete it.\n";
    std::cout << "    -v/--version        print tool version.\n";
    std::cout << "    -m/--modules        module name, can add more, split by ','(like entry1,entry2,...).\n";
    std::cout << "    -j/--json           config.json path.\n";
    std::cout << "    -e/--startId        start id mask, e.g 0x01000000,";
    std::cout << " in [0x01000000, 0x06FFFFFF),[0x08000000, 0xFFFFFFFF)\n";
    std::cout << "    -x/--append         resources folder path\n";
    std::cout << "    -z/--combine        flag for incremental compilation\n";
    std::cout << "    -h/--help           Displays this help menu\n";
    std::cout << "    -l/--fileList       input json file of the option set, e.g resConfig.json.";
    std::cout << " For details, see the developer documentation.\n";
    std::cout << "    --ids               save id_defined.json direcory\n";
    std::cout << "    --defined-ids       input id_defined.json path\n";
    std::cout << "    --dependEntry       Build result directory of the specified entry module when the feature";
    std::cout << " module resources are independently built in the FA model.\n";
    std::cout << "    --icon-check        Enable the PNG image verification function for icons and startwindows.\n";
    std::cout << "    --target-config     When used with '-i', selective compilation is supported.\n";
    std::cout << "    --compressed-config opt-compression.json path.\n";
    std::cout << "    dump [config]       Dump rsource in hap to json. If with 'config', will only dump limit path.\n";
}

template<class T>
T &CmdParser<T>::GetCmdParser()
{
    return cmdParser_;
}

template<class T>
uint32_t CmdParser<T>::Parse(int argc, char *argv[])
{
    return cmdParser_.Parse(argc, argv);
}

template<class T>
uint32_t CmdParser<T>::ExecCommand()
{
    return cmdParser_.ExecCommand();
};
}
}
}
#endif