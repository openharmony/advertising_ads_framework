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

#ifndef OHOS_CLOUD_ADVERTISING_LOAD_CALLBACK_H
#define OHOS_CLOUD_ADVERTISING_LOAD_CALLBACK_H

#include <string>
#include <vector>

#include "iremote_broker.h"
#include "want.h"

namespace OHOS {
namespace Cloud {
class IAdLoadCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.cloud.Ads.IAdLoadCallback");

    virtual void OnAdLoadSuccess(const std::string &result) = 0;
    virtual void OnAdLoadMultiSlotsSuccess(const std::string &result) = 0;
    virtual void OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg) = 0;

    enum class Message {
        AD_LOAD = 1,
        MULTI_AD_LOAD = 2,
        AD_LOAD_PARAMS_ERROR = 401,
        DEVICE_NOT_SUPPORT_ERROR = 801,
        AD_LOAD_INNER_ERROR = 100001,
        AD_LOAD_FAIL = 100003,
    };
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_LOAD_CALLBACK_H