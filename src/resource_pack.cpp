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

#include "resource_pack.h"
#include<algorithm>
#include<iomanip>
#include "file_entry.h"
#include "file_manager.h"
#include "header.h"
#include "increment_manager.h"
#include "resource_merge.h"
#include "resource_table.h"
#include "resource_append.h"
#include "preview_manager.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
ResourcePack::ResourcePack(const PackageParser &packageParser):packageParser_(packageParser)
{
}

uint32_t ResourcePack::Package()
{
    if (packageParser_.GetPreviewMode()) {
        return PackPreview();
    }

    if (!packageParser_.GetAppend().empty()) {
        return PackAppend();
    }

    if (packageParser_.GetCombine()) {
        return PackCombine();
    }
    return PackNormal();
}

// below private founction
uint32_t ResourcePack::Init()
{
    InitHeaderCreater();
    if (InitOutput() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (InitConfigJson() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (InitModule() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::InitModule()
{
    IdWorker::ResourceIdCluster hapType = IdWorker::ResourceIdCluster::RES_ID_APP;
    string packageName = packageParser_.GetPackageName();
    if (packageName == "ohos.global.systemres") {
        hapType = IdWorker::ResourceIdCluster::RES_ID_SYS;
    }

    moduleName_ = configJson_.GetModuleName();
    vector<string> moduleNames = packageParser_.GetModuleNames();
    IdWorker &idWorker = IdWorker::GetInstance();
    int32_t startId = packageParser_.GetStartId();
    if (startId > 0) {
        return idWorker.Init(hapType, startId);
    }

    if (moduleNames.empty()) {
        return idWorker.Init(hapType);
    } else {
        sort(moduleNames.begin(), moduleNames.end());
        auto it = find_if(moduleNames.begin(), moduleNames.end(), [this](auto iter) {
                return moduleName_ == iter;
            });
        if (it == moduleNames.end()) {
            string buffer(" ");
            for_each(moduleNames.begin(), moduleNames.end(), [&buffer](auto &iter) {
                    buffer.append(" " + iter + " ");
                });
            cerr << "Error: module name '" << moduleName_ << "' not in [" << buffer << "]" << endl;
            return RESTOOL_ERROR;
        }

        int32_t startId = ((it - moduleNames.begin()) + 1) * 0x01000000;
        if (startId >= 0x07000000) {
            startId = startId + 0x01000000;
        }
        return idWorker.Init(hapType, startId);
    }
    return RESTOOL_SUCCESS;
}

void ResourcePack::InitHeaderCreater()
{
    using namespace placeholders;
    headerCreaters_.emplace(".txt", bind(&ResourcePack::GenerateTextHeader, this, _1));
    headerCreaters_.emplace(".js", bind(&ResourcePack::GenerateJsHeader, this, _1));
    headerCreaters_.emplace(".h", bind(&ResourcePack::GenerateCplusHeader, this, _1));
}

uint32_t ResourcePack::InitOutput() const
{
    string cachePath = packageParser_.GetCachePath();
    string indexPath = FileEntry::FilePath(cachePath).Append(IncrementManager::ID_JSON_FILE).GetPath();
    if (!cachePath.empty() && ResourceUtil::FileExist(indexPath)) {
        return RESTOOL_SUCCESS;
    }

    bool forceWrite = packageParser_.GetForceWrite();
    string output = packageParser_.GetOutput();
    string resourcesPath = FileEntry::FilePath(output).Append(RESOURCES_DIR).GetPath();
    if (ResourceUtil::FileExist(resourcesPath)) {
        if (!forceWrite) {
            cerr << "Error: output path '" << resourcesPath << "' exists." << endl;
            return RESTOOL_ERROR;
        }

        if (!ResourceUtil::RmoveAllDir(resourcesPath)) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::GenerateHeader() const
{
    auto headerPaths = packageParser_.GetResourceHeaders();
    string textPath = FileEntry::FilePath(packageParser_.GetOutput()).Append("ResourceTable.txt").GetPath();
    headerPaths.push_back(textPath);
    for (const auto &headerPath : headerPaths) {
        string extension = FileEntry::FilePath(headerPath).GetExtension();
        auto it = headerCreaters_.find(extension);
        if (it == headerCreaters_.end()) {
            cout << "Warning: don't support header file format '" << headerPath << "'" << endl;
            continue;
        }
        if (it->second(headerPath) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::InitConfigJson()
{
    string config = packageParser_.GetConfig();
    if (config.empty()) {
        if (packageParser_.GetInputs().size() > 1) {
            cerr << "Error: more input path, -j config.json empty" << endl;
            return RESTOOL_ERROR;
        }
        config = FileEntry::FilePath(packageParser_.GetInputs()[0]).Append(CONFIG_JSON).GetPath();
        if (!ResourceUtil::FileExist(config)) {
            config = FileEntry::FilePath(packageParser_.GetInputs()[0]).Append(MODULE_JSON).GetPath();
        }
    }

    if (FileEntry::FilePath(config).GetFilename() == MODULE_JSON) {
        ConfigParser::SetUseModule();
    }
    configJson_ = ConfigParser(config);
    if (configJson_.Init() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::GenerateTextHeader(const string &headerPath) const
{
    Header textHeader(headerPath);
    bool first = true;
    uint32_t result = textHeader.Create([](stringstream &buffer) {},
        [&first](stringstream &buffer, const IdWorker::ResourceId& resourceId) {
            if (first) {
                first = false;
            } else {
                buffer << "\n";
            }
            buffer << resourceId.type << " " << resourceId.name;
            buffer << " 0x" << hex << setw(8)  << setfill('0') << resourceId.id;
        }, [](stringstream &buffer) {});
    if (result != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::GenerateCplusHeader(const string &headerPath) const
{
    Header cplusHeader(headerPath);
    uint32_t result = cplusHeader.Create([](stringstream &buffer) {
        buffer << Header::LICENSE_HEADER << "\n";
        buffer << "#ifndef RESOURCE_TABLE_H\n";
        buffer << "#define RESOURCE_TABLE_H\n\n";
        buffer << "#include<stdint.h>\n\n";
        buffer << "namespace OHOS {\n";
    }, [](stringstream &buffer, const IdWorker::ResourceId& resourceId) {
        string name = resourceId.type + "_" + resourceId.name;
        transform(name.begin(), name.end(), name.begin(), ::toupper);
        buffer << "const int32_t " << name << " = ";
        buffer << "0x" << hex << setw(8)  << setfill('0') << resourceId.id << ";\n";
    }, [](stringstream &buffer){
        buffer << "}\n";
        buffer << "#endif";
    });
    return result;
}

uint32_t ResourcePack::GenerateJsHeader(const std::string &headerPath) const
{
    Header JsHeader(headerPath);
    string itemType;
    uint32_t result = JsHeader.Create([](stringstream &buffer) {
        buffer << Header::LICENSE_HEADER << "\n";
        buffer << "export default {\n";
    }, [&itemType](stringstream &buffer, const IdWorker::ResourceId& resourceId) {
        if (itemType != resourceId.type) {
            if (!itemType.empty()) {
                buffer << "\n" << "    " << "},\n";
            }
            buffer << "    " << resourceId.type << " : {\n";
            itemType = resourceId.type;
        } else {
            buffer << ",\n";
        }
        buffer << "        " << resourceId.name << " : " << resourceId.id;
    }, [](stringstream &buffer){
        buffer << "\n" << "    " << "}\n";
        buffer << "}\n";
    });
    return result;
}

uint32_t ResourcePack::CopyRawFile(const vector<string> &inputs) const
{
    for (const auto &input : inputs) {
        string rawfilePath = FileEntry::FilePath(input).Append(RAW_FILE_DIR).GetPath();
        if (!ResourceUtil::FileExist(rawfilePath)) {
            continue;
        }

        if (!FileEntry::IsDirectory(rawfilePath)) {
            cerr << "Error: '" << rawfilePath << "' not directory." << endl;
            return RESTOOL_ERROR;
        }

        string dst = FileEntry::FilePath(packageParser_.GetOutput())
            .Append(RESOURCES_DIR).Append(RAW_FILE_DIR).GetPath();
        if (CopyRawFileImpl(rawfilePath, dst) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::CopyRawFileImpl(const string &src, const string &dst) const
{
    if (!ResourceUtil::CreateDirs(dst)) {
        return RESTOOL_ERROR;
    }

    FileEntry f(src);
    if (!f.Init()) {
        return RESTOOL_ERROR;
    }
    for (const auto &entry : f.GetChilds()) {
        string filename = entry->GetFilePath().GetFilename();
        if (ResourceUtil::IsIgnoreFile(filename, entry->IsFile())) {
            continue;
        }

        string subPath = FileEntry::FilePath(dst).Append(filename).GetPath();
        if (!entry->IsFile()) {
            if (CopyRawFileImpl(entry->GetFilePath().GetPath(), subPath) != RESTOOL_SUCCESS) {
                return RESTOOL_ERROR;
            }
        } else {
            if (ResourceUtil::FileExist(subPath)) {
                continue;
            }
            if (!ResourceUtil::CopyFleInner(entry->GetFilePath().GetPath(), subPath)) {
                return RESTOOL_ERROR;
            }
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::GenerateConfigJson()
{
    if (configJson_.ParseRefence() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    string outputPath = FileEntry::FilePath(packageParser_.GetOutput())
        .Append(ConfigParser::GetConfigName()).GetPath();
    return configJson_.Save(outputPath);
}

uint32_t ResourcePack::ScanResources(const vector<string> &inputs, const string &output)
{
    auto &fileManager = FileManager::GetInstance();
    fileManager.SetModuleName(moduleName_);
    string cachePath = packageParser_.GetCachePath();
    if (cachePath.empty()) {
        if (fileManager.ScanModules(inputs, output) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
        return RESTOOL_SUCCESS;
    }

    auto &incrementManager = IncrementManager::GetInstance();
    if (incrementManager.Init(cachePath, inputs, output, moduleName_) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    if (fileManager.ScanIncrement(output) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::PackNormal()
{
    if (Init() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    ResourceMerge resourceMerge;
    if (resourceMerge.Init() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (ScanResources(resourceMerge.GetInputs(), packageParser_.GetOutput()) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GenerateHeader() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (CopyRawFile(resourceMerge.GetInputs()) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GenerateConfigJson() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    ResourceTable resourceTable;
    if (resourceTable.CreateResourceTable() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::PackPreview()
{
    PreviewManager preview;
    preview.SetPriority(packageParser_.GetPriority());
    return preview.ScanModules(packageParser_.GetInputs(), packageParser_.GetOutput());
}

uint32_t ResourcePack::PackAppend()
{
    ResourceAppend resourceAppend(packageParser_);
    if (!packageParser_.GetAppend().empty()) {
        return resourceAppend.Append();
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::PackCombine()
{
    if (Init() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    ResourceAppend resourceAppend(packageParser_);
    if (resourceAppend.Combine() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GenerateConfigJson() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GenerateHeader() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}
}
}
}
