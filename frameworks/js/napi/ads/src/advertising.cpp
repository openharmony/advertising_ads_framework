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

#include <fstream>
#include <memory>
#include <uv.h>
#include "napi_common_want.h"
#include "napi/native_common.h"
#include "cJSON.h"
#include "securec.h"
#include "want_agent_helper.h"
#include "ad_hilog_wreapper.h"
#include "ad_load_napi_common.h"
#include "ad_inner_error_code.h"
#include "ad_constant.h"
#include "ad_common_util.h"
#include "ad_json_util.h"
#include "ad_load_service.h"
#include "ad_napi_common_error.h"
#include "ability_manager_client.h"
#include "config_policy_utils.h"
#include "iad_request_body.h"
#include "advertising.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
using Want = OHOS::AAFwk::Want;
static constexpr const int MAX_STRING_LENGTH = 65536;
static const int32_t LOAD_AD_TYPE = 1;
static const int32_t LOAD_MULTI_AD_TYPE = 2;
static const int32_t SHOW_AD_PARA = 3;
static const int32_t SHOW_AD_MIN_PARA = 3;
static const int32_t CONTEXT_INDEX = 2;
static const int32_t GET_BODY_PARA_NUM = 2;
static const int32_t AD_LOADER_PARA = 3;
static const int32_t AD_STANDARD_SIZE = 6;
const std::string AD_LOADER_CLASS_NAME = "AdLoader";
const std::string DEFAULT_JSON_STR = "{}";
thread_local napi_ref Advertising::adRef_ = nullptr;
static const int32_t STR_MAX_SIZE = 256;
static const int32_t PARAM_ERROR_CODE = 401;
static const int32_t DEVICE_ERROR_CODE = 801;
static const int32_t CUSTOM_DATA_MAX_SIZE = 1024 * 1024; // 1M

napi_value NapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

int32_t GetInt32FromValue(napi_env env, napi_value value)
{
    int32_t ret = 2;
    NAPI_CALL_BASE(env, napi_get_value_int32(env, value, &ret), ret);
    return ret;
}

std::string GetStringFromValueUtf8(napi_env env, napi_value value)
{
    std::string result;
    char str[MAX_STRING_LENGTH] = { 0 };
    size_t length = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, str, MAX_STRING_LENGTH, &length), result);
    if (length > 0) {
        return result.append(str, length);
    }
    return result;
}

void GetAdConfigItem(const char *path, CloudServiceProvider &cloudServiceProvider)
{
    std::ifstream inFile(path, std::ios::in);
    if (!inFile.is_open()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Open file error.");
        return;
    }
    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    cJSON *root = cJSON_Parse(fileContent.c_str());
    if (root == nullptr) {
        ADS_HILOGE(ADS_MODULE_JS_NAPI, "Parse fileContent failed.");
        inFile.close();
        cJSON_Delete(root);
        return;
    }
    cJSON *cloudServiceBundleName = cJSON_GetObjectItem(root, "providerBundleName");
    cJSON *cloudServiceAbilityName = cJSON_GetObjectItem(root, "providerAbilityName");
    cJSON *cloudServiceUEAAbilityName = cJSON_GetObjectItem(root, "providerUEAAbilityName");
    if (cloudServiceBundleName == nullptr || cloudServiceAbilityName == nullptr ||
        cloudServiceUEAAbilityName == nullptr) {
        ADS_HILOGE(ADS_MODULE_JS_NAPI, "cJSON_GetObjectItem VALUE IS NULL.");
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
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI,
        "Cloud Service from config bundleName is %{public}s, abilityName is %{public}s, ueaAbility is "
        "%{public}s",
        cloudServiceProvider.bundleName.c_str(), cloudServiceProvider.abilityName.c_str(),
        cloudServiceProvider.ueaAbilityName.c_str());
}

void GetCloudServiceProvider(CloudServiceProvider &cloudServiceProvider)
{
    char pathBuff[MAX_PATH_LEN];
    GetOneCfgFile(DEPENDENCY_CONFIG_FILE_EXT.c_str(), pathBuff, MAX_PATH_LEN);
    char realPath[PATH_MAX];
    if (realpath(pathBuff, realPath) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "get ext config fail, enter config");
        GetOneCfgFile(Cloud::DEPENDENCY_CONFIG_FILE_RELATIVE_PATH.c_str(), pathBuff, MAX_PATH_LEN);
        if (realpath(pathBuff, realPath) == nullptr) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "get config fail, return");
            return;
        }
        GetAdConfigItem(realPath, cloudServiceProvider);
    } else {
        GetAdConfigItem(realPath, cloudServiceProvider);
    }
}

