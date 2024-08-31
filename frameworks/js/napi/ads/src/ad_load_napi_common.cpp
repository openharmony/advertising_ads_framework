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
#include <cstddef>

#include "napi_common_want.h"
#include "want_params.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi/native_common.h"
#include "ad_hilog_wreapper.h"
#include "iad_load_callback.h"
#include "ad_inner_error_code.h"
#include "ad_napi_common_error.h"
#include "ad_load_napi_common.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
inline void parseAdArray(napi_env env, const std::vector<AAFwk::Want> &adsArray, napi_value &inpute)
{
    for (uint32_t i = 0; i < adsArray.size(); ++i) {
        napi_value ad = AppExecFwk::WrapWantParams(env, adsArray.at(i).GetParams());
        napi_set_element(env, inpute, i, ad);
    }
}

inline int32_t ErrCodeConvert(int32_t kitErrCode)
{
    switch (kitErrCode) {
        case static_cast<int32_t>(Cloud::IAdLoadCallback::Message::AD_LOAD_PARAMS_ERROR):
            return PARAM_ERR;
        case static_cast<int32_t>(Cloud::IAdLoadCallback::Message::AD_LOAD_FAIL):
            return REQUEST_FAIL;
        case static_cast<int32_t>(Cloud::IAdLoadCallback::Message::DEVICE_NOT_SUPPORT_ERROR):
            return DEVICE_ERR;
        default:
            return INNER_ERR;
    }
}

bool InitAdLoadCallbackWorkEnv(napi_env env, uv_loop_s **loop, uv_work_t **work, AdCallbackParam **param)
{
    napi_get_uv_event_loop(env, loop);
    if (*loop == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "loop instance is nullptr");
        return false;
    }
    *work = new (std::nothrow) uv_work_t;
    if (*work == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "work is null");
        return false;
    }
    *param = new (std::nothrow) AdCallbackParam();
    if (*param == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to create AdCallbackParam");
        delete *work;
        *work = nullptr;
        *loop = nullptr;
        return false;
    }
    (*param)->env = env;
    return true;
}

AdLoadListenerCallback::AdLoadListenerCallback(napi_env env, AdJSCallback callback) : env_(env), callback_(callback) {}

AdLoadListenerCallback::~AdLoadListenerCallback() {}

void UvQueneWorkOnAdLoadSuccess(uv_work_t *work, int status)
{
    if ((work == nullptr) || (work->data == nullptr)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "OnAdLoadSuccess work or data is nullptr");
        return;
    }
    AdCallbackParam *data = reinterpret_cast<AdCallbackParam *>(work->data);
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(data->env, &scope);
    napi_value result = nullptr;
    napi_create_string_utf8(data->env, data->ads.c_str(), NAPI_AUTO_LENGTH, &result);
    napi_value undefined = nullptr;
    napi_get_undefined(data->env, &undefined);
    napi_value onAdLoadSuccessFunc = nullptr;
    napi_value resultOut = nullptr;
    napi_get_reference_value(data->env, data->callback.onAdLoadSuccess, &onAdLoadSuccessFunc);
    napi_call_function(data->env, undefined, onAdLoadSuccessFunc, 1, &result, &resultOut);
    napi_close_handle_scope(data->env, scope);
    delete data;
    data = nullptr;
    delete work;
}

void UvQueneWorkOnAdLoadMultiSlotsSuccess(uv_work_t *work, int status)
{
    if ((work == nullptr) || (work->data == nullptr)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "OnAdLoadMultiSlotsSuccess work or data is nullptr");
        return;
    }
    AdCallbackParam *data = reinterpret_cast<AdCallbackParam *>(work->data);
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(data->env, &scope);
    napi_value result = nullptr;
    napi_create_string_utf8(data->env, data->multiAds.c_str(), NAPI_AUTO_LENGTH, &result);
    napi_value undefined = nullptr;
    napi_get_undefined(data->env, &undefined);
    napi_value onAdLoadMultiSlotsSuccessFunc = nullptr;
    napi_value resultOut = nullptr;
    napi_get_reference_value(data->env, data->callback.onAdLoadSuccess, &onAdLoadMultiSlotsSuccessFunc);
    napi_call_function(data->env, undefined, onAdLoadMultiSlotsSuccessFunc, 1, &result, &resultOut);
    napi_close_handle_scope(data->env, scope);
    delete data;
    data = nullptr;
    delete work;
}

