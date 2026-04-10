/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "js_adsservice_extension_context.h"
#include "js_extension_context.h"
#include "js_runtime_utils.h"
#include "ad_hilog_wreapper.h"
#include "ipc_skeleton.h"

using namespace OHOS::AbilityRuntime;

namespace OHOS {
namespace AdsExtension {
namespace {
class JsAdsServiceExtensionContext final {
public:
    explicit JsAdsServiceExtensionContext(
        const std::shared_ptr<AdsServiceExtensionContext>& context) : context_(context) {}
    ~JsAdsServiceExtensionContext() = default;

    static void Finalizer(napi_env env, void* data, void* hint)
    {
        (void)hint;
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtensionContext::Finalizer is called");
        std::unique_ptr<JsAdsServiceExtensionContext>(static_cast<JsAdsServiceExtensionContext*>(data));
    }
private:
    std::weak_ptr<AdsServiceExtensionContext> context_;
};
} // namespace

napi_value CreateJsAdsServiceExtensionContext(
    napi_env env, std::shared_ptr<AdsServiceExtensionContext> context)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "CreateJsAdsServiceExtensionContext begin");
    // 可逃逸的scope管理
    napi_escapable_handle_scope scope;
    napi_open_escapable_handle_scope(env, &scope);

    napi_value object = CreateJsExtensionContext(env, context);
    std::unique_ptr<JsAdsServiceExtensionContext> jsContext =
        std::make_unique<JsAdsServiceExtensionContext>(context);
    if (!object) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "object is nullptr.");
        // 关闭 scope
        napi_close_escapable_handle_scope(env, scope);
        return nullptr;
    }
    napi_wrap(env, object, jsContext.release(), JsAdsServiceExtensionContext::Finalizer, nullptr, nullptr);

    // 将 object 逃逸出当前 scope，使其在父 scope 中可用
    napi_value escapedObject = nullptr;
    napi_escape_handle(env, scope, object, &escapedObject);

    // 关闭 scope
    napi_close_escapable_handle_scope(env, scope);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "CreateJsAdsServiceExtensionContext end");
    return escapedObject;
}
} // namespace AdsExtension
} // namespace OHOS