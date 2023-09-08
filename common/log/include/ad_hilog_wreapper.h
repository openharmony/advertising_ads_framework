/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_CLOUD_ADS_HILOG_WRAPPER_H
#define OHOS_CLOUD_ADS_HILOG_WRAPPER_H

#include "hilog/log.h"

namespace OHOS {
namespace Cloud {
// param of log interface, such as ADS_HILOGF.
enum AdsSubModule {
    ADS_MODULE_INNERKIT = 0,
    ADS_MODULE_CLIENT,
    ADS_MODULE_SERVICE,
    ADS_MODULE_COMMON,
    ADS_MODULE_JS_NAPI,
    ADS_MODULE_BUTT,
};

static constexpr unsigned int ADS_DOMAIN_ID = 0xD004704;

static constexpr OHOS::HiviewDFX::HiLogLabel ADS_MODULE_LABEL[ADS_MODULE_BUTT] = {
    {LOG_CORE, ADS_DOMAIN_ID, "AdsInnerKit"},
    {LOG_CORE, ADS_DOMAIN_ID, "AdsClient"},
    {LOG_CORE, ADS_DOMAIN_ID, "AdsService"},
    {LOG_CORE, ADS_DOMAIN_ID, "AdsCommon"},
    {LOG_CORE, ADS_DOMAIN_ID, "AdsJSNAPI"},
};

#define R_FILENAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define R_FORMATED(fmt, ...) "[%{public}s] %{public}s# " fmt, R_FILENAME, __FUNCTION__, ##__VA_ARGS__

#define ADS_HILOGF(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Fatal(OHOS::Cloud::ADS_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define ADS_HILOGE(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Error(OHOS::Cloud::ADS_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define ADS_HILOGW(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Warn(OHOS::Cloud::ADS_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define ADS_HILOGI(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Info(OHOS::Cloud::ADS_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
#define ADS_HILOGD(module, ...) \
    (void)OHOS::HiviewDFX::HiLog::Debug(OHOS::Cloud::ADS_MODULE_LABEL[module], R_FORMATED(__VA_ARGS__))
}  // namespace Cloud
}  // namespace OHOS
#endif  // OHOS_CLOUD_ADS_HILOG_WRAPPER_H
