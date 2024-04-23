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

#include <string>

#include "ad_constant.h"
#include "ad_hilog_wreapper.h"
#include "ad_request_body_stub.h"

namespace OHOS {
namespace Cloud {
AdRequestBodyStub::AdRequestBodyStub() {}
AdRequestBodyStub::~AdRequestBodyStub() {}

std::string Str16ToStr8(const std::u16string &str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::string result = convert.to_bytes(str);
    return result;
}

int32_t AdRequestBodyStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    if (code == static_cast<uint32_t>(IAdRequestBody::Message::REQUEST_BODY_CODE)) {
        int32_t resultCode = data.ReadInt32();
        std::string resultMsg = Str16ToStr8(data.ReadString16());
        ADS_HILOGD(OHOS::Cloud::ADS_MODULE_COMMON, "request body return code = %{public}u", resultCode);
        bool isResolved = (resultCode == IPC_SUCCESS && !resultMsg.empty());
        OnRequestBodyReturn(resultMsg, isResolved);
    } else {
        ADS_HILOGD(OHOS::Cloud::ADS_MODULE_COMMON, "no business, code = %{public}u, flags = %{public}u", code,
            option.GetFlags());
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return ERR_NONE;
}
} // namespace Cloud
} // namespace OHOS