napi_value GetLongStringProperty(const napi_env &env, napi_value &value, const std::string &key,
    std::string &stringValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetLongStringProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    char str[CUSTOM_DATA_MAX_SIZE] = { 0 };
    size_t strLen = 0;
    bool hasLongStrProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasLongStrProperty));
    if (hasLongStrProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_string) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. String expected.");
            return nullptr;
        }
        NAPI_CALL(env, napi_get_value_string_utf8(env, result, str, CUSTOM_DATA_MAX_SIZE, &strLen));
        if (strLen > CUSTOM_DATA_MAX_SIZE) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "data over size");
            return nullptr;
        }
        stringValue = str;
    }
    return NapiGetNull(env);
}

napi_value GetShortStringProperty(const napi_env &env, napi_value &value, const std::string &key,
    std::string &stringValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetShortStringProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    char str[STR_MAX_SIZE] = { 0 };
    size_t strLen = 0;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_string) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. String expected.");
            return nullptr;
        }
        NAPI_CALL(env, napi_get_value_string_utf8(env, result, str, STR_MAX_SIZE - 1, &strLen));
        stringValue = str;
    }
    return NapiGetNull(env);
}

napi_value GetBoolProperty(const napi_env &env, napi_value &value, const std::string &key, bool &boolValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetBoolProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_boolean) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. Boolean expected.");
            return nullptr;
        }
        napi_get_value_bool(env, result, &boolValue);
    }
    return NapiGetNull(env);
}

napi_value GetInt32Property(const napi_env &env, napi_value &value, const std::string &key, uint32_t &intValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetInt32Property enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    bool hasProperty = false;
    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_number) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. Number expected.");
            return nullptr;
        }
        napi_get_value_uint32(env, result, &intValue);
    }
    return NapiGetNull(env);
}

napi_value GetStringArrayProperty(const napi_env &env, napi_value &value, const std::string &key,
    std::vector<std::string> &arrayValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetStringArrayProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    bool isArray = false;
    char str[STR_MAX_SIZE] = { 0 };
    size_t strLen = 0;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        napi_is_array(env, result, &isArray);
        if (!isArray) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse param is not array");
            return nullptr;
        }
        uint32_t length = 0;
        napi_get_array_length(env, result, &length);
        if (length <= 0) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse array size is invalid");
            return nullptr;
        }
        for (uint32_t i = 0; i < length; ++i) {
            napi_value singleString = nullptr;
            napi_get_element(env, result, i, &singleString);
            NAPI_CALL(env, napi_typeof(env, singleString, &valuetype));
            if (valuetype != napi_string) {
                ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. String expected.");
                return nullptr;
            }
            if (memset_s(str, STR_MAX_SIZE, 0, STR_MAX_SIZE) != 0) {
                ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "memset_s failed.");
                return nullptr;
            }
            NAPI_CALL(env, napi_get_value_string_utf8(env, singleString, str, STR_MAX_SIZE - 1, &strLen));
            arrayValue.emplace_back(str);
        }
    }
    return NapiGetNull(env);
}

napi_value GetWantProperty(const napi_env &env, napi_value &value, const std::string &key,
    AAFwk::WantParams &wantParams)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetWantProperty enter");
    napi_valuetype valuetype = napi_undefined;
    napi_value result = nullptr;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_object) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. Object expected.");
            return nullptr;
        }
        if (!OHOS::AppExecFwk::UnwrapWantParams(env, result, wantParams)) {
            return nullptr;
        }
    }
    return NapiGetNull(env);
}

void NapiNumberAdParamHandler(const std::string &strName, int32_t displayOptionValue, cJSON *root)
{
    if ((strName == "adWidth" || strName == "adHeight" || strName == "adType") && displayOptionValue == 0) {
        return;
    }
    cJSON_AddItemToObject(root, strName.c_str(), cJSON_CreateNumber(displayOptionValue));
}

