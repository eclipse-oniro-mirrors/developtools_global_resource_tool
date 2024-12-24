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

#include "select_compile_parse.h"
#include <algorithm>
#include "key_parser.h"
#include "cmd/package_parser.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
vector<Mccmnc> SelectCompileParse::mccmncArray_ = {};
vector<Locale> SelectCompileParse::localeArray_ = {};

// eg: Device[ phone, car ];ColorMode[ dark ]
bool SelectCompileParse::ParseTargetConfig(const string &limitParams, TargetConfig &targetConfig)
{
    vector<string> limitArray;
    ResourceUtil::Split(limitParams, limitArray, ";");
    if (limitArray.empty()) {
        return false;
    }
    for (auto &it : limitArray) {
        vector<string> limit;
        ResourceUtil::Split(it, limit, "[");
        if (limit.size() != 2) { // 2 means the size of the valid parameter after split by '['
            return false;
        }
        ResourceUtil::RemoveSpaces(limit.back());
        if (limit.size() < 2 || limit.back().back() != ']') { // 2 means characters other than ']'
            return false;
        }
        limit.back().pop_back();
        vector<string> limitValues;
        ResourceUtil::Split(limit.back(), limitValues, ",");
        if (limitValues.empty()) {
            return false;
        }
        auto &limitType = limit.front();
        ResourceUtil::RemoveSpaces(limitType);
        transform(limitType.begin(), limitType.end(), limitType.begin(), ::tolower);
        if (!KeyParser::ParseLimit(limitType, limitValues, targetConfig)) {
            return false;
        }
    }
    return true;
}

bool SelectCompileParse::IsSelectCompile(vector<KeyParam> &keyParams)
{
    if (keyParams.empty()) {
        return true;
    }
    auto &cmdParser = CmdParser<PackageParser>::GetInstance().GetCmdParser();
    bool isTtargetConfig = cmdParser.IsTargetConfig();
    if (!isTtargetConfig) {
        return true;
    }
    TargetConfig targetConfig = cmdParser.GetTargetConfigValues();
    map<KeyType, function<bool(size_t &)>> selectableFuncMatch {
        {KeyType::MCC, bind(&IsSelectableMccmnc, keyParams, placeholders::_1, targetConfig.mccmnc)},
        {KeyType::LANGUAGE, bind(&IsSelectableLocale, keyParams, placeholders::_1, targetConfig.locale)},
        {KeyType::ORIENTATION, bind(&IsSelectableOther, keyParams, placeholders::_1, targetConfig.orientation)},
        {KeyType::DEVICETYPE, bind(&IsSelectableOther, keyParams, placeholders::_1, targetConfig.device)},
        {KeyType::NIGHTMODE, bind(&IsSelectableOther, keyParams, placeholders::_1, targetConfig.colormode)},
        {KeyType::RESOLUTION, bind(&IsSelectableOther, keyParams, placeholders::_1, targetConfig.density)},
    };
    for (size_t index = 0; index < keyParams.size(); index++) {
        auto iter = selectableFuncMatch.find(keyParams[index].keyType);
        if (iter == selectableFuncMatch.end()) {
            continue;
        }
        if (!iter->second(index)) {
            return false;
        }
    }
    return true;
}

// {KeyType::OTHER, 0} indicates that the default value is equal to null
void SelectCompileParse::InitMccmnc(vector<KeyParam> &limit)
{
    if (!mccmncArray_.empty()) {
        return;
    }
    for (size_t i = 0; i < limit.size(); i++) {
        if (limit[i].keyType == KeyType::MCC) {
            mccmncArray_.push_back({limit[i], {KeyType::OTHER, 0}});
        }
        if (limit[i].keyType == KeyType::MNC) {
            mccmncArray_.back().mnc = limit[i];
        }
    }
}

bool SelectCompileParse::IsSelectableMccmnc(vector<KeyParam> &keyParams, size_t &index, vector<KeyParam> &limit)
{
    if (limit.empty()) {
        return true;
    }
    Mccmnc mccmncLimit({keyParams[index++], {KeyType::OTHER, 0}});
    if (index < keyParams.size() && keyParams[index].keyType == KeyType::MNC) {
        mccmncLimit.mnc = keyParams[index];
    } else {
        index--;
    }
    InitMccmnc(limit);
    return find(mccmncArray_.begin(), mccmncArray_.end(), mccmncLimit) != mccmncArray_.end();
}

void SelectCompileParse::InitLocale(vector<KeyParam> &limit)
{
    if (!localeArray_.empty()) {
        return;
    }
    for (size_t i = 0; i < limit.size(); i++) {
        if (limit[i].keyType == KeyType::LANGUAGE) {
            localeArray_.push_back({limit[i], {KeyType::OTHER, 0},
                {KeyType::OTHER, 0}});
        }
        if (limit[i].keyType == KeyType::SCRIPT) {
            localeArray_.back().script = limit[i];
        }
        if (limit[i].keyType == KeyType::REGION) {
            localeArray_.back().region = limit[i];
        }
    }
}

bool SelectCompileParse::IsSelectableLocale(vector<KeyParam> &keyParams, size_t &index, vector<KeyParam> &limit)
{
    if (limit.empty()) {
        return true;
    }
    Locale localeLimit({keyParams[index++], {KeyType::OTHER, 0}, {KeyType::OTHER, 0}});
    for (; index < keyParams.size(); index++) {
        if (keyParams[index].keyType == KeyType::SCRIPT) {
            localeLimit.script = keyParams[index];
            continue;
        }
        if (keyParams[index].keyType == KeyType::REGION) {
            localeLimit.region = keyParams[index];
            break;
        }
        index--;
        break;
    }
    InitLocale(limit);
    return find(localeArray_.begin(), localeArray_.end(), localeLimit) != localeArray_.end();
}

bool SelectCompileParse::IsSelectableOther(vector<KeyParam> &keyParams, size_t &index, vector<KeyParam> &limit)
{
    if (limit.empty()) {
        return true;
    }
    return find(limit.begin(), limit.end(), keyParams[index]) != limit.end();
}

}
}
}
