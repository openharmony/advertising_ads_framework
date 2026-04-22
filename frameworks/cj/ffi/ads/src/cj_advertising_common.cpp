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

void FillAdInfo(CAdvertisement* res, cJSON* rewarded, cJSON* shown, cJSON* clicked)
{
    res->rewarded = cJSON_IsTrue(rewarded);
    res->shown = cJSON_IsTrue(shown);
    res->clicked = cJSON_IsTrue(clicked);
}

static void CleanupCAdvertisementOnFailure(CAdvertisement* res)
{
    if (res == nullptr) {
        return;
    }
    if (res->rewardVerifyConfig.headers != nullptr) {
        for (int32_t k = 0; k < res->rewardVerifyConfig.size; ++k) {
            if (res->rewardVerifyConfig.headers[k].key != nullptr) {
                free(res->rewardVerifyConfig.headers[k].key);
            }
            if (res->rewardVerifyConfig.headers[k].value != nullptr) {
                free(res->rewardVerifyConfig.headers[k].value);
            }
        }
        free(res->rewardVerifyConfig.headers);
        res->rewardVerifyConfig.headers = nullptr;
    }
    res->rewardVerifyConfig.size = 0;
    if (res->uniqueId != nullptr) {
        free(res->uniqueId);
        res->uniqueId = nullptr;
    }
}

static bool ParseRewardVerifyConfig(cJSON* rewardVerifyConfig, CAdvertisement* res)
{
    int32_t configSize = cJSON_GetArraySize(rewardVerifyConfig);
    res->rewardVerifyConfig.size = configSize;
    if (configSize <= 0) {
        return true;
    }
    res->rewardVerifyConfig.headers = static_cast<CHashStrPair*>(malloc(sizeof(CHashStrPair) * configSize));
    if (res->rewardVerifyConfig.headers == nullptr) {
        res->rewardVerifyConfig.size = 0;
        free(res->uniqueId);
        res->uniqueId = nullptr;
        return false;
    }
    for (int32_t j = 0; j < configSize; ++j) {
        res->rewardVerifyConfig.headers[j].key = nullptr;
        res->rewardVerifyConfig.headers[j].value = nullptr;
    }
    for (int32_t j = 0; j < configSize; ++j) {
        cJSON* cHashStrPairJson = cJSON_GetArrayItem(rewardVerifyConfig, j);
        if (!cJSON_IsObject(cHashStrPairJson)) {
            CleanupCAdvertisementOnFailure(res);
            return false;
        }
        cJSON* key = cJSON_GetObjectItem(cHashStrPairJson, "key");
        cJSON* value = cJSON_GetObjectItem(cHashStrPairJson, "value");
        if (!cJSON_IsString(key) || !cJSON_IsString(value)) {
            CleanupCAdvertisementOnFailure(res);
            return false;
        }
        res->rewardVerifyConfig.headers[j].key = MallocCString(key->valuestring);
        res->rewardVerifyConfig.headers[j].value = MallocCString(value->valuestring);
        if (res->rewardVerifyConfig.headers[j].key == nullptr ||
            res->rewardVerifyConfig.headers[j].value == nullptr) {
            CleanupCAdvertisementOnFailure(res);
            return false;
        }
    }
    return true;
}

bool JsonStr2CAdvertisement(cJSON* cAdvertisementJson, CAdvertisement* res)
{
    if (cAdvertisementJson == nullptr || res == nullptr) {
        return false;
    }
    cJSON* adType = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_AD_TYPE.c_str());
    cJSON* uniqueId = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_UNIQUE_ID.c_str());
    cJSON* rewarded = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_REWARDED.c_str());
    cJSON* shown = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_SHOWN.c_str());
    cJSON* clicked = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_CLICKED.c_str());
    cJSON* rewardConfig = cJSON_GetObjectItem(cAdvertisementJson, Cloud::AD_RESPONSE_REWARD_CONFIG.c_str());
    if (!cJSON_IsNumber(adType) || !cJSON_IsString(uniqueId) ||
        !cJSON_IsBool(rewarded) || !cJSON_IsBool(shown) ||
        !cJSON_IsBool(clicked) || !cJSON_IsArray(rewardConfig)) {
        return false;
    }
    res->adType = adType->valueint;
    size_t len = strlen(uniqueId->valuestring);
    if (len == 0) {
        return false;
    }
    res->uniqueId = MallocCString(uniqueId->valuestring);
    if (res->uniqueId == nullptr) {
        return false;
    }
    FillAdInfo(res, rewarded, shown, clicked);
    return ParseRewardVerifyConfig(rewardConfig, res);
}

void FreeCAdvertisement(CAdvertisement* ad)
{
    if (ad == nullptr) {
        return;
    }
    if (ad->uniqueId != nullptr) {
        free(ad->uniqueId);
        ad->uniqueId = nullptr;
    }
    if (ad->rewardVerifyConfig.headers != nullptr) {
        for (int32_t k = 0; k < ad->rewardVerifyConfig.size; ++k) {
            if (ad->rewardVerifyConfig.headers[k].key != nullptr) {
                free(ad->rewardVerifyConfig.headers[k].key);
            }
            if (ad->rewardVerifyConfig.headers[k].value != nullptr) {
                free(ad->rewardVerifyConfig.headers[k].value);
            }
        }
        free(ad->rewardVerifyConfig.headers);
        ad->rewardVerifyConfig.headers = nullptr;
        ad->rewardVerifyConfig.size = 0;
    }
    if (ad->extraAttrs.head != nullptr) {
        free(ad->extraAttrs.head);
        ad->extraAttrs.head = nullptr;
        ad->extraAttrs.size = 0;
    }
}

bool JsonStr2CAdvertisementArr(cJSON* advertisementArrJson, CAdvertisementArr* res)
{
    if (advertisementArrJson == NULL || res == NULL) {
        return false;
    }
    res->size = cJSON_GetArraySize(advertisementArrJson);
    if (res->size <= 0) {
        return false;
    }
    res->head = static_cast<CAdvertisement*>(malloc(sizeof(CAdvertisement) * res->size));
    if (res->head == nullptr) {
        res->size = 0;
        return false;
    }
    for (int64_t i = 0; i < res->size; ++i) {
        res->head[i].uniqueId = nullptr;
        res->head[i].rewardVerifyConfig.headers = nullptr;
        res->head[i].rewardVerifyConfig.size = 0;
        res->head[i].extraAttrs.head = nullptr;
        res->head[i].extraAttrs.size = 0;
        cJSON* cAdvertisementJson = cJSON_GetArrayItem(advertisementArrJson, i);
        if (cAdvertisementJson == nullptr || !JsonStr2CAdvertisement(cAdvertisementJson, &res->head[i])) {
            for (int64_t j = 0; j < i; ++j) {
                FreeCAdvertisement(&res->head[j]);
            }
            free(res->head);
            res->head = nullptr;
            res->size = 0;
            return false;
        }
    }
    return true;
}

void FreeCAdvertisementArr(CAdvertisementArr* arr)
{
    if (arr == nullptr) {
        return;
    }
    if (arr->head != nullptr) {
        for (int64_t i = 0; i < arr->size; ++i) {
            FreeCAdvertisement(&arr->head[i]);
        }
        free(arr->head);
        arr->head = nullptr;
    }
    arr->size = 0;
}

void FreeCAdvertisementHashStrArr(CAdvertisementHashStrArr* arr)
{
    if (arr == nullptr) {
        return;
    }
    if (arr->headers != nullptr) {
        for (int64_t i = 0; i < arr->size; ++i) {
            if (arr->headers[i].key != nullptr) {
                free(arr->headers[i].key);
            }
            if (arr->headers[i].value != nullptr) {
                FreeCAdvertisementArr(arr->headers[i].value);
                free(arr->headers[i].value);
            }
        }
        free(arr->headers);
        arr->headers = nullptr;
    }
    arr->size = 0;
}

static bool ParseHashStrArrPair(cJSON* pairJson, CAdvertisementHashStrArrPair* pair)
{
    cJSON* keyJson = cJSON_GetArrayItem(pairJson, 0);
    if (!cJSON_IsString(keyJson)) {
        return false;
    }
    pair->key = MallocCString(keyJson->valuestring);
    if (pair->key == nullptr) {
        return false;
    }
    pair->value = static_cast<CAdvertisementArr*>(malloc(sizeof(CAdvertisementArr)));
    if (pair->value == nullptr) {
        free(pair->key);
        pair->key = nullptr;
        return false;
    }
    pair->value->head = nullptr;
    pair->value->size = 0;
    cJSON* valueJson = cJSON_GetArrayItem(pairJson, 1);
    if (!cJSON_IsObject(valueJson)) {
        free(pair->key);
        pair->key = nullptr;
        free(pair->value);
        pair->value = nullptr;
        return false;
    }
    if (!JsonStr2CAdvertisementArr(valueJson, pair->value)) {
        free(pair->key);
        pair->key = nullptr;
        FreeCAdvertisementArr(pair->value);
        free(pair->value);
        pair->value = nullptr;
        return false;
    }
    return true;
}

static bool InitHashStrArrHeaders(CAdvertisementHashStrArr* res, int64_t size)
{
    if (size <= 0) {
        res->size = 0;
        return false;
    }
    if (static_cast<size_t>(size) > SIZE_MAX / sizeof(CAdvertisementHashStrArrPair)) {
        res->size = 0;
        return false;
    }
    size_t allocSize = sizeof(CAdvertisementHashStrArrPair) * size;
    res->headers = static_cast<CAdvertisementHashStrArrPair*>(malloc(allocSize));
    if (res->headers == nullptr) {
        res->size = 0;
        return false;
    }
    for (int64_t i = 0; i < size; ++i) {
        res->headers[i].key = nullptr;
        res->headers[i].value = nullptr;
    }
    return true;
}

bool JsonStr2CAdvertisementHashStrArr(cJSON* cAdvertisementHashStrArrJson, CAdvertisementHashStrArr* res)
{
    if (cAdvertisementHashStrArrJson == NULL || res == NULL) {
        return false;
    }
    res->size = cJSON_GetArraySize(cAdvertisementHashStrArrJson);
    if (res->size <= 0) {
        return false;
    }
    if (!InitHashStrArrHeaders(res, res->size)) {
        return false;
    }
    for (int64_t i = 0; i < res->size; ++i) {
        cJSON* pairJson = cJSON_GetArrayItem(cAdvertisementHashStrArrJson, i);
        if (pairJson == NULL || !ParseHashStrArrPair(pairJson, &res->headers[i])) {
            FreeCAdvertisementHashStrArr(res);
            return false;
        }
    }
    return true;
}

} // namespace Advertising
} // namespace OHOS