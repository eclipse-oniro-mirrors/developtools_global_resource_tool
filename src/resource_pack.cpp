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

#include "resource_pack.h"
#include <algorithm>
#include <iomanip>
#include "file_entry.h"
#include "file_manager.h"
#include "header.h"
#include "resource_check.h"
#include "resource_merge.h"
#include "resource_table.h"
#include "compression_parser.h"
#include "binary_file_packer.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
ResourcePack::ResourcePack(const PackageParser &packageParser):packageParser_(packageParser)
{
}

uint32_t ResourcePack::Package()
{
    if (!packageParser_.GetAppend().empty()) {
        return PackAppend();
    }

    if (packageParser_.GetCombine()) {
        return PackCombine();
    }

    if (packageParser_.ExistIndex()) {
        return PackOverlap();
    }
    return PackNormal();
}

uint32_t ResourcePack::InitCompression()
{
    if (!packageParser_.GetCompressionPath().empty()) {
        auto compressionMgr = CompressionParser::GetCompressionParser(packageParser_.GetCompressionPath());
        compressionMgr->SetOutPath(packageParser_.GetOutput());
        if (compressionMgr->Init() != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
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
    ResourceIdCluster hapType = ResourceIdCluster::RES_ID_APP;
    string packageName = packageParser_.GetPackageName();
    if (packageName == "ohos.global.systemres") {
        hapType = ResourceIdCluster::RES_ID_SYS;
    }

    moduleName_ = configJson_.GetModuleName();
    vector<string> moduleNames = packageParser_.GetModuleNames();
    IdWorker &idWorker = IdWorker::GetInstance();
    int64_t startId = static_cast<int64_t>(packageParser_.GetStartId());
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
            for_each(moduleNames.begin(), moduleNames.end(), [&buffer](const auto &iter) {
                    buffer.append(" " + iter + " ");
                });
            cerr << "Error: module name '" << moduleName_ << "' not in [" << buffer << "]" << endl;
            return RESTOOL_ERROR;
        }

        startId = ((it - moduleNames.begin()) + 1) * 0x01000000;
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
    bool forceWrite = packageParser_.GetForceWrite();
    bool combine = packageParser_.GetCombine();
    string output = packageParser_.GetOutput();
    string resourcesPath = FileEntry::FilePath(output).Append(RESOURCES_DIR).GetPath();
    if (ResourceUtil::FileExist(resourcesPath)) {
        if (!forceWrite) {
            cerr << "Error: output path exists." << NEW_LINE_PATH << resourcesPath << endl;
            return RESTOOL_ERROR;
        }

        if (!ResourceUtil::RmoveAllDir(resourcesPath)) {
            return combine ? RESTOOL_SUCCESS : RESTOOL_ERROR;
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
        config = ResourceUtil::GetMainPath(packageParser_.GetInputs()[0]).Append(CONFIG_JSON).GetPath();
        if (!ResourceUtil::FileExist(config)) {
            config = ResourceUtil::GetMainPath(packageParser_.GetInputs()[0]).Append(MODULE_JSON).GetPath();
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
        [&first](stringstream &buffer, const ResourceId& resourceId) {
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
    }, [](stringstream &buffer, const ResourceId& resourceId) {
        string name = resourceId.type + "_" + resourceId.name;
        transform(name.begin(), name.end(), name.begin(), ::toupper);
        buffer << "const int32_t " << name << " = ";
        buffer << "0x" << hex << setw(8)  << setfill('0') << resourceId.id << ";\n";
    }, [](stringstream &buffer) {
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
    }, [&itemType](stringstream &buffer, const ResourceId& resourceId) {
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
    }, [](stringstream &buffer) {
        buffer << "\n" << "    " << "}\n";
        buffer << "}\n";
    });
    return result;
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

void ResourcePack::CheckConfigJson()
{
    ResourceCheck resourceCheck(configJson_.GetCheckNode());
    resourceCheck.CheckConfigJson();
}

uint32_t ResourcePack::ScanResources(const vector<string> &inputs, const string &output)
{
    auto &fileManager = FileManager::GetInstance();
    fileManager.SetModuleName(moduleName_);
    if (packageParser_.GetOverlap()) {
        vector<string> hapResInput{inputs[0]};
        if (fileManager.ScanModules(hapResInput, output, configJson_.IsHar()) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
        fileManager.SetHapMode(false);
        vector<string> resInputs(inputs.begin() + 1, inputs.end());
        if (fileManager.ScanModules(resInputs, output, configJson_.IsHar()) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    } else {
        if (fileManager.ScanModules(inputs, output, configJson_.IsHar()) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::PackNormal()
{
    if (InitCompression() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (Init() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    ResourceMerge resourceMerge;
    if (resourceMerge.Init() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    BinaryFilePacker rawFilePacker(packageParser_, moduleName_);
    std::future<uint32_t> copyFuture = rawFilePacker.CopyBinaryFileAsync(resourceMerge.GetInputs());
    uint32_t packQualifierRet = PackQualifierResources(resourceMerge);
    if (packQualifierRet != RESTOOL_SUCCESS) {
        rawFilePacker.StopCopy();
    }
    uint32_t copyRet = copyFuture.get();
    return packQualifierRet == RESTOOL_SUCCESS && copyRet == RESTOOL_SUCCESS ? RESTOOL_SUCCESS : RESTOOL_ERROR;
}

uint32_t ResourcePack::PackQualifierResources(const ResourceMerge &resourceMerge)
{
    if (ScanResources(resourceMerge.GetInputs(), packageParser_.GetOutput()) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GenerateHeader() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (GenerateConfigJson() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (!FileManager::GetInstance().ScaleIcons(packageParser_.GetOutput(), configJson_.GetCheckNode())) {
        return RESTOOL_ERROR;
    }

    if (packageParser_.GetIconCheck()) {
        CheckConfigJson();
    }

    ResourceTable resourceTable;
    if (!packageParser_.GetDependEntry().empty()) {
        if (HandleFeature() != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
        if (GenerateHeader() != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }

    if (resourceTable.CreateResourceTable() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::HandleFeature()
{
    string output = packageParser_.GetOutput();
    string featureDependEntry = packageParser_.GetDependEntry();
    if (featureDependEntry.empty()) {
        return RESTOOL_SUCCESS;
    }
    string jsonFile = FileEntry::FilePath(featureDependEntry).Append(CONFIG_JSON).GetPath();
    ConfigParser entryJson(jsonFile);
    entryJson.SetDependEntry(true);
    if (entryJson.Init() != RESTOOL_SUCCESS) {
        cerr << "Error: config json invalid." << NEW_LINE_PATH << jsonFile << endl;
        return RESTOOL_ERROR;
    }

    int64_t labelId = entryJson.GetAbilityLabelId();
    int64_t iconId = entryJson.GetAbilityIconId();
    if (labelId <= 0 || iconId <= 0) {
        cerr << "Error: Entry MainAbility must have 'icon' and 'label'." << endl;
        return RESTOOL_ERROR;
    }
    string path = FileEntry::FilePath(featureDependEntry).Append(RESOURCE_INDEX_FILE).GetPath();
    map<int64_t, vector<ResourceItem>> resInfoLocal;
    ResourceTable resourceTable;
    if (resourceTable.LoadResTable(path, resInfoLocal) != RESTOOL_SUCCESS) {
        cerr << "Error: LoadResTable fail." << endl;
        return RESTOOL_ERROR;
    }
    jsonFile = FileEntry::FilePath(output).Append(CONFIG_JSON).GetPath();
    ConfigParser config(jsonFile);
    if (config.Init() != RESTOOL_SUCCESS) {
        cerr << "Error: config json invalid." << NEW_LINE_PATH << jsonFile << endl;
        return RESTOOL_ERROR;
    }
    vector<ResourceItem> items;
    if (FindResourceItems(resInfoLocal, items, labelId) != RESTOOL_SUCCESS ||
        HandleLabel(items, config) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    items.clear();
    if (FindResourceItems(resInfoLocal, items, iconId) != RESTOOL_SUCCESS ||
        HandleIcon(items, config) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    string outputPath = FileEntry::FilePath(output).Append(ConfigParser::GetConfigName()).GetPath();
    if (config.Save(outputPath) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    entryJson.SetDependEntry(false);
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::FindResourceItems(const map<int64_t, vector<ResourceItem>> &resInfoLocal,
                                         vector<ResourceItem> &items, int64_t id) const
{
    auto ret = resInfoLocal.find(id);
    if (ret == resInfoLocal.end()) {
        cerr << "Error: FindResourceItems don't found '" << id << "'." << endl;
        return RESTOOL_ERROR;
    }
    ResType type = ResType::INVALID_RES_TYPE;
    items = ret->second;
    if (items.empty()) {
        cerr << "Error: FindResourceItems resource item empty '" << id << "'." << endl;
        return RESTOOL_ERROR;
    }
    for (auto &it : items) {
        if (type == ResType::INVALID_RES_TYPE) {
            type = it.GetResType();
        }
        if (type != it.GetResType()) {
            cerr << "Error: FindResourceItems invalid restype '" << ResourceUtil::ResTypeToString(type);
            cerr << "' vs '"  << ResourceUtil::ResTypeToString(it.GetResType()) << "'." << endl;
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::HandleLabel(vector<ResourceItem> &items, ConfigParser &config) const
{
    int64_t nextId = 0;
    string idName;
    for (auto it : items) {
        if (it.GetResType() != ResType::STRING) {
            cerr << "Error: HandleLabel invalid restype '";
            cerr << ResourceUtil::ResTypeToString(it.GetResType()) << "'." << endl;
            return RESTOOL_ERROR;
        }
        idName = it.GetName() + "_entry";
        it.SetName(idName);
        string data(reinterpret_cast<const char *>(it.GetData()));
        if (it.GetDataLength() - 1 < 0) {
            return RESTOOL_ERROR;
        }
        if (!it.SetData(reinterpret_cast<const int8_t *>(data.c_str()), it.GetDataLength() - 1)) {
            return RESTOOL_ERROR;
        }
        if (nextId <= 0) {
            nextId = IdWorker::GetInstance().GenerateId(ResType::STRING, idName);
        }
        SaveResourceItem(it, nextId);
    }
    string label = "$string:" +idName;
    config.SetAppLabel(label, nextId);
    return RESTOOL_SUCCESS;
}

bool ResourcePack::CopyIcon(string &dataPath, const string &idName, string &fileName) const
{
    string featureDependEntry = packageParser_.GetDependEntry();
    string source = FileEntry::FilePath(featureDependEntry).Append(dataPath).GetPath();
    string suffix = FileEntry::FilePath(source).GetExtension();
    fileName = idName + suffix;
    string output = packageParser_.GetOutput();
#ifdef _WIN32
    ResourceUtil::StringReplace(dataPath, SEPARATOR, WIN_SEPARATOR);
#endif
    string dstDir = FileEntry::FilePath(output).Append(dataPath).GetParent().GetPath();
    string dst = FileEntry::FilePath(dstDir).Append(fileName).GetPath();
    if (!ResourceUtil::CreateDirs(dstDir)) {
        cerr << "Error: Create Dirs fail '" << dstDir << "'."<< endl;
        return false;
    }
    if (!ResourceUtil::CopyFileInner(source, dst)) {
        cerr << "Error: copy file fail from '" << source << "' to '" << dst << "'." << endl;
        return false;
    }
    return true;
}

uint32_t ResourcePack::HandleIcon(vector<ResourceItem> &items, ConfigParser &config) const
{
    int64_t nextId = 0;
    string idName;
    for (auto it : items) {
        if (it.GetResType() != ResType::MEDIA) {
            cerr << "Error: HandleLabel invalid restype '";
            cerr << ResourceUtil::ResTypeToString(it.GetResType()) << "'." << endl;
            return RESTOOL_ERROR;
        }
        string dataPath(reinterpret_cast<const char *>(it.GetData()));
        string::size_type pos = dataPath.find_first_of(SEPARATOR);
        if (pos == string::npos) {
            cerr << "Error: HandleIcon invalid '" << dataPath << "'."<< endl;
            return RESTOOL_ERROR;
        }
        dataPath = dataPath.substr(pos + 1);
        idName = it.GetName() + "_entry";
        string fileName;
        if (!CopyIcon(dataPath, idName, fileName)) {
            return RESTOOL_ERROR;
        }
        string data = FileEntry::FilePath(moduleName_).Append(dataPath).GetParent().Append(fileName).GetPath();
        ResourceUtil::StringReplace(data, WIN_SEPARATOR, SEPARATOR);
        ResourceItem resourceItem(fileName, it.GetKeyParam(), ResType::MEDIA);
        resourceItem.SetLimitKey(it.GetLimitKey());
        if (!resourceItem.SetData(reinterpret_cast<const int8_t *>(data.c_str()), data.length())) {
            return RESTOOL_ERROR;
        }
        if (nextId <= 0) {
            nextId = IdWorker::GetInstance().GenerateId(ResType::MEDIA, idName);
        }
        SaveResourceItem(resourceItem, nextId);
    }
    string icon = "$media:" + idName;
    config.SetAppIcon(icon, nextId);
    return RESTOOL_SUCCESS;
}

void ResourcePack::SaveResourceItem(const ResourceItem &resourceItem, int64_t nextId) const
{
    map<int64_t, vector<ResourceItem>> resInfo;
    vector<ResourceItem> vet;
    vet.push_back(resourceItem);
    resInfo.insert(make_pair(nextId, vet));
    FileManager &fileManager = FileManager::GetInstance();
    fileManager.MergeResourceItem(resInfo);
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

    if (packageParser_.GetIconCheck()) {
        CheckConfigJsonForCombine(resourceAppend);
    }

    if (GenerateHeader() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourcePack::PackOverlap()
{
    if (InitCompression() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (Init() != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    ResourceMerge resourceMerge;
    if (resourceMerge.Init(packageParser_) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    BinaryFilePacker rawFilePacker(packageParser_, moduleName_);
    rawFilePacker.SetCopyHapMode(true);
    std::future<uint32_t> copyFuture = rawFilePacker.CopyBinaryFileAsync(resourceMerge.GetInputs());

    if (LoadHapResources() != RESTOOL_SUCCESS) {
        rawFilePacker.StopCopy();
        return RESTOOL_ERROR;
    }

    if (PackQualifierResources(resourceMerge) != RESTOOL_SUCCESS) {
        rawFilePacker.StopCopy();
        return RESTOOL_ERROR;
    }

    uint32_t copyRet = copyFuture.get();
    return copyRet == RESTOOL_SUCCESS ? RESTOOL_SUCCESS : RESTOOL_ERROR;
}

uint32_t ResourcePack::LoadHapResources()
{
    ResourceTable resourceTabel;
    map<int64_t, vector<ResourceItem>> items;
    string resourceIndexPath =
        FileEntry::FilePath(packageParser_.GetInputs()[0]).GetParent().Append(RESOURCE_INDEX_FILE).GetPath();
    if (resourceTabel.LoadResTable(resourceIndexPath, items) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    IdWorker::GetInstance().LoadIdFromHap(items);

    FileManager &fileManager = FileManager::GetInstance();
    fileManager.SetModuleName(moduleName_);
    if (fileManager.MergeResourceItem(items) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    fileManager.SetHapItems();
    fileManager.SetHapMode(true);
    return RESTOOL_SUCCESS;
}

void ResourcePack::CheckConfigJsonForCombine(ResourceAppend &resourceAppend)
{
    ResourceCheck resourceCheck(configJson_.GetCheckNode(), make_shared<ResourceAppend>(resourceAppend));
    resourceCheck.CheckConfigJsonForCombine();
}

}
}
}