napi_value ParseObjectFromJs(napi_env env, napi_value argv, cJSON *root)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseObjectFromJs enter");
    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv, &valuetype));
    if (valuetype != napi_object) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. argv Object expected.");
        return nullptr;
    }
    napi_value valueList = nullptr;
    uint32_t valueCount = 0;
    napi_value elementName = nullptr;
    napi_value elementValue = nullptr;
    napi_valuetype elementType = napi_undefined;
    NAPI_CALL(env, napi_get_property_names(env, argv, &valueList));
    NAPI_CALL(env, napi_get_array_length(env, valueList, &valueCount));
    for (uint32_t index = 0; index < valueCount; index++) {
        NAPI_CALL(env, napi_get_element(env, valueList, index, &elementName));
        std::string strName = GetStringFromValueUtf8(env, elementName);
        NAPI_CALL(env, napi_get_named_property(env, argv, strName.c_str(), &elementValue));
        NAPI_CALL(env, napi_typeof(env, elementValue, &elementType));
        switch (elementType) {
            case napi_string: {
                std::string displayOptionValue = GetStringFromValueUtf8(env, elementValue);
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi_string key is : %{public}s ", strName.c_str());
                cJSON_AddItemToObject(root, strName.c_str(), cJSON_CreateString(displayOptionValue.c_str()));
                break;
            }
            case napi_boolean: {
                bool displayOptionValue = false;
                NAPI_CALL(env, napi_get_value_bool(env, elementValue, &displayOptionValue));
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi_boolean key is : %{public}s ", strName.c_str());
                cJSON_AddItemToObject(root, strName.c_str(), cJSON_CreateBool(displayOptionValue));
                break;
            }
            case napi_number: {
                int32_t displayOptionValue = GetInt32FromValue(env, elementValue);
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi_number key is : %{public}s ", strName.c_str());
                NapiNumberAdParamHandler(strName, displayOptionValue, root);
                break;
            }
            default:
                break;
        }
    }
    return NapiGetNull(env);
}

napi_value RemoveAlreadyParsedProperties(const napi_env &env, napi_value &argv,
    const std::vector<std::string> &keyArray)
{
    for (auto key : keyArray) {
        bool hasProperty = false;
        bool deleteresult = false;
        NAPI_CALL(env, napi_has_named_property(env, argv, key.c_str(), &hasProperty));
        if (hasProperty) {
            napi_value propertKey = nullptr;
            napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &propertKey);
            NAPI_CALL(env, napi_delete_property(env, argv, propertKey, &deleteresult));
            if (!deleteresult) {
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "delete Property failed");
                return nullptr;
            }
        }
    }
    return NapiGetNull(env);
}

napi_value ParseAdvertismentByAd(napi_env &env, napi_value &argv, Advertisment &advertisment, cJSON *root)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdvertismentByAd enter");
    napi_valuetype valuetype;
    // argv[1]
    NAPI_CALL(env, napi_typeof(env, argv, &valuetype));
    if (valuetype != napi_object) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. argv[1] Object expected.");
        return nullptr;
    }
    // peoperty size
    napi_value valueList = nullptr;
    uint32_t valueCount = 0;
    NAPI_CALL(env, napi_get_property_names(env, argv, &valueList));
    NAPI_CALL(env, napi_get_array_length(env, valueList, &valueCount));
    if (GetInt32Property(env, argv, AD_RESPONSE_AD_TYPE, advertisment.adType) == nullptr) {
        return nullptr;
    }
    if (GetShortStringProperty(env, argv, AD_RESPONSE_UNIQUE_ID, advertisment.uniqueId) == nullptr) {
        return nullptr;
    }
    if (GetBoolProperty(env, argv, AD_RESPONSE_REWARDED, advertisment.rewarded) == nullptr) {
        return nullptr;
    }
    if (GetBoolProperty(env, argv, AD_RESPONSE_SHOWN, advertisment.shown) == nullptr) {
        return nullptr;
    }
    if (GetBoolProperty(env, argv, AD_RESPONSE_CLICKED, advertisment.clicked) == nullptr) {
        return nullptr;
    }
    if (static_cast<int>(valueCount) > AD_STANDARD_SIZE) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "advertisment has extra value");
        std::vector<std::string> keyArray { AD_RESPONSE_AD_TYPE, AD_RESPONSE_REWARD_CONFIG, AD_RESPONSE_UNIQUE_ID,
            AD_RESPONSE_REWARDED, AD_RESPONSE_SHOWN, AD_RESPONSE_CLICKED };
        if (RemoveAlreadyParsedProperties(env, argv, keyArray) == nullptr) {
            return nullptr;
        }
        ParseObjectFromJs(env, argv, root);
    }
    return NapiGetNull(env);
}

