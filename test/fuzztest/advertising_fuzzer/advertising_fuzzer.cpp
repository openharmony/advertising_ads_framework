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

#include "advertising_fuzzer.h"

#include <string>
#include "ad_service.h"
#include "advertising_service_ipc_interface_code.h"
#include "ad_hilog_wreapper.h"
#include "ad_load_callback_stub.h"
#include "message_option.h"
#include "iremote_broker.h"
#include "iremote_object.h"

using namespace std;
using namespace OHOS::Cloud;

class MockAdLoadListenerCallback : public OHOS::Cloud::AdLoadCallbackStub {
public:
    void OnAdLoadSuccess(const std::vector<OHOS::AAFwk::Want> &result) override
    {
        return;
    }
    void OnAdLoadMultiSlotsSuccess(const std::map<std::string, std::vector<OHOS::AAFwk::Want>> &result) override
    {
        return;
    }
    void OnAdLoadFailure(int32_t resultCode, const std::string &resultMsg) override
    {
        return;
    }
    OHOS::sptr<OHOS::IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

namespace OHOS {
constexpr size_t AD_THRESHOLD = 2;
const std::u16string AD_INTERFACE_TOKEN = u"ohos.cloud.advertising.IAdvertisingService";

bool AdvertisingFuzzTest(const uint8_t *rawData, size_t size)
{
    uint32_t requestCode = static_cast<uint32_t>(OHOS::Cloud::AdsInterfaceCode::LOAD_AD);
    MessageParcel data;
    data.WriteInterfaceToken(AD_INTERFACE_TOKEN);
    std::string testRequest(reinterpret_cast<const char *>(rawData), size);
    data.WriteString(testRequest);
    std::string testOption = "test";
    data.WriteString(testOption);
    sptr<IAdLoadCallback> remote = new (std::nothrow) MockAdLoadListenerCallback();
    data.WriteRemoteObject(remote->AsObject());
    int32_t testLoadType = 1;
    data.WriteInt32(testLoadType);
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    auto adService = sptr<Cloud::AdvertisingService>(new (std::nothrow) Cloud::AdvertisingService());
    adService->OnRemoteRequest(requestCode, data, reply, option);
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    if (size < OHOS::AD_THRESHOLD) {
        return 0;
    }
    OHOS::AdvertisingFuzzTest(data, size);
    return 0;
}