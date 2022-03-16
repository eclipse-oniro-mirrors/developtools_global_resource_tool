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
#include<fstream>
#include<iostream>
#include "dirent.h"
#include "sys/stat.h"
#include "unistd.h"

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
    struct dirent *entry;
    string filePath = filePath_.GetPath();
    DIR *handle = opendir(filePath.c_str());
    while ((entry = readdir(handle)) != nullptr) {
        string filename(entry->d_name);
        if (IsIgnore(filename)) {
            continue;
        }

        filePath = filePath_.GetPath() + SEPARATE + filename;
        unique_ptr<FileEntry> f = make_unique<FileEntry>(filePath);
        cout <<  filePath << endl;
        f->Init();
        children.push_back(move(f));
    }
    closedir(handle);
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
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
        return false;
    }
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

bool FileEntry::CopyFile(const string &src, const string &dst)
{
    ifstream in(src, ios::binary);
    ofstream out(dst, ios::binary);
    if (!in || !out) {
        cerr << "Error: CopyFile '" << src << "' or '" << dst << "' open fail." << endl;
        return false;
    }
    out << in.rdbuf();
    return true;
}

bool FileEntry::IsDirectory(const string &path)
{
    struct stat s;
    stat(path.c_str(), &s);
    return S_ISDIR(s.st_mode);
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
    if (entry.IsFile()) {
        return remove(entry.GetFilePath().GetPath().c_str()) == 0;
    }

    for (const auto &iter : entry.GetChilds()) {
        if (!RemoveAllDirInner(*iter)) {
            return false;
        }
    }
    return rmdir(entry.GetFilePath().GetPath().c_str()) == 0;
}

bool FileEntry::CreateDirsInner(const string &path, string::size_type offset)
{
    string::size_type pos = path.find_first_of(SEPARATE.front(), offset);
    if (pos == string::npos) {
#if _WIN32
        return mkdir(path.c_str());
#else
        return mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0;
#endif
    }

    string subPath = path.substr(0, pos);
    if (!Exist(subPath)) {
#if _WIN32
        if (mkdir(subPath.c_str()) != 0) {
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
}
}
}
