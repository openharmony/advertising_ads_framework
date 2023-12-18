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
#include "json/json.h"
#include "securec.h"
#include "want_agent_helper.h"
#include "ad_hilog_wreapper.h"
#include "ad_load_napi_common.h"
#include "ad_inner_error_code.h"
#include "ad_constant.h"
#include "ad_common_util.h"
#include "ad_load_service.h"
#include "ability_manager_client.h"
#include "config_policy_utils.h"
#include "advertising.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
using Want = OHOS::AAFwk::Want;
static constexpr const int MAX_STRING_LENGTH = 65536;
static const int32_t LOAD_AD_TYPE = 1;
static const int32_t LOAD_MULTI_AD_TYPE = 2;
static const int32_t SHOW_AD_PARA = 2;
static const int32_t AD_LOADER_PARA = 3;
static const int32_t AD_STANDARD_SIZE = 6;
const std::string AD_LOADER_CLASS_NAME = "AdLoader";
const std::string DEFAULT_JSON_STR = "{}";
thread_local napi_ref Advertising::adRef_ = nullptr;
static const int32_t STR_MAX_SIZE = 256;
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
    char str[MAX_STRING_LENGTH] = {0};
    size_t length = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, str, MAX_STRING_LENGTH, &length), result);
    if (length > 0) {
        return result.append(str, length);
    }
    return result;
}

void GetCloudServiceProvider(CloudServiceProvider &cloudServiceProvider)
{
    char pathBuff[MAX_PATH_LEN];
    GetOneCfgFile(DEPENDENCY_CONFIG_FILE_RELATIVE_PATH.c_str(), pathBuff, MAX_PATH_LEN);
    char realPath[PATH_MAX];
    if (realpath(pathBuff, realPath) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Parse realpath fail");
        return;
    }

    std::ifstream ifs;
    ifs.open(realPath);
    if (!ifs) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Open file error.");
        return;
    }

    Json::Value jsonValue;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &jsonValue, &errs)) {
        ifs.close();
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Read file failed %{public}s.", errs.c_str());
        return;
    }

    Json::Value cloudServiceBundleName = jsonValue["providerBundleName"];
    Json::Value cloudServiceAbilityName = jsonValue["providerAbilityName"];
    Json::Value cloudServiceUIAbilityName = jsonValue["providerUIAbilityName"];
    cloudServiceProvider.bundleName = cloudServiceBundleName[0].asString();
    cloudServiceProvider.abilityName = cloudServiceAbilityName[0].asString();
    cloudServiceProvider.uiAbilityName = cloudServiceUIAbilityName[0].asString();

    ifs.close();

    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI,
        "Cloud Service from config bundleName is %{public}s, abilityName is %{public}s, uiAbility is "
        "%{public}s",
        cloudServiceProvider.bundleName.c_str(), cloudServiceProvider.abilityName.c_str(),
        cloudServiceProvider.uiAbilityName.c_str());
}

napi_value GetLongStringProperty(const napi_env &env, napi_value &value, const std::string &key,
    std::string &stringValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetLongStringProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    char str[CUSTOM_DATA_MAX_SIZE] = {0};
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
    char str[STR_MAX_SIZE] = {0};
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
    char str[STR_MAX_SIZE] = {0};
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

void NapiNumberAdParamHandler(const std::string &strName, int32_t displayOptionValue, Json::Value &root)
{
    if ((strName == "adWidth" || strName == "adHeight") && displayOptionValue == 0) {
        return;
    }
    root[strName] = displayOptionValue;
}

napi_value ParseObjectFromJs(napi_env env, napi_value argv, Json::Value &root)
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
                root[strName] = displayOptionValue;
                break;
            }
            case napi_boolean: {
                bool displayOptionValue = false;
                NAPI_CALL(env, napi_get_value_bool(env, elementValue, &displayOptionValue));
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi_boolean key is : %{public}s ", strName.c_str());
                root[strName] = displayOptionValue;
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

napi_value ParseAdvertismentByAd(napi_env &env, napi_value &argv, Advertisment &advertisment, Json::Value &root)
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

void AssembShowAdParas(Want &want, const Advertisment &advertisment, Json::Value &root)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AssembShowAdParas enter");
    root[AD_RESPONSE_AD_TYPE] = advertisment.adType;
    root[AD_RESPONSE_REWARD_CONFIG] = {{}, {}};
    root[AD_RESPONSE_UNIQUE_ID] = advertisment.uniqueId;
    root[AD_RESPONSE_REWARDED] = advertisment.rewarded;
    root[AD_RESPONSE_SHOWN] = advertisment.shown;
    root[AD_RESPONSE_CLICKED] = advertisment.clicked;
    std::string advertismentString = Json::FastWriter().write(root);
    want.SetParam(AD_ADVERTISMENT, advertismentString);
}