void AssembShowAdParas(Want &want, const Advertisment &advertisment, cJSON *root)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AssembShowAdParas enter");
    cJSON_AddItemToObject(root, AD_RESPONSE_AD_TYPE.c_str(), cJSON_CreateNumber(advertisment.adType));
    cJSON_AddItemToObject(root, AD_RESPONSE_REWARD_CONFIG.c_str(), nullptr);
    cJSON_AddItemToObject(root, AD_RESPONSE_UNIQUE_ID.c_str(), cJSON_CreateString(advertisment.uniqueId.c_str()));
    cJSON_AddItemToObject(root, AD_RESPONSE_REWARDED.c_str(), cJSON_CreateBool(advertisment.rewarded));
    cJSON_AddItemToObject(root, AD_RESPONSE_SHOWN.c_str(), cJSON_CreateBool(advertisment.shown));
    cJSON_AddItemToObject(root, AD_RESPONSE_CLICKED.c_str(), cJSON_CreateBool(advertisment.clicked));
    std::string advertismentString = AdJsonUtil::ToString(root);
    want.SetParam(AD_ADVERTISMENT, advertismentString);
}

void StartUIExtensionAbility(Want &want, std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "begin StartUIExtensionAbility");
    auto uiExtCallback = std::make_shared<UIExtensionCallback>(abilityContext);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "set uiExtCallback");
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallbacks = {
        std::bind(&UIExtensionCallback::OnRelease, uiExtCallback, std::placeholders::_1),
        std::bind(&UIExtensionCallback::OnResult, uiExtCallback, std::placeholders::_1, std::placeholders::_2),
        std::bind(&UIExtensionCallback::OnReceive, uiExtCallback, std::placeholders::_1),
        std::bind(&UIExtensionCallback::OnError,
            uiExtCallback,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3),
    };
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "set extensionCallbacks");
    Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "set config");
    auto uiContent = abilityContext->GetUIContent();
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "set uiContent");
    if (uiContent == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "StartUIExtensionAbility fail uiContent is null");
        return;
    }
    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallbacks, config);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI,
        "StartUIExtensionAbility CreateModalUIExtension session id is %{public}d", sessionId);
    if (sessionId == 0) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "StartUIExtensionAbility CreateModalUIExtension fail");
        return;
    }
    uiExtCallback->SetSessionId(sessionId);
}

bool GetContextFromJs(napi_env env, napi_value obj,
    std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> &abilityContext)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseContextFromJs enter");
    bool stageMode = false;
    napi_status status = OHOS::AbilityRuntime::IsStageContext(env, obj, stageMode);
    if (status != napi_ok || !stageMode) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "it is not a stage mode");
        return false;
    }

    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, obj);
    if (context == nullptr) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetAbilityContext get context failed.");
        return false;
    }
    abilityContext = OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    if (abilityContext == nullptr) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetAbilityContext AbilityRuntime convert context failed");
        return false;
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "end ParseAbilityContextReq");
    return true;
}

void ShowAdHandler(napi_env env, napi_value &obj, Want &want,
    const CloudServiceProvider &cloudServiceProvider, const cJSON *root)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "start ShowAdHandler.");
    std::string bundleName = cloudServiceProvider.bundleName;
    std::string invokeBundleName = AdCommonUtil::GetBundleName();
    if (invokeBundleName.empty()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "get current bundlename failed");
    }
    if (obj == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "wrong number of show ad arguments");
        return;
    }
    std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext = nullptr;
    if (!GetContextFromJs(env, obj, abilityContext)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetContextFromJs err.");
        return;
    }
    std::string abilityName = cloudServiceProvider.ueaAbilityName;
    want.SetElementName(bundleName, abilityName);
    want.SetParam(AD_BUNDLE_NAME, invokeBundleName);
    want.SetParam(AD_UI_EXTENSION_TYPE_KEY, AD_UI_EXTENSION_TYPE_VALUE);
    StartUIExtensionAbility(want, abilityContext);
}

