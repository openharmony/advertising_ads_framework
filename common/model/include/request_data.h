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

#ifndef OHOS_CLOUD_ADVERTISING_AD_DATA_H
#define OHOS_CLOUD_ADVERTISING_AD_DATA_H

#include <cstdint>
#include <new>
#include <string>
#include <vector>

#include "want.h"
#include "want_params.h"
#include "parcel.h"

namespace OHOS {
namespace Cloud {
namespace AdsSAData {
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
} // namespace AdsSAData
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_CLIENT_H