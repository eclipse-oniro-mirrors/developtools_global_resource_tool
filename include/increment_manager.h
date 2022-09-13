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

#ifndef OHOS_RESTOOL_INCREMENT_MANAGER_H
#define OHOS_RESTOOL_INCREMENT_MANAGER_H

#include<string>
#include "resource_util.h"
#include "resource_item.h"
#include "singleton.h"

#include "increment_list.h"
#include "increment_index.h"

namespace OHOS {
namespace Global {
namespace Restool {
class IncrementManager : public Singleton<IncrementManager> {
public:
    struct FileIncrement : FileInfo {
        std::string extension;
        std::string relativePath;
        std::string inputPath;
        int32_t priority;
    };

    struct ModuleInfo {
        std::string rootPath;
        std::vector<IncrementList::FileIncrement> fileIncrements;
    };

    virtual ~IncrementManager();
    uint32_t Init(const std::string &cachePath, const std::vector<std::string> &folder,
        const std::string &outputPath, const std::string &moduleName);
    const std::map<int32_t, std::vector<ResourceItem>> &GetResourceItems() const
    {
        return items_;
    };
    const std::map<ResType, std::vector<DirectoryInfo>> &GetScanDirs() const
    {
        return scanDirs_;
    };
    bool FirstIncrement() const
    {
        return firstIncrement_;
    };
    bool Enable() const
    {
        return enalbe_;
    };

    static const std::string ID_JSON_FILE;
private:
    bool InitIdJson();
    bool InitList(std::vector<IncrementList::FileIncrement> &dels) const;
    bool ScanModules(const std::vector<IncrementList::FileIncrement> &dels);
    void FlushId();
    bool SaveIdJson() const;
    bool LoadIdJson();
    void PushScanDir(const std::map<ResType, std::vector<DirectoryInfo>> &scanDirs);
    void DeleteRawFile(std::vector<IncrementList::FileIncrement> &dels) const;
    bool ClearSolidXml() const;
    std::string cachePath_;
    std::vector<std::string> folder_;
    std::string outputPath_;
    std::string moduleName_;
    std::map<int32_t, std::vector<ResourceItem>> items_;
    std::map<ResType, std::vector<DirectoryInfo>> scanDirs_;
    bool enalbe_ = false;
    bool firstIncrement_ = true;
};
}
}
}
#endif
