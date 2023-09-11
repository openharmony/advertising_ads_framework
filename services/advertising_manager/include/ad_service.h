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

#ifndef OHOS_CLOUD_ADVERTISING_SERVICE_H
#define OHOS_CLOUD_ADVERTISING_SERVICE_H

#include <mutex>
#include <string>

#include "request_data.h"
#include "ad_service_stub.h"
#include "bundle_mgr_interface.h"
#include "iremote_proxy.h"
#include "ability_connect_callback_stub.h"
#include "system_ability.h"
#include "errors.h"
#include "ad_load_proxy.h"
#include "iad_load_proxy.h"
#include "iad_load_callback.h"

namespace OHOS {
namespace Cloud {
enum class AdsServiceRunningState {
    STATE_NOT_START,
    STATE_RUNNING
};

struct AdServiceElementName {
    std::string bundleName;
    std::string extensionName;
    int userId;
};

class AdvertisingService : public SystemAbility, public AdvertisingStub {
    DECLARE_SYSTEM_ABILITY(AdvertisingService);

public:
    DISALLOW_COPY_AND_MOVE(AdvertisingService);
    AdvertisingService(int32_t saId, bool runOnCreate);
    static sptr<AdvertisingService> GetInstance();
    AdvertisingService();
    virtual ~AdvertisingService() override;
    ErrCode LoadAd(const std::string &request, const std::string &options, const sptr<IRemoteObject> &callback,
        uint32_t callingUid, int32_t loadAdType) override;
    void GetCloudServiceProvider(AdServiceElementName &cloudServiceProvider);

protected:
    void OnStart() override;
    void OnStop() override;

private:
    bool ConnectAdKit(uint32_t callingUid, const sptr<AdRequestData> &data, const sptr<IAdLoadCallback> &callback,
        int32_t loadAdType);
    AdsServiceRunningState state_;
    static std::mutex lock_;
    static sptr<AdvertisingService> instance_;
    AdServiceElementName adServiceElementName_;
};

class AdRequestConnection : public AAFwk::AbilityConnectionStub {
public:
    AdRequestConnection(uint32_t callingUid, const sptr<AdRequestData> &data, const sptr<IAdLoadCallback> &callback,
        int32_t loadAdType)
        : callingUid_(callingUid), data_(data), callback_(callback), loadAdType_(loadAdType){};
    ~AdRequestConnection() = default;

    void OnAbilityConnectDone(const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject,
        int32_t resultCode) override;
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override;

private:
    uint32_t callingUid_;
    sptr<AdRequestData> data_;
    sptr<IAdLoadCallback> callback_;
    int32_t loadAdType_;
    sptr<AdLoadSendRequestProxy> proxy_{ nullptr };
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_SERVICE_H