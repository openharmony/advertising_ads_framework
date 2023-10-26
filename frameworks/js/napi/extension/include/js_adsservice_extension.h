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

#ifndef JS_ADSSERVICE_EXTENSION_H
#define JS_ADSSERVICE_EXTENSION_H

#include "adsservice_extension.h"
#include "js_runtime.h"
#include "native_engine/native_reference.h"

namespace OHOS {
namespace AdsExtension {
class JsAdsServiceExtension : public AdsServiceExtension {
public:
    explicit JsAdsServiceExtension(AbilityRuntime::JsRuntime& jsRuntime);
    virtual ~JsAdsServiceExtension() override;

    /**
     * @brief Create JsAdsServiceExtension.
     *
     * @param runtime The runtime.
     * @return The JsAdsServiceExtension instance.
     */
    static JsAdsServiceExtension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);

    /**
     * @brief Init the extension.
     *
     * @param record the extension record.
     * @param application the application info.
     * @param handler the extension handler.
     * @param token the remote token.
     */
    void Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord> &record,
        const std::shared_ptr<AppExecFwk::OHOSApplication> &application,
        std::shared_ptr<AppExecFwk::AbilityHandler> &handler,
        const sptr<IRemoteObject> &token) override;

    /**
     * @brief Called when this adsservice extension is connected for the first time.
     *
     * You can override this function to implement your own processing logic.
     *
     * @param want Indicates the {@link Want} structure containing connection information
     * about the adsservice extension.
     * @return Returns a pointer to the <b>sid</b> of the connected adsservice extension.
     */
    sptr<IRemoteObject> OnConnect(const AAFwk::Want &want) override;

private:
    void BindContext(napi_env env, napi_value obj);
    bool GetSrcPathAndModuleName(std::string& srcPath, std::string& moduleName);
    napi_value CallOnConnect(const AAFwk::Want &want);

    napi_env env_ = nullptr;
    AbilityRuntime::JsRuntime& jsRuntime_;
    std::unique_ptr<NativeReference> jsObj_;
};
} // namespace AdsExtension
} // namespace OHOS

#endif  // JS_ADSSERVICE_EXTENSION_H