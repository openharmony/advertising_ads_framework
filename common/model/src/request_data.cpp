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

#include "ad_hilog_wreapper.h"
#include "request_data.h"

namespace OHOS {
namespace Cloud {
namespace AdsSAData {
bool AdRequestParams::ReadFromParcel(Parcel &parcel)
{
    if ((!parcel.ReadString(adId)) || (!parcel.ReadUint32(adType)) || (!parcel.ReadUint32(adCount)) ||
        (!parcel.ReadUint32(adWidth)) || (!parcel.ReadUint32(adHeight))) {
        return false;
    }
    sptr<AAFwk::WantParams> paramsPtr = parcel.ReadParcelable<AAFwk::WantParams>();
    if (paramsPtr == nullptr) {
        return false;
    }
    adRequestExtra = *paramsPtr;
    return true;
}

bool AdRequestParams::Marshalling(Parcel &parcel) const
{
    return parcel.WriteString(adId) && parcel.WriteUint32(adType) && parcel.WriteUint32(adCount) &&
        parcel.WriteUint32(adWidth) && parcel.WriteUint32(adHeight);
}

AdRequestParams *AdRequestParams::Unmarshalling(Parcel &parcel)
{
    AdRequestParams *info = new (std::nothrow) AdRequestParams();
    if ((info != nullptr) && (!info->ReadFromParcel(parcel))) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_COMMON, "ad requestParams read from parcel failed");
        delete info;
        info = nullptr;
    }
    return info;
}

bool AdOptions::ReadFromParcel(Parcel &parcel)
{
    if ((!parcel.ReadUint32(tagForChildProtection)) || (!parcel.ReadString(adContentClassification)) ||
        (!parcel.ReadUint32(nonPersonalizedAd))) {
        return false;
    }
    sptr<AAFwk::WantParams> paramsPtr = parcel.ReadParcelable<AAFwk::WantParams>();
    if (paramsPtr == nullptr) {
        return false;
    }
    adOptionsExtrea = *paramsPtr;
    return true;
}

bool AdOptions::Marshalling(Parcel &parcel) const
{
    return parcel.WriteUint32(tagForChildProtection) && parcel.WriteString(adContentClassification) &&
        parcel.WriteUint32(nonPersonalizedAd);
}

AdOptions *AdOptions::Unmarshalling(Parcel &parcel)
{
    AdOptions *info = new (std::nothrow) AdOptions();
    if ((info != nullptr) && (!info->ReadFromParcel(parcel))) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_COMMON, "ad options read from parcel failed");
        delete info;
        info = nullptr;
    }
    return info;
}
} // namespace AdsSAData
} // namespace Cloud
} // namespace OHOS