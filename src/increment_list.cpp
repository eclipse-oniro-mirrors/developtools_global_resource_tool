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

#include "increment_list.h"
#include<algorithm>
#include<iostream>
#include "file_entry.h"
#include "key_parser.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const string IncrementList::RESTOOL_LIST_FILE = "restool_list.json";
IncrementList::IncrementList(const string &listPath, const vector<string> &folder)
    : listPath_(listPath), folder_(folder)
{
}

bool IncrementList::Parse(std::vector<FileIncrement> &fileList) const
{
    Json::Value listJson;
    if (!ResourceUtil::OpenJsonFile(listPath_, listJson)) {
        return false;
    }

    if (ParseArray(listJson["del"], fileList, true) &&
        ParseArray(listJson["fix"], fileList, false)) {
        return true;
    }
    return false;
}

// below private
bool IncrementList::GetFromPath(const string &filePath, FileIncrement &info) const
{
    int32_t priority = GetPriority(filePath);
    if (priority  < 0) {
        cerr << "Error: '" << RESTOOL_LIST_FILE << "' invalid." << NEW_LINE_PATH << filePath << endl;
        return false;
    }
    info.rootPath = folder_[priority];
    string rootPath = FileEntry::FilePath(info.rootPath).Append("").GetPath();
    info.relativePath = filePath.substr(rootPath.length());
    vector<string> segments;
    if (!ParseSegment(info.relativePath, segments)) {
        return false;
    }

    info.dirPath = FileEntry::FilePath(filePath).GetParent().GetPath();
    info.filePath = filePath;
    info.limitKey = segments[SEG_LIMIT_KEY];
    if (info.limitKey == RAW_FILE_DIR) {
        info.dirType = ResType::INVALID_RES_TYPE;
        return true;
    }
    if (!KeyParser::Parse(info.limitKey, info.keyParams)) {
        cerr << "Error: '" << info.limitKey << "' invalid limit key." << NEW_LINE_PATH << filePath << endl;
        return false;
    }

    info.fileCluster = segments[SEG_FILE_CLUSTER];
    info.dirType = ResourceUtil::GetResTypeByDir(info.fileCluster);
    if (info.dirType == ResType::INVALID_RES_TYPE) {
        cerr << "Error: '" << info.fileCluster << "' invalid ResType." << NEW_LINE_PATH << filePath << endl;
        return false;
    }
    info.filename = segments[SEG_FILE_NAME];
    return true;
}

int32_t IncrementList::GetPriority(const string &filePath) const
{
    auto result = find_if(folder_.begin(), folder_.end(), [&filePath](auto &iter) {
        string rootPath = FileEntry::FilePath(iter).Append("").GetPath();
        if (filePath.length() <= rootPath.length()) {
            return false;
        }
        if (filePath.compare(0, rootPath.length(), rootPath.c_str()) == 0) {
            return true;
        }
        return true;
    });
    if (result == folder_.end()) {
        return -1;
    }
    return (result - folder_.begin());
}

bool IncrementList::ParseSegment(const string &filePath, vector<string> &segments) const
{
    vector<string> segs = FileEntry::FilePath(filePath).GetSegments();
    copy(segs.begin(), segs.end(), back_inserter(segments));

    if (segments.size() >= (SEG_RESOURCE + 1) && segments[SEG_RESOURCE] != RESOURCES_DIR) {
        cerr << "Error: don't contain '" << RESOURCES_DIR << "'." << NEW_LINE_PATH << filePath << endl;
        return false;
    }

    if (segments.size() >= (SEG_LIMIT_KEY + 1) && segments[SEG_LIMIT_KEY] == RAW_FILE_DIR) {
        return true;
    }

    if (segments.size() != SEG_MAX) {
        cerr << "Error: segments != " << SEG_MAX << NEW_LINE_PATH << filePath << endl;
        return false;
    }
    return true;
}

bool IncrementList::IteratorArray(const Json::Value &array, function<bool(const string &)> callback) const
{
    if (array.empty()) {
        return true;
    }

    if (!array.isArray()) {
        cerr << "Error: "<< RESTOOL_LIST_FILE << ",not array." << endl;
        return false;
    }

    for (Json::ArrayIndex i = 0; i < array.size(); i++) {
        if (!array[i].isString()) {
            cerr << "Error: " << RESTOOL_LIST_FILE << ",not string." << endl;
            return false;
        }
        if (callback && !callback(array[i].asString())) {
            return false;
        }
    }
    return true;
}

bool IncrementList::ParseArray(const Json::Value &array, vector<FileIncrement> &fileList, bool isDeleted) const
{
    return IteratorArray(array, [this, &fileList, &isDeleted](const string &filePath) {
        FileIncrement info;
        info.isDeleted = isDeleted;
        if (!GetFromPath(filePath, info)) {
            return false;
        }
        fileList.push_back(info);
        return true;
    });
}
}
}
}