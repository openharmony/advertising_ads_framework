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

#include <fcntl.h>
#include <fstream>
#include <type_traits>
#include "json/json.h"
#include "iremote_proxy.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "iremote_broker.h"
#include "want.h"
#include "want_params.h"
#include "want_params_wrapper.h"
#include "system_ability.h"
#include "config_policy_utils.h"
#include "system_ability_definition.h"
#include "extension_ability_info.h"
#include "element_name.h"
#include "config_policy_utils.h"

#include "ad_service_proxy.h"
#include "ad_hilog_wreapper.h"
#include "ad_constant.h"
#include "iad_load_callback.h"
#include "ad_inner_error_code.h"
#include "request_data.h"
#include "iad_load_callback.h"
#include "ad_service.h"

#include "ability_connect_callback_stub.h"
#include "ability_manager_client.h"

namespace OHOS {
namespace Cloud {
REGISTER_SYSTEM_ABILITY_BY_ID(AdvertisingService, ADVERTISING_ID, true);

void AdRequestConnection::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int32_t resultCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "AdRequestConnection  OnAbilityConnectDone %{public}d.", resultCode);
    if (resultCode != ERR_OK) {
        return;
    }
    proxy_ = (new (std::nothrow) AdLoadSendRequestProxy(remoteObject));
    if (proxy_ == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "ad load get send request proxy failed.");
        return;
    }
    proxy_->SendAdLoadRequest(callingUid_, data_, callback_, loadAdType_);
}

void AdRequestConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "ad load on service disconnected.");
    proxy_ = nullptr;
}

std::mutex AdvertisingService::lock_;
sptr<AdvertisingService> AdvertisingService::instance_;

AdvertisingService::AdvertisingService(int32_t saId, bool runOnCreate)
    : SystemAbility(saId, runOnCreate), state_(AdsServiceRunningState::STATE_NOT_START)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "AdvertisingService start");
    adServiceElementName_.userId = 0;
}

AdvertisingService::AdvertisingService() : state_(AdsServiceRunningState::STATE_NOT_START)
{
    adServiceElementName_.userId = 0;
}

AdvertisingService::~AdvertisingService() {};

sptr<AdvertisingService> AdvertisingService::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(lock_);
        if (instance_ == nullptr) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "create ad instance");
            instance_ = new AdvertisingService;
        }
    }
    return instance_;
}

void AdvertisingService::GetCloudServiceProvider(AdServiceElementName &cloudServiceProvider)
{
    char pathBuff[MAX_PATH_LEN];
    GetOneCfgFile(DEPENDENCY_CONFIG_FILE_RELATIVE_PATH.c_str(), pathBuff, MAX_PATH_LEN);
    char realPath[PATH_MAX];
    if (realpath(pathBuff, realPath) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "Parse realpath fail");
        return;
    }
    std::ifstream ifs;
    ifs.open(realPath);
    if (!ifs) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "Open file error.");
        return;
    }
    Json::Value jsonValue;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &jsonValue, &errs)) {
        ifs.close();
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "Read file failed %{public}s.", errs.c_str());
        return;
    }
    Json::Value cloudServiceBundleName = jsonValue["providerBundleName"];
    Json::Value cloudServiceAbilityName = jsonValue["providerAbilityName"];

    cloudServiceProvider.bundleName = cloudServiceBundleName[0].asString();
    cloudServiceProvider.extensionName = cloudServiceAbilityName[0].asString();
    cloudServiceProvider.userId = USER_ID;
    ifs.close();
}

ErrCode AdvertisingService::LoadAd(const std::string &request, const std::string &options,
    const sptr<IRemoteObject> &callback, uint32_t callingUid, int32_t loadAdType)
{
    if (adServiceElementName_.bundleName.empty() || adServiceElementName_.extensionName.empty()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "adServiceElementName is null, read from config");
        GetCloudServiceProvider(adServiceElementName_);
    }
    sptr<IAdLoadCallback> castedCallback = iface_cast<IAdLoadCallback>(callback);
    if (castedCallback == nullptr) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "castedCallback is null");
        return ERR_AD_COMMON_AD_SA_REMOTE_OBJECT_ERROR;
    }
    std::string collection = "{}";
    auto *data = new (std::nothrow) AdRequestData(request, options, collection);
    if (ConnectAdKit(callingUid, data, castedCallback, loadAdType)) {
        return ERR_OK;
    } else {
        castedCallback->OnAdLoadFailure(ERR_AD_COMMON_AD_CONNECT_KIT_ERROR, "connect ad kit fail");
        return ERR_AD_COMMON_AD_CONNECT_KIT_ERROR;
    }
    return ERR_OK;
}

void AdvertisingService::OnStart()
{
    if (state_ == AdsServiceRunningState::STATE_RUNNING) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "AdvertisingService is already running");
        return;
    }
    if (!Publish(AdvertisingService::GetInstance())) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "AdvertisingService init failed");
        return;
    }
    state_ = AdsServiceRunningState::STATE_RUNNING;
    AddSystemAbilityListener(ADVERTISING_ID);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "AdvertisingService start success");
    return;
}

void AdvertisingService::OnStop()
{
    if (state_ != AdsServiceRunningState::STATE_RUNNING) {
        return;
    }
    state_ = AdsServiceRunningState::STATE_NOT_START;
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "AdvertisingService stop success");
}

bool AdvertisingService::ConnectAdKit(uint32_t callingUid, const sptr<AdRequestData> &data,
    const sptr<IAdLoadCallback> &callback, int32_t loadAdType)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE,
        "Begin connect extension ability, bundleName is %{public}s, extension name is %{public}s, userId is %{public}d",
        adServiceElementName_.bundleName.c_str(), adServiceElementName_.extensionName.c_str(),
        adServiceElementName_.userId);
    OHOS::AAFwk::Want connectionWant;
    connectionWant.SetElementName(adServiceElementName_.bundleName, adServiceElementName_.extensionName);
    sptr<AdRequestConnection> serviceConnection =
        new (std::nothrow) AdRequestConnection(callingUid, data, callback, loadAdType);
    ErrCode errCode = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(connectionWant, serviceConnection,
        adServiceElementName_.userId);
    if (errCode != ERR_OK) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to connect ability");
        return false;
    }
    return true;
}
} // namespace Cloud
} // namespace OHOS