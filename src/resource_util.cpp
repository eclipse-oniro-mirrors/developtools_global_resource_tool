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

#include "resource_util.h"
#include<algorithm>
#include<fstream>
#include<iostream>
#include<regex>
#include<cstdlib>
#include "file_entry.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const map<string, ResourceUtil::IgnoreType> ResourceUtil::IGNORE_FILE_REGEX = {
    { "\\.git", IgnoreType::IGNORE_ALL },
    { "\\.svn", IgnoreType::IGNORE_ALL },
    { ".+\\.scc", IgnoreType::IGNORE_ALL },
    { "\\.ds_store", IgnoreType::IGNORE_ALL },
    { "desktop\\.ini", IgnoreType::IGNORE_ALL },
    { "picasa\\.ini", IgnoreType::IGNORE_ALL },
    { "\\..+", IgnoreType::IGNORE_ALL },
    { "_.+", IgnoreType::IGNORE_DIR },
    { "cvs", IgnoreType::IGNORE_ALL },
    { "thumbs\\.db", IgnoreType::IGNORE_ALL },
    { ".+~", IgnoreType::IGNORE_ALL }
};

void ResourceUtil::Split(const string &str, vector<string> &out, const string &splitter)
{
    string::size_type len = str.size();
    string::size_type begin = 0;
    string::size_type end = str.find(splitter, begin);
    while (end != string::npos) {
        string sub = str.substr(begin, end - begin);
        out.push_back(sub);
        begin = end + splitter.size();
        if (begin >= len) {
            break;
        }
        end = str.find(splitter, begin);
    }

    if (begin < len) {
        out.push_back(str.substr(begin));
    }
}

bool ResourceUtil::FileExist(const string &path)
{
    return FileEntry::Exist(path);
}

bool ResourceUtil::RmoveAllDir(const string &path)
{
    return FileEntry::RemoveAllDir(path);
}

bool ResourceUtil::OpenJsonFile(const string &path, Json::Value &root)
{
    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) {
        cerr << "Error: open json failed '" << path << "', reason: " << strerror(errno) << endl;
        return false;
    }

    Json::CharReaderBuilder readBuilder;
    readBuilder["collectComments"] = false;
    readBuilder["failIfExtra"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(readBuilder, ifs, &root, &errs)) {
        cerr << "Error: parseFromStream failed." << NEW_LINE_PATH << path;
        cerr << "\n" << errs << endl;
        ifs.close();
        return false;
    }
    ifs.close();
    return true;
}

bool ResourceUtil::SaveToJsonFile(const string &path, const Json::Value &root)
{
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "    ";
    writerBuilder["emitUTF8"] = true;
    unique_ptr<Json::StreamWriter> writer(writerBuilder.newStreamWriter());
    ofstream out(path, ofstream::out | ofstream::binary);
    if (!out.is_open()) {
        cerr << "Error: open failed '" << path <<"', reason: " << strerror(errno) << endl;
        return false;
    }
    writer->write(root, &out);
    out.close();
    return true;
}

ResType ResourceUtil::GetResTypeByDir(const string &name)
{
    auto ret = g_fileClusterMap.find(name);
    if (ret == g_fileClusterMap.end()) {
        return ResType::INVALID_RES_TYPE;
    }
    return ret->second;
}

string ResourceUtil::ResTypeToString(ResType type)
{
    auto ret = find_if(g_fileClusterMap.begin(), g_fileClusterMap.end(), [type](auto iter) {
        return iter.second == type;
    });
    if (ret != g_fileClusterMap.end()) {
        return ret->first;
    }

    ret = find_if(g_contentClusterMap.begin(), g_contentClusterMap.end(), [type](auto iter) {
        return iter.second == type;
    });
    if (ret != g_contentClusterMap.end()) {
        return ret->first;
    }
    return "";
}

string ResourceUtil::GetIdName(const string &name, ResType type)
{
    if (type != ResType::MEDIA && type != ResType::LAYOUT && type != ResType::PROF &&
        type != ResType::ANIMATION && type != ResType::GRAPHIC) {
        return name;
    }

    string::size_type pos = name.find_last_of(".");
    if (pos != string::npos) {
        return name.substr(0, pos);
    }
    return name;
}

string ResourceUtil::ComposeStrings(const vector<string> &contents, bool addNull)
{
    string result;
    for (const auto &iter : contents) {
        if (iter.length() > UINT16_MAX) {
            return "";
        }

        uint16_t size = iter.length();
        if (addNull) {
            size += sizeof(char);
        }
        result.append(sizeof(char), (size & 0xff));
        result.append(sizeof(char), (size >> 8));
        result.append(iter);
        result.append(sizeof(char), '\0');
        if (result.length() > UINT16_MAX) {
            return "";
        }
    }
    return result;
}

vector<string> ResourceUtil::DecomposeStrings(const string &content)
{
    vector<string> result;
    size_t length = content.length();
    size_t pos = 0;
    const size_t HEAD_LENGTH = 2;
    while (pos < length) {
        if (pos + HEAD_LENGTH >= length) {
            result.clear();
            return result;
        }
        uint16_t size = (content[pos] & 0xff) | ((content[pos + 1] & 0xff) << 8);
        pos += HEAD_LENGTH;

        if (pos + size >= length) {
            result.clear();
            return result;
        }
        string buffer = content.substr(pos, size);
        result.push_back(buffer);
        pos += size + sizeof(char);
    }
    return result;
}

ResType ResourceUtil::GetResTypeFromString(const string &type)
{
    ResType resType = GetResTypeByDir(type);
    if (resType != ResType::INVALID_RES_TYPE) {
        return resType;
    }

    auto ret = g_contentClusterMap.find(type);
    if (ret != g_contentClusterMap.end()) {
        return ret->second;
    }
    return ResType::INVALID_RES_TYPE;
}

bool ResourceUtil::CopyFleInner(const string &src, const string &dst)
{
    if (!FileEntry::CopyFileInner(src, dst)) {
        cerr << "Error: copy file fail from '" << src << "' to '" << dst << "'." << endl;
        return false;
    }
    return true;
}

bool ResourceUtil::CreateDirs(const string &filePath)
{
    if (FileExist(filePath)) {
        return true;
    }
    
    if (!FileEntry::CreateDirs(filePath)) {
        cerr << "Error: create dir fail '" << filePath << "'." << endl;
        return false;
    }
    return true;
}

bool ResourceUtil::IsIgnoreFile(const string &filename, bool isFile)
{
    string key = filename;
    transform(key.begin(), key.end(), key.begin(), ::tolower);
    for (const auto &iter : IGNORE_FILE_REGEX) {
        if ((iter.second == IgnoreType::IGNORE_FILE && !isFile) ||
            (iter.second == IgnoreType::IGNORE_DIR && isFile)) {
            continue;
        }
        if (regex_match(key, regex(iter.first))) {
            return true;
        }
    }
    return false;
}

bool ResourceUtil::NeedConverToSolidXml(ResType resType)
{
    if (resType == ResType::LAYOUT || resType == ResType::ANIMATION ||
        resType == ResType::GRAPHIC) {
        return true;
    }
    return false;
}

string ResourceUtil::GenerateHash(const string &key)
{
    hash<string> hash_function;
    return to_string(hash_function(key));
}

string ResourceUtil::RealPath(const string &path)
{
    return FileEntry::RealPath(path);
}

bool ResourceUtil::IslegalPath(const string &path)
{
    return path == "element" || path == "media" || path == "profile";
}
}
}
}
