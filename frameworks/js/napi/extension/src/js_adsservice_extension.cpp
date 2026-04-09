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

#include "js_adsservice_extension.h"
#include "js_adsservice_extension_context.h"
#include "js_extension_context.h"
#include "ability_info.h"
#include "ad_hilog_wreapper.h"
#include "js_runtime.h"
#include "js_runtime_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_common_want.h"
#include "napi_remote_object.h"

using namespace OHOS::AbilityRuntime;
using namespace OHOS::AppExecFwk;

namespace OHOS {
namespace AdsExtension {
namespace {
    constexpr size_t ARGC_ONE = 1;
}

napi_value AttachAdsServiceExtensionContext(napi_env env, void *value, void *)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "call");
    if (value == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "invalid parameter.");
        return nullptr;
    }

    // 创建escapable handle scope来管理返回的napi_value
    napi_escapable_handle_scope scope;
    napi_open_escapable_handle_scope(env, &scope);

    auto ptr = reinterpret_cast<std::weak_ptr<AdsServiceExtensionContext> *>(value)->lock();
    if (ptr == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "invalid context.");
        napi_close_escapable_handle_scope(env, scope);
        return nullptr;
    }
    napi_value object = CreateJsAdsServiceExtensionContext(env, ptr);
    auto module = JsRuntime::LoadSystemModuleByEngine(env,
        "advertising.AdsServiceExtensionContext", &object, 1);
    if (module == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "load module failed.");
        napi_close_escapable_handle_scope(env, scope);
        return nullptr;
    }
    auto contextObj = module->GetNapiValue();
    napi_coerce_to_native_binding_object(
        env, contextObj, DetachCallbackFunc, AttachAdsServiceExtensionContext, value, nullptr);
    auto workContext = new (std::nothrow) std::weak_ptr<AdsServiceExtensionContext>(ptr);
    if (workContext == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "workContext is null.");
        napi_close_escapable_handle_scope(env, scope);
        return nullptr;
    }
    napi_wrap(env, contextObj, workContext,
        [](napi_env, void *data, void *) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI,
                "Finalizer for weak_ptr adsservice extension context is called");
            delete static_cast<std::weak_ptr<AdsServiceExtensionContext> *>(data);
        },
        nullptr, nullptr);

    // 在所有操作完成后关闭scope
    napi_close_escapable_handle_scope(env, scope);
    return contextObj;
}

JsAdsServiceExtension* JsAdsServiceExtension::Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtension::Create");
    return new(std::nothrow) JsAdsServiceExtension(static_cast<AbilityRuntime::JsRuntime&>(*runtime));
}

JsAdsServiceExtension::JsAdsServiceExtension(AbilityRuntime::JsRuntime& jsRuntime) : jsRuntime_(jsRuntime)
{
    AbilityRuntime::HandleScope handleScope(jsRuntime_);
    env_ = jsRuntime_.GetNapiEnv();
}

JsAdsServiceExtension::~JsAdsServiceExtension()
{
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Js service extension destructor.");
    jsRuntime_.FreeNativeReference(std::move(jsObj_));
}

void JsAdsServiceExtension::Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord> &record,
    const std::shared_ptr<AppExecFwk::OHOSApplication> &application,
    std::shared_ptr<AppExecFwk::AbilityHandler> &handler, const sptr<IRemoteObject> &token)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtension::Init");
    AdsServiceExtension::Init(record, application, handler, token);
    std::string srcPath = "";
    std::string moduleName = "";
    if (!GetSrcPathAndModuleName(srcPath, moduleName)) {
        return;
    }
    jsObj_ = jsRuntime_.LoadModule(moduleName, srcPath, abilityInfo_->hapPath,
        abilityInfo_->compileMode == AbilityRuntime::CompileMode::ES_MODULE);
    if (!jsObj_) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Failed to get jsObj_");
        return;
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtension::Init ConvertNativeValueTo.");
    napi_handle_scope scope;
    napi_status status = napi_open_handle_scope(env_, &scope);
    if (status != napi_ok) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Init Failed to open handle scope");
        return;
    }
    napi_value obj = jsObj_->GetNapiValue();
    if (!obj) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Failed to get JsAdsServiceExtension object");
        napi_close_handle_scope(env_, scope);
        return;
    }

    BindContext(env_, obj);
    // 关闭 HandleScope
    napi_close_handle_scope(env_, scope);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtension init end");
}

