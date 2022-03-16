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

#ifndef OHOS_RESTOOL_INCREMENT_LIST_H
#define OHOS_RESTOOL_INCREMENT_LIST_H

#include<functional>
#include<vector>
#include "resource_data.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
class IncrementList {
public:
    struct FileIncrement : FileInfo {
        bool isDeleted;
        std::string rootPath;
        std::string relativePath;
    };
    IncrementList(const std::string &listPath, const std::vector<std::string> &folder);
    virtual ~IncrementList() {};
    bool Parse(std::vector<FileIncrement> &fileList) const;

    static const std::string RESTOOL_LIST_FILE;
private:
    bool GetFromPath(const std::string &filePath, FileIncrement &info) const;
    int32_t GetPriority(const std::string &filePath) const;
    bool ParseSegment(const std::string &filePath, std::vector<std::string> &segments) const;
    bool IteratorArray(const Json::Value &array, std::function<bool(const std::string &)> callback) const;
    bool ParseArray(const Json::Value &array, std::vector<FileIncrement> &fileList, bool isDeleted) const;
    const std::string &listPath_;
    const std::vector<std::string> &folder_;
    enum PathSegment {
        SEG_RESOURCE = 0,
        SEG_LIMIT_KEY = 1,
        SEG_FILE_CLUSTER = 2,
        SEG_FILE_NAME = 3,
        SEG_MAX
    };
};
}
}
}
#endif
