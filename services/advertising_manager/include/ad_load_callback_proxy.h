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

#ifndef OHOS_CLOUD_ADVERTISING_LOAD_CALLBACK_PROXY_H
#define OHOS_CLOUD_ADVERTISING_LOAD_CALLBACK_PROXY_H

#include <iremote_proxy.h>
#include "iad_load_callback.h"

namespace OHOS {
namespace Cloud {
class AdLoadCallbackProxy : public IRemoteProxy<IAdLoadCallback> {
public:
    explicit AdLoadCallbackProxy(const sptr<IRemoteObject> &remote);
    ~AdLoadCallbackProxy();

    void OnAdLoadSuccess(const std::vector<AAFwk::Want> &result) override;
    void OnAdLoadMultiSlotsSuccess(const std::map<std::string, std::vector<AAFwk::Want>> &result) override;
    void OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg) override;

private:
    ErrCode SendRequest(IAdLoadCallback::Message code, MessageParcel &data, MessageParcel &reply);
private:
    static inline BrokerDelegator<AdLoadCallbackProxy> delegator_;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_LOAD_CALLBACK_PROXY_H