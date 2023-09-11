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

#ifndef OHOS_CLOUD_ADVERTISING_PROXY_H
#define OHOS_CLOUD_ADVERTISING_PROXY_H

#include "iremote_proxy.h"
#include "request_data.h"

#include "ad_service_interface.h"
#include "advertising_service_ipc_interface_code.h"

namespace OHOS {
namespace Cloud {
class AdvertisingProxy : public IRemoteProxy<IAdvertisingService> {
public:
    explicit AdvertisingProxy(const sptr<IRemoteObject> &impl);
    ~AdvertisingProxy() override;

    ErrCode LoadAd(const std::string &request, const std::string &options, const sptr<IRemoteObject> &callback,
        uint32_t callingUid, int32_t loadAdType) override;

private:
    ErrCode SendRequest(AdsInterfaceCode code, MessageParcel &data, MessageParcel &reply);

private:
    static inline BrokerDelegator<AdvertisingProxy> delegator_;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_PROXY_H