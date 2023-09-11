/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");;
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

#ifndef OHOS_CLOUD_NAPI_ADVERTISING_H
#define OHOS_CLOUD_NAPI_ADVERTISING_H

#include <cstdint>
#include <new>
#include <string>
#include <vector>
#include <map>

#include "iad_load_callback.h"
#include "ad_load_napi_common.h"
#include "request_data.h"
#include "want.h"
#include "want_params.h"
#include "parcel.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "errors.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
using namespace OHOS::Cloud;

static const int8_t NO_ERROR = 0;
static const int8_t ERR_START_ABILITY_FAILED = 1;

struct Advertisment {
    uint32_t adType;
    std::map<std::string, std::string> rewardVerifyConfig;
    std::string uniqueId;
    bool rewarded;
    bool shown;
    bool clicked;
    AAFwk::WantParams adExtrea;
};

struct AdRequestParams : public Parcelable {
    std::string adId;
    uint32_t adType;
    uint32_t adCount;
    uint32_t adWidth;
    uint32_t adHeight;
    AAFwk::WantParams adRequestExtra;
    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static AdRequestParams *Unmarshalling(Parcel &parcel);
};

struct AdOptions : public Parcelable {
    uint32_t tagForChildProtection;
    std::string adContentClassification = "";
    uint32_t nonPersonalizedAd;
    AAFwk::WantParams adOptionsExtrea;
    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static AdOptions *Unmarshalling(Parcel &parcel);
};

struct ShowAdRequest {
    napi_env env = nullptr;
    napi_async_work asyncWork;
    OHOS::AAFwk::Want want;
    int8_t errorCode = NO_ERROR;
};

struct AdvertisingRequestContext {
    napi_env env = nullptr;
    napi_async_work asyncWork;
    napi_ref callbackRef = nullptr;
    int8_t errorCode = NO_ERROR;
    std::string requestString;
    std::string optionString;
    AdsSAData::AdRequestParams requestParams;
    AdsSAData::AdOptions adOption;
    AdJSCallback callback;
    sptr<IAdLoadCallback> adLoadCallback = nullptr;
};

struct MultiSlotsRequestContext {
    napi_env env = nullptr;
    napi_async_work asyncWork;
    int8_t errorCode = NO_ERROR;
    std::string mulitRequestString;
    std::string mulitOptionString;
    sptr<IAdLoadCallback> mulitAdLoadCallback = nullptr;
};

struct CloudServiceProvider {
    std::string bundleName;
    std::string abilityName;
    std::string uiAbilityName;
};

class Advertising {
public:
    Advertising();
    ~Advertising();
    static napi_value AdvertisingInit(napi_env env, napi_value exports);

private:
    static napi_value ShowAd(napi_env env, napi_callback_info info);

    static napi_value LoadAd(napi_env env, napi_callback_info info);

    static napi_value LoadAdWithMultiSlots(napi_env env, napi_callback_info info);

    static napi_value JsConstructor(napi_env env, napi_callback_info cbinfo);

    static thread_local napi_ref adRef_;
};

class AdLoader {
public:
    explicit AdLoader();
    ~AdLoader();

private:
    void loadAd();
};
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS
#endif // OHOS_CLOUD_NAPI_ADVERTISING_H