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

#include <string>
#include <vector>
#include <map>

#include "want.h"
#include "cJSON.h"
#include "ad_constant.h"
#include "ad_hilog_wreapper.h"
#include "ad_inner_error_code.h"
#include "ad_json_util.h"
#include "ad_load_callback_stub.h"

namespace OHOS {
namespace Cloud {
static const int32_t LOAD_AD_SUCCESS = 200;

AdLoadCallbackStub::AdLoadCallbackStub() {}
AdLoadCallbackStub::~AdLoadCallbackStub() {}

inline std::string Str16ToStr8(const std::u16string &str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::string result = convert.to_bytes(str);
    return result;
}

void CommonParse(AAFwk::Want &want, cJSON *item)
{
    cJSON *node = nullptr;
    cJSON_ArrayForEach(node, item)
    {
        if (node == nullptr || node->string == nullptr) {
            continue;
        }
        std::string valuestring;
        bool boolValue;
        std::string defaultValue;
        switch (node->type) {
            case cJSON_Number:
                want.SetParam(node->string, node->valueint);
                break;
            case cJSON_String:
                valuestring = node->valuestring;
                want.SetParam(node->string, valuestring);
                break;
            case cJSON_True:
            case cJSON_False:
                boolValue = cJSON_IsTrue(node);
                want.SetParam(node->string, boolValue);
                break;
            default:
                defaultValue = AdJsonUtil::ToString(node);
                want.SetParam(node->string, defaultValue);
                break;
        }
    }
}

inline void ParseSingleAd(std::vector<AAFwk::Want> &ads, cJSON *item)
{
    AAFwk::Want want;
    CommonParse(want, item);
    ads.emplace_back(want);
}

void ParseAdArray(std::string adsString, std::vector<AAFwk::Want> &ads)
{
    cJSON *root = cJSON_Parse(adsString.c_str());
    if (!AdJsonUtil::IsValid(root)) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_COMMON, "parse kit return ad array failed");
        return;
    }
    if (cJSON_IsArray(root)) {
        int size = cJSON_GetArraySize(root);
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "ads size is: %{public}d.", size);
        for (int i = 0; i < size; i++) {
            cJSON *item = cJSON_GetArrayItem(root, i);
            ParseSingleAd(ads, item);
        }
    }
    cJSON_Delete(root);
}

void ParseAdMap(std::string adsString, std::map<std::string, std::vector<AAFwk::Want>> &ads)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "multi solts kit return enter");
    cJSON *root = cJSON_Parse(adsString.c_str());
    if (!AdJsonUtil::IsValid(root)) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_COMMON, "parse kit return ad map failed");
        return;
    }
    cJSON *item = nullptr;
    cJSON_ArrayForEach(item, root)
    {
        if (item == nullptr || item->string == nullptr) {
            continue;
        }
        std::string key = item->string;
        std::vector<AAFwk::Want> want;
        std::string value = AdJsonUtil::ToString(item);
        ParseAdArray(value, want);
        ads[key] = want;
    }
    cJSON_Delete(root);
}

int AdLoadCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    switch (code) {
        case static_cast<uint32_t>(IAdLoadCallback::Message::AD_LOAD): {
            int32_t resultCode = data.ReadInt32();
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "single slot kit return code = %{public}u", resultCode);
            std::string resultMsg = Str16ToStr8(data.ReadString16());
            if (resultCode == LOAD_AD_SUCCESS) {
                std::vector<AAFwk::Want> ads;
                ParseAdArray(resultMsg, ads);
                OnAdLoadSuccess(ads);
            } else {
                OnAdLoadFailure(resultCode, resultMsg);
            }
            break;
        }
        case static_cast<uint32_t>(IAdLoadCallback::Message::MULTI_AD_LOAD): {
            int32_t resultCode = data.ReadInt32();
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "multi slots kit return code = %{public}u", resultCode);
            std::string msg = Str16ToStr8(data.ReadString16());
            if (resultCode == LOAD_AD_SUCCESS) {
                std::map<std::string, std::vector<AAFwk::Want>> adsMap;
                ParseAdMap(msg, adsMap);
                OnAdLoadMultiSlotsSuccess(adsMap);
            } else {
                OnAdLoadFailure(resultCode, msg);
            }
            break;
        }
        default:
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "default, code = %{public}u, flags = %{public}u", code,
                option.GetFlags());
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return ERR_NONE;
}
} // namespace Cloud
} // namespace OHOS