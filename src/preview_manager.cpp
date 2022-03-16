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

#include "preview_manager.h"
#include<iostream>
#include "factory_resource_compiler.h"
#include "file_entry.h"
#include "key_parser.h"
#include "resource_module.h"
#include "resource_util.h"
#include "sqlite_database.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
PreviewManager::~PreviewManager()
{
    SqliteDatabase &database = SqliteDatabase::GetInstance();
    database.CloseDatabase();
}

uint32_t PreviewManager::ScanModules(const vector<string> &modulePaths, const string &output)
{
    SqliteDatabase &database = SqliteDatabase::GetInstance();
    string dbPath = FileEntry::FilePath(output).Append("resources.db").GetPath();
    database.Init(dbPath);
    if (!database.OpenDatabase()) {
        return RESTOOL_ERROR;
    }

    int32_t priority = 0;
    if (priority_ >= 0) {
        priority = priority_;
    }

    for (const auto &iter : modulePaths) {
        if (FileEntry::IsDirectory(iter)) {
            ResourceModule resourceMoudle(iter, output, "");
            resourceMoudle.SetPreviewMode(true);
            database.SetPriority(priority);
            if (resourceMoudle.ScanResource() != RESTOOL_SUCCESS) {
                return RESTOOL_ERROR;
            }
        } else if (!ScanFile(iter, priority)) {
            return RESTOOL_ERROR;
        }
        if (priority_ >= 0) {
            continue;
        }
        priority++;
    }
    return RESTOOL_SUCCESS;
}

bool PreviewManager::ScanFile(const string &filePath, int32_t priority)
{
    if (!ResourceUtil::FileExist(filePath)) {
        cerr << "Error: " << filePath << " non't exist." << endl;
        return false;
    }
    FileInfo fileInfo;
    fileInfo.filePath = filePath;
    fileInfo.filename = FileEntry::FilePath(filePath).GetFilename();
    fileInfo.dirPath = FileEntry::FilePath(filePath).GetParent().GetPath();
    fileInfo.fileCluster = FileEntry::FilePath(fileInfo.dirPath).GetFilename();
    fileInfo.limitKey = FileEntry::FilePath(fileInfo.dirPath).GetParent().GetFilename();

    fileInfo.dirType = ResourceUtil::GetResTypeByDir(fileInfo.fileCluster);
    if (fileInfo.dirType == ResType::INVALID_RES_TYPE) {
        return false;
    }

    if (!KeyParser::Parse(fileInfo.limitKey, fileInfo.keyParams)) {
        return false;
    }

    unique_ptr<IResourceCompiler> resourceCompiler =
        FactoryResourceCompiler::CreateCompiler(fileInfo.dirType, "");
    resourceCompiler->SetPreviewMode(true);
    resourceCompiler->SetModuleName("");
    if (resourceCompiler->Compile(fileInfo) != RESTOOL_SUCCESS) {
        return false;
    }
    return true;
}
}
}
}
