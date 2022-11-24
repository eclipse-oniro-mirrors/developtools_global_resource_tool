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

#ifndef OHOS_RESTOOL_RESOURCE_UTIL_H
#define OHOS_RESTOOL_RESOURCE_UTIL_H

#include<vector>
#include "resource_data.h"
#include "json/json.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ResourceUtil {
public:
    /**
     * @brief split the string with given splitter.
     * @param str: input string.
     * @param out: the array of strings computed by splitter.
     * @param splitter: the split string.
     */
    static void Split(const std::string &str, std::vector<std::string> &out, const std::string &splitter);

    /**
     * @brief Replace sub-string in string
     * @param sourceStr: The original string to operate on
     * @param oldStr: The string to be replaced
     * @param newStr: The new string used
     */
    static void StringReplace(std::string &sourceStr, const std::string &oldStr, const std::string &newStr);

    /**
     * @brief check file exist.
     * @param path: file path.
     * @return true if exist, other false.
     */
    static bool FileExist(const std::string &path);

    /**
     * @brief remove all files in the directory.
     * @param path: input directory.
     * @return true if remove success, other false.
     */
    static bool RmoveAllDir(const std::string &path);

    /**
     * @brief open json file.
     * @param path: json file path.
     * @param root: json root node
     * @return true if open success, other false.
     */
    static bool OpenJsonFile(const std::string &path, Json::Value &root);

    /**
     * @brief save json file.
     * @param path: json file path.
     * @param root: json root node
     * @return true if save success, other false.
     */
    static bool SaveToJsonFile(const std::string &path, const Json::Value &root);

    /**
     * @brief get resource type from directory.
     * @param name: directory name.
     * @return resource type.
     */
    static ResType GetResTypeByDir(const std::string &name);

    /**
     * @brief ResType to string
     * @param type: ResType
     * @return resource type string.
     */
    static std::string ResTypeToString(ResType type);

    /**
     * @brief get id name
     * @param name; id name or file name
     * @param type: ResType
     * @return return id name.
     */
    static std::string GetIdName(const std::string &name, ResType type);

    /**
     * @brief compose multi strings to string
     * @param contents: multi strings
     * @param addNull: if true, string length contains '\0'.
     * @return return string, empty if error
     */
    static std::string ComposeStrings(const std::vector<std::string> &contents, bool addNull = false);

    /**
     * @brief decompose string to multi strings
     * @param content: string
     * @return return string vector, empty if error
     */
    static std::vector<std::string> DecomposeStrings(const std::string &content);

    /**
     * @brief string to ResType
     * @param type: string
     * @return return ResType
     */
    static ResType GetResTypeFromString(const std::string &type);

    /**
     * @brief copy file
     * @param src: source file path
     * @param dst: destination file path
     * @return true if success, other false
     */
    static bool CopyFleInner(const std::string &src, const std::string &dst);

    /**
     * @brief create directories
     * @param filePath: directory path
     * @return true if success, other false
     */
    static bool CreateDirs(const std::string &filePath);

    /**
     * @brief ignore file or directory
     * @param filename: file or directory name
     * @param isFile: ture if is file, other false
     * @return true if ignore, other false
     */
    static bool IsIgnoreFile(const std::string &filename, bool isFile);

    /**
     * @brief need convert to solid xml
     * @param resType: ResType
     * @return true if need, other false
     */
    static bool NeedConverToSolidXml(ResType resType);

    /**
     * @brief generate hash string
     * @param key: string
     * @return hash string
     */
    static std::string GenerateHash(const std::string &key);

    /**
     * @brief get an absolute pathname
     * @param path pathname
     * @return absolut pathname
     */
    static std::string RealPath(const std::string &path);

    /**
     * @brief check the directory is legal
     * @param path pathname
     * @return true is legal, other false;
     */
    static bool IslegalPath(const std::string &path);

    /**
     * @brief get an keyParams for limitkey
     * @param keyParams
     * @return limitkey
     */
    static std::string PaserKeyParam(const std::vector<KeyParam> &keyParams);
private:
    enum class IgnoreType {
        IGNORE_FILE,
        IGNORE_DIR,
        IGNORE_ALL
    };
    static const std::map<std::string, IgnoreType> IGNORE_FILE_REGEX;
    static std::string GetLocaleLimitkey(const KeyParam &KeyParam);
    static std::string GetDeviceTypeLimitkey(const KeyParam &KeyParam);
    static std::string GetResolutionLimitkey(const KeyParam &KeyParam);
    static std::string GetKeyParamValue(const KeyParam &KeyParam);
};
}
}
}
#endif
