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

#include<getopt.h>
#include<iostream>
#include<functional>
#include<vector>
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
    int32_t GetStartId() const;
    const std::string &GetCachePath() const;
    bool IsFileList() const;
    bool GetPreviewMode() const;
    int32_t GetPriority() const;
    const std::vector<std::string> &GetAppend() const;
    bool GetCombine() const;
    const std::string &GetDependEntry() const;

private:
    void InitCommand();
    uint32_t ParseCommand(int argc, char *argv[]);
    uint32_t AddInput(const std::string& argValue);
    uint32_t AddPackageName(const std::string& argValue);
    uint32_t AddOutput(const std::string& argValue);
    uint32_t AddResourceHeader(const std::string& argValue);
    uint32_t ForceWrite();
    uint32_t PrintVersion();
    uint32_t AddMoudleNames(const std::string& argValue);
    uint32_t AddConfig(const std::string& argValue);
    uint32_t AddStartId(const std::string& argValue);
    uint32_t AddCachePath(const std::string& argValue);
    uint32_t CheckParam() const;
    uint32_t HandleProcess(int c, const std::string& argValue);
    uint32_t ParseFileList(const std::string& fileListPath);
    uint32_t SetPreviewMode();
    uint32_t SetPriority(const std::string& argValue);
    uint32_t AddAppend(const std::string& argValue);
    uint32_t SetCombine();
    uint32_t AddDependEntry(const std::string& argValue);
    uint32_t ShowHelp() const;
    bool IsAscii(const std::string& argValue) const;

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
    int32_t startId_ = 0;
    std::string cachePath_;
    bool isFileList_ = false;
    bool previewMode_ = false;
    int32_t priority_ = -1;
    std::vector<std::string> append_;
    bool combine_ = false;
    std::string dependEntry_;
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
    std::cout << "    -i    input resource path, can add more.\n";
    std::cout << "    -p    package name.\n";
    std::cout << "    -o    output path.\n";
    std::cout << "    -r    resource header file path(like ./ResourceTable.js, ./ResrouceTable.h).\n";
    std::cout << "    -f    if output path exists,force delete it.\n";
    std::cout << "    -v    print tool version.\n";
    std::cout << "    -m    module name, can add more, split by ','(like entry1,entry2,...).\n";
    std::cout << "    -j    config.json path.\n";
    std::cout << "    -e    start id mask, e.g 0x01000000, in [0x01000000, 0x06FFFFFF),[0x08000000, 0x41FFFFFF)\n";
    std::cout << "    -x    resources folder path\n";
    std::cout << "    -z    flag for incremental compilation\n";
    std::cout << "    -h    Displays this help menu\n";
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