napi_value Advertising::ShowAd(napi_env env, napi_callback_info info)
{
    size_t argc = SHOW_AD_PARA;
    napi_value argv[SHOW_AD_PARA] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < SHOW_AD_MIN_PARA) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "wrong number of showad arguments");
        return NapiGetNull(env);
    }
    cJSON *adDisplayOptionsRoot = cJSON_CreateObject();
    if (ParseObjectFromJs(env, argv[1], adDisplayOptionsRoot) == nullptr) { // 2 params
        napi_throw_error(env, std::to_string(PARAM_ERROR_CODE).c_str(), "Invalid input parameter.");
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseDisplayOptionsByShowAd failed");
        cJSON_Delete(adDisplayOptionsRoot);
        return NapiGetNull(env);
    }
    std::string displayOptionsString = DEFAULT_JSON_STR;
    if (adDisplayOptionsRoot->child != nullptr) {
        displayOptionsString = AdJsonUtil::ToString(adDisplayOptionsRoot);
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "enter show ad display is %{public}s", displayOptionsString.c_str());
    Want want;
    Advertisment advertisment;
    cJSON *adRoot = cJSON_CreateObject();
    if (ParseAdvertismentByAd(env, argv[0], advertisment, adRoot) == nullptr) {
        napi_throw_error(env, std::to_string(PARAM_ERROR_CODE).c_str(), "Invalid input parameter.");
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdvertismentByAd failed");
        cJSON_Delete(adRoot);
        cJSON_Delete(adDisplayOptionsRoot);
        return NapiGetNull(env);
    }
    // assemble
    AssembShowAdParas(want, advertisment, adRoot);
    cJSON_Delete(adRoot);
    CloudServiceProvider cloudServiceProvider;
    GetCloudServiceProvider(cloudServiceProvider);
    want.SetParam(AD_DISPLAY_OPTIONS, displayOptionsString);
    ShowAdHandler(env, argv[CONTEXT_INDEX], want, cloudServiceProvider, adDisplayOptionsRoot);
    cJSON_Delete(adDisplayOptionsRoot);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "end ShowAd");
    return NapiGetNull(env);
}

bool GetCallbackProperty(napi_env env, napi_value obj, napi_ref &property, int argc)
{
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, obj, &valueType), false);
    if (valueType != napi_function) {
        return false;
    }
    NAPI_CALL_BASE(env, napi_create_reference(env, obj, argc, &property), false);
    return true;
}

bool GetNamedFunction(napi_env env, napi_value object, const std::string &name, napi_ref &funcRef)
{
    napi_value value = nullptr;
    napi_get_named_property(env, object, name.c_str(), &value);
    return GetCallbackProperty(env, value, funcRef, 1);
}

bool ParseJSCallback(const napi_env &env, const napi_value &argv, AdJSCallback &callback)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv, &valueType);
    if (valueType != napi_object) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdJSCallback parse failed");
        return false;
    }
    bool hasPropFailed = false;
    napi_has_named_property(env, argv, "onAdLoadFailure", &hasPropFailed);
    bool hasPropSuccess = false;
    napi_has_named_property(env, argv, "onAdLoadSuccess", &hasPropSuccess);
    if (!hasPropFailed || !hasPropSuccess) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdJSCallback has no function");
        return false;
    }
    return GetNamedFunction(env, argv, "onAdLoadFailure", callback.onAdLoadFailure) &&
        GetNamedFunction(env, argv, "onAdLoadSuccess", callback.onAdLoadSuccess);
}

napi_value ParseContextForLoadAd(napi_env env, napi_callback_info info, AdvertisingRequestContext *context)
{
    size_t argc = AD_LOADER_PARA;
    napi_value argv[AD_LOADER_PARA] = { nullptr };
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    // argv[0]
    cJSON *requestRoot = cJSON_CreateObject();
    if (ParseObjectFromJs(env, argv[0], requestRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdRequestByLoadAd failed");
        cJSON_Delete(requestRoot);
        return NapiGetNull(env);
    }
    std::string requestRootString = AdJsonUtil::ToString(requestRoot);
    cJSON_ReplaceItemInObject(requestRoot, "oaid", cJSON_CreateString("********-****-****-************"));
    std::string requestParam = AdJsonUtil::ToString(requestRoot);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "requestParam is: %{public}s", requestParam.c_str());
    cJSON_Delete(requestRoot);
    context->requestString = requestRootString;
    // argv[1]
    cJSON *optionRoot = cJSON_CreateObject();
    if (ParseObjectFromJs(env, argv[1], optionRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdOptionsByLoadAd failed");
        cJSON_Delete(optionRoot);
        return NapiGetNull(env);
    }
    std::string optionRootString = DEFAULT_JSON_STR;
    if (optionRoot->child != nullptr) {
        optionRootString = AdJsonUtil::ToString(optionRoot);
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "optionRootString is: %{public}s", optionRootString.c_str());
    cJSON_Delete(optionRoot);
    context->optionString = optionRootString;
    // argv[2]
    AdJSCallback callback;
    ParseJSCallback(env, argv[2], callback);
    context->adLoadCallback = new (std::nothrow) AdLoadListenerCallback(env, callback); // 2 params
    return NapiGetNull(env);
}

napi_value Advertising::LoadAd(napi_env env, napi_callback_info info)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Advertising::LoadAd enter");
    auto *asyncContext = new (std::nothrow) AdvertisingRequestContext();
    if (asyncContext == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "create asyncContext failed");
        return NapiGetNull(env);
    }
    asyncContext->env = env;
    ParseContextForLoadAd(env, info, asyncContext);
    napi_value resourceName = nullptr;
    NAPI_CALL(env, napi_create_string_latin1(env, "LoadAd", NAPI_AUTO_LENGTH, &resourceName));
    NAPI_CALL(env, napi_create_async_work(
        env, nullptr, resourceName,
        [](napi_env env, void *data) {
            auto *asyncContext = reinterpret_cast<AdvertisingRequestContext *>(data);
            ErrCode errCode = AdLoadService::GetInstance()->LoadAd(asyncContext->requestString,
                asyncContext->optionString, asyncContext->adLoadCallback, LOAD_AD_TYPE);
            asyncContext->errorCode = errCode;
        },
        [](napi_env env, napi_status status, void *data) {
            auto *asyncContext = reinterpret_cast<AdvertisingRequestContext *>(data);
            napi_delete_async_work(env, asyncContext->asyncWork);
            delete asyncContext;
            asyncContext = nullptr;
        },
        reinterpret_cast<void *>(asyncContext), &asyncContext->asyncWork));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncContext->asyncWork, napi_qos_user_initiated));
    return NapiGetNull(env);
}

