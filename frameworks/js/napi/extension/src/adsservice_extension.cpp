/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "adsservice_extension.h"
#include "js_adsservice_extension.h"
#include "ability_local_record.h"
#include "ad_hilog_wreapper.h"

namespace OHOS {
namespace AdsExtension {
void AdsServiceExtension::Init(const std::shared_ptr<AbilityRuntime::AbilityLocalRecord> &record,
    const std::shared_ptr<AbilityRuntime::OHOSApplication> &application,
    std::shared_ptr<AbilityRuntime::AbilityHandler> &handler,
    const sptr<IRemoteObject> &token)
{
    ExtensionBase<AdsServiceExtensionContext>::Init(record, application, handler, token);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdsServiceExtension begin init context");
}

std::shared_ptr<AdsServiceExtensionContext> AdsServiceExtension::CreateAndInitContext(
    const std::shared_ptr<AbilityRuntime::AbilityLocalRecord> &record,
    const std::shared_ptr<AbilityRuntime::OHOSApplication> &application,
    std::shared_ptr<AbilityRuntime::AbilityHandler> &handler,
    const sptr<IRemoteObject> &token)
{
    std::shared_ptr<AdsServiceExtensionContext> context =
        ExtensionBase<AdsServiceExtensionContext>::CreateAndInitContext(record, application, handler, token);
    if (!context) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdsServiceExtension::CreateAndInitContext context is nullptr");
    }
    return context;
}

AdsServiceExtension* AdsServiceExtension::Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    if (!runtime) {
        return new AdsServiceExtension();
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "create AdsServiceExtension");
    switch (runtime->GetLanguage()) {
        case AbilityRuntime::Runtime::Language::JS: {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "create js AdsServiceExtension");
            return JsAdsServiceExtension::Create(runtime);
        }
        default: {
            return new AdsServiceExtension();
        }
    }
    return nullptr;
}
} // namespace AdsExtension
} // namespace OHOS