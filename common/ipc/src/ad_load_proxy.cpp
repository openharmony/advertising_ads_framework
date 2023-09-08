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

#include "ad_load_proxy.h"
#include "ad_inner_error_code.h"
#include "ad_constant.h"
#include "ad_hilog_wreapper.h"
#include "errors.h"
#include "iad_load_callback.h"

namespace OHOS {
namespace Cloud {
inline std::u16string Str8ToStr16(const std::string &str)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "Str8ToStr16");
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string result = convert.from_bytes(str);
    return result;
}

ErrCode AdLoadSendRequestProxy::SendAdLoadRequest(uint32_t callingUid, const sptr<AdRequestData> &requestData,
    const sptr<IAdLoadCallback> &callback, int32_t loadAdType)
{
    if (callback == nullptr) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "callback is null");
    }
    MessageParcel data;
    if (!data.WriteInt32(callingUid)) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to callingUid");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    if (!data.WriteRemoteObject(callback->AsObject())) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to WriteRemoteObject");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    if (!data.WriteString16(Str8ToStr16(requestData->adRequest))) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to adRequest");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    if (!data.WriteString16(Str8ToStr16(requestData->adOption))) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to adOption");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    if (!data.WriteString16(Str8ToStr16(requestData->collection))) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to collection");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    return SendAdLoadIpcRequest(loadAdType, data);
}

ErrCode AdLoadSendRequestProxy::SendAdLoadIpcRequest(int32_t code, MessageParcel &data)
{
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    sptr<IRemoteObject> remote = Remote();
    ErrCode result = remote->SendRequest(static_cast<uint32_t>(code), data, reply, option);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "AdLoadSendRequestProxy SendRequest result = %{public}d", result);
    if (result != ERR_OK) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to SendRequest, code = %{public}d, result = %{public}d",
            code, result);
        return ERR_AD_COMMON_SEND_REQUEST_ERROR;
    }
    if (!reply.ReadInt32()) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to read result for ad kit service return");
        return ERR_AD_COMMON_AD_READ_PARCEL_ERROR;
    }
    return result;
}
} // namespace Cloud
} // namespace OHOS