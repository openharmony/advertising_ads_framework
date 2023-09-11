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

#include "iremote_broker.h"
#include "ad_hilog_wreapper.h"
#include "ad_service_interface.h"
#include "ad_inner_error_code.h"
#include "message_option.h"
#include "advertising_service_ipc_interface_code.h"
#include "ad_service_proxy.h"

namespace OHOS {
namespace Cloud {
AdvertisingProxy::AdvertisingProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IAdvertisingService>(impl) {}

AdvertisingProxy::~AdvertisingProxy() {}

ErrCode AdvertisingProxy::LoadAd(const std::string &request, const std::string &options,
    const sptr<IRemoteObject> &callback, uint32_t callingUid, int32_t loadAdType)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to write descriptor!");
        return ERR_AD_COMMON_AD_WRITE_DESCRIPTOR_ERROR;
    }
    if (!data.WriteString(request)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to write ad request");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    if (!data.WriteString(options)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to write ad options");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    if (!data.WriteRemoteObject(callback)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to write remote object for callback");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    if (!data.WriteInt32(loadAdType)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to write ad load type");
        return ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR;
    }
    MessageParcel reply;
    ErrCode result = SendRequest(AdsInterfaceCode::LOAD_AD, data, reply);
    if (result != ERR_OK) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to send request, errCode: %{public}d", result);
        return result;
    }
    if (!reply.ReadInt32(result)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to read result for check os account constraint enable.");
        return ERR_AD_COMMON_AD_READ_PARCEL_ERROR;
    }
    return result;
}

ErrCode AdvertisingProxy::SendRequest(AdsInterfaceCode code, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "remote is nullptr, code = %{public}d", code);
        return ERR_AD_COMMON_AD_NULL_PTR_ERROR;
    }
    MessageOption option(MessageOption::TF_SYNC);
    int32_t result = remote->SendRequest(static_cast<uint32_t>(code), data, reply, option);
    if (result != ERR_OK) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_CLIENT, "failed to SendRequest, code = %{public}d, result = %{public}d",
            code, result);
        return ERR_AD_COMMON_SEND_REQUEST_ERROR;
    }
    return ERR_OK;
}
} // namespace Cloud
} // namespace OHOS