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

#ifndef OHOS_RESTOOL_RESOURCE_MODULE_INC_H
#define OHOS_RESTOOL_RESOURCE_MODULE_INC_H

#include "increment_list.h"
#include "resource_module.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ResourceModuleInc : public ResourceModule {
public:
    ResourceModuleInc(const std::string &modulePath, const std::string &moduleOutput,
		      const std::string &moduleName, const std::vector<std::string> &folder);
    virtual ~ResourceModuleInc() {};
    uint32_t ScanResource(const std::vector<IncrementList::FileIncrement> &fileIncrements);
    uint32_t SaveIndex() const;
private:
    const std::vector<std::string> &folder_;
};
}
}
}
#endif
