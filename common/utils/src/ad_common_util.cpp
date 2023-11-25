/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#include "ad_common_util.h"
#include "ad_hilog_wreapper.h"
#include "bundle_mgr_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Cloud {
using namespace OHOS;

static constexpr char DEFAULT_BUNDLE_NAME[] = "";

sptr<AppExecFwk::IBundleMgr> GetBundleManager()
{
    sptr<ISystemAbilityManager> systemManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemManager == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_COMMON, "Get system ability manager failed!");
        return nullptr;
    }
    return iface_cast<AppExecFwk::IBundleMgr>(systemManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID));
}

std::string AdCommonUtil::GetBundleName()
{
    sptr<AppExecFwk::IBundleMgr> bundleInstance = GetBundleManager();
    if (bundleInstance == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "bundle instance is null!");
        return DEFAULT_BUNDLE_NAME;
    }
    AppExecFwk::BundleInfo bundleInfo;
    auto ret = bundleInstance->GetBundleInfoForSelf(0, bundleInfo);
    if (ret != ERR_OK) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "GetBundleInfoForSelf failed! ret=%{public}d", ret);
        return DEFAULT_BUNDLE_NAME;
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "Get bundle name info is %{public}d: %{public}s", bundleInfo.uid,
        bundleInfo.name.c_str());
    return bundleInfo.name;
}
} // namespace Cloud
} // namespace OHOS