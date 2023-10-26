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

#include "ad_init.h"
#include "ad_hilog_wreapper.h"
#include "ad_inner_error_code.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

extern const char _binary_advertising_js_start[];
extern const char _binary_advertising_js_end[];
extern const char _binary_advertising_abc_start[];
extern const char _binary_advertising_abc_end[];

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
EXTERN_C_START
/*
 * Module export function
 */
static napi_value Init(napi_env env, napi_value exports)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Napi begin init.");

    /*
     * Properties define
     */
    Advertising::AdvertisingInit(env, exports);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Napi end init.");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module _module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "advertising",
    .nm_priv = ((void *)0),
    .reserved = { 0 }
};

extern "C" __attribute__((visibility("default"))) void NAPI_advertising_GetJSCode(const char** buf, int* bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_advertising_js_start;
    }

    if (bufLen != nullptr) {
        *bufLen = _binary_advertising_js_end - _binary_advertising_js_start;
    }
}

extern "C" __attribute__((visibility("default"))) void NAPI_advertising_GetABCCode(const char** buf, int* bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_advertising_abc_start;
    }

    if (bufLen != nullptr) {
        *bufLen = _binary_advertising_abc_end - _binary_advertising_abc_start;
    }
}

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&_module);
}
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS