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
#ifndef OHOS_CJ_ADVERTISING_IMPL_H
#define OHOS_CJ_ADVERTISING_IMPL_H

#include "ability.h"
#include "ability_context.h"
#include "ffi_remote_data.h"
#include "ui_content.h"
#include "want.h"

#include "cj_advertising_common.h"
#include "cj_advertising_ffi.h"

namespace OHOS {
namespace Advertising {

struct GetAdRequestBodyContext {
    int32_t errCode;
    std::string params;
    std::string option;
    std::string resultBody;
};

struct CloudServiceProvider {
    std::string bundleName;
    std::string abilityName;
    std::string uiAbilityName;
    std::string ueaAbilityName;
};

class CJAdvertisingImpl : public OHOS::FFI::FFIData {
    DECL_TYPE(CJAdvertisingImpl, OHOS::FFI::FFIData)
public:
    explicit CJAdvertisingImpl(AbilityRuntime::AbilityContext* abilityContext);
    ~CJAdvertisingImpl() = default;
    int32_t loadAd(CAdRequestParams adParam, CAdOptions adOptions, CAdLoadListenerId callbackId);
    int32_t loadAdWithMultiSlots(CAdRequestParamsArr adParam,
                                 CAdOptions adOptions,
                                 CMultiSlotsAdLoadListenerId callbackId);
    static int32_t showAd(CAdvertisement cAdvertisement,
                          CAdDisplayOptions cAdDisplayOptions,
                          std::shared_ptr<AbilityRuntime::AbilityContext> context);
    static std::string getAdRequestBody(CAdRequestParamsArr adParams, CAdOptions adOptions, int32_t* errorCode);
private:
    std::shared_ptr<AbilityRuntime::AbilityContext> abilityContext_;
};

class UIExtensionCallback {
public:
    explicit UIExtensionCallback(std::shared_ptr<AbilityRuntime::AbilityContext> abilityContext);
    void SetSessionId(int32_t sessionId);
    void OnRelease(int32_t releaseCode);
    void OnResult(int32_t resultCode, const OHOS::AAFwk::Want& result);
    void OnReceive(const OHOS::AAFwk::WantParams& request);
    void OnError(int32_t code, const std::string& name, const std::string& message);
    void OnRemoteReady(const std::shared_ptr<OHOS::Ace::ModalUIExtensionProxy>& uiProxy);
    void OnDestroy();
    void SendMessageBack();

private:
    int32_t sessionId_ = 0;
    int32_t resultCode_ = 0;
    std::string resultUri_ = "";
    std::string resultMode_ = "";
    std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext_;
    std::condition_variable cbFinishCondition_;
    std::mutex cbMutex_;
    bool isCallbackReturned_ = false;

    void CloseModalUI();
};
} // Advertising
} // OHOS
#endif // OHOS_CJ_ADVERTISING_IMPL_H