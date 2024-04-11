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

#ifndef OHOS_CLOUD_ADS_INIT_DEFINE_H
#define OHOS_CLOUD_ADS_INIT_DEFINE_H

#include <string>

namespace OHOS {
namespace Cloud {
// The system component ID of the advertising is 6104.
static const int32_t ADVERTISING_ID = 6104;

// request ad
static const int32_t SEND_LOAD_AD_REQUEST_CODE = 1;

// request multi solts ad
static const int32_t SEND_LOAD_MULTI_SOLTS_AD_REQUEST_CODE = 2;

// The connection timeout is 3s.
static const int8_t CONNECT_TIME_OUT = 3;

// user id
static const int32_t USER_ID = 100;

// config
static const std::string DEPENDENCY_CONFIG_FILE_RELATIVE_PATH = "etc/advertising/ads_framework/ad_service_config.json";

// config_ext
static const std::string DEPENDENCY_CONFIG_FILE_EXT = "etc/advertising/ads_framework/ad_service_config_ext.json";

// AdRequestParams-adId
static const std::string AD_REQUEST_PARAM_ID = "adId";

// AdRequestParams-adType
static const std::string AD_REQUEST_PARAM_TYPE = "adType";

// AdRequestParams-adCount
static const std::string AD_REQUEST_PARAM_COUNT = "adCount";

// AdRequestParams-adWidth
static const std::string AD_REQUEST_PARAM_WIDTH = "adWidth";

// AdRequestParams-adHeight
static const std::string AD_REQUEST_PARAM_HEIGHT = "adHeight";

// AdRequestParams-extra
static const std::string AD_REQUEST_PARAM_EXTRA = "adExtra";

// AdOptions-tagForChildProtection
static const std::string TAG_FOR_CHILD_PROTECTION = "tagForChildProtection";

// AdOptions-adContentClassification
static const std::string AD_CONTENT_CLASSIFICATION = "adContentClassification";

// AdOptions-nonPersonalizedAd
static const std::string NON_PERSONALIZED_AD = "nonPersonalizedAd";

// AdOptions-nonPersonalizedAd
static const std::string AD_OPTIONS_EXTRA = "optionsExtra";

// AdDisplayOptions-customData
static const std::string AD_DISPLAY_OPTIONS_DATA = "customData";

// AdDisplayOptions-userId
static const std::string AD_DISPLAY_OPTIONS_USER = "userId";

// AdDisplayOptions-useMobileDataReminder
static const std::string AD_DISPLAY_OPTIONS_REMINDER = "useMobileDataReminder";

// AdDisplayOptions-mute
static const std::string AD_DISPLAY_OPTIONS_MUTE = "mute";

// AdDisplayOptions-audioFocusType
static const std::string AD_DISPLAY_OPTIONS_TYPE = "audioFocusType";

// Advertisement-adType
static const std::string AD_RESPONSE_AD_TYPE = "adType";

// Advertisement-rewardVerifyConfig
static const std::string AD_RESPONSE_REWARD_CONFIG = "rewardVerifyConfig";

// Advertisement-uniqueId
static const std::string AD_RESPONSE_UNIQUE_ID = "uniqueId";

// Advertisement-rewarded
static const std::string AD_RESPONSE_REWARDED = "rewarded";

// Advertisement-shown
static const std::string AD_RESPONSE_SHOWN = "shown";

// Advertisement-clicked
static const std::string AD_RESPONSE_CLICKED = "clicked";

// bundle name
static const std::string AD_DEFAULT_BUNDLE_NAME = "";

// ability name
static const std::string AD_DEFAULT_ABILITY_NAME = "";

// show ad ability name
static const std::string AD_DEFAULT_SHOW_AD_ABILITY_NAME = "";

// AdDisplayOptions
static const std::string AD_DISPLAY_OPTIONS = "AdDisplayOptions";

// Instance key
static const std::string FULL_SCREEN_SHOW_ONCE_KEY = "instanceKey";

// Advertisement
static const std::string AD_ADVERTISMENT = "Advertisement";

static const std::string AD_BUNDLE_NAME = "bundleName";

static const std::string AD_UI_EXTENSION_TYPE_KEY = "ability.want.params.uiExtensionType";

static const std::string AD_UI_EXTENSION_TYPE_VALUE = "ads";
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADS_INIT_DEFINE_H