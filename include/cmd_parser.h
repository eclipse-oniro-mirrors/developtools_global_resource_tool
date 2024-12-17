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

#ifndef OHOS_RESTOOL_CMD_PARSER_H
#define OHOS_RESTOOL_CMD_PARSER_H

#include <getopt.h>
#include <iostream>
#include <functional>
#include <vector>
#include "singleton.h"
#include "resource_data.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ICmdParser {
public:
    virtual uint32_t Parse(int argc, char *argv[]) = 0;
};

class PackageParser : public ICmdParser {
public:
    PackageParser() {};
    virtual ~PackageParser() = default;
    uint32_t Parse(int argc, char *argv[]) override;
    const std::vector<std::string> &GetInputs() const;
    const std::string &GetPackageName() const;
    const std::string &GetOutput() const;
    const std::vector<std::string> &GetResourceHeaders() const;
    bool GetForceWrite() const;
    const std::vector<std::string> &GetModuleNames() const;
    const std::string &GetConfig() const;
    const std::string &GetRestoolPath() const;
    uint32_t GetStartId() const;
    bool IsFileList() const;
    const std::vector<std::string> &GetAppend() const;
    bool GetCombine() const;
    const std::string &GetDependEntry() const;
    const std::string &GetIdDefinedOutput() const;
    const std::string &GetIdDefinedInputPath() const;
    bool GetIconCheck() const;
    const TargetConfig &GetTargetConfigValues() const;
    bool IsTargetConfig() const;
    const std::vector<std::string> &GetSysIdDefinedPaths() const;
    const std::string &GetCompressionPath() const;
    bool IsOverlap() const;

private:
    void InitCommand();
    uint32_t ParseCommand(int argc, char *argv[]);
    uint32_t CheckError(int argc, char *argv[], int c, int optIndex);
    uint32_t AddInput(const std::string& argValue);
    uint32_t AddPackageName(const std::string& argValue);
    uint32_t AddOutput(const std::string& argValue);
    uint32_t AddResourceHeader(const std::string& argValue);
    uint32_t ForceWrite();
    uint32_t PrintVersion();
    uint32_t AddMoudleNames(const std::string& argValue);
    uint32_t AddConfig(const std::string& argValue);
    uint32_t AddStartId(const std::string& argValue);
    void AdaptResourcesDirForInput();
    uint32_t CheckParam() const;
    uint32_t HandleProcess(int c, const std::string& argValue);
    uint32_t ParseFileList(const std::string& fileListPath);
    uint32_t AddAppend(const std::string& argValue);
    uint32_t SetCombine();
    uint32_t AddDependEntry(const std::string& argValue);
    uint32_t ShowHelp() const;
    uint32_t SetIdDefinedOutput(const std::string& argValue);
    uint32_t SetIdDefinedInputPath(const std::string& argValue);
    uint32_t AddSysIdDefined(const std::string& argValue);
    bool IsAscii(const std::string& argValue) const;
    bool IsLongOpt(int argc, char *argv[]) const;
    uint32_t IconCheck();
    uint32_t ParseTargetConfig(const std::string& argValue);
    uint32_t AddCompressionPath(const std::string& argValue);

    static const struct option CMD_OPTS[];
    static const std::string CMD_PARAMS;
    using HandleArgValue = std::function<uint32_t(const std::string&)>;
    std::map<int32_t, HandleArgValue> handles_;
    std::vector<std::string> inputs_;
    std::string packageName_;
    std::string output_;
    std::vector<std::string> resourceHeaderPaths_;
    bool forceWrite_ = false;
    std::vector<std::string> moduleNames_;
    std::string configPath_;
    std::string restoolPath_;
    uint32_t startId_ = 0;
    bool isFileList_ = false;
    std::vector<std::string> append_;
    bool combine_ = false;
    std::string dependEntry_;
    std::string idDefinedOutput_;
    std::string idDefinedInputPath_;
    bool isIconCheck_ = false;
    TargetConfig targetConfig_;
    bool isTtargetConfig_;
    std::vector<std::string> sysIdDefinedPaths_;
    std::string compressionPath_;
};

template<class T>
class CmdParser : public Singleton<CmdParser<T>> {
public:
    T &GetCmdParser();
    uint32_t Parse(int argc, char *argv[]);
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
}

template<class T>
T &CmdParser<T>::GetCmdParser()
{
    return cmdParser_;
}

template<class T>
uint32_t CmdParser<T>::Parse(int argc, char *argv[])
{
    if (cmdParser_.Parse(argc, argv) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}
#endif
