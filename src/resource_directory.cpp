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

#include "resource_directory.h"
#include<iostream>
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
bool ResourceDirectory::ScanResources(const string &resourcesDir, function<bool(const DirectoryInfo&)> callback) const
{
    for (auto &it : filesystem::directory_iterator(resourcesDir)) {
        string limitKey = it.path().filename().string();
        if (ResourceUtil::IsIgnoreFile(limitKey, it.status().type())) {
            continue;
        }

        if (!it.is_directory()) {
            cerr << "Error: '" << it.path().string() << "' not directory." << endl;
            return false;
        }

        if (limitKey == RAW_FILE_DIR) {
            continue;
        }

        if (!ScanResourceLimitKeyDir(it.path().string(), limitKey, callback)) {
            return false;
        }
    }
    return true;
}

// below private
bool ResourceDirectory::ScanResourceLimitKeyDir(const string &resourceTypeDir, const string &limitKey,
        function<bool(const DirectoryInfo&)> callback) const
{
    vector<KeyParam> keyParams;
    if (!KeyParser::Parse(limitKey, keyParams)) {
        cerr << "Error: " << resourceTypeDir << ",invalid limit key '" << limitKey << "'" << endl;
        return false;
    }

    for (auto &it : filesystem::directory_iterator(resourceTypeDir)) {
        string dirPath = it.path().string();
        string fileCluster = it.path().filename().string();
        if (ResourceUtil::IsIgnoreFile(fileCluster, it.status().type())) {
            continue;
        } 

        if (!it.is_directory()) {
            cerr << "Error: '" << dirPath << "' not directory." << endl;
            return false;
        }

        ResType type = ResourceUtil::GetResTypeByDir(fileCluster);
        if (type == ResType::INVALID_RES_TYPE) {
            string array("[ ");
            for (auto item : g_fileClusterMap) {
                array.append("'" + item.first + "' ");
            }
            array.append("]");
            cerr << "Error: " << dirPath << " ,invalid directory name '";
            cerr << fileCluster << "', must in " << array << endl;
            return false;
        }
        DirectoryInfo info = { limitKey, fileCluster, dirPath, keyParams, type };
        if (callback && !callback(info)) {
            return false;
        }
    }
    return true;
}
}
}
}