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
#include<filesystem>
#include<iostream>
#include "factory_resource_compiler.h"
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
    string dbPath = filesystem::path(output).append("resources.db").string();
    database.Init(dbPath);
    if(!database.OpenDatabase()) {
        return RESTOOL_ERROR;
    }

    int32_t priority = 0;
    if (priority_ >= 0) {
        priority = priority_;
    }

    for (const auto &iter : modulePaths) {
        if (filesystem::is_directory(iter)) {
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
    fileInfo.filename = filesystem::path(filePath).filename().string();
    fileInfo.dirPath = filesystem::path(filePath).parent_path().string();
    fileInfo.fileCluster = filesystem::path(fileInfo.dirPath).filename().string();
    fileInfo.limitKey = filesystem::path(fileInfo.dirPath).parent_path().filename().string();

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