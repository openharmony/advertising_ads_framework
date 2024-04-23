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

#include "ad_napi_common_error.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
napi_value GenerateAdBusinessError(napi_env env, int32_t jsErrorCode, const std::string &jsErrorMsg)
{
    napi_value errCode = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, jsErrorCode, &errCode));
    napi_value errMsg = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, jsErrorMsg.c_str(), NAPI_AUTO_LENGTH, &errMsg));
    napi_value error = nullptr;
    NAPI_CALL(env, napi_create_error(env, nullptr, errMsg, &error));
    NAPI_CALL(env, napi_set_named_property(env, error, "code", errCode));
    NAPI_CALL(env, napi_set_named_property(env, error, "message", errMsg));
    return error;
}
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS