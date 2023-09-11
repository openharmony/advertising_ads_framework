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

#include <cinttypes>
#include <new>
#include <mutex>
#include "system_ability_definition.h"
#include "system_ability_load_callback_stub.h"
#include "iservice_registry.h"

#include "ad_hilog_wreapper.h"
#include "ad_service_client.h"
#include "ad_constant.h"
#include "ad_inner_error_code.h"

namespace OHOS {
namespace Cloud {
static const int8_t LOAD_AD_SERVICE_TIME_OUT = 10;
std::mutex AdvertisingServiceClient::lock;
sptr<AdvertisingServiceClient> AdvertisingServiceClient::instance;

class AdvertisingLoadCallback : public SystemAbilityLoadCallbackStub {
public:
    void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject) override
    {
        if (systemAbilityId != ADVERTISING_ID) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT,
                "OnLoadSystemAbilitySuccess systemAbility is not oaid service: %{public}d.", systemAbilityId);
            return;
        }
        AdvertisingServiceClient::GetInstance()->LoadAdSASucess(remoteObject);
    }

    void OnLoadSystemAbilityFail(int32_t systemAbilityId) override
    {
        if (systemAbilityId != ADVERTISING_ID) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT,
                "OnLoadSystemAbilityFail systemAbility is not oaid service: %{public}d.", systemAbilityId);
            return;
        }
        AdvertisingServiceClient::GetInstance()->LoadAdSAFail();
    }
};

AdvertisingServiceClient::AdvertisingServiceClient()
{
    deathRecipient = new (std::nothrow) AdvertisingSADeathRecipient();
}

AdvertisingServiceClient::~AdvertisingServiceClient()
{
    if (adServiceProxy != nullptr) {
        auto remoteObject = adServiceProxy->AsObject();
        if (remoteObject != nullptr && deathRecipient != nullptr) {
            remoteObject->RemoveDeathRecipient(deathRecipient);
        }
    }
}

sptr<AdvertisingServiceClient> AdvertisingServiceClient::GetInstance()
{
    if (instance == nullptr) {
        std::lock_guard<std::mutex> autoLock(lock);
        if (instance == nullptr) {
            instance = new AdvertisingServiceClient;
        }
    }
    return instance;
}

bool AdvertisingServiceClient::LoadService()
{
    if (isServiceReady) {
        return true;
    }
    std::lock_guard<std::mutex> lock(loadAdServiceLock);
    if (isServiceReady) {
        return true;
    }
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "ad get SystemAbilityManager failed.");
        return false;
    }
    sptr<AdvertisingLoadCallback> loadCallback = new (std::nothrow) AdvertisingLoadCallback();
    if (loadCallback == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "new AdvertisingLoadCallback failed");
        return false;
    }

    int32_t result = systemAbilityManager->LoadSystemAbility(ADVERTISING_ID, loadCallback);
    if (result != ERR_OK) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "LoadSystemAbility %{public}d failed, result: %{public}d.",
            ADVERTISING_ID, result);
        return false;
    }
    std::unique_lock<std::mutex> adConditionLock(conditionLock);
    auto waitStatus = condition.wait_for(adConditionLock, std::chrono::seconds(LOAD_AD_SERVICE_TIME_OUT),
        [this]() { return isServiceReady; });
    if (!waitStatus) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "Load AD SystemAbility timeout.");
        return false;
    }
    return true;
}

void AdvertisingServiceClient::OnRemoteAdSADied(const wptr<IRemoteObject> &remote)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CLIENT, "OnRemoteAdSADied");
    std::unique_lock<std::mutex> lock(conditionLock);
    if (adServiceProxy != nullptr) {
        auto remoteObject = adServiceProxy->AsObject();
        if (remoteObject != nullptr && deathRecipient != nullptr) {
            remoteObject->RemoveDeathRecipient(deathRecipient);
        }
        adServiceProxy = nullptr;
    }
    isServiceReady = false;
}

ErrCode AdvertisingServiceClient::LoadAd(const std::string &request, const std::string &adOptions,
    const sptr<IAdLoadCallback> &callback, int32_t loadAdType)
{
    if (callback == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "callback is nullptr");
        return ERR_AD_COMMON_NAPI_CALLBACK_NULL_ERROR;
    }
    if (!LoadService()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "reload ad service");
        LoadService();
    }
    if (adServiceProxy == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CLIENT, "adServiceProxy is nullptr");
        return ERR_AD_COMMON_AD_PROXY_NULL_ERROR;
    }
    return adServiceProxy->LoadAd(request, adOptions, callback->AsObject(), 0, loadAdType);
}

void AdvertisingServiceClient::LoadAdSASucess(const sptr<IRemoteObject> &remoteObject)
{
    std::unique_lock<std::mutex> lock(conditionLock);
    if (remoteObject == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "Load ad service remoteObject is null");
        return;
    }
    if (deathRecipient != nullptr) {
        remoteObject->AddDeathRecipient(deathRecipient);
    }
    adServiceProxy = iface_cast<IAdvertisingService>(remoteObject);
    isServiceReady = true;
    condition.notify_one();
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CLIENT, "Load ad service success");
}

void AdvertisingServiceClient::LoadAdSAFail()
{
    std::unique_lock<std::mutex> lock(conditionLock);
    isServiceReady = false;
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CLIENT, "Load ad service fail");
}

AdvertisingSADeathRecipient::AdvertisingSADeathRecipient() {}

void AdvertisingSADeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CLIENT, "Ads Remote systemAbility died");
    AdvertisingServiceClient::GetInstance()->OnRemoteAdSADied(object);
}
} // namespace Cloud
} // namespace OHOS