napi_value Advertising::ShowAd(napi_env env, napi_callback_info info)
{
    size_t argc = SHOW_AD_PARA;
    napi_value argv[SHOW_AD_PARA] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < SHOW_AD_PARA) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "wrong number of showad arguments");
        return NapiGetNull(env);
    }
    Json::Value adDisplayOptionsRoot;
    if (ParseObjectFromJs(env, argv[1], adDisplayOptionsRoot) == nullptr) { // 2 params
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseDisplayOptionsByShowAd failed");
        return NapiGetNull(env);
    }
    std::string displayOptionsString =
        adDisplayOptionsRoot.empty() ? DEFAULT_JSON_STR : Json::FastWriter().write(adDisplayOptionsRoot);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "enter show ad display is %{public}s", displayOptionsString.c_str());
    Want want;
    Advertisment advertisment;
    Json::Value adRoot;
    if (ParseAdvertismentByAd(env, argv[0], advertisment, adRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdvertismentByAd failed");
        return NapiGetNull(env);
    }
    // assemble
    AssembShowAdParas(want, advertisment, adRoot);
    CloudServiceProvider cloudServiceProvider;
    GetCloudServiceProvider(cloudServiceProvider);
    std::string bundleName = cloudServiceProvider.bundleName;
    std::string abilityName = cloudServiceProvider.uiAbilityName;
    want.SetElementName(bundleName, abilityName);
    want.SetParam(AD_DISPLAY_OPTIONS, displayOptionsString);
    want.SetParam("ability.params.backToOtherMissionStack", true);
    std::string invokeBundleName = AdCommonUtil::GetBundleName();
    if (invokeBundleName.empty()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "get current bundlename failed");
    }
    want.SetParam(FULL_SCREEN_SHOW_ONCE_KEY, invokeBundleName);
    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want);
    if (ret != ERR_SEND_OK) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Fail to show ad, err:%{public}d", ret);
    }
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
    napi_value argv[AD_LOADER_PARA] = {nullptr};
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    // argv[0]
    Json::Value requestRoot;
    if (ParseObjectFromJs(env, argv[0], requestRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdRequestByLoadAd failed");
        return NapiGetNull(env);
    }
    std::string requestRootString = Json::FastWriter().write(requestRoot);
    requestRoot["oaid"] = "********-****-****-************";
    std::string requestParam = Json::FastWriter().write(requestRoot);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "requestParam is: %{public}s", requestParam.c_str());
    context->requestString = requestRootString;
    // argv[1]
    Json::Value optionRoot;
    if (ParseObjectFromJs(env, argv[1], optionRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdOptionsByLoadAd failed");
        return NapiGetNull(env);
    }
    std::string optionRootString = optionRoot.empty() ? DEFAULT_JSON_STR : Json::FastWriter().write(optionRoot);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "optionRootString is: %{public}s", optionRootString.c_str());
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
            if ((asyncContext->errorCode != 0) && (asyncContext->adLoadCallback != nullptr)) {
                asyncContext->adLoadCallback->OnAdLoadFailure(asyncContext->errorCode, "failed");
            }
            napi_delete_async_work(env, asyncContext->asyncWork);
            delete asyncContext;
            asyncContext = nullptr;
        },
        reinterpret_cast<void *>(asyncContext), &asyncContext->asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->asyncWork));
    return NapiGetNull(env);
}

bool GetAdsArray(napi_env env, napi_value argv, Json::Value &root)
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
        Json::Value singleRoot;
        if (ParseObjectFromJs(env, adParam, singleRoot) == nullptr) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "multi slots parse single ad failed");
            return false;
        }
        root[static_cast<int>(i)] = singleRoot;
    }
    return true;
}

napi_value ParseContextForMultiSlots(napi_env env, napi_callback_info info, MultiSlotsRequestContext *context)
{
    size_t argc = AD_LOADER_PARA;
    napi_value argv[AD_LOADER_PARA] = {nullptr};
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    // argv[0]
    Json::Value requestRoot;
    if (!GetAdsArray(env, argv[0], requestRoot)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetAdsArray failed");
        return NapiGetNull(env);
    }
    std::string requestRootString = Json::FastWriter().write(requestRoot);
    uint32_t length = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &length));
    for (size_t i = 0; i < length; i++) {
      Json::Value singleRoot = requestRoot[static_cast<int>(i)];
      singleRoot["oaid"] = "********-****-****-************";
      requestRoot[static_cast<int>(i)] = singleRoot;
    }
    std::string requestParam = Json::FastWriter().write(requestRoot);
    ADS_HILOGD(OHOS::Cloud::ADS_MODULE_JS_NAPI, "requestParam is: %{public}s", requestParam.c_str());
    context->mulitRequestString = requestRootString;
    // argv[1]
    Json::Value optionRoot;
    if (ParseObjectFromJs(env, argv[1], optionRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse multi ad option failed");
        return NapiGetNull(env);
    }
    std::string optionRootString = optionRoot.empty() ? DEFAULT_JSON_STR : Json::FastWriter().write(optionRoot);
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
            if ((asyncContext->errorCode != 0) && (asyncContext->mulitAdLoadCallback != nullptr)) {
                asyncContext->mulitAdLoadCallback->OnAdLoadFailure(asyncContext->errorCode, "failed");
            }
            napi_delete_async_work(env, asyncContext->asyncWork);
            delete asyncContext;
            asyncContext = nullptr;
        },
        reinterpret_cast<void *>(asyncContext), &asyncContext->asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->asyncWork));
    return NapiGetNull(env);
}

napi_value Advertising::AdvertisingInit(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptor[] = {
        DECLARE_NAPI_FUNCTION("showAd", ShowAd),
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
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS
