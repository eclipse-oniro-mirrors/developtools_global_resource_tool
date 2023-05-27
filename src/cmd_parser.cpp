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
#include <algorithm>
#include "cmd_list.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const struct option PackageParser::CMD_OPTS[] = {
    { "inputPath", required_argument, nullptr, Option::INPUTPATH },
    { "packageName", required_argument, nullptr, Option::PACKAGENAME },
    { "outputPath", required_argument, nullptr, Option::OUTPUTPATH },
    { "resHeader", required_argument, nullptr, Option::RESHEADER },
    { "forceWrite", no_argument, nullptr, Option::FORCEWRITE },
    { "version", no_argument, nullptr, Option::VERSION},
    { "modules", required_argument, nullptr, Option::MODULES },
    { "json", required_argument, nullptr, Option::JSON },
    { "startId", required_argument, nullptr, Option::STARTID },
    { "fileList", required_argument, nullptr, Option::FILELIST },
    { "append", required_argument, nullptr, Option::APPEND },
    { "combine", required_argument, nullptr, Option::COMBINE },
    { "dependEntry", required_argument, nullptr, Option::DEPENDENTRY },
    { "help", no_argument, nullptr, Option::HELP},
    { "ids", required_argument, nullptr, Option::IDS},
    { "defined-ids", required_argument, nullptr, Option::DEFINED_IDS},
    { 0, 0, 0, 0},
};

const string PackageParser::CMD_PARAMS = ":i:p:o:r:m:j:e:l:x:fhvz";