bool GetAdsArray(napi_env env, napi_value argv, cJSON *root)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, argv, &isArray), false);
    if (!isArray) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "multi slots request is not array");
        return false;
    }
    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, argv, &length), false);
    if (length == 0) {
        return false;
    }
    for (size_t i = 0; i < length; i++) {
        napi_value adParam = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, argv, i, &adParam), false);
        cJSON *singleRoot = cJSON_CreateObject();
        if (ParseObjectFromJs(env, adParam, singleRoot) == nullptr) {
            cJSON_Delete(singleRoot);
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "multi slots parse single ad failed");
            return false;
        }
        if (!cJSON_AddItemToArray(root, singleRoot)) {
            cJSON_Delete(singleRoot);
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "add singleRoot to root failed");
            return false;
        }
    }
    return true;
}

napi_value ParseContextForMultiSlots(napi_env env, napi_callback_info info, MultiSlotsRequestContext *context)
{
    size_t argc = AD_LOADER_PARA;
    napi_value argv[AD_LOADER_PARA] = { nullptr };
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    // argv[0]
    cJSON *requestRoot = cJSON_CreateArray();
    if (!GetAdsArray(env, argv[0], requestRoot)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetAdsArray failed");
        cJSON_Delete(requestRoot);
        return NapiGetNull(env);
    }
    std::string requestRootString = AdJsonUtil::ToString(requestRoot);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "requestParam is: %{public}s", requestRootString.c_str());
    cJSON_Delete(requestRoot);
    context->mulitRequestString = requestRootString;
    // argv[1]
    cJSON *optionRoot = cJSON_CreateObject();
    if (ParseObjectFromJs(env, argv[1], optionRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse multi ad option failed");
        cJSON_Delete(optionRoot);
        return NapiGetNull(env);
    }
    std::string optionRootString = DEFAULT_JSON_STR;
    if (optionRoot->child != nullptr) {
        optionRootString = AdJsonUtil::ToString(optionRoot);
    }
    cJSON_Delete(optionRoot);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "optionRootString is: %{public}s", optionRootString.c_str());
    context->mulitOptionString = optionRootString;
    //  argv[2]
    AdJSCallback callback;
    ParseJSCallback(env, argv[2], callback);
    context->mulitAdLoadCallback = new (std::nothrow) AdLoadListenerCallback(env, callback);
    return NapiGetNull(env);
}

napi_value Advertising::LoadAdWithMultiSlots(napi_env env, napi_callback_info info)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "LoadAdWithMultiSlots enter");
    auto *asyncContext = new (std::nothrow) MultiSlotsRequestContext();
    if (asyncContext == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "create multi slots context failed");
        return NapiGetNull(env);
    }
    asyncContext->env = env;
    ParseContextForMultiSlots(env, info, asyncContext);
    napi_value resourceName = nullptr;
    NAPI_CALL(env, napi_create_string_latin1(env, "LoadAdWithMultiSlots", NAPI_AUTO_LENGTH, &resourceName));
    NAPI_CALL(env, napi_create_async_work(
        env, nullptr, resourceName,
        [](napi_env env, void *data) {
            auto *asyncContext = reinterpret_cast<MultiSlotsRequestContext *>(data);
            ErrCode errCode = AdLoadService::GetInstance()->LoadAd(asyncContext->mulitRequestString,
                asyncContext->mulitOptionString, asyncContext->mulitAdLoadCallback, LOAD_MULTI_AD_TYPE);
            asyncContext->errorCode = errCode;
        },
        [](napi_env env, napi_status status, void *data) {
            auto *asyncContext = reinterpret_cast<MultiSlotsRequestContext *>(data);
            napi_delete_async_work(env, asyncContext->asyncWork);
            delete asyncContext;
            asyncContext = nullptr;
        },
        reinterpret_cast<void *>(asyncContext), &asyncContext->asyncWork));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncContext->asyncWork, napi_qos_user_initiated));
    return NapiGetNull(env);
}

