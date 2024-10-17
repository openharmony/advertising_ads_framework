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

#include "cj_advertising_impl.h"

#include <cstdint>
#include <fstream>

#include "cj_lambda.h"
#include "config_policy_utils.h"

#include "ad_common_util.h"
#include "ad_constant.h"
#include "ad_hilog_wreapper.h"
#include "ad_json_util.h"
#include "cj_advertising_error.h"
#include "cj_advertising_load_service.h"

namespace OHOS {
namespace Advertising {
static const int32_t LOAD_AD_TYPE = 1;
static const int32_t LOAD_MULTI_AD_TYPE = 2;

void GetAdConfigItem(const char *path, CloudServiceProvider &cloudServiceProvider)
{
    std::ifstream inFile(std::string{path}, std::ios::in);
    if (!inFile.is_open()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "Open file error.");
        return;
    }
    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    cJSON *root = cJSON_Parse(fileContent.c_str());
    if (root == nullptr) {
        ADS_HILOGE(Cloud::ADS_MODULE_CJ_FFI, "Parse fileContent failed.");
        inFile.close();
        cJSON_Delete(root);
        return;
    }
    cJSON *cloudServiceBundleName = cJSON_GetObjectItem(root, "providerBundleName");
    cJSON *cloudServiceAbilityName = cJSON_GetObjectItem(root, "providerAbilityName");
    cJSON *cloudServiceUEAAbilityName = cJSON_GetObjectItem(root, "providerUEAAbilityName");
    if (cloudServiceBundleName == nullptr || cloudServiceAbilityName == nullptr ||
        cloudServiceUEAAbilityName == nullptr) {
        ADS_HILOGE(Cloud::ADS_MODULE_CJ_FFI, "cJSON_GetObjectItem VALUE IS NULL.");
        inFile.close();
        cJSON_Delete(root);
        return;
    }
    if (cJSON_IsArray(cloudServiceBundleName) && cJSON_IsArray(cloudServiceAbilityName) &&
        cJSON_IsArray(cloudServiceUEAAbilityName)) {
            if (cJSON_IsString(cJSON_GetArrayItem(cloudServiceBundleName, 0))) {
                cloudServiceProvider.bundleName = cJSON_GetArrayItem(cloudServiceBundleName, 0)->valuestring;
            }
            if (cJSON_IsString(cJSON_GetArrayItem(cloudServiceAbilityName, 0))) {
                cloudServiceProvider.abilityName = cJSON_GetArrayItem(cloudServiceAbilityName, 0)->valuestring;
            }
            if (cJSON_IsString(cJSON_GetArrayItem(cloudServiceUEAAbilityName, 0))) {
                cloudServiceProvider.ueaAbilityName = cJSON_GetArrayItem(cloudServiceUEAAbilityName, 0)->valuestring;
            }
    }
    inFile.close();
    cJSON_Delete(root);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI,
        "Cloud Service from config bundleName is %{public}s, abilityName is %{public}s, ueaAbility is "
        "%{public}s",
        cloudServiceProvider.bundleName.c_str(), cloudServiceProvider.abilityName.c_str(),
        cloudServiceProvider.ueaAbilityName.c_str());
}

void GetCloudServiceProvider(CloudServiceProvider &cloudServiceProvider)
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
        GetAdConfigItem(realPath, cloudServiceProvider);
    } else {
        GetAdConfigItem(realPath, cloudServiceProvider);
    }
}

void StartUIExtensionAbility(Want &want, std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> context)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "begin StartUIExtensionAbility");
    auto uiExtCallback = std::make_shared<UIExtensionCallback>(context);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "set uiExtCallback");
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallbacks = {
        [uiExtCallback](int32_t releaseCode) { uiExtCallback->OnRelease(releaseCode); },
    };
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "set extensionCallbacks");
    Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "set config");
    auto uiContent = context->GetUIContent();
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "set uiContent");
    if (uiContent == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "StartUIExtensionAbility fail uiContent is null");
        return;
    }
    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallbacks, config);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_CJ_FFI,
        "StartUIExtensionAbility CreateModalUIExtension session id is %{public}d", sessionId);
    if (sessionId == 0) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "StartUIExtensionAbility CreateModalUIExtension fail");
        return;
    }
    uiExtCallback->SetSessionId(sessionId);
}

