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

#ifndef OHOS_RESTOOL_SQLITE_DATABASE_H
#define OHOS_RESTOOL_SQLITE_DATABASE_H

#include<string>
#include "resource_item.h"
#include "singleton.h"
#include "sqlite3.h"

namespace OHOS {
namespace Global {
namespace Restool {
class SqliteDatabase : public Singleton<SqliteDatabase> {
public:
    void Init(const std::string &dbPath);
    bool OpenDatabase();
    void CloseDatabase();
    bool Insert(const ResourceItem &resourceItem);
    void SetPriority(int32_t priority)
    {
        priority_ = priority;
    };
private:
    bool Query(const ResourceItem &resourceItem, int32_t &id);
    bool FindMaxId();
    bool CreateTable();
    std::string GetValue(const ResourceItem &resourceItem) const;
    std::string dbPath_;
    sqlite3 *db_ = nullptr;
    static int32_t id_;
    int32_t priority_ = 0;
};
}
}
}
#endif