napi_value ParseAdRequestBodyParms(napi_env env, napi_callback_info info, GetAdRequestBodyContext *context)
{
    size_t argc = GET_BODY_PARA_NUM;
    napi_value argv[GET_BODY_PARA_NUM] = { nullptr };
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    // AdRequestParams[]
    cJSON *parms = cJSON_CreateArray();
    if (!GetAdsArray(env, argv[0], parms)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse get request body parms failed");
        cJSON_Delete(parms);
        napi_throw_error(env, std::to_string(PARAM_ERROR_CODE).c_str(), "Invalid input parameter.");
        return NapiGetNull(env);
    }
    std::string parmsString = AdJsonUtil::ToString(parms);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "reuqest body param is: %{public}s", parmsString.c_str());
    cJSON_Delete(parms);
    context->parms = parmsString;
    // AdOptions
    cJSON *optionsObject = cJSON_CreateObject();
    if (ParseObjectFromJs(env, argv[1], optionsObject) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse get request body options failed");
        cJSON_Delete(optionsObject);
        napi_throw_error(env, std::to_string(PARAM_ERROR_CODE).c_str(), "Invalid input parameter.");
        return NapiGetNull(env);
    }
    std::string optionString = DEFAULT_JSON_STR;
    if (optionsObject->child != nullptr) {
        optionString = AdJsonUtil::ToString(optionsObject);
    }
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "reuqest body option is: %{public}s", optionString.c_str());
    cJSON_Delete(optionsObject);
    context->option = optionString;
    return NapiGetNull(env);
}

void ExecuteCBWithPromise(napi_env env, void *data)
{
    auto *bodyContext = reinterpret_cast<GetAdRequestBodyContext *>(data);
    sptr<IAdRequestBody> callback = new (std::nothrow) AdRequestBodyAsync(bodyContext->env, bodyContext->deferred);
    if (callback == nullptr) {
        bodyContext->errCode = INNER_ERR;
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "create AdRequestBodyAsync callback failed");
        return;
    }
    bodyContext->errCode =
        AdLoadService::GetInstance()->RequestAdBody(bodyContext->parms, bodyContext->option, callback);
}

void CompleteCBWithPromise(napi_env env, napi_status status, void *data)
{
    auto *bodyContext = reinterpret_cast<GetAdRequestBodyContext *>(data);
    if (bodyContext->errCode != ERR_SEND_OK) {
        int32_t jsErrorCode = bodyContext->errCode;
        std::string jsErrorMsg = "System internal error.";
        switch (jsErrorCode) {
            case PARAM_ERROR_CODE:
                jsErrorMsg = "Invalid input parameter.";
                break;
            case DEVICE_ERROR_CODE:
                jsErrorMsg = "Device not supported.";
                break;
            default:
                break;
        }
        napi_value error = nullptr;
        error = GenerateAdBusinessError(env, jsErrorCode, jsErrorMsg);
        napi_reject_deferred(env, bodyContext->deferred, error);
    }
    delete bodyContext;
    bodyContext = nullptr;
}

napi_value Advertising::GetAdRequestBody(napi_env env, napi_callback_info info)
{
    auto *reuqestBodyContext = new (std::nothrow) GetAdRequestBodyContext();
    if (reuqestBodyContext == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "create request body context failed");
        return NapiGetNull(env);
    }
    reuqestBodyContext->env = env;
    napi_value promise = nullptr;
    napi_deferred deferred = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    reuqestBodyContext->deferred = deferred;
    ParseAdRequestBodyParms(env, info, reuqestBodyContext);
    napi_value functionName = nullptr;
    NAPI_CALL(env, napi_create_string_latin1(env, "GetAdRequestBody", NAPI_AUTO_LENGTH, &functionName));
    NAPI_CALL(env, napi_create_async_work(env, nullptr, functionName, ExecuteCBWithPromise, CompleteCBWithPromise,
        reinterpret_cast<void *> (reuqestBodyContext), &reuqestBodyContext->asyncWork));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, reuqestBodyContext->asyncWork, napi_qos_user_initiated));
    return promise;
}

