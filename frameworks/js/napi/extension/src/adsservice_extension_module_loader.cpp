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

#include "adsservice_extension_module_loader.h"
#include "adsservice_extension.h"
#include "ad_hilog_wreapper.h"

namespace OHOS::AdsExtension {
AdsServiceExtensionModuleLoader::AdsServiceExtensionModuleLoader() = default;
AdsServiceExtensionModuleLoader::~AdsServiceExtensionModuleLoader() = default;

AbilityRuntime::Extension *AdsServiceExtensionModuleLoader::Create(
    const std::unique_ptr<AbilityRuntime::Runtime> &runtime) const
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Create runtime");
    return AdsServiceExtension::Create(runtime);
}

std::map<std::string, std::string> AdsServiceExtensionModuleLoader::GetParams()
{
    std::map<std::string, std::string> params;
    // type means extension type in ExtensionAbilityType of extension_ability_info.h, 20 means adsService.
    params.insert(std::pair<std::string, std::string>("type", "20"));
    params.insert(std::pair<std::string, std::string>("name", "AdsServiceExtension"));
    return params;
}

extern "C" __attribute__((visibility("default"))) void *OHOS_EXTENSION_GetExtensionModule()
{
    return &AdsServiceExtensionModuleLoader::GetInstance();
}
} // namespace OHOS::AdsExtension