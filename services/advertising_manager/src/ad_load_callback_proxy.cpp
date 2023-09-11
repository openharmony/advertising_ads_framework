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

#include "errors.h"
#include "ad_hilog_wreapper.h"
#include "ad_load_callback_proxy.h"
#include "ad_inner_error_code.h"

namespace OHOS {
namespace Cloud {
AdLoadCallbackProxy::AdLoadCallbackProxy(const sptr<IRemoteObject> &object) : IRemoteProxy<IAdLoadCallback>(object) {}

AdLoadCallbackProxy::~AdLoadCallbackProxy() {}

void AdLoadCallbackProxy::OnAdLoadSuccess(const std::vector<AAFwk::Want> &result)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "failed to write descriptor!");
        return;
    }
    if (!data.WriteInt32(result.size())) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "failed to write ad size");
        return;
    }
    for (auto parcelable : result) {
        if (!data.WriteParcelable(&parcelable)) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "failed to write result");
            return;
        }
    }
    SendRequest(IAdLoadCallback::Message::AD_LOAD, data, reply);
}

void AdLoadCallbackProxy::OnAdLoadMultiSlotsSuccess(const std::map<std::string, std::vector<AAFwk::Want>> &result)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "failed to write descriptor!");
        return;
    }
    SendRequest(IAdLoadCallback::Message::MULTI_AD_LOAD, data, reply);
}

void AdLoadCallbackProxy::OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg)
{
    MessageParcel data;
    MessageParcel reply;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "failed to write descriptor!");
        return;
    }
    if (!data.WriteInt32(resultCode)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "failed to write resultCode %{public}d.", resultCode);
        return;
    }
    if (!data.WriteString(resultMsg)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "failed to write result");
        return;
    }
    SendRequest(IAdLoadCallback::Message::AD_LOAD_FAIL, data, reply);
}

ErrCode AdLoadCallbackProxy::SendRequest(IAdLoadCallback::Message code, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remoteCallback = Remote();
    if (remoteCallback == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_COMMON, "remote is nullptr, code = %{public}d", code);
        return ERR_AD_COMMON_AD_NULL_PTR_ERROR;
    }
    MessageOption option(MessageOption::TF_SYNC);
    int32_t result = remoteCallback->SendRequest(static_cast<uint32_t>(code), data, reply, option);
    if (result != ERR_OK) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_COMMON, "failed to SendRequest, code = %{public}d, result = %{public}d",
            code, result);
        return ERR_AD_COMMON_SEND_REQUEST_ERROR;
    }
    return ERR_OK;
}
}
}