uint32_t PackageParser::Parse(int argc, char *argv[])
{
    InitCommand();
    if (ParseCommand(argc, argv) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    if (CheckParam() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    AdaptResourcesDirForInput();
    return RESTOOL_SUCCESS;
}

const vector<string> &PackageParser::GetInputs() const
{
    return inputs_;
}

const string &PackageParser::GetPackageName() const
{
    return packageName_;
}

const string &PackageParser::GetOutput() const
{
    return output_;
}

const vector<string> &PackageParser::GetResourceHeaders() const
{
    return resourceHeaderPaths_;
}

bool PackageParser::GetForceWrite() const
{
    return forceWrite_;
}

const vector<string> &PackageParser::GetModuleNames() const
{
    return moduleNames_;
}

const string &PackageParser::GetConfig() const
{
    return configPath_;
}

const string &PackageParser::GetRestoolPath() const
{
    return restoolPath_;
}

int32_t PackageParser::GetStartId() const
{
    return startId_;
}

const string &PackageParser::GetCachePath() const
{
    return cachePath_;
}

const string &PackageParser::GetDependEntry() const
{
    return dependEntry_;
}

uint32_t PackageParser::AddInput(const string& argValue)
{
    auto ret = find_if(inputs_.begin(), inputs_.end(), [argValue](auto iter) {return argValue == iter;});
    if (ret != inputs_.end()) {
        cerr << "Error: repeat input '" << argValue << "'" << endl;
        return RESTOOL_ERROR;
    }

    if (!IsAscii(argValue)) {
        return RESTOOL_ERROR;
    }
    inputs_.push_back(argValue);
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::AddPackageName(const string& argValue)
{
    if (!packageName_.empty()) {
        cerr << "Error: double package name " << packageName_ << " vs " << argValue << endl;
        return RESTOOL_ERROR;
    }

    packageName_ = argValue;
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::AddOutput(const string& argValue)
{
    if (!output_.empty()) {
        cerr << "Error: double output " << output_ << " vs " << argValue << endl;
        return RESTOOL_ERROR;
    }

    output_ = ResourceUtil::RealPath(argValue);
    if (output_.empty()) {
        cerr << "Error: invalid output '" << argValue << "'" << endl;
        return RESTOOL_ERROR;
    }
    if (!IsAscii(output_)) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::AddResourceHeader(const string& argValue)
{
    if (find(resourceHeaderPaths_.begin(), resourceHeaderPaths_.end(), argValue) != resourceHeaderPaths_.end()) {
        cerr << "Error: '" << argValue << "' input duplicated." << endl;
        return RESTOOL_ERROR;
    }
    resourceHeaderPaths_.push_back(argValue);
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::ForceWrite()
{
    forceWrite_ = true;
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::PrintVersion()
{
    cout << "Info: Restool version= " << RESTOOL_VERSION << endl;
    exit(RESTOOL_SUCCESS);
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::AddMoudleNames(const string& argValue)
{
    if (!moduleNames_.empty()) {
        cerr << "Error: -m double module name '" << argValue << "'" << endl;
        return RESTOOL_ERROR;
    }

    ResourceUtil::Split(argValue, moduleNames_, ",");
    for (auto it = moduleNames_.begin(); it != moduleNames_.end(); it++) {
        auto ret = find_if(moduleNames_.begin(), moduleNames_.end(), [it](auto iter) {return *it == iter;});
        if (ret != it) {
            cerr << "Error: double module name '" << *it << "'" << endl;
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::AddConfig(const string& argValue)
{
    if (!configPath_.empty()) {
        cerr << "Error: double config.json " << configPath_ << " vs " << argValue << endl;
        return RESTOOL_ERROR;
    }

    configPath_ = argValue;
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::AddStartId(const string& argValue)
{
    startId_ = strtol(argValue.c_str(), nullptr, 16); // 16 is hexadecimal number
    if ((startId_ >= 0x01000000 && startId_ < 0x06ffffff) || (startId_ >= 0x08000000 && startId_ < 0x41ffffff)) {
        return RESTOOL_SUCCESS;
    }
    cerr << "Error: invalid start id " << argValue << endl;
    return RESTOOL_ERROR;
}

uint32_t PackageParser::AddCachePath(const string& argValue)
{
    cachePath_ = argValue;
    return RESTOOL_SUCCESS;
}

// -i input directory, add the resource directory
void PackageParser::AdaptResourcesDirForInput()
{
    if (!isFileList_ && !combine_) { // -l and increment compile -i, no need to add resource directory
        for (auto &path : inputs_) {
            path = FileEntry::FilePath(path).Append(RESOURCES_DIR).GetPath();
        }
    }
}

uint32_t PackageParser::CheckParam() const
{
    if (inputs_.empty() && append_.empty()) {
        cerr << "Error: input path empty." << endl;
        return RESTOOL_ERROR;
    }

    if (output_.empty()) {
        cerr << "Error: output path empty." << endl;
        return RESTOOL_ERROR;
    }

    if (previewMode_ || !append_.empty()) {
        return RESTOOL_SUCCESS;
    }
    if (packageName_.empty()) {
        cerr << "Error: package name empty." << endl;
        return RESTOOL_ERROR;
    }

    if (resourceHeaderPaths_.empty()) {
        cerr << "Error: resource header path empty." << endl;
        return RESTOOL_ERROR;
    }

    if (startId_ != 0 && !idDefinedInputPath_.empty()) {
        cerr << "Error: set -e and --defined-ids cannot be used together." << endl;
        return RESTOOL_ERROR;
    }

    return RESTOOL_SUCCESS;
}

bool PackageParser::IsFileList() const
{
    return isFileList_;
}

uint32_t PackageParser::SetPreviewMode()
{
    previewMode_ = true;
    return RESTOOL_SUCCESS;
}

bool PackageParser::GetPreviewMode() const
{
    return previewMode_;
}

int32_t PackageParser::GetPriority() const
{
    return priority_;
}

uint32_t PackageParser::SetPriority(const string& argValue)
{
    priority_ = atoi(argValue.c_str());
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::AddAppend(const string& argValue)
{
    string appendPath = ResourceUtil::RealPath(argValue);
    if (appendPath.empty()) {
        cout << "Warning: invaild compress '" << argValue << "'" << endl;
        appendPath = argValue;
    }
    auto ret = find_if(append_.begin(), append_.end(), [appendPath](auto iter) {return appendPath == iter;});
    if (ret != append_.end()) {
        cerr << "Error: repeat input '" << argValue << "'" << endl;
        return RESTOOL_ERROR;
    }
    if (!IsAscii(appendPath)) {
        return RESTOOL_ERROR;
    }
    append_.push_back(appendPath);
    return RESTOOL_SUCCESS;
}

const vector<string> &PackageParser::GetAppend() const
{
    return append_;
}

uint32_t PackageParser::SetCombine()
{
    combine_ = true;
    return RESTOOL_SUCCESS;
}

bool PackageParser::GetCombine() const
{
    return combine_;
}

uint32_t PackageParser::AddDependEntry(const string& argValue)
{
    dependEntry_ = argValue;
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::ShowHelp() const
{
    auto &parser = CmdParser<PackageParser>::GetInstance();
    parser.ShowUseage();
    exit(RESTOOL_SUCCESS);
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::SetIdDefinedOutput(const string& argValue)
{
    idDefinedOutput_ = argValue;
    return RESTOOL_SUCCESS;
}

const string &PackageParser::GetIdDefinedOutput() const
{
    return idDefinedOutput_;
}

uint32_t PackageParser::SetIdDefinedInputPath(const string& argValue)
{
    idDefinedInputPath_ = argValue;
    return RESTOOL_SUCCESS;
}

const string &PackageParser::GetIdDefinedInputPath() const
{
    return idDefinedInputPath_;
}

bool PackageParser::IsAscii(const string& argValue) const
{
#ifdef __WIN32
    auto result = find_if(argValue.begin(), argValue.end(), [](auto iter) {
        if ((iter & 0x80) != 0) {
            return true;
        }
        return false;
    });
    if (result != argValue.end()) {
        cerr << "Error: '" << argValue << "' must be ASCII" << endl;
        return false;
    }
#endif
    return true;
}

void PackageParser::InitCommand()
{
    using namespace placeholders;
    handles_.emplace(Option::INPUTPATH, bind(&PackageParser::AddInput, this, _1));
    handles_.emplace(Option::PACKAGENAME, bind(&PackageParser::AddPackageName, this, _1));
    handles_.emplace(Option::OUTPUTPATH, bind(&PackageParser::AddOutput, this, _1));
    handles_.emplace(Option::RESHEADER, bind(&PackageParser::AddResourceHeader, this, _1));
    handles_.emplace(Option::FORCEWRITE, [this](const string &) -> uint32_t { return ForceWrite(); });
    handles_.emplace(Option::VERSION, [this](const string &) -> uint32_t { return PrintVersion(); });
    handles_.emplace(Option::MODULES, bind(&PackageParser::AddMoudleNames, this, _1));
    handles_.emplace(Option::JSON, bind(&PackageParser::AddConfig, this,  _1));
    handles_.emplace(Option::STARTID, bind(&PackageParser::AddStartId, this, _1));
    handles_.emplace(Option::APPEND, bind(&PackageParser::AddAppend, this, _1));
    handles_.emplace(Option::COMBINE, [this](const string &) -> uint32_t { return SetCombine(); });
    handles_.emplace(Option::DEPENDENTRY, bind(&PackageParser::AddDependEntry, this, _1));
    handles_.emplace(Option::HELP, [this](const string &) -> uint32_t { return ShowHelp(); });
    handles_.emplace(Option::IDS, bind(&PackageParser::SetIdDefinedOutput, this, _1));
    handles_.emplace(Option::DEFINED_IDS, bind(&PackageParser::SetIdDefinedInputPath, this, _1));
}

uint32_t PackageParser::HandleProcess(int c, const string& argValue)
{
    auto handler = handles_.find(c);
    if (handler == handles_.end()) {
        cout << "Error: unsupport " << c << endl;
        return RESTOOL_ERROR;
    }
    return handler->second(argValue);
}

uint32_t PackageParser::ParseFileList(const string& fileListPath)
{
    isFileList_ = true;
    CmdList cmdList;
    if (cmdList.Init(fileListPath, [this](int c, const string &argValue) -> int32_t {
        return HandleProcess(c, argValue);
    }) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t PackageParser::ParseCommand(int argc, char *argv[])
{
    restoolPath_ = string(argv[0]);
    while (true) {
        int optIndex = -1;
        opterr = 0;
        int c = getopt_long(argc, argv, CMD_PARAMS.c_str(), CMD_OPTS, &optIndex);
        if (optIndex != -1) {
            string curOpt = (optarg == nullptr) ? argv[optind - 1] : argv[optind - 2];
            if (curOpt != ("--" + string(CMD_OPTS[optIndex].name))) {
                cout << "Error: unknown option " << curOpt << endl;
                return RESTOOL_ERROR;
            }
        }
        if (c == Option::END) {
            if (argc == optind) {
                break;
            }
            string errmsg = "Error: invalid arguments : ";
            for (int i = optind; i < argc; i++) {
                errmsg.append(argv[i]).append(" ");
            }
            cout << errmsg << endl;
            return RESTOOL_ERROR;
        }
        if (c == Option::UNKNOWN) {
            string optUnknown = (optopt == 0) ? argv[optind - 1] : ("-" + string(1, optopt));
            cout << "Error: unknown option " << optUnknown << endl;
            return RESTOOL_ERROR;
        }
        if (c == Option::NO_ARGUMENT) {
            if (IsLongOpt(argv)) {
                cout << "Error: option " << argv[optind - 1] << " must have argument" << endl;
            } else {
                cout << "Error: unknown option " << argv[optind - 1] << endl;
            }
            return RESTOOL_ERROR;
        }
        string argValue = (optarg != nullptr) ? optarg : "";
        if (c == Option::FILELIST) {
            return ParseFileList(argValue);
        }
        if (HandleProcess(c, argValue) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

bool PackageParser::IsLongOpt(char *argv[]) const
{
    for (auto iter : CMD_OPTS) {
        if (optopt == iter.val && argv[optind - 1] == ("--" + string(iter.name))) {
            return true;
        }
    }
    return false;
}
}
}
}
