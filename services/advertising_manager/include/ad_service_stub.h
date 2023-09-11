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

#ifndef OHOS_CLOUD_ADVERTISING_STUB_H
#define OHOS_CLOUD_ADVERTISING_STUB_H

#include <map>
#include <string>
#include "ad_service_interface.h"

#include "iremote_stub.h"
#include "errors.h"
#include "message_parcel.h"

namespace OHOS {
namespace Cloud {
class AdvertisingStub : public IRemoteStub<IAdvertisingService> {
public:
    using AdsProcessFunc = ErrCode (AdvertisingStub::*)(uint32_t code, MessageParcel &data, MessageParcel &reply,
        uint32_t callingUid);
    AdvertisingStub();
    ~AdvertisingStub() override;
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ErrCode ProcessLoadAd(uint32_t code, MessageParcel &data, MessageParcel &reply, uint32_t callingUid);

private:
    static const std::map<uint32_t, AdsProcessFunc> messageProcessMap;
    DISALLOW_COPY_AND_MOVE(AdvertisingStub);
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_STUB_H
