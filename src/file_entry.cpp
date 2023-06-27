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

#include "file_entry.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include "dirent.h"
#include "sys/stat.h"
#include "unistd.h"
#ifdef _WIN32
#include "shlwapi.h"
#include "windows.h"
#endif
#include "resource_data.h"

namespace OHOS {
namespace Global {
namespace Restool {
#ifdef _WIN32
const std::string FileEntry::SEPARATE = "\\";
#else
const std::string FileEntry::SEPARATE = "/";
#endif

using namespace std;
FileEntry::FileEntry(const string &path)
    : filePath_(path), isFile_(false)
{
}

FileEntry::~FileEntry()
{
}

bool FileEntry::Init()
{
    string filePath = filePath_.GetPath();
    if (!Exist(filePath)) {
        cerr << "Error: '" << filePath << "' not exists." << endl;
        return false;
    }

    isFile_ = !IsDirectory(filePath);
    return true;
}

const vector<unique_ptr<FileEntry>> FileEntry::GetChilds() const
{
    vector<unique_ptr<FileEntry>> children;
    string filePath = filePath_.GetPath();
#ifdef _WIN32
    WIN32_FIND_DATA findData;
    string temp(filePath + "\\*.*");
    HANDLE handle = FindFirstFile(AdapateLongPath(temp).c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE) {
        return children;
    }

    do {
        string filename(findData.cFileName);
        if (IsIgnore(filename)) {
            continue;
        }

        filePath = filePath_.GetPath() + SEPARATE + filename;
        unique_ptr<FileEntry> f = make_unique<FileEntry>(filePath);
        f->Init();
        children.push_back(move(f));
    } while (FindNextFile(handle, &findData));
    FindClose(handle);
#else
    DIR *handle = opendir(filePath.c_str());
    struct dirent *entry;
    while ((entry = readdir(handle)) != nullptr) {
        string filename(entry->d_name);
        if (IsIgnore(filename)) {
            continue;
        }

        filePath = filePath_.GetPath() + SEPARATE + filename;
        unique_ptr<FileEntry> f = make_unique<FileEntry>(filePath);
        f->Init();
        children.push_back(move(f));
    }
    closedir(handle);
#endif
    return children;
}

bool FileEntry::IsFile() const
{
    return isFile_;
}

const FileEntry::FilePath &FileEntry::GetFilePath() const
{
    return filePath_;
}

bool FileEntry::Exist(const string &path)
{
#ifdef _WIN32
    return PathFileExists(AdapateLongPath(path).c_str());
#else
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
        return false;
    }
#endif
    return true;
}

bool FileEntry::RemoveAllDir(const string &path)
{
    FileEntry f(path);
    if (!f.Init()) {
        return false;
    }

    if (f.IsFile()) {
        cerr << "Error: RemoveAllDir '" << path << "' not directory." << endl;
        return false;
    }
    return RemoveAllDirInner(f);
}

bool FileEntry::CreateDirs(const string &path)
{
    return CreateDirsInner(path, 0);
}

bool FileEntry::CopyFileInner(const string &src, const string &dst)
{
#ifdef _WIN32
    if (!CopyFile(AdapateLongPath(src).c_str(), AdapateLongPath(dst).c_str(), false)) {
        cerr << "Error: copy file fail from '" << src << "' to '" << dst << "'. reason:" << strerror(errno) << endl;
        return false;
    }
#else
    ifstream in(src, ios::binary);
    ofstream out(dst, ios::binary);
    if (!in || !out) {
        cerr << "Error: open failed '" << src << "' or '" << dst << "'. reason:" << strerror(errno) << endl;
        return false;
    }
    out << in.rdbuf();
#endif
    return true;
}

bool FileEntry::IsDirectory(const string &path)
{
#ifdef _WIN32
    if (!PathIsDirectory(AdapateLongPath(path).c_str())) {
        return false;
    }
    return true;
#else
    struct stat s;
    stat(path.c_str(), &s);
    return S_ISDIR(s.st_mode);
#endif
}

string FileEntry::RealPath(const string &path)
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (!PathCanonicalize(buffer, path.c_str())) {
        return "";
    }

