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

#ifndef OHOS_CLOUD_ADVERTISING_CLIENT_H
#define OHOS_CLOUD_ADVERTISING_CLIENT_H

#include <mutex>
#include <string>

#include "iad_load_callback.h"
#include "request_data.h"
#include "ad_service_interface.h"
#include "iremote_object.h"
#include "refbase.h"
#include "errors.h"
namespace OHOS {
namespace Cloud {
class AdvertisingSADeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit AdvertisingSADeathRecipient();
    virtual ~AdvertisingSADeathRecipient() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;

private:
    DISALLOW_COPY_AND_MOVE(AdvertisingSADeathRecipient);
};

class AdvertisingServiceClient : public RefBase {
public:
    DISALLOW_COPY_AND_MOVE(AdvertisingServiceClient);
    static sptr<AdvertisingServiceClient> GetInstance();
    ErrCode LoadAd(const std::string &request, const std::string &adOptions,
        const sptr<IAdLoadCallback> &callback, int32_t loadAdType);
    void OnRemoteAdSADied(const wptr<IRemoteObject> &object);
    void LoadAdSAFail();
    void LoadAdSASucess(const sptr<IRemoteObject> &remoteObject);

private:
    AdvertisingServiceClient();
    virtual ~AdvertisingServiceClient() override;
    static std::mutex lock;
    static sptr<AdvertisingServiceClient> instance;
    bool LoadService();
    bool isServiceReady = false;
    std::mutex loadAdServiceLock;
    std::condition_variable condition;
    std::mutex conditionLock;
    sptr<IAdvertisingService> adServiceProxy;
    sptr<AdvertisingSADeathRecipient> deathRecipient;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_CLIENT_H