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

#include "ipc_skeleton.h"
#include "ipc_types.h"
#include "message_parcel.h"
#include "ad_hilog_wreapper.h"
#include "ad_inner_error_code.h"
#include "request_data.h"
#include "ad_service.h"
#include "advertising_service_ipc_interface_code.h"
#include "ad_service_stub.h"

namespace OHOS {
namespace Cloud {
const std::map<uint32_t, AdvertisingStub::AdsProcessFunc> AdvertisingStub::messageProcessMap = {
    {
        static_cast<uint32_t>(AdsInterfaceCode::LOAD_AD),
        &AdvertisingStub::ProcessLoadAd,
    },
};

AdvertisingStub::AdvertisingStub() {}
AdvertisingStub::~AdvertisingStub() {}

int AdvertisingStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "invoke cp bundlename is: %{public}u.", static_cast<int>(uid));
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to check descriptor! code %{public}u.", code);
        return ERR_AD_COMMON_CHECK_DESCRIPTOR_ERROR;
    }
    auto messageProc = messageProcessMap.find(code);
    if (messageProc != messageProcessMap.end()) {
        auto messageProcFunction = messageProc->second;
        if (messageProcFunction != nullptr) {
            return (this->*messageProcFunction)(code, data, reply, static_cast<int>(uid));
        }
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode AdvertisingStub::ProcessLoadAd(uint32_t code, MessageParcel &data, MessageParcel &reply, uint32_t callingUid)
{
    std::string adRequest = data.ReadString();
    std::string adOptions = data.ReadString();
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_SERVICE, "ProcessLoadAd adRequest is: %{public}s; adOptions is %{public}s",
        adRequest.c_str(), adOptions.c_str());
    sptr<IRemoteObject> callback = data.ReadRemoteObject();
    int32_t loadType = data.ReadInt32();
    ErrCode result = ERR_OK;
    if ((adRequest.empty()) || (callback == nullptr)) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "invalid parameters");
        result = ERR_AD_COMMON_AD_SERVICE_INVALID_PARAMETER_ERROR;
    } else {
        result = AdvertisingService::GetInstance()->LoadAd(adRequest, adOptions, callback, callingUid, loadType);
    }
    if (!reply.WriteInt32(result)) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_SERVICE, "failed to write reply");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}
} // namespace Cloud
} // namespace OHOS