void UvQueneWorkOnAdLoadFailed(uv_work_t *work, int status)
{
    if ((work == nullptr) || (work->data == nullptr)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "OnAdLoadFailed work or data is nullptr");
        return;
    }
    AdCallbackParam *data = reinterpret_cast<AdCallbackParam *>(work->data);
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(data->env, &scope);
    napi_value results[2] = {0};
    napi_create_int32(data->env, data->errCode, &results[0]);
    napi_create_string_utf8(data->env, data->errMsg.c_str(), NAPI_AUTO_LENGTH, &results[1]);
    napi_value undefined = nullptr;
    napi_get_undefined(data->env, &undefined);
    napi_value onAdLoadFailedFunc = nullptr;
    napi_value resultOut = nullptr;
    napi_get_reference_value(data->env, data->callback.onAdLoadFailure, &onAdLoadFailedFunc);
    napi_call_function(data->env, undefined, onAdLoadFailedFunc, 2, results, &resultOut); // 2 params
    napi_close_handle_scope(data->env, scope);
    delete data;
    data = nullptr;
    delete work;
}

void AdLoadListenerCallback::OnAdLoadSuccess(const std::string &result)
{
    uv_loop_s *loop = nullptr;
    uv_work_t *work = nullptr;
    AdCallbackParam *param = nullptr;
    if (!InitAdLoadCallbackWorkEnv(env_, &loop, &work, &param)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to init ad load work environment");
        return;
    }
    param->ads = result;
    param->callback = callback_;
    work->data = reinterpret_cast<void *>(param);
    uv_queue_work(
        loop, work, [](uv_work_t *work) {}, UvQueneWorkOnAdLoadSuccess);
}

void AdLoadListenerCallback::OnAdLoadMultiSlotsSuccess(const std::string &result)
{
    uv_loop_s *loop = nullptr;
    uv_work_t *work = nullptr;
    AdCallbackParam *param = nullptr;
    if (!InitAdLoadCallbackWorkEnv(env_, &loop, &work, &param)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to init multi ad load work environment");
        return;
    }
    param->multiAds = result;
    param->callback = callback_;
    work->data = reinterpret_cast<void *>(param);
    uv_queue_work(
        loop, work, [](uv_work_t *work) {}, UvQueneWorkOnAdLoadMultiSlotsSuccess);
}

void AdLoadListenerCallback::OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg)
{
    uv_loop_s *loop = nullptr;
    uv_work_t *work = nullptr;
    AdCallbackParam *param = nullptr;
    if (!InitAdLoadCallbackWorkEnv(env_, &loop, &work, &param)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to init ad load work environment");
        return;
    }
    param->errCode = ErrCodeConvert(resultCode);
    param->errMsg = resultMsg;
    param->callback = callback_;
    ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "LoadAdFailure. errorCode:  %{public}d  errorMsg:  %{public}s",
        param->errCode, param->errMsg.c_str());
    work->data = reinterpret_cast<void *>(param);
    uv_queue_work(
        loop, work, [](uv_work_t *work) {}, UvQueneWorkOnAdLoadFailed);
}

AdRequestBodyAsync::AdRequestBodyAsync(napi_env env, napi_deferred deferred) : env_(env), deferred_(deferred) {}

AdRequestBodyAsync::~AdRequestBodyAsync() {}

void UvQueueWorkOnAdRequestBody(uv_work_t *work, int status)
{
    if (work == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "work is null");
        return;
    }
    if (work->data == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "work data is null");
        return;
    }
    napi_handle_scope scope = nullptr;
    AdCallbackParam *data = reinterpret_cast<AdCallbackParam *>(work->data);
    napi_open_handle_scope(data->env, &scope);
    if (scope == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "open scope failed");
        delete data;
        work->data = nullptr;
        return;
    }
    napi_value result = nullptr;
    if (data->errCode == ERR_SEND_OK) {
        napi_create_string_utf8(data->env, data->body.c_str(), NAPI_AUTO_LENGTH, &result);
        napi_resolve_deferred(data->env, data->deferred, result);
    } else {
        napi_value error = nullptr;
        int32_t jsErrorCode = data->errCode;
        std::string jsErrorMsg = data->body;
        error = GenerateAdBusinessError(data->env, jsErrorCode, jsErrorMsg);
        napi_reject_deferred(data->env, data->deferred, error);
    }
    napi_close_handle_scope(data->env, scope);
    delete data;
    data = nullptr;
    delete work;
}

void AdRequestBodyAsync::OnRequestBodyReturn(int32_t resultCode, const std::string &body, bool isResolved)
{
    uv_loop_s *loop = nullptr;
    uv_work_t *work = nullptr;
    AdCallbackParam *param = nullptr;
    if (!InitAdLoadCallbackWorkEnv(env_, &loop, &work, &param)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to init ad request body work environment");
        return;
    }
    param->errCode = isResolved ? ERR_SEND_OK : resultCode;
    param->body = body;
    param->deferred = deferred_;
    work->data = reinterpret_cast<void *>(param);
    uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {}, UvQueueWorkOnAdRequestBody, uv_qos_default);
}
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS