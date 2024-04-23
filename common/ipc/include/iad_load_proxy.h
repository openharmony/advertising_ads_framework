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

#ifndef OHOS_CLOUD_ADVERTISING_LOAD_IPROXY_H
#define OHOS_CLOUD_ADVERTISING_LOAD_IPROXY_H

#include <string>
#include <vector>

#include "iremote_broker.h"
#include "iad_load_callback.h"
#include "iad_request_body.h"
#include "refbase.h"

namespace OHOS {
namespace Cloud {
struct AdRequestData : public RefBase {
    std::string adRequest;
    std::string adOption;
    std::string collection;

    AdRequestData(std::string remoteAdRequest, std::string remoteAdOption, std::string remoteCollection)
        : adRequest(remoteAdRequest), adOption(remoteAdOption), collection(remoteCollection)
    {}
};

class IAdLoadSendRequest : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Cloud.Ads.IAdLoadSendRequest");

    virtual ErrCode SendAdLoadRequest(const sptr<AdRequestData> &data, const sptr<IAdLoadCallback> &callback,
        int32_t loadAdType) = 0;
};

class IAdRequestBodySend : public IRemoteBroker {
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Cloud.Ads.IAdRequestBodySend");

    virtual void SendAdBodyRequest(const sptr<AdRequestData> &data, const sptr<IAdRequestBody> &callback) = 0;
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_LOAD_IPROXY_H