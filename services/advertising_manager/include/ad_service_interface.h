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

#ifndef OHOS_CLOUD_ADVERTISING_INTERFACE_H
#define OHOS_CLOUD_ADVERTISING_INTERFACE_H

#include "request_data.h"

#include "iremote_broker.h"
#include "iremote_object.h"
#include "errors.h"

namespace OHOS {
namespace Cloud {
class IAdvertisingService : public IRemoteBroker {
public:
    virtual ErrCode LoadAd(const std::string &request, const std::string &options, const sptr<IRemoteObject> &callback,
        uint32_t callingUid, int32_t loadAdType) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.cloud.advertising.IAdvertisingService");
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_INTERFACE_H