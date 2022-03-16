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

#include "sqlite_database.h"
#include<iostream>

#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
int32_t SqliteDatabase::id_ = 0;
void SqliteDatabase::Init(const string &dbPath)
{
    dbPath_ = dbPath;
}

bool SqliteDatabase::OpenDatabase()
{
    int result = sqlite3_open(dbPath_.c_str(), &db_);
    if (result) {
        cerr << "Can't open database: " << sqlite3_errmsg(db_) << endl;
        return false;
    }

    if (!CreateTable()) {
        return false;
    }

    if (!FindMaxId()) {
        return false;
    }
    return true;
}

void SqliteDatabase::CloseDatabase()
{
    if (db_ == nullptr) {
        return;
    }
    sqlite3_close(db_);
    db_ = nullptr;
}

bool SqliteDatabase::Insert(const ResourceItem &resourceItem)
{
    if (resourceItem.GetResType() == ResType::ID) {
        return true;
    }

    int32_t id = -1;
    if (!Query(resourceItem, id)) {
        return false;
    }

    if (id >= 0) {
        return true;
    }

    string data = GetValue(resourceItem);
    string sql = "insert into resource_index values(" + to_string(id_) + ",'" + resourceItem.GetName() + "','" + \
        data + "'," + \
        to_string((int32_t)resourceItem.GetResType()) + ",'" + resourceItem.GetFilePath() + \
        "','" + resourceItem.GetLimitKey() + "'," + to_string(priority_) + ")";
    auto callback = [](void *owner, int argc, char **argv, char **azColName) -> int {
        return 0;
    };

    char *zErrMsg = 0;
    int result = sqlite3_exec(db_, sql.c_str(), callback, nullptr, &zErrMsg);
    if (result) {
        cerr << "insert sql error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    id_++;
    return true;
}

bool SqliteDatabase::Query(const ResourceItem &resourceItem, int32_t &id)
{
    string sql = "select id from resource_index where name='" + resourceItem.GetName() + \
        "' and type='" + to_string((int32_t)resourceItem.GetResType()) + \
        "' and limitkey='" + resourceItem.GetLimitKey() + \
        "' and priority=" + to_string(priority_);
    auto callback = [](void *id, int argc, char **argv, char **azColName) -> int {
        if (argc > 0 && id) {
            (*static_cast<int *>(id)) = atoi(argv[0]);
        }
        return 0;
    };

    id = -1;
    char *zErrMsg = 0;
    int result = sqlite3_exec(db_, sql.c_str(), callback, &id, &zErrMsg);
    if (result) {
        cerr << "query sql error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }

    if (id < 0) {
        return true;
    }

    sql = "update resource_index set value='" + GetValue(resourceItem) + \
         "', path='" + resourceItem.GetFilePath() + \
         "' where id = " + to_string(id);
    result = sqlite3_exec(db_, sql.c_str(), callback, nullptr, &zErrMsg);
    if (result) {
        cerr << "query sql error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool SqliteDatabase::FindMaxId()
{
    string sql = "select max(id) from resource_index";
    auto callback = [](void *owner, int argc, char **argv, char **azColName) -> int {
        if (argc > 0 && argv[0]) {
            id_ = atoi(argv[0]) + 1;
        }
        return 0;
    };

    char *zErrMsg = 0;
    int result = sqlite3_exec(db_, sql.c_str(), callback, nullptr, &zErrMsg);
    if (result) {
        cerr << "find max id sql error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool SqliteDatabase::CreateTable()
{
    string sql = "select * from sqlite_master where name = 'resource_index'";
    auto callback = [](void *find, int argc, char **argv, char **azColName) -> int {
        if (argc > 0 && find) {
            (*static_cast<bool *>(find)) = true;
        }
        return 0;
    };

    bool find = false;
    char *zErrMsg = 0;
    int result = sqlite3_exec(db_, sql.c_str(), callback, &find, &zErrMsg);
    if (result) {
        cerr << "search table error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }

    if (find) {
        return true;
    }

    sql = "create table resource_index(id int, name, value image, type int, path, limitkey, priority int)";
    result = sqlite3_exec(db_, sql.c_str(), callback, nullptr, &zErrMsg);
    if (result) {
        cerr << "create table error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

string SqliteDatabase::GetValue(const ResourceItem &resourceItem) const
{
    string data(reinterpret_cast<const char *>(resourceItem.GetData()), resourceItem.GetDataLength());
    if (resourceItem.GetResType() == ResType::STRARRAY || resourceItem.GetResType() == ResType::INTARRAY ||
        resourceItem.GetResType() == ResType::THEME || resourceItem.GetResType() == ResType::PATTERN ||
        resourceItem.GetResType() == ResType::PLURAL) {
        vector<string> contents = ResourceUtil::DecomposeStrings(data);
        Json::Value array(Json::arrayValue);
        for (const auto &iter : contents) {
            array.append(iter);
        }
        data = array.toStyledString();
    }
    return data;
}
}
}
}
