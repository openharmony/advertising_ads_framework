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

#include "cj_advertising_load_service.h"

#include <fstream>

#include "ability_connect_callback_stub.h"
#include "ability_manager_client.h"
#include "config_policy_utils.h"
#include "want.h"
#include "want_params.h"
#include "want_params_wrapper.h"

#include "ad_constant.h"
#include "ad_hilog_wreapper.h"
#include "ad_inner_error_code.h"
#include "iad_load_callback.h"
#include "request_data.h"

namespace OHOS {
namespace Advertising {

AdLoadListenerCallback::AdLoadListenerCallback(AdLoaderCallbackRef callback) : callback_(callback) {}
AdLoadListenerCallback::AdLoadListenerCallback(MultiAdLoaderCallbackRef callback)
    : multiCallback_(callback), loadAdType(1) {}

AdLoadListenerCallback::~AdLoadListenerCallback() {}

void AdLoadListenerCallback::OnAdLoadSuccess(const std::string &result)
{
    cJSON* adsArrayJson = cJSON_Parse(result.c_str());
    CAdvertisementArr adsArray;
    if (!JsonStr2CAdvertisementArr(adsArrayJson, &adsArray)) {
        OnAdLoadFailure(Cloud::AdsError::REQUEST_FAIL, "request fail");
        return;
    }
    callback_.OnAdLoadSuccess(adsArray);
}

void AdLoadListenerCallback::OnAdLoadMultiSlotsSuccess(const std::string &result)
{
    cJSON* adsMapJson = cJSON_Parse(result.c_str());
    CAdvertisementHashStrArr adsMap;
    if (!JsonStr2CAdvertisementHashStrArr(adsMapJson, &adsMap)) {
        OnAdLoadFailure(Cloud::AdsError::REQUEST_FAIL, "request fail");
        cJSON_Delete(adsMapJson);
        return;
    }
    cJSON_Delete(adsMapJson);
    multiCallback_.OnAdLoadSuccess(adsMap);
}

void AdLoadListenerCallback::OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg)
{
    char* str = MallocCString(resultMsg);
    if (loadAdType == 0) {
        callback_.OnAdLoadFailure(resultCode, str);
    } else {
        multiCallback_.OnAdLoadFailure(resultCode, str);
    }
    delete[] str;
}

std::mutex AdLoadService::lock_;
sptr<AdLoadService> AdLoadService::instance_;

AdLoadService::AdLoadService()
{
    adServiceElementName_.userId = 0;
}

AdLoadService::~AdLoadService() {};

sptr<AdLoadService> AdLoadService::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(lock_);
        if (instance_ == nullptr) {
            instance_ = new AdLoadService;
        }
    }
    return instance_;
}

void AdLoadService::GetAdServiceElement(AdServiceElementName &adServiceElementName)
{
    char pathBuff[MAX_PATH_LEN];
    GetOneCfgFile(Cloud::DEPENDENCY_CONFIG_FILE_EXT.c_str(), pathBuff, MAX_PATH_LEN);
    char realPath[PATH_MAX] = {0};
    if (strlen(pathBuff) >= PATH_MAX || realpath(pathBuff, realPath) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "get ext config fail, enter config");
        GetOneCfgFile(Cloud::DEPENDENCY_CONFIG_FILE_RELATIVE_PATH.c_str(), pathBuff, MAX_PATH_LEN);
        if (strlen(pathBuff) >= PATH_MAX || realpath(pathBuff, realPath) == nullptr) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "get config fail, return");
            return;
        }
        GetConfigItem(realPath, adServiceElementName);
    } else {
        GetConfigItem(realPath, adServiceElementName);
    }
}

ErrCode AdLoadService::LoadAd(const std::string &request, const std::string &options,
    const sptr<Cloud::IAdLoadCallback> &callback, int32_t loadAdType)
{
    if (adServiceElementName_.bundleName.empty() || adServiceElementName_.extensionName.empty()) {
        GetAdServiceElement(adServiceElementName_);
    }
    if (callback == nullptr) {
        return Cloud::ERR_AD_COMMON_AD_SA_REMOTE_OBJECT_ERROR;
    }
    std::string collection = "{}";
    auto *data = new (std::nothrow) Cloud::AdRequestData(request, options, collection);
    if (ConnectAdKit(data, callback, loadAdType)) {
        return ERR_OK;
    } else {
        callback->OnAdLoadFailure(Cloud::ERR_AD_COMMON_AD_CONNECT_KIT_ERROR, "connect ad kit fail");
        return Cloud::ERR_AD_COMMON_AD_CONNECT_KIT_ERROR;
    }
    return ERR_OK;
}

int32_t AdLoadService::RequestAdBody(const std::string &request,
                                     const std::string &options,
                                     const sptr<Cloud::IAdRequestBody> &callback)
{
    ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "enter RequestAdBody");
    if (adServiceElementName_.bundleName.empty() || adServiceElementName_.extensionName.empty()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "adServiceElementName is null, read from config");
        GetAdServiceElement(adServiceElementName_);
    }
    if (callback == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "ad load callback is null");
        return Cloud::INNER_ERR;
    }
    auto *data = new (std::nothrow) Cloud::AdRequestData(request, options, "");
    OHOS::AAFwk::Want connectionWant;
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "connect extension ability, apiServiceName is %{public}s,",
        adServiceElementName_.apiServiceName.c_str());
    connectionWant.SetElementName(adServiceElementName_.bundleName, adServiceElementName_.apiServiceName);
    auto serviceConnection =
        sptr<AdRequestConnection>(new (std::nothrow) AdRequestConnection(data, callback, adServiceElementName_));
    ErrCode errCode = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(connectionWant, serviceConnection,
        adServiceElementName_.userId);
    if (errCode != ERR_OK) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "failed to connect ability");
        return Cloud::INNER_ERR;
    }
    return Cloud::ERR_SEND_OK;
}