void JsAdsServiceExtension::BindContext(napi_env env, napi_value obj)
{
    auto context = GetContext();
    if (!context) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Failed to get context");
        return;
    }
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtension::BindContext");
    // 创建 HandleScope 管理
    napi_handle_scope scope;
    napi_open_handle_scope(env_, &scope);

    napi_value contextObj = CreateJsAdsServiceExtensionContext(env_, context);
    auto shellContextRef = jsRuntime_.LoadSystemModule("advertising.AdsServiceExtensionContext", &contextObj, ARGC_ONE);
    if (!shellContextRef) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "shellContextRef is nullptr.");
        napi_close_handle_scope(env_, scope);
        return;
    }
    contextObj = shellContextRef->GetNapiValue();
    context->Bind(jsRuntime_, shellContextRef.release());
    napi_set_named_property(env_, obj, "context", contextObj);

    if (!contextObj) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Failed to get adsservice extension native object");
        napi_close_handle_scope(env_, scope);
        return;
    }
    napi_wrap(env_, contextObj, new std::weak_ptr<AbilityRuntime::Context>(context),
        [](napi_env env, void* data, void*) {
            delete static_cast<std::weak_ptr<AbilityRuntime::Context>*>(data);
        }, nullptr, nullptr);
    napi_close_handle_scope(env_, scope);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "BindContext end");
}

bool JsAdsServiceExtension::GetSrcPathAndModuleName(std::string& srcPath, std::string& moduleName)
{
    if (!Extension::abilityInfo_) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "abilityInfo_ is nullptr");
        return false;
    }
    if (!Extension::abilityInfo_->isModuleJson) {
        srcPath.append(Extension::abilityInfo_->package);
        srcPath.append("/assets/js/");
        if (!Extension::abilityInfo_->srcPath.empty()) {
            srcPath.append(Extension::abilityInfo_->srcPath);
        }
        srcPath.append("/").append(Extension::abilityInfo_->name).append(".abc");
    } else if (!Extension::abilityInfo_->srcEntrance.empty()) {
        srcPath.append(Extension::abilityInfo_->moduleName + "/");
        srcPath.append(Extension::abilityInfo_->srcEntrance);
        srcPath.erase(srcPath.rfind('.'));
        srcPath.append(".abc");
    } else {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Failed to get srcPath");
        return false;
    }
    moduleName = Extension::abilityInfo_->moduleName;
    moduleName.append("::").append(abilityInfo_->name);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "moduleName:%{public}s, srcPath:%{public}s.",
        moduleName.c_str(), srcPath.c_str());
    return true;
}

sptr<IRemoteObject> JsAdsServiceExtension::OnConnect(const AAFwk::Want &want)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtension::OnConnect start");
    // 用于跨napi context传递napi值
    napi_escapable_handle_scope scope;
    napi_open_escapable_handle_scope(env_, &scope);
    napi_value result = CallOnConnect(want);
    auto remoteObj = NAPI_ohos_rpc_getNativeRemoteObject(env_, result);
    if (remoteObj == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "remoteObj null.");
    }

    // 关闭 scope
    napi_close_escapable_handle_scope(env_, scope);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "JsAdsServiceExtension::OnConnect end");
    return remoteObj;
}

napi_value JsAdsServiceExtension::CallOnConnect(const AAFwk::Want &want)
{
    Extension::OnConnect(want);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "call");
    napi_value napiWant = OHOS::AppExecFwk::WrapWant(env_, want);
    napi_value argv[] = {napiWant};
    if (!jsObj_) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Not found AdsServiceExtension.js");
        return nullptr;
    }

    napi_value obj = jsObj_->GetNapiValue();
    if (!CheckTypeForNapiValue(env_, obj, napi_object)) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Failed to get AdsServiceExtension object");
        return nullptr;
    }

    napi_value method = nullptr;
    napi_get_named_property(env_, obj, "onConnect", &method);
    if (method == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Failed to get onConnect from AdsServiceExtension object");
        return nullptr;
    }
    napi_value remoteNative = nullptr;
    napi_call_function(env_, obj, method, ARGC_ONE, argv, &remoteNative);
    if (remoteNative == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "remoteNative nullptr.");
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ok");
    return remoteNative;
}
} // namespace AdsExtension
} // namespace OHOS
