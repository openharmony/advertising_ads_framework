/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CJ_ADVERTISING_LOAD_SERVICE_H
#define OHOS_CJ_ADVERTISING_LOAD_SERVICE_H

#include <mutex>
#include <string>

#include "ability_connect_callback_stub.h"
#include "bundle_mgr_interface.h"
#include "errors.h"
#include "refbase.h"
#include "request_data.h"
#include "want.h"

#include "cj_advertising_common.h"
#include "ad_load_callback_stub.h"
#include "ad_load_proxy.h"
#include "ad_request_body_stub.h"
#include "iad_load_proxy.h"
#include "iad_load_callback.h"
#include "iad_request_body.h"
namespace OHOS {
namespace Advertising {

struct AdLoaderCallbackRef {
    std::function<void(CAdvertisementArr)> OnAdLoadSuccess;
    std::function<void(int32_t, char*)> OnAdLoadFailure;
};

struct MultiAdLoaderCallbackRef {
    std::function<void(CAdvertisementHashStrArr)> OnAdLoadSuccess;
    std::function<void(int32_t, char*)> OnAdLoadFailure;
};

struct AdServiceElementName {
    std::string bundleName;
    std::string extensionName;
    std::string apiServiceName;
    int userId;
};

struct AdCallbackParam {
    int32_t errCode;
    std::string errMsg;
    std::string body;
    std::string ads;
    std::string multiAds;
    AdLoaderCallbackRef callback;
    MultiAdLoaderCallbackRef multiCallback;
};

class AdLoadListenerCallback : public Cloud::AdLoadCallbackStub {
public:
    explicit AdLoadListenerCallback(AdLoaderCallbackRef callback);
    explicit AdLoadListenerCallback(MultiAdLoaderCallbackRef callback);
    ~AdLoadListenerCallback();
    void OnAdLoadSuccess(const std::string &result);
    void OnAdLoadMultiSlotsSuccess(const std::string &result);
    void OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg);

private:
    AdLoaderCallbackRef callback_;
    MultiAdLoaderCallbackRef multiCallback_;
    int32_t loadAdType = 0;
};

class AdLoadService : public RefBase {
public:
    static sptr<AdLoadService> GetInstance();
    AdLoadService();
    ~AdLoadService();
    ErrCode LoadAd(const std::string &request,
                   const std::string &options,
                   const sptr<Cloud::IAdLoadCallback> &callback,
                   int32_t loadAdType);
    int32_t RequestAdBody(const std::string &request,
                          const std::string &options,
                          const sptr<Cloud::IAdRequestBody> &callback);
    void GetAdServiceElement(AdServiceElementName &adServiceElementName);

private:
    bool ConnectAdKit(const sptr<Cloud::AdRequestData> &data, const sptr<Cloud::IAdLoadCallback> &callback,
        int32_t loadAdType);
    void GetConfigItem(const char *path, AdServiceElementName &adServiceElementName);
    static std::mutex lock_;
    static sptr<AdLoadService> instance_;
    AdServiceElementName adServiceElementName_;
};

class AdRequestConnection : public AAFwk::AbilityConnectionStub {
public:
    AdRequestConnection(const sptr<Cloud::AdRequestData> &data,
                        const sptr<Cloud::IAdLoadCallback> &callback,
                        int32_t loadAdType,
                        AdServiceElementName &element)
        : data_(data), callback_(callback), loadAdType_(loadAdType), currAdServiceElementName_(element) {}

    AdRequestConnection(const sptr<Cloud::AdRequestData> &data,
                        const sptr<Cloud::IAdRequestBody> &callback,
                        AdServiceElementName &element)
        : data_(data), bodyCallback_(callback), currAdServiceElementName_(element) {}

    ~AdRequestConnection() = default;

    void OnAbilityConnectDone(const AppExecFwk::ElementName &element,
                              const sptr<IRemoteObject> &remoteObject,
                              int32_t resultCode);

    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode);

private:
    sptr<Cloud::AdRequestData> data_;
    sptr<Cloud::IAdLoadCallback> callback_;
    sptr<Cloud::IAdRequestBody> bodyCallback_;
    int32_t loadAdType_{0};
    sptr<Cloud::AdLoadSendRequestProxy> proxy_{ nullptr };
    sptr<Cloud::AdRequestBodySendProxy> bodyProxy_{ nullptr };
    AdServiceElementName currAdServiceElementName_;
};

class AdRequestBodyAsync : public Cloud::AdRequestBodyStub {
public:
    explicit AdRequestBodyAsync(std::function<void(std::string)> getBodyInfo);
    ~AdRequestBodyAsync();

    void OnRequestBodyReturn(int32_t resultCode, const std::string &body, bool isResolved);
private:
    std::function<void(std::string)> getBodyInfo_;
};
} // namespace Advertising
} // namespace OHOS
#endif // OHOS_CJ_ADVERTISING_LOAD_SERVICE_H