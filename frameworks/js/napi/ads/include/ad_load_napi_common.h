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

#ifndef OHOS_CLOUD_NAPI_ADVERTISING_COMMON_H
#define OHOS_CLOUD_NAPI_ADVERTISING_COMMON_H

#include <mutex>
#include <thread>
#include <vector>
#include <uv.h>
#include <string>
#include "ad_load_callback_stub.h"
#include "ad_request_body_stub.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi/native_common.h"
#include "want.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
using namespace OHOS::Cloud;

class AdLoadListenerCallback;

struct AdJSCallback {
    napi_ref onAdLoadFailure = nullptr;
    napi_ref onAdLoadSuccess = nullptr;
};

struct AdCallbackParam {
    napi_env env = nullptr;
    napi_deferred deferred;
    int32_t errCode;
    std::string errMsg;
    std::string body;
    std::string ads;
    std::string multiAds;
    AdJSCallback callback;
};

class AdLoadListenerCallback : public AdLoadCallbackStub {
public:
    explicit AdLoadListenerCallback(napi_env env, AdJSCallback callback);
    ~AdLoadListenerCallback();
    void OnAdLoadSuccess(const std::string &result) override;
    void OnAdLoadMultiSlotsSuccess(const std::string &result) override;
    void OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg) override;

private:
    napi_env env_ = nullptr;
    AdJSCallback callback_;
};

class AdRequestBodyAsync : public AdRequestBodyStub {
public:
    explicit AdRequestBodyAsync(napi_env env, napi_deferred deferred);
    ~AdRequestBodyAsync();

    void OnRequestBodyReturn(int32_t resultCode, const std::string &body, bool isResolved);

private:
    napi_env env_ = nullptr;
    napi_deferred deferred_ = nullptr;
};
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS
#endif // OHOS_CLOUD_NAPI_ADVERTISING_COMMON_H