void AdLoadService::GetConfigItem(const char *path, AdServiceElementName &adServiceElementName)
{
    std::ifstream inFile(path, std::ios::in);
    if (!inFile.is_open()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "Open file error.");
        return;
    }
    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    cJSON *root = cJSON_Parse(fileContent.c_str());
    if (root == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "ParseJsonFromFile failed.");
        inFile.close();
        return;
    }
    cJSON *cloudServiceBundleName = cJSON_GetObjectItem(root, "providerBundleName");
    cJSON *cloudServiceAbilityName = cJSON_GetObjectItem(root, "providerAbilityName");
    cJSON *apiServiceName = cJSON_GetObjectItem(root, "providerApiAbilityName");
    if (cloudServiceBundleName == nullptr || cloudServiceAbilityName == nullptr ||
        apiServiceName == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "cJSON_GetObjectItem VALUE IS NULL");
        inFile.close();
        cJSON_Delete(root);
        return;
    }
    if (cJSON_IsArray(cloudServiceBundleName) && cJSON_IsArray(cloudServiceAbilityName) &&
        cJSON_IsArray(apiServiceName)) {
        if (cJSON_IsString(cJSON_GetArrayItem(cloudServiceBundleName, 0))) {
            adServiceElementName.bundleName = cJSON_GetArrayItem(cloudServiceBundleName, 0)->valuestring;
        }
        if (cJSON_IsString(cJSON_GetArrayItem(cloudServiceAbilityName, 0))) {
            adServiceElementName.extensionName = cJSON_GetArrayItem(cloudServiceAbilityName, 0)->valuestring;
        }
        if (cJSON_IsString(cJSON_GetArrayItem(apiServiceName, 0))) {
            adServiceElementName.apiServiceName = cJSON_GetArrayItem(apiServiceName, 0)->valuestring;
        }
    }
    adServiceElementName.userId = Cloud::USER_ID;
    inFile.close();
    cJSON_Delete(root);
}

bool AdLoadService::ConnectAdKit(const sptr<Cloud::AdRequestData> &data, const sptr<Cloud::IAdLoadCallback> &callback,
    int32_t loadAdType)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI,
        "Begin connect extension ability, bundleName is %{public}s, extension name is %{public}s, userId is %{public}d",
        adServiceElementName_.bundleName.c_str(), adServiceElementName_.extensionName.c_str(),
        adServiceElementName_.userId);
    OHOS::AAFwk::Want connectionWant;
    connectionWant.SetElementName(adServiceElementName_.bundleName, adServiceElementName_.extensionName);
    sptr<AdRequestConnection> serviceConnection =
        new (std::nothrow) AdRequestConnection(data, callback, loadAdType, adServiceElementName_);
    ErrCode errCode = AAFwk::AbilityManagerClient::GetInstance()->ConnectExtensionAbility(connectionWant,
        serviceConnection, adServiceElementName_.userId);
    if (errCode != ERR_OK) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "failed to connect ability");
        return false;
    }
    return true;
}

void AdRequestConnection::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
                                               const sptr<IRemoteObject> &remoteObject,
                                               int32_t resultCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "AdRequestConnection  OnAbilityConnectDone %{public}d.", resultCode);
    if (resultCode != ERR_OK) {
        return;
    }
    if (element.GetAbilityName() == currAdServiceElementName_.extensionName) {
        proxy_ = (new (std::nothrow) Cloud::AdLoadSendRequestProxy(remoteObject));
        if (proxy_ == nullptr) {
            ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "ad load get send request proxy failed.");
            return;
        }
        proxy_->SendAdLoadRequest(data_, callback_, loadAdType_);
    } else if (element.GetAbilityName() == currAdServiceElementName_.apiServiceName) {
        bodyProxy_ = (new (std::nothrow) Cloud::AdRequestBodySendProxy(remoteObject));
        if (bodyProxy_ == nullptr) {
            ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "make ad request body proxy failed.");
            return;
        }
        bodyProxy_->SendAdBodyRequest(data_, bodyCallback_);
    } else {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "not match any service.");
    }
}

void AdRequestConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName& element, int resultCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "ad load on service disconnected.");
    proxy_ = nullptr;
}

AdRequestBodyAsync::AdRequestBodyAsync(std::function<void(std::string)> getBodyInfo) : getBodyInfo_(getBodyInfo) {}

AdRequestBodyAsync::~AdRequestBodyAsync() {}

void AdRequestBodyAsync::OnRequestBodyReturn(int32_t resultCode,
                                             const std::string& body,
                                             [[maybe_unused]] bool isResolved)
{
    getBodyInfo_(body);
}

} // namespace Advertising
} // namespace OHOS