napi_value Advertising::AdvertisingInit(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptor[] = {
        DECLARE_NAPI_FUNCTION("showAd", ShowAd),
        DECLARE_NAPI_FUNCTION("getAdRequestBody", GetAdRequestBody),
    };
    NAPI_CALL(env,
        napi_define_properties(env, exports, sizeof(descriptor) / sizeof(napi_property_descriptor), descriptor));
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("loadAd", LoadAd),
        DECLARE_NAPI_FUNCTION("loadAdWithMultiSlots", LoadAdWithMultiSlots),
    };
    napi_value constructor = nullptr;
    NAPI_CALL(env, napi_define_class(env, AD_LOADER_CLASS_NAME.c_str(), AD_LOADER_CLASS_NAME.size(), JsConstructor,
        nullptr, sizeof(properties) / sizeof(napi_property_descriptor), properties, &constructor));
    NAPI_CALL(env, napi_set_named_property(env, exports, AD_LOADER_CLASS_NAME.c_str(), constructor));
    return exports;
}

napi_value Advertising::JsConstructor(napi_env env, napi_callback_info info)
{
    napi_value thisVar = nullptr;
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    NAPI_ASSERT(env, status == napi_ok, "ad loader get context failed");
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "ad loader expect the constructor to use the ability context");
    return thisVar;
}

UIExtensionCallback::UIExtensionCallback(std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext)
{
    abilityContext_ = abilityContext;
}

void UIExtensionCallback::SetSessionId(int32_t sessionId)
{
    sessionId_ = sessionId;
}

bool GetUIcontentFromBaseContext(OHOS::AbilityRuntime::AbilityContext *abilityContext,
    Ace::UIContent *&uiContent)
{
    if (abilityContext == nullptr) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "abilityContext is nullptr");
        return false;
    }

    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "get uiContext by ability context");
    uiContent = abilityContext->GetUIContent();
    if (uiContent == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "UIContent is nullptr");
        return false;
    }
    return true;
}

void UIExtensionCallback::CloseModalUI()
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "CloseModalUI");
    Ace::UIContent *uiContent;
    if (!GetUIcontentFromBaseContext((this->abilityContext_).get(), uiContent) || uiContent == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "get uicontent error in release.");
        return;
    };
    uiContent->CloseModalUIExtension(this->sessionId_);
}

void UIExtensionCallback::OnRelease(int32_t releaseCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI,
        "UIExtensionComponent OnRelease(), releaseCode: %{public}d", releaseCode);
    this->CloseModalUI();
}

void UIExtensionCallback::OnError(int32_t code, const std::string &name, const std::string &message)
{
    AdsError curError = INNER_ERR;
    int32_t SHOWAD_UEA_INNERERR = 1011;
    if (code == SHOWAD_UEA_INNERERR) {
        SHOWAD_UEA_INNERERR = curError;
    } else {
        SHOWAD_UEA_INNERERR = code;
    }
    const std::string errorInfo = "system internal error";
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI,
        "UIExtensionComponent OnError(), Code: %{public}d, name: %{public}s, message: %{public}s,error: %{public}s",
        SHOWAD_UEA_INNERERR,
        name.c_str(),
        message.c_str(),
        errorInfo.c_str());
    this->CloseModalUI();
}

void UIExtensionCallback::OnResult(int32_t resultCode, [[maybe_unused]] const OHOS::AAFwk::Want &result)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "UIExtensionComponent OnResult(), Code: %{public}d", resultCode);
    this->CloseModalUI();
}

void UIExtensionCallback::OnReceive([[maybe_unused]] const OHOS::AAFwk::WantParams &request)
{
    const std::string CONTENT_RECODE_ISNULL = "contentRecordNull";
    const std::string PROCESS_SHOWAD_FAILED = "processShowAdFailed";
    const int32_t DEFAULT_VALUE = 10101010;
    const int32_t resUeaOfShowAdOne = request.GetIntParam(CONTENT_RECODE_ISNULL, DEFAULT_VALUE);
    const int32_t resUeaOfShowAdTwo = request.GetIntParam(PROCESS_SHOWAD_FAILED, DEFAULT_VALUE);
    AdsError curError = DISPLAY_ERR;
    if (resUeaOfShowAdOne == curError || resUeaOfShowAdTwo == curError) {
        const std::string message = "Failed to display the ad";
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI,
                   "UIExtensionComponent OnReceive(), Code: %{public}d, message: %{public}s",
                   curError,
                   message.c_str());
        this->CloseModalUI();
    }
}

} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS
