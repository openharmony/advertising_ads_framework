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

#ifndef OHOS_CJ_ADVERTISING_COMMON_H
#define OHOS_CJ_ADVERTISING_COMMON_H

#include <cstdint>
#include <string>

#include "cJSON.h"
#include "cj_common_ffi.h"

extern "C" {
struct NativeOptionUInt32 {
    bool hasValue;
    uint32_t value;
};

struct NativeOptionCString {
    bool hasValue;
    const char* value;
};

struct NativeOptionBool {
    bool hasValue;
    bool value;
};

struct CParameter {
    int8_t valueType;
    char *key;
    void *value;
    int64_t size;
};

struct CParameterArr {
    CParameter *head;
    int64_t size;
};

struct CHashStrPair {
    char *key;
    char *value;
};

struct CHashStrArr {
    CHashStrPair *headers;
    int64_t size;
};

struct CAdvertisement {
    uint32_t adType;
    char *uniqueId;
    bool rewarded;
    bool shown;
    bool clicked;
    CHashStrArr rewardVerifyConfig;
    CParameterArr extraAttrs;
};

struct CAdvertisementArr {
    CAdvertisement *head;
    int64_t size;
};

struct CAdvertisementHashStrArrPair {
    char *key;
    CAdvertisementArr *value;
};

struct CAdvertisementHashStrArr {
    CAdvertisementHashStrArrPair *headers;
    int64_t size;
};

struct CAdRequestParams {
    char *adId;
    NativeOptionUInt32 adType;
    NativeOptionUInt32 adCount;
    NativeOptionUInt32 adWidth;
    NativeOptionUInt32 adHeight;
    NativeOptionCString adSearchKeyword;
    CParameterArr extraAttrs;
    bool hasExtraAttrs = false;
};

struct CAdRequestParamsArr {
    CAdRequestParams *head;
    int64_t size;
};

struct CAdOptions {
    int32_t tagForChildProtection = -1;
    NativeOptionCString adContentClassification;
    NativeOptionUInt32 nonPersonalizedAd;
    CParameterArr extraAttrs;
    bool hasExtraAttrs = false;
};

struct CAdDisplayOptions {
    NativeOptionCString customData;
    NativeOptionCString userId;
    NativeOptionBool useMobileDataReminder;
    NativeOptionBool mute;
    NativeOptionUInt32 audioFocusType;
    CParameterArr extraAttrs;
    bool hasExtraAttrs = false;
};

struct CAdLoadListenerId {
    void (*onLoadSuccess)(CAdvertisementArr);
    void (*onLoadFailure)(int32_t, char*);
};

struct CMultiSlotsAdLoadListenerId {
    void (*onLoadSuccess)(CAdvertisementHashStrArr);
    void (*onLoadFailure)(int32_t, char*);
};
}

namespace OHOS {
namespace Advertising {
char* MallocCString(const std::string& origin);

void Adparameters2CJSONObject(CParameterArr parameters, cJSON* root);

void ConvertCAdadvertisement2CJSON(CAdvertisement advertisement, cJSON* advertisementRoot);

bool ConvertCAdRequestParams2CJSON(CAdRequestParams adParam, cJSON* requestRoot);

bool ConvertCAdOptions2CJSON(CAdOptions adOptions, cJSON* optionRoot);

bool ConvertCAdRequestParamsArr2CJSON(CAdRequestParamsArr adParam, cJSON* requestArrRoot);

bool ConvertCAdDisplayOptions2CJSON(CAdDisplayOptions displayOptions, cJSON* displayOptionRoot);

bool JsonStr2CAdvertisement(cJSON* cAdvertisementJson, CAdvertisement* res);

bool JsonStr2CAdvertisementArr(cJSON* cAdvertisementArrJson, CAdvertisementArr* res);

bool JsonStr2CAdvertisementHashStrArr(cJSON* cAdvertisementHashStrArrJson, CAdvertisementHashStrArr* res);
} // namespace Advertising
} // namespace OHOS
#endif // OHOS_CJ_ADVERTISING_COMMON_H