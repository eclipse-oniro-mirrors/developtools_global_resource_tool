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

#include "i_resource_compiler.h"
#include <algorithm>
#include <iostream>
#include "file_entry.h"
#include "id_worker.h"
#include "resource_util.h"
#include "restool_errors.h"
#include "sqlite_database.h"
#include "xml_converter.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
IResourceCompiler::IResourceCompiler(ResType type, const string &output)
    :type_(type), output_(output)
{
}

IResourceCompiler::~IResourceCompiler()
{
    nameInfos_.clear();
    resourceInfos_.clear();
}

uint32_t IResourceCompiler::Compile(const vector<DirectoryInfo> &directoryInfos)
{
    vector<FileInfo> fileInfos;
    map<string, vector<FileInfo>> setsByDirectory;
    for (const auto &directoryInfo : directoryInfos) {
        string outputFolder = GetOutputFolder(directoryInfo);
        FileEntry f(directoryInfo.dirPath);
        if (!f.Init()) {
            return RESTOOL_ERROR;
        }
        for (const auto &it : f.GetChilds()) {
            if (ResourceUtil::IsIgnoreFile(it->GetFilePath().GetFilename(), it->IsFile())) {
                continue;
            }

            if (!it->IsFile()) {
                cout << "Error: '" << it->GetFilePath().GetPath() << "' not regular." << endl;
                return RESTOOL_ERROR;
            }

            FileInfo fileInfo = { directoryInfo, it->GetFilePath().GetPath(), it->GetFilePath().GetFilename() };
            fileInfos.push_back(fileInfo);
            setsByDirectory[outputFolder].push_back(fileInfo);
        }
    }

    sort(fileInfos.begin(), fileInfos.end(), [](const auto &a, const auto &b) {
        return a.filePath < b.filePath;
    });
    for (const auto &fileInfo : fileInfos) {
        if (CompileSingleFile(fileInfo) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }

    if (previewMode_) {
        return RESTOOL_SUCCESS;
    }
    if (NeedIfConvertToSolidXml() && ConvertToSolidXml(setsByDirectory) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return PostCommit();
}

const map<int32_t, vector<ResourceItem>> &IResourceCompiler::GetResult() const
{
    return resourceInfos_;
}

uint32_t IResourceCompiler::Compile(const FileInfo &fileInfo)
{
    if (CompileSingleFile(fileInfo) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }

    if (previewMode_) {
        return RESTOOL_SUCCESS;
    }
    map<string, vector<FileInfo>> setsByDirectory;
    if (NeedIfConvertToSolidXml()) {
        string outputFolder = FileEntry::FilePath(output_).Append(RESOURCES_DIR)
            .Append(fileInfo.limitKey).Append(fileInfo.fileCluster).GetPath();
        setsByDirectory[outputFolder].push_back(fileInfo);
    }

    if (ConvertToSolidXml(setsByDirectory) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return PostCommit();
}

uint32_t IResourceCompiler::CompileForAppend(const FileInfo &fileInfo)
{
    return CompileSingleFile(fileInfo);
}

const map<pair<ResType, string>, vector<ResourceItem>> &IResourceCompiler::GetResourceItems() const
{
    return nameInfos_;
}

void IResourceCompiler::SetModuleName(const string &moduleName)
{
    moduleName_ = moduleName;
}

uint32_t IResourceCompiler::ConvertToSolidXml(const map<string, vector<FileInfo>> &setsByDirectory)
{
    for (const auto &iter : setsByDirectory) {
        vector<string> xmlPaths;
        ListXmlFile(iter.second, xmlPaths);
        if (xmlPaths.empty()) {
            continue;
        }

        string xmlOutPath = iter.first;
        if (!ResourceUtil::CreateDirs(xmlOutPath)) {
            return RESTOOL_ERROR;
        }

        sort(xmlPaths.begin(), xmlPaths.end());
        XmlConverter xmlConverter(xmlPaths, xmlOutPath);
        if (!xmlConverter.GenerateSolidXml()) {
            return RESTOOL_ERROR;
        }

        if (!xmlConverter.GenerateKey()) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t IResourceCompiler::CompileSingleFile(const FileInfo &fileInfo)
{
    return RESTOOL_SUCCESS;
}

uint32_t IResourceCompiler::PostCommit()
{
    IdWorker &idWorker = IdWorker::GetInstance();
    for (const auto &nameInfo : nameInfos_) {
        int32_t id = idWorker.GenerateId(nameInfo.first.first, nameInfo.first.second);
        if (id < 0) {
            cerr << "Error: restype='" << ResourceUtil::ResTypeToString(nameInfo.first.first) << "' name='";
            cerr << nameInfo.first.second << "' id not be defined." << endl;
            return RESTOOL_ERROR;
        }
        resourceInfos_.emplace(id, nameInfo.second);
    }
    return RESTOOL_SUCCESS;
}

bool IResourceCompiler::MergeResourceItem(const ResourceItem &resourceItem)
{
    if (previewMode_) {
        return SqliteDatabase::GetInstance().Insert(resourceItem);
    }
    string idName = ResourceUtil::GetIdName(resourceItem.GetName(), resourceItem.GetResType());
    if (!IdWorker::GetInstance().IsValidName(idName)) {
        cerr << "Error: invalid idName '" << idName << "'."<< NEW_LINE_PATH <<  resourceItem.GetFilePath() << endl;
        return false;
    }
    auto item = nameInfos_.find(make_pair(resourceItem.GetResType(), idName));
    if (item == nameInfos_.end()) {
        nameInfos_[make_pair(resourceItem.GetResType(), idName)].push_back(resourceItem);
        return true;
    }

    auto ret = find_if(item->second.begin(), item->second.end(), [resourceItem](auto &iter) {
        return resourceItem.GetLimitKey() == iter.GetLimitKey();
    });
    if (ret != item->second.end()) {
        cerr << "Error: resource '" << idName << "' first declared." << NEW_LINE_PATH << ret->GetFilePath() << endl;
        cerr << "but declare again." << NEW_LINE_PATH << resourceItem.GetFilePath() << endl;
        return false;
    }

    nameInfos_[make_pair(resourceItem.GetResType(), idName)].push_back(resourceItem);
    return true;
}

void IResourceCompiler::ListXmlFile(const vector<FileInfo> &fileInfos, vector<string> &xmlPaths) const
{
    for (const auto &fileInfo : fileInfos) {
        if (!IsXmlFile(fileInfo)) {
            continue;
        }

        if (HasConvertedToSolidXml(fileInfo)) {
            continue;
        }
        xmlPaths.push_back(fileInfo.filePath);
    }
}

bool IResourceCompiler::IsXmlFile(const FileInfo &fileInfo) const
{
    if (FileEntry::FilePath(fileInfo.filePath).GetExtension() != ".xml") {
        return false;
    }
    return true;
}

bool IResourceCompiler::HasConvertedToSolidXml(const FileInfo &fileInfo) const
{
    string solidXmlPath = FileEntry::FilePath(output_).Append(RESOURCES_DIR).Append(fileInfo.limitKey)
        .Append(fileInfo.fileCluster).Append(fileInfo.filename).ReplaceExtension(".sxml").GetPath();
    if (ResourceUtil::FileExist(solidXmlPath)) {
        return true;
    }
    return false;
}

bool IResourceCompiler::NeedIfConvertToSolidXml() const
{
    return ResourceUtil::NeedConverToSolidXml(type_);
}

string IResourceCompiler::GetOutputFolder(const DirectoryInfo &directoryInfo) const
{
    string outputFolder = FileEntry::FilePath(output_).Append(RESOURCES_DIR)
        .Append(directoryInfo.limitKey).Append(directoryInfo.fileCluster).GetPath();
    return outputFolder;
}
}
}
}
