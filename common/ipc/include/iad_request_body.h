/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_ADVERTISING_AD_REQUEST_BODY_H
#define OHOS_CLOUD_ADVERTISING_AD_REQUEST_BODY_H

#include <string>

#include "iremote_broker.h"

namespace OHOS {
namespace Cloud {
class IAdRequestBody : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.cloud.Ads.IAdRequestBody");
    virtual void OnRequestBodyReturn(const std::string &body, bool isResolved) = 0;

    enum class Message {
        REQUEST_BODY_CODE = 1,
        REQUEST_BODY_PARAMS_ERROR = 401,
        REQUEST_BODY_INNER_ERROR = 21800001,
    };
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_AD_REQUEST_BODY_H