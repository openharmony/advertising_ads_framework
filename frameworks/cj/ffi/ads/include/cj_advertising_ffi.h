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

#ifndef OHOS_CJ_ADVERTISING_FFI_H
#define OHOS_CJ_ADVERTISING_FFI_H

#include <cstdint>

#include "ability.h"
#include "cj_common_ffi.h"
#include "ffi_remote_data.h"

#include "cj_advertising_common.h"

#ifndef FFI_EXPORT
#ifndef WINDOWS_PLATFORM
#define FFI_EXPORT __attribute__((visibility("default")))
#else
#define FFI_EXPORT __declspec(dllexport)
#endif
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

FFI_EXPORT int64_t FfiOHOSAdvertisingAdLoaderConstructor(OHOS::AbilityRuntime::AbilityContext* abilityContext);

FFI_EXPORT int32_t FfiOHOSAdvertisingShowAd(CAdvertisement cAdvertisement,
                                            CAdDisplayOptions cAdDisplayOptions,
                                            OHOS::AbilityRuntime::AbilityContext* abilityContext);

FFI_EXPORT int32_t FfiOHOSAdvertisingAdLoaderLoadAd(int64_t id,
                                                    CAdRequestParams adParam,
                                                    CAdOptions adOptions,
                                                    CAdLoadListenerId callbackId);

FFI_EXPORT int32_t FfiOHOSAdvertisingAdLoaderLoadAdWithMultiSlots(int64_t id,
                                                                  CAdRequestParamsArr adParams,
                                                                  CAdOptions adOptions,
                                                                  CMultiSlotsAdLoadListenerId callbackId);

FFI_EXPORT char* FfiOHOSAdvertisingGetAdRequestBody(CAdRequestParamsArr adParams,
                                                    CAdOptions adOptions,
                                                    int32_t* errorCode);
}
#endif // OHOS_CJ_ADVERTISING_FFI_H