    if (PathIsRelative(buffer)) {
        char current[MAX_PATH];
        if (!GetCurrentDirectory(MAX_PATH, current)) {
            return "";
        }

        char temp[MAX_PATH];
        if (!PathCombine(temp, current, buffer)) {
            return "";
        }
        if (!Exist(string(temp))) {
            return "";
        }
        return string(temp);
    }
#else
    char buffer[PATH_MAX];
    if (!realpath(path.c_str(), buffer)) {
        return "";
    }
#endif
    if (!Exist(string(buffer))) {
        return "";
    }
    return string(buffer);
}

FileEntry::FilePath::FilePath(const string &path) : filePath_(path)
{
    Format();
    Init();
}

FileEntry::FilePath::~FilePath()
{
}

FileEntry::FilePath FileEntry::FilePath::Append(const string &path)
{
    Format();
    string filePath = filePath_ + SEPARATE + path;
    return FilePath(filePath);
}

FileEntry::FilePath FileEntry::FilePath::ReplaceExtension(const string &extension)
{
    string filePath;
    if (!parent_.empty()) {
        filePath += parent_ + SEPARATE;
    }

    filePath += filename_.substr(0, filename_.length() - extension_.length()) + extension;
    return FilePath(filePath);
}

FileEntry::FilePath FileEntry::FilePath::GetParent()
{
    return FilePath(parent_);
}

const string &FileEntry::FilePath::GetPath() const
{
    return filePath_;
}

const string &FileEntry::FilePath::GetFilename() const
{
    return filename_;
}

const string &FileEntry::FilePath::GetExtension() const
{
    return extension_;
}

const vector<string> FileEntry::FilePath::GetSegments() const
{
    vector<string> segments;
    string::size_type offset = 0;
    string::size_type pos = filePath_.find_first_of(SEPARATE.front(), offset);
    while (pos != string::npos) {
        segments.push_back(filePath_.substr(offset, pos - offset));
        offset = pos + 1;
        pos = filePath_.find_first_of(SEPARATE.front(), offset);
    }

    if (offset < filePath_.length()) {
        segments.push_back(filePath_.substr(offset));
    }
    return segments;
}

// below private
bool FileEntry::IsIgnore(const string &filename) const
{
    if (filename == "." || filename == "..") {
        return true;
    }
    return false;
}

bool FileEntry::RemoveAllDirInner(const FileEntry &entry)
{
    string path = entry.GetFilePath().GetPath();
    if (entry.IsFile()) {
#ifdef _WIN32
        bool result = remove(AdapateLongPath(path).c_str()) == 0;
#else
        bool result = remove(path.c_str()) == 0;
#endif
        if (!result) {
            cerr << "Error: remove file '" << path << "' failed, reason: " << strerror(errno) << endl;
            return false;
        }
        return true;
    }

    for (const auto &iter : entry.GetChilds()) {
        if (!RemoveAllDirInner(*iter)) {
            return false;
        }
    }
#ifdef _WIN32
    bool result = rmdir(AdapateLongPath(path).c_str()) == 0;
#else
    bool result = rmdir(path.c_str()) == 0;
#endif
    if (!result) {
        cerr << "Error: remove directory '" << path << "' failed, reason: " << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool FileEntry::CreateDirsInner(const string &path, string::size_type offset)
{
    string::size_type pos = path.find_first_of(SEPARATE.front(), offset);
    if (pos == string::npos) {
#ifdef _WIN32
        return CreateDirectory(AdapateLongPath(path).c_str(), nullptr) != 0;
#else
        return mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0;
#endif
    }

    string subPath = path.substr(0, pos + 1);
    if (!Exist(subPath)) {
#ifdef _WIN32
        if (!CreateDirectory(AdapateLongPath(subPath).c_str(), nullptr)) {
#else
        if (mkdir(subPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
#endif
            return false;
        }
    }
    return CreateDirsInner(path, pos + 1);
}

void FileEntry::FilePath::Format()
{
    if (filePath_.back() != SEPARATE.front()) {
        return;
    }
    filePath_.pop_back();
}

void FileEntry::FilePath::Init()
{
    filename_ = filePath_;
    string::size_type pos = filePath_.find_last_of(SEPARATE.front());
    if (pos != string::npos) {
        parent_ = filePath_.substr(0, pos);
        if (pos + 1 < filePath_.length()) {
            filename_ = filePath_.substr(pos + 1);
        }
    }

    pos = filename_.find_last_of('.');
    if (pos != string::npos && pos + 1 < filename_.length()) {
        extension_ = filename_.substr(pos);
    }
}

string FileEntry::AdapateLongPath(const string &path)
{
#ifdef _WIN32
    if (path.size() >= MAX_PATH -12) { //the max file path can not exceed 260 - 12
        return LONG_PATH_HEAD + path;
    }
#endif
    return path;
}
}
}
}
