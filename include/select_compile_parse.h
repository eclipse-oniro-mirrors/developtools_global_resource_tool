/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_RESTOOL_SELECT_COMPILE_PARSE_H
#define OHOS_RESTOOL_SELECT_COMPILE_PARSE_H

#include "resource_data.h"

namespace OHOS {
namespace Global {
namespace Restool {
class SelectCompileParse {
public:
    static bool ParseTargetConfig(const std::string &limitParams, TargetConfig &targetConfig);
    static bool IsSelectCompile(std::vector<KeyParam> &keyParams);

private:
    static bool IsSelectableMccmnc(std::vector<KeyParam> &keyParams, size_t &index, std::vector<KeyParam> &limit);
    static bool IsSelectableLocale(std::vector<KeyParam> &keyParams, size_t &index, std::vector<KeyParam> &limit);
    static bool IsSelectableOther(std::vector<KeyParam> &keyParams, size_t &index, std::vector<KeyParam> &limit);
    static void InitMccmnc(std::vector<KeyParam> &limit);
    static void InitLocale(std::vector<KeyParam> &limit);
    static std::vector<Mccmnc> mccmncArray_;
    static std::vector<Locale> localeArray_;
};
}
}
}
#endif