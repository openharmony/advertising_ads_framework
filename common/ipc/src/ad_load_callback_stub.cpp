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
#include "json/json.h"
#include "ad_constant.h"
#include "ad_hilog_wreapper.h"
#include "ad_inner_error_code.h"
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

inline void CommonParse(AAFwk::Want &want, Json::Value &root)
{
    Json::Value::Members members = root.getMemberNames();
    for (auto iter = members.begin(); iter != members.end(); iter++) {
        auto member = root[*iter];
        auto key = std::string(*iter);
        switch (member.type()) {
            case Json::intValue:
                want.SetParam(key, member.asInt());
                break;
            case Json::stringValue:
                want.SetParam(key, member.asString());
                break;
            case Json::booleanValue:
                want.SetParam(key, member.asBool());
                break;
            default:
                std::string defaultValue = Json::FastWriter().write(member);
                want.SetParam(key, defaultValue);
                break;
        }
    }
}

inline void ParseSingleAd(std::vector<AAFwk::Want> &ads, Json::Value &root)
{
    AAFwk::Want want;
    CommonParse(want, root);
    ads.emplace_back(want);
}

void ParseAdArray(std::string adsString, std::vector<AAFwk::Want> &ads)
{
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(adsString, root)) {
        if (root.type() == Json::arrayValue) {
            uint32_t size = root.size();
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "ads size is: %{public}u.", size);
            for (uint32_t i = 0; i < size; i++) {
                ParseSingleAd(ads, root[i]);
            }
        }
    }
}

void ParseAdMap(std::string adsString, std::map<std::string, std::vector<AAFwk::Want>> &ads)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "multi solts kit return enter");
    Json::Reader reader;
    Json::Value root;
    bool parseResult = reader.parse(adsString, root);
    if (!parseResult) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "parse ad map result is = %{public}d", parseResult);
        return;
    }
    for (Json::ValueIterator itr = root.begin(); itr != root.end(); itr++) {
        std::string key = itr.key().asString();
        std::vector<AAFwk::Want> want;
        std::string value = Json::FastWriter().write(*itr);
        ParseAdArray(value, want);
        ads[key] = want;
    }
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