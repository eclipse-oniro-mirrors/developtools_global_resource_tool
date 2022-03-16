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

#ifndef OHOS_RESTOOL_FILE_COMBINE_H
#define OHOS_RESTOOL_FILE_COMBINE_H

#include "resource_directory.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ModuleCombine {
public:
    ModuleCombine(const std::string &modulePath, const std::string &outputPath);
    virtual ~ModuleCombine() {};
    bool Combine();

private:
    bool CombineDirectory(const DirectoryInfo &directoryInfo);
    bool CombineSolidXml(const std::string &src, const std::string &dst,
        const std::map<std::string, std::string> &sxmlPaths);
    const std::string &modulePath_;
    const std::string &outputPath_;
};
}
}
}
#endif
