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

#include "cj_advertising_ffi.h"
#include "cj_advertising_error.h"
#include "cj_advertising_impl.h"

int64_t FfiOHOSAdvertisingAdLoaderConstructor(OHOS::AbilityRuntime::AbilityContext* abilityContext)
{
    if (abilityContext == nullptr) {
        return -1;
    }
    auto adLoader = OHOS::FFI::FFIData::Create<OHOS::Advertising::CJAdvertisingImpl>(abilityContext);
    if (adLoader == nullptr) {
        return -1;
    }
    return adLoader->GetID();
}

int32_t FfiOHOSAdvertisingShowAd(CAdvertisement cAdvertisement,
                                 CAdDisplayOptions cAdDisplayOptions,
                                 OHOS::AbilityRuntime::AbilityContext* abilityContext)
{
    if (abilityContext == nullptr) {
        return OHOS::Advertising::ERR_CJ_PARAMETER_ERROR;
    }
    auto sptrContext = std::shared_ptr<OHOS::AbilityRuntime::AbilityContext>(abilityContext);
    return OHOS::Advertising::CJAdvertisingImpl::showAd(cAdvertisement, cAdDisplayOptions, sptrContext);
}


int32_t FfiOHOSAdvertisingAdLoaderLoadAd(int64_t id,
                                         CAdRequestParams adParam,
                                         CAdOptions adOptions,
                                         CAdLoadListenerId callbackId)
{
    auto advertising = OHOS::FFI::FFIData::GetData<OHOS::Advertising::CJAdvertisingImpl>(id);
    if (advertising == nullptr) {
        return OHOS::Advertising::ERR_CJ_SYSTEM_SERVICE_EXCEPTION;
    }
    return advertising->loadAd(adParam, adOptions, callbackId);
}

int32_t FfiOHOSAdvertisingAdLoaderLoadAdWithMultiSlots(int64_t id,
                                                       CAdRequestParamsArr adParams,
                                                       CAdOptions adOptions,
                                                       CMultiSlotsAdLoadListenerId callbackId)
{
    auto advertising = OHOS::FFI::FFIData::GetData<OHOS::Advertising::CJAdvertisingImpl>(id);
    if (advertising == nullptr) {
        return OHOS::Advertising::ERR_CJ_SYSTEM_SERVICE_EXCEPTION;
    }
    return advertising->loadAdWithMultiSlots(adParams, adOptions, callbackId);
}

char* FfiOHOSAdvertisingGetAdRequestBody(CAdRequestParamsArr adParams, CAdOptions adOptions, int32_t* errorCode)
{
    auto res = OHOS::Advertising::CJAdvertisingImpl::getAdRequestBody(adParams, adOptions, errorCode);
    char* tmp = OHOS::Advertising::MallocCString(res);
    return tmp;
}