/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "resource_append.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <regex>
#include "config_parser.h"
#include "header.h"
#include "id_worker.h"
#include "key_parser.h"
#include "reference_parser.h"
#include "resource_table.h"
#include "resource_util.h"
#ifdef __WIN32
#include "windows.h"
#endif
#include "securec.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;

ResourceAppend::ResourceAppend(const PackageParser &packageParser) : packageParser_(packageParser)
{
}

uint32_t ResourceAppend::Append()
{
    string outputPath = packageParser_.GetOutput();
    for (const auto &iter : packageParser_.GetAppend()) {
        if (!ScanResources(iter, outputPath)) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

uint32_t ResourceAppend::Combine()
{
    for (const auto &iter : packageParser_.GetInputs()) {
        if (!Combine(iter)) {
            return RESTOOL_ERROR;
        }
    }

    if (!ParseRef()) {
        return false;
    }

    ResourceTable resourceTable;
    if (resourceTable.CreateResourceTable(items_) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

// private
bool ResourceAppend::Combine(const string &folderPath)
{
    FileEntry entry(folderPath);
    if (!entry.Init()) {
        return false;
    }

    itemsForModule_.clear();
    for (const auto &child : entry.GetChilds()) {
        if (!child->IsFile()) {
            cerr << "Error:" << child->GetFilePath().GetPath()  << " not file" << endl;
            return false;
        }

        if (!LoadResourceItem(child->GetFilePath().GetPath())) {
            return false;
        }
    }
    return true;
}

bool ResourceAppend::ParseRef()
{
    for (auto &iter : refs_) {
        ReferenceParser ref;
        if (iter->GetResType() == ResType::PROF) {
            string dst = FileEntry::FilePath(packageParser_.GetOutput()).Append(RESOURCES_DIR)
                .Append("base").Append("profile").Append(iter->GetName()).GetPath();
            if (ref.ParseRefInJson(iter->GetFilePath(), dst) != RESTOOL_SUCCESS) {
                return false;
            }

            if (ResourceUtil::FileExist(dst)) {
                iter->SetData(reinterpret_cast<const int8_t *>(dst.c_str()), dst.length());
            }
            continue;
        }

        if (ref.ParseRefInResourceItem(*iter) != RESTOOL_SUCCESS) {
            return false;
        }
    }
    return true;
}

bool ResourceAppend::ScanResources(const string &resourcePath, const string &outputPath)
{
    if (!ResourceUtil::FileExist(resourcePath)) {
        string filePath = FileEntry::FilePath(outputPath).Append(ResourceUtil::GenerateHash(resourcePath)).GetPath();
        if (remove(filePath.c_str()) != 0) {
            cerr << "Error: remove failed '" << filePath << "', reason: " << strerror(errno) << endl;
            return false;
        }
        return true;
    }

    FileEntry entry(resourcePath);
    if (!entry.Init()) {
        return false;
    }

    if (entry.IsFile()) {
        return ScanSingleFile(resourcePath, outputPath);
    }

    return ScanSubResources(entry, resourcePath, outputPath);
}

bool ResourceAppend::ScanSubResources(const FileEntry entry, const string &resourcePath, const string &outputPath)
{
    vector<KeyParam> keyParams;
    if (KeyParser::Parse(entry.GetFilePath().GetFilename(), keyParams)) {
        for (const auto &child : entry.GetChilds()) {
            if (!ResourceUtil::IslegalPath(child->GetFilePath().GetFilename())) {
                continue;
            }
            if (!ScanIegalResources(child->GetFilePath().GetPath(), outputPath)) {
                return false;
            }
        }
        return true;
    }

    if (ResourceUtil::IslegalPath(entry.GetFilePath().GetFilename())) {
        return ScanIegalResources(resourcePath, outputPath);
    }

    return ScanSubLimitkeyResources(entry, resourcePath, outputPath);
}

bool ResourceAppend::ScanSubLimitkeyResources(const FileEntry entry, const string &resourcePath,
    const string &outputPath)
{
    for (const auto &child : entry.GetChilds()) {
        string limitKey = child->GetFilePath().GetFilename();
        if (ResourceUtil::IsIgnoreFile(limitKey, child->IsFile())) {
            continue;
        }

        if (limitKey == RAW_FILE_DIR) {
            if (!ScanRawFiles(child->GetFilePath().GetPath(), outputPath)) {
                return false;
            }
            continue;
        }

        if (child->IsFile()) {
            cerr << "Error: " << child->GetFilePath().GetPath() << " not directory" << endl;
            return false;
        }

        if (!ScanLimitKey(child, limitKey, outputPath)) {
            return false;
        }
    }
    return true;
}

bool ResourceAppend::ScanIegalResources(const string &resourcePath, const string &outputPath)
{
    FileEntry entry(resourcePath);
    if (!entry.Init()) {
        return false;
    }
    for (const auto &child : entry.GetChilds()) {
        if (!ScanSingleFile(child->GetFilePath().GetPath(), outputPath)) {
            return false;
        }
    }
    return true;
}
bool ResourceAppend::ScanLimitKey(const unique_ptr<FileEntry> &entry,
    const string &limitKey, const string outputPath)
{
    vector<KeyParam> keyParams;
    if (!KeyParser::Parse(limitKey, keyParams)) {
        cerr << "Error: invalid limit key '" << limitKey << "'.";
        cerr << NEW_LINE_PATH << entry->GetFilePath().GetPath() << endl;
        return false;
    }

    for (const auto &child : entry->GetChilds()) {
        string fileCuster = child->GetFilePath().GetFilename();
        if (ResourceUtil::IsIgnoreFile(fileCuster, child->IsFile())) {
            continue;
        }

        if (child->IsFile()) {
            cerr << "Error: " << child->GetFilePath().GetPath() << " not directory" << endl;
            return false;
        }

        ResType resType = ResourceUtil::GetResTypeByDir(fileCuster);
        if (resType == ResType::INVALID_RES_TYPE) {
            cerr << "Error: invalid resType." << NEW_LINE_PATH << child->GetFilePath().GetPath() << endl;
            return false;
        }

        DirectoryInfo directoryInfo = { limitKey, fileCuster, child->GetFilePath().GetPath(), keyParams, resType};
        if (!ScanFiles(child, directoryInfo, outputPath)) {
            return false;
        }
    }
    return true;
}


bool ResourceAppend::ScanFiles(const unique_ptr<FileEntry> &entry,
    const DirectoryInfo &directoryInfo, const string &outputPath)
{
    for (const auto &child : entry->GetChilds()) {
        string filename =  child->GetFilePath().GetFilename();
        if (ResourceUtil::IsIgnoreFile(filename, child->IsFile())) {
            continue;
        }

        if (!child->IsFile()) {
            cerr << "Error: '" << child->GetFilePath().GetPath() << "' not file." << endl;
            return false;
        }

        FileInfo fileInfo = {directoryInfo, child->GetFilePath().GetPath(), filename};
        if (!ScanFile(fileInfo, outputPath)) {
            return false;
        }
    }
    return true;
}

bool ResourceAppend::ScanFile(const FileInfo &fileInfo, const string &outputPath)
{
    unique_ptr<IResourceCompiler> resourceCompiler =
        FactoryResourceCompiler::CreateCompilerForAppend(fileInfo.dirType, outputPath);
    if (resourceCompiler == nullptr) {
        return true;
    }

    if (resourceCompiler->CompileForAppend(fileInfo) != RESTOOL_SUCCESS) {
        return false;
    }

    ostringstream outStream;
    const auto &items = resourceCompiler->GetResourceItems();
    for (const auto &item : items) {
        for (const auto &resourceItem : item.second) {
            if (!WriteResourceItem(resourceItem, outStream)) {
                return false;
            }
        }
    }

    string hash = ResourceUtil::GenerateHash(fileInfo.filePath);
    FileEntry::FilePath output(outputPath);
    if (!WriteFileInner(outStream, output.Append(hash).GetPath())) {
        return false;
    }
    return true;
}

bool ResourceAppend::ScanSingleFile(const string &filePath, const string &outputPath)
{
    if (filePath.find(RAW_FILE_DIR) != string::npos) {
        return WriteRawFile(filePath, outputPath);
    }

    FileEntry::FilePath path(filePath);
    string fileCuster = path.GetParent().GetFilename();
    ResType resType = ResourceUtil::GetResTypeByDir(fileCuster);
    if (resType == ResType::INVALID_RES_TYPE) {
        cerr << "Error: invalid resType." << NEW_LINE_PATH << filePath << endl;
        return false;
    }

    string limitKey = path.GetParent().GetParent().GetFilename();
    vector<KeyParam> keyParams;
    if (!KeyParser::Parse(limitKey, keyParams)) {
        cerr << "Error: invalid limit key." << NEW_LINE_PATH << filePath << endl;
        return false;
    }

    DirectoryInfo directoryInfo = {limitKey, fileCuster, path.GetParent().GetPath(), keyParams, resType};
    FileInfo fileInfo = {directoryInfo, filePath, path.GetFilename() };
    if (!ScanFile(fileInfo, outputPath)) {
        return false;
    }
    return true;
}

bool ResourceAppend::WriteFileInner(ostringstream &outStream, const string &outputPath) const
{
#ifdef __WIN32
    HANDLE hWriteFile = CreateFile(outputPath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hWriteFile == INVALID_HANDLE_VALUE) {
        cerr << "Error: '" << outputPath << "' " << GetLastError() << endl;
        return false;
    }

    DWORD writeBytes;
    if (!WriteFile(hWriteFile, outStream.str().c_str(), outStream.tellp(), &writeBytes, nullptr)) {
        cerr << "Error: write '" << outputPath << "' " << GetLastError() << endl;
        CloseHandle(hWriteFile);
        return false;
    }
    CloseHandle(hWriteFile);
#else
    ofstream out(outputPath, ofstream::out | ofstream::binary);
    if (!out.is_open()) {
        cerr << "Error: open failed '" << outputPath << "', reason: " << strerror(errno) << endl;
        return false;
    }
    out << outStream.str();
#endif
    return true;
}

bool ResourceAppend::WriteResourceItem(const ResourceItem &resourceItem, ostringstream &out)
{
    int32_t size = resourceItem.GetName().length();
    out.write(reinterpret_cast<const char *>(&size), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(resourceItem.GetName().c_str()), size);

    size = resourceItem.GetLimitKey().length();
    out.write(reinterpret_cast<const char *>(&size), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(resourceItem.GetLimitKey().c_str()), size);

    size = resourceItem.GetFilePath().length();
    out.write(reinterpret_cast<const char *>(&size), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(resourceItem.GetFilePath().c_str()), size);

    int32_t type = static_cast<int32_t>(resourceItem.GetResType());
    out.write(reinterpret_cast<const char *>(&type), sizeof(int32_t));

    size = resourceItem.GetKeyParam().size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(int32_t));
    for (const auto &keyParam : resourceItem.GetKeyParam()) {
        out.write(reinterpret_cast<const char *>(&keyParam.keyType), sizeof(int32_t));
        out.write(reinterpret_cast<const char *>(&keyParam.value), sizeof(int32_t));
    }

    size =  resourceItem.GetDataLength();
    out.write(reinterpret_cast<const char *>(&size), sizeof(int32_t));
    out.write(reinterpret_cast<const char *>(resourceItem.GetData()), size);
    return true;
}

bool ResourceAppend::LoadResourceItem(const string &filePath)
{
#ifdef __WIN32
    return LoadResourceItemWin(filePath);
#else
    ifstream in(filePath, ifstream::in | ifstream::binary);
    if (!in.is_open()) {
        cerr << "Error: open failed '" << filePath << "', reason: " << strerror(errno) << endl;
        return false;
    }
    
    in.seekg(0, in.end);
    int32_t length = in.tellg();
    in.seekg(0, in.beg);
    if (length <= 0) {
        cerr << "Error: invalid file size = " << length << NEW_LINE_PATH << filePath << endl;
        return false;
    }
    char buffer[length];
    in.read(buffer, length);
    return LoadResourceItemFromMem(buffer, length);
#endif
}

bool ResourceAppend::ScanRawFiles(const string &path, const string &outputPath)
{
    FileEntry entry(path);
    if (!entry.Init()) {
        return false;
    }

    for (const auto &child : entry.GetChilds()) {
        string filename =  child->GetFilePath().GetFilename();
        if (ResourceUtil::IsIgnoreFile(filename, child->IsFile())) {
            continue;
        }

        bool ret = false;
        if (child->IsFile()) {
            ret = WriteRawFile(child->GetFilePath().GetPath(), outputPath);
        } else {
            ret = ScanRawFiles(child->GetFilePath().GetPath(), outputPath);
        }

        if (!ret) {
            return false;
        }
    }
    return true;
}

bool ResourceAppend::WriteRawFile(const string &filePath, const string &outputPath)
{
    string::size_type pos = filePath.find(RAW_FILE_DIR);
    if (pos == string::npos) {
        cerr << "Error: invaild raw file." << NEW_LINE_PATH << filePath << endl;
        return false;
    }

    string sub = filePath.substr(pos);
    sub = FileEntry::FilePath(RESOURCES_DIR).Append(sub).GetPath();
    vector<KeyParam> keyParams;
    ResourceItem resourceItem("", keyParams, ResType::RAW);
    resourceItem.SetData(sub);
    resourceItem.SetFilePath(filePath);
    resourceItem.SetLimitKey("");

    ostringstream outStream;
    if (!WriteResourceItem(resourceItem, outStream)) {
        return false;
    }

    string hash = ResourceUtil::GenerateHash(filePath);
    FileEntry::FilePath output(outputPath);
    if (!WriteFileInner(outStream, output.Append(hash).GetPath())) {
        return false;
    }
    return true;
}

bool ResourceAppend::Push(const shared_ptr<ResourceItem> &resourceItem)
{
    string idName = ResourceUtil::GetIdName(resourceItem->GetName(), resourceItem->GetResType());
    int32_t id = IdWorker::GetInstance().GenerateId(resourceItem->GetResType(), idName);
    if (id < 0) {
        return false;
    }

    if (!CheckModuleResourceItem(resourceItem, id)) {
        return false;
    }

    const auto &result = items_.find(id);
    if (result == items_.end()) {
        items_[id].push_back(resourceItem);
        AddRef(resourceItem);
        return true;
    }

    if (find_if(result->second.begin(), result->second.end(), [resourceItem](auto &iter) {
              return resourceItem->GetLimitKey() == iter->GetLimitKey();
        }) != result->second.end()) {
        return true;
    }

    items_[id].push_back(resourceItem);
    AddRef(resourceItem);
    return true;
}

void ResourceAppend::AddRef(const shared_ptr<ResourceItem> &resourceItem)
{
    string data(reinterpret_cast<const char *>(resourceItem->GetData()), resourceItem->GetDataLength());
    ResType resType = resourceItem->GetResType();
    if (resType == ResType::MEDIA) {
        return;
    }

    if (resType == ResType::PROF) {
        if (resourceItem->GetLimitKey() != "base" ||
            FileEntry::FilePath(resourceItem->GetFilePath()).GetExtension() != ".json") {
            return;
        }
        refs_.push_back(resourceItem);
        return;
    }

    if (regex_match(data, regex(".*\\$.+:.*"))) {
        refs_.push_back(resourceItem);
    }
}

bool ResourceAppend::LoadResourceItemFromMem(const char buffer[], int32_t length)
{
    int32_t offset = 0;
    do {
        // name
        string nameStr = ParseString(buffer, length, offset);
        // limit key
        string limitKeyStr = ParseString(buffer, length, offset);
        // file path
        string filePathStr = ParseString(buffer, length, offset);
        // ResType
        int32_t type = ParseInt32(buffer, length, offset);
        ResType resType = static_cast<ResType>(type);
        // keyParam
        int32_t keyParamSize = ParseInt32(buffer, length, offset);
        vector<KeyParam> keyParams;
        for (int i = 0; i < keyParamSize; i++) {
            KeyParam keyParam;
            keyParam.keyType = static_cast<KeyType>(ParseInt32(buffer, length, offset));
            keyParam.value = ParseInt32(buffer, length, offset);
            keyParams.push_back(keyParam);
        }
        // data
        string data = ParseString(buffer, length, offset);
        if (resType ==  ResType::RAW) {
            FileEntry::FilePath outPath(packageParser_.GetOutput());
            if (!ResourceUtil::CreateDirs(outPath.Append(data).GetParent().GetPath())) {
                return false;
            }

            if (!ResourceUtil::FileExist(filePathStr)) {
                continue;
            }

            if (!ResourceUtil::CopyFleInner(filePathStr, outPath.Append(data).GetPath())) {
                return false;
            }
            continue;
        }

        shared_ptr<ResourceItem> resourceItem = make_shared<ResourceItem>(nameStr, keyParams, resType);
        resourceItem->SetData(reinterpret_cast<const int8_t *>(data.c_str()), data.length());
        resourceItem->SetLimitKey(limitKeyStr);
        resourceItem->SetFilePath(filePathStr);
        if (!Push(resourceItem)) {
            return false;
        }
    } while (offset < length);
    return true;
}

string ResourceAppend::ParseString(const char buffer[], int32_t length, int32_t &offset) const
{
    int32_t size = ParseInt32(buffer, length, offset);
    if (size < 0 || offset + size > length) {
        offset = length;
        return "";
    }

    if (size == 0) {
        return "";
    }

    string value(buffer + offset, size);
    offset += size;
    return value;
}

int32_t ResourceAppend::ParseInt32(const char buffer[], int32_t length, int32_t &offset) const
{
    if (offset + static_cast<int32_t>(sizeof(int32_t)) > length) {
        offset = length;
        return -1;
    }

    int32_t size = 0;
    if (memcpy_s(&size, sizeof(int32_t), buffer  + offset, sizeof(int32_t)) != EOK) {
        offset = length;
        return -1;
    }
    offset += sizeof(int32_t);
    return size;
}

bool ResourceAppend::CheckModuleResourceItem(const shared_ptr<ResourceItem> &resourceItem, int32_t  id)
{
    const auto &result = itemsForModule_.find(id);
    if (result == itemsForModule_.end()) {
        itemsForModule_[id].push_back(resourceItem);
        return true;
    }

    const auto &ret = find_if(result->second.begin(), result->second.end(), [resourceItem](auto iter) {
             return  resourceItem->GetLimitKey() == iter->GetLimitKey();
    });

    if (ret != result->second.end()) {
        cerr << "Error: '" << resourceItem->GetName() << "' conflict, first declared.";
        cerr << NEW_LINE_PATH << (*ret)->GetFilePath() << endl;
        cerr << "but declared again." << NEW_LINE_PATH << resourceItem->GetFilePath() << endl;
        return false;
    }

    itemsForModule_[id].push_back(resourceItem);
    return true;
}

#ifdef __WIN32
bool ResourceAppend::LoadResourceItemWin(const string &filePath)
{
    bool result = false;
    HANDLE hReadFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_READONLY | FILE_FLAG_RANDOM_ACCESS, nullptr);
    if (hReadFile == INVALID_HANDLE_VALUE) {
        cerr << "Error: "<< GetLastError() << NEW_LINE_PATH << filePath << endl;
        return result;
    }

    DWORD fileSize = GetFileSize(hReadFile, nullptr);
    HANDLE hFileMap = CreateFileMapping(hReadFile, nullptr, PAGE_READONLY, 0, fileSize, nullptr);
    if (hFileMap == INVALID_HANDLE_VALUE) {
        cerr << "Error: create file mapping " << GetLastError() << endl;
        CloseHandle(hReadFile);
        return result;
    }

    void* pBuffer = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
    if (pBuffer == nullptr) {
        cerr << "Error: map view of file " << GetLastError() << endl;
        CloseHandle(hReadFile);
        return result;
    }

    char* buffer = reinterpret_cast<char *>(pBuffer);
    result = LoadResourceItemFromMem(buffer, fileSize);
    UnmapViewOfFile(hFileMap);
    CloseHandle(hReadFile);
    return result;
}
#endif
}
}
}