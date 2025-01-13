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

#include "cj_advertising_common.h"

#include <cstring>

#include "securec.h"

#include "ad_constant.h"

namespace OHOS {
namespace Advertising {

const int8_t I32_TYPE = 0;
const int8_t STR_TYPE = 1;
const int8_t BOOL_TYPE = 2;

char* MallocCString(const std::string& origin)
{
    if (origin.empty()) {
        return nullptr;
    }
    auto len = origin.length() + 1;
    char* res = static_cast<char*>(malloc(sizeof(char) * len));
    if (res == nullptr) {
        return nullptr;
    }
    return std::char_traits<char>::copy(res, origin.c_str(), len);
}

void Adparameters2CJSONObject(CParameterArr parameters, cJSON* root)
{
    for (int64_t i = 0; i < parameters.size; ++i) {
        auto head = parameters.head + i;
        auto key = std::string(head->key);
        if (head->valueType == I32_TYPE) {
            cJSON_AddItemToObject(root, key.c_str(), cJSON_CreateNumber(*static_cast<double *>(head->value)));
        } else if (head->valueType == STR_TYPE) {
            cJSON_AddItemToObject(
                root, key.c_str(), cJSON_CreateString(static_cast<char *>(head->value)));
        } else if (head->valueType == BOOL_TYPE) {
            cJSON_AddItemToObject(root, key.c_str(), cJSON_CreateBool(*static_cast<bool *>(head->value)));
        }
    }
}

void ConvertCAdadvertisement2CJSON(CAdvertisement advertisement, cJSON* advertisementRoot)
{
    cJSON_AddItemToObject(
        advertisementRoot, Cloud::AD_RESPONSE_AD_TYPE.c_str(), cJSON_CreateNumber(advertisement.adType));
    cJSON_AddItemToObject(
        advertisementRoot, Cloud::AD_RESPONSE_UNIQUE_ID.c_str(), cJSON_CreateString(advertisement.uniqueId));
    cJSON_AddItemToObject(
        advertisementRoot, Cloud::AD_RESPONSE_REWARDED.c_str(), cJSON_CreateBool(advertisement.rewarded));
    cJSON_AddItemToObject(advertisementRoot, Cloud::AD_RESPONSE_SHOWN.c_str(), cJSON_CreateBool(advertisement.shown));
    cJSON_AddItemToObject(
        advertisementRoot, Cloud::AD_RESPONSE_CLICKED.c_str(), cJSON_CreateBool(advertisement.clicked));
    cJSON_AddItemToObject(advertisementRoot, Cloud::AD_RESPONSE_REWARD_CONFIG.c_str(), NULL);
}

bool ConvertCAdRequestParams2CJSON(CAdRequestParams adParam, cJSON* requestRoot)
{
    cJSON_AddItemToObject(requestRoot, Cloud::AD_REQUEST_PARAM_ID.c_str(), cJSON_CreateString(adParam.adId));
    if (adParam.adType.hasValue) {
        cJSON_AddItemToObject(
            requestRoot, Cloud::AD_REQUEST_PARAM_TYPE.c_str(), cJSON_CreateNumber(adParam.adType.value));
    }

    if (adParam.adCount.hasValue) {
        cJSON_AddItemToObject(
            requestRoot, Cloud::AD_REQUEST_PARAM_COUNT.c_str(), cJSON_CreateNumber(adParam.adCount.value));
    }

    if (adParam.adHeight.hasValue) {
        cJSON_AddItemToObject(
            requestRoot, Cloud::AD_REQUEST_PARAM_HEIGHT.c_str(), cJSON_CreateNumber(adParam.adHeight.value));
    }

    if (adParam.adWidth.hasValue) {
        cJSON_AddItemToObject(
            requestRoot, Cloud::AD_REQUEST_PARAM_WIDTH.c_str(), cJSON_CreateNumber(adParam.adWidth.value));
    }

    if (adParam.adSearchKeyword.hasValue) {
        cJSON_AddItemToObject(
            requestRoot, Cloud::AD_REQUEST_PARAM_ID.c_str(), cJSON_CreateString(adParam.adSearchKeyword.value));
    }

    Adparameters2CJSONObject(adParam.extraAttrs, requestRoot);
    return true;
}

bool ConvertCAdOptions2CJSON(CAdOptions adOptions, cJSON* optionRoot)
{
    cJSON_AddItemToObject(
        optionRoot, Cloud::TAG_FOR_CHILD_PROTECTION.c_str(), cJSON_CreateNumber(adOptions.tagForChildProtection));

    if (adOptions.adContentClassification.hasValue) {
        cJSON_AddItemToObject(optionRoot, Cloud::AD_CONTENT_CLASSIFICATION.c_str(),
            cJSON_CreateString(adOptions.adContentClassification.value));
    }

    if (adOptions.nonPersonalizedAd.hasValue) {
        cJSON_AddItemToObject(
            optionRoot, Cloud::NON_PERSONALIZED_AD.c_str(), cJSON_CreateNumber(adOptions.nonPersonalizedAd.value));
    }

    if (adOptions.hasExtraAttrs) {
        Adparameters2CJSONObject(adOptions.extraAttrs, optionRoot);
    }

    return true;
}

bool ConvertCAdRequestParamsArr2CJSON(CAdRequestParamsArr adParam, cJSON* requestArrRoot)
{
    for (int64_t i = 0; i < adParam.size; ++i) {
        cJSON* requestRoot = cJSON_CreateObject();
        if (requestRoot == nullptr) {
            return false;
        }
        ConvertCAdRequestParams2CJSON(adParam.head[i], requestRoot);
        cJSON_AddItemToArray(requestArrRoot, requestRoot);
    }
    return true;
}

bool ConvertCAdDisplayOptions2CJSON(CAdDisplayOptions displayOptions, cJSON* displayOptionRoot)
{
    if (displayOptions.customData.hasValue) {
        cJSON_AddItemToObject(displayOptionRoot,
            Cloud::AD_DISPLAY_OPTIONS_DATA.c_str(), cJSON_CreateString(displayOptions.customData.value));
    }

    if (displayOptions.userId.hasValue) {
        cJSON_AddItemToObject(displayOptionRoot,
            Cloud::AD_DISPLAY_OPTIONS_USER.c_str(), cJSON_CreateString(displayOptions.userId.value));
    }

    if (displayOptions.useMobileDataReminder.hasValue) {
        cJSON_AddItemToObject(displayOptionRoot, Cloud::AD_DISPLAY_OPTIONS_REMINDER.c_str(),
            cJSON_CreateBool(displayOptions.useMobileDataReminder.value));
    }

    if (displayOptions.mute.hasValue) {
        cJSON_AddItemToObject(
            displayOptionRoot, Cloud::AD_DISPLAY_OPTIONS_MUTE.c_str(), cJSON_CreateBool(displayOptions.mute.value));
    }

    if (displayOptions.audioFocusType.hasValue) {
        cJSON_AddItemToObject(displayOptionRoot,
            Cloud::AD_DISPLAY_OPTIONS_TYPE.c_str(), cJSON_CreateNumber(displayOptions.audioFocusType.value));
    }

    if (displayOptions.hasExtraAttrs) {
        Adparameters2CJSONObject(displayOptions.extraAttrs, displayOptionRoot);
    }

    return true;
}

bool JsonStr2CAdvertisement(cJSON* cAdvertisementJson, CAdvertisement* res)
{
    cJSON* adType = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_AD_TYPE.c_str());
    cJSON* uniqueId = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_UNIQUE_ID.c_str());
    cJSON* rewarded = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_REWARDED.c_str());
    cJSON* shown = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_SHOWN.c_str());
    cJSON* clicked = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_CLICKED.c_str());
    cJSON* rewardVerifyConfig = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_REWARD_CONFIG.c_str());
    if (cJSON_IsNumber(adType) && cJSON_IsString(uniqueId) && cJSON_IsBool(rewarded) &&
        cJSON_IsBool(shown) && cJSON_IsBool(clicked) && cJSON_IsArray(rewardVerifyConfig)) {
        res->adType = sizeof(adType->valueint);
        char *uniqueIdStr = cJSON_Print(uniqueId);
        size_t len = 0;
        if (uniqueIdStr) {
            len = strlen(uniqueIdStr) - 1;
            cJSON_free(uniqueIdStr);
        }
        if (strncpy_s(res->uniqueId, len, uniqueId->valuestring, len) != 0) {
            return false;
        }
        res->uniqueId[len] = '\0';
        res->rewarded = cJSON_IsTrue(rewarded);
        res->shown = cJSON_IsTrue(shown);
        res->clicked = cJSON_IsTrue(clicked);
        int32_t rewardVerifyConfigSize = cJSON_GetArraySize(rewardVerifyConfig);
        res->rewardVerifyConfig.size = rewardVerifyConfigSize;
        for (int32_t j = 0; j < rewardVerifyConfigSize; ++j) {
            cJSON* cHashStrPairJson = cJSON_GetArrayItem(rewardVerifyConfig, j);
            if (!cJSON_IsObject(cHashStrPairJson)) {
                cAdvertisementJson = NULL;
                return false;
            }
            cJSON* key = cJSON_GetObjectItem(cHashStrPairJson, "key");
            cJSON* value = cJSON_GetObjectItem(cHashStrPairJson, "value");
            if (!cJSON_IsString(key) || !cJSON_IsString(value)) {
                cAdvertisementJson = NULL;
                return false;
            }
            size_t strLen1 = strlen(res->rewardVerifyConfig.headers[j].key) - 1;
            if (strncpy_s(res->rewardVerifyConfig.headers[j].key, strLen1, key->valuestring, strLen1) != 0) {
                return false;
            }
            res->rewardVerifyConfig.headers[j].key[strLen1] = '\0';
            size_t strLen2 = strlen(res->rewardVerifyConfig.headers[j].value) - 1;
            if (strncpy_s(res->rewardVerifyConfig.headers[j].value, strLen2, value->valuestring, strLen2) != 0) {
                return false;
            }
            res->rewardVerifyConfig.headers[j].value[strLen2] = '\0';
        }
    }
    cAdvertisementJson = NULL;
    return true;
}

bool JsonStr2CAdvertisementArr(cJSON* advertisementArrJson, CAdvertisementArr* res)
{
    if (advertisementArrJson == NULL) {
        return false;
    }
    res->size = cJSON_GetArraySize(advertisementArrJson);
    for (int64_t i = 0; i < res->size; ++i) {
        cJSON* cAdvertisementJson = cJSON_GetArrayItem(advertisementArrJson, i);
        if (cAdvertisementJson == nullptr || !JsonStr2CAdvertisement(cAdvertisementJson, &res->head[i])) {
            res = nullptr;
            return false;
        }
    }
    return true;
}

bool JsonStr2CAdvertisementHashStrArr(cJSON* cAdvertisementHashStrArrJson, CAdvertisementHashStrArr* res)
{
    if (cAdvertisementHashStrArrJson == NULL) {
        return false;
    }
    res->size = cJSON_GetArraySize(cAdvertisementHashStrArrJson);
    for (int64_t i = 0; i < res->size; ++i) {
        cJSON* pairJson = cJSON_GetArrayItem(cAdvertisementHashStrArrJson, i);
        if (pairJson == NULL) {
            return false;
        }
        cJSON* keyJson = cJSON_GetArrayItem(pairJson, 0);
        if (!cJSON_IsString(keyJson)) {
            return false;
        }
        size_t strLen = sizeof(res->headers[i].key) - 1;
        if (strncpy_s(res->headers[i].key, strLen, keyJson->valuestring, strLen) != 0) {
            return false;
        }
        res->headers[i].key[strLen] = '\0';
        cJSON* valueJson = cJSON_GetArrayItem(pairJson, 1);
        if (!cJSON_IsObject(valueJson)) {
            return false;
        }
        if (!JsonStr2CAdvertisementArr(valueJson, res->headers[i].value)) {
            return false;
        }
    }
    return true;
}

} // namespace Advertising
} // namespace OHOS