void ShowAdHandler(Want &want,
                   const CloudServiceProvider& cloudServiceProvider,
                   std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> context)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "start ShowAdHandler.");
    std::string bundleName = cloudServiceProvider.bundleName;
    std::string invokeBundleName = Cloud::AdCommonUtil::GetBundleName();
    if (invokeBundleName.empty()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "get current bundlename failed");
    }
    std::string abilityName = cloudServiceProvider.ueaAbilityName;
    want.SetElementName(bundleName, abilityName);
    want.SetParam(Cloud::AD_BUNDLE_NAME, invokeBundleName);
    want.SetParam(Cloud::AD_UI_EXTENSION_TYPE_KEY, Cloud::AD_UI_EXTENSION_TYPE_VALUE);
    StartUIExtensionAbility(want, context);
}

int32_t CJAdvertisingImpl::showAd(CAdvertisement cAdvertisement,
                                  CAdDisplayOptions cAdDisplayOptions,
                                  std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> context)
{
    cJSON* adDisplayOptionsRoot = cJSON_CreateObject();
    if (!ConvertCAdDisplayOptions2CJSON(cAdDisplayOptions, adDisplayOptionsRoot)) {
        return -1;
    }
    std::string displayOptionsString = Cloud::AdJsonUtil::ToString(adDisplayOptionsRoot);

    cJSON *advertisementRoot = cJSON_CreateObject();
    ConvertCAdadvertisement2CJSON(cAdvertisement, advertisementRoot);
    std::string advertisementString = Cloud::AdJsonUtil::ToString(advertisementRoot);
    cJSON_Delete(advertisementRoot);
    Want want;
    want.SetParam(Cloud::AD_DISPLAY_OPTIONS, displayOptionsString);
    CloudServiceProvider cloudServiceProvider;
    GetCloudServiceProvider(cloudServiceProvider);
    ShowAdHandler(want, cloudServiceProvider, context);
    cJSON_Delete(adDisplayOptionsRoot);
    return 0;
}

CJAdvertisingImpl::CJAdvertisingImpl(AbilityRuntime::AbilityContext* abilityContext)
{
    abilityContext_ = std::shared_ptr<AbilityRuntime::AbilityContext>(abilityContext);
}

int32_t CJAdvertisingImpl::loadAd(CAdRequestParams adParam, CAdOptions adOptions, CAdLoadListenerId callbackId)
{
    cJSON* requestRoot = cJSON_CreateObject();
    if (requestRoot == nullptr) {
        return -1;
    }
    if (!ConvertCAdRequestParams2CJSON(adParam, requestRoot)) {
        return -1;
    }
    std::string requestRootString = Cloud::AdJsonUtil::ToString(requestRoot);

    cJSON* optionRoot = cJSON_CreateObject();
    if (optionRoot == nullptr) {
        return -1;
    }
    if (!ConvertCAdOptions2CJSON(adOptions, optionRoot)) {
        return -1;
    }
    std::string optionRootString = Cloud::AdJsonUtil::ToString(optionRoot);

    AdLoaderCallbackRef adLoaderCallbackRef;
    adLoaderCallbackRef.OnAdLoadSuccess = CJLambda::Create(callbackId.onLoadSuccess);
    adLoaderCallbackRef.OnAdLoadFailure = CJLambda::Create(callbackId.onLoadFailure);
    auto* adLoadCallback = new (std::nothrow) AdLoadListenerCallback(adLoaderCallbackRef);
    ErrCode errorCode =
        AdLoadService::GetInstance()->LoadAd(requestRootString, optionRootString, adLoadCallback, LOAD_AD_TYPE);
    if ((errorCode != 0) && (adLoadCallback != nullptr)) {
        adLoadCallback->OnAdLoadFailure(errorCode, "failed");
    }
    return errorCode;
}

int32_t CJAdvertisingImpl::loadAdWithMultiSlots(CAdRequestParamsArr adParam,
                                                CAdOptions adOptions,
                                                CMultiSlotsAdLoadListenerId callbackId)
{
    cJSON* requestArrRoot = cJSON_CreateObject();
    if (requestArrRoot == nullptr) {
        return -1;
    }
    if (!ConvertCAdRequestParamsArr2CJSON(adParam, requestArrRoot)) {
        return -1;
    }
    std::string requestArrRootString = Cloud::AdJsonUtil::ToString(requestArrRoot);

    cJSON* optionRoot = cJSON_CreateObject();
    if (optionRoot == nullptr) {
        return -1;
    }
    if (!ConvertCAdOptions2CJSON(adOptions, optionRoot)) {
        return -1;
    }
    std::string mulitOptionString = Cloud::AdJsonUtil::ToString(optionRoot);

    MultiAdLoaderCallbackRef multiAdLoaderCallbackRef;
    multiAdLoaderCallbackRef.OnAdLoadSuccess = CJLambda::Create(callbackId.onLoadSuccess);
    multiAdLoaderCallbackRef.OnAdLoadFailure = CJLambda::Create(callbackId.onLoadFailure);
    auto* multiAdLoadCallback = new (std::nothrow) AdLoadListenerCallback(multiAdLoaderCallbackRef);
    ErrCode errCode = AdLoadService::GetInstance()->LoadAd(
        requestArrRootString, mulitOptionString, multiAdLoadCallback, LOAD_MULTI_AD_TYPE);
    if ((errCode != 0) && (multiAdLoadCallback != nullptr)) {
        multiAdLoadCallback->OnAdLoadFailure(errCode, "failed");
    }
    return errCode;
}

std::string CJAdvertisingImpl::getAdRequestBody(CAdRequestParamsArr adParams, CAdOptions adOptions, int32_t* errorCode)
{
    std::string resultInfo = "";
    cJSON* requestArrRoot = cJSON_CreateObject();
    if (requestArrRoot == nullptr) {
        *errorCode = ERR_CJ_PARAMETER_ERROR;
        return resultInfo;
    }
    if (ConvertCAdRequestParamsArr2CJSON(adParams, requestArrRoot) == false) {
        *errorCode = ERR_CJ_PARAMETER_ERROR;
        return resultInfo;
    }
    std::string requestArrRootString = Cloud::AdJsonUtil::ToString(requestArrRoot);

    cJSON* optionRoot = cJSON_CreateObject();
    if (optionRoot == nullptr) {
        *errorCode = ERR_CJ_PARAMETER_ERROR;
        return resultInfo;
    }
    if (ConvertCAdOptions2CJSON(adOptions, optionRoot) == false) {
        *errorCode = ERR_CJ_PARAMETER_ERROR;
        return resultInfo;
    }
    std::string optionRootString = Cloud::AdJsonUtil::ToString(optionRoot);

    std::function<void(std::string)> lambda = [&resultInfo](std::string body) { resultInfo = body; };
    sptr<Cloud::IAdRequestBody> callback = new (std::nothrow) AdRequestBodyAsync(lambda);
    if (callback == nullptr) {
        *errorCode = ERR_CJ_PARAMETER_ERROR;
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CJ_FFI, "create AdRequestBodyAsync callback failed");
        return resultInfo;
    }
    *errorCode = AdLoadService::GetInstance()->RequestAdBody(requestArrRootString, optionRootString, callback);
    return resultInfo;
}

UIExtensionCallback::UIExtensionCallback(std::shared_ptr<AbilityRuntime::AbilityContext> abilityContext)
{
    abilityContext_ = abilityContext;
}

void UIExtensionCallback::SetSessionId(int32_t sessionId)
{
    sessionId_ = sessionId;
}

bool GetUIcontentFromBaseContext(OHOS::AbilityRuntime::AbilityContext* context,
                                 Ace::UIContent*& uiContent)
{
    if (context == nullptr) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "abilityContext is nullptr");
        return false;
    }

    if (context != nullptr) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "get uiContext by ability context");
        uiContent = context->GetUIContent();
    } else {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "get uiContext failed.");
        return false;
    }

    return true;
}

void UIExtensionCallback::CloseModalUI()
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI, "CloseModalUI");
    Ace::UIContent *uiContent;
    if (!GetUIcontentFromBaseContext((this->abilityContext_).get(), uiContent)) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_CJ_FFI, "get uicontent error in release.");
        return;
    };
    uiContent->CloseModalUIExtension(this->sessionId_);
}

void UIExtensionCallback::OnRelease(int32_t releaseCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CJ_FFI,
        "UIExtensionComponent OnRelease(), releaseCode = %{public}d", releaseCode);
    this->CloseModalUI();
}
} // namespace Advertising
} // namespace OHOS