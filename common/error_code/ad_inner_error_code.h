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

#ifndef ADS_ERROR_CODE_H
#define ADS_ERROR_CODE_H

#include <map>
#include <cstdint>
#include "errors.h"

namespace OHOS {
namespace Cloud {
#define ADS_SERVICE_NAME "AdsService"

// The param input is invalid
const int32_t ERR_AD_INPUT_PARAM_INVALID = 401;

// check descriptor fail
const int32_t ERR_AD_COMMON_CHECK_DESCRIPTOR_ERROR = 1;

// read ad size fail
const int32_t ERR_AD_COMMON_READ_AD_SIZE_ERROR = 2;

// read ad fail
const int32_t ERR_AD_COMMON_READ_AD_ERROR = 3;

// send request fail
const int32_t ERR_AD_COMMON_SEND_REQUEST_ERROR = 4;

// excute napi callback is null
const int32_t ERR_AD_COMMON_NAPI_CALLBACK_NULL_ERROR = 5;

// ad proxy is null
const int32_t ERR_AD_COMMON_AD_PROXY_NULL_ERROR = 6;

// write descriptor error
const int32_t ERR_AD_COMMON_AD_WRITE_DESCRIPTOR_ERROR = 7;

// write parcel error
const int32_t ERR_AD_COMMON_AD_WRITE_PARCEL_ERROR = 8;

// read parcel error
const int32_t ERR_AD_COMMON_AD_READ_PARCEL_ERROR = 9;

// null ptr error
const int32_t ERR_AD_COMMON_AD_NULL_PTR_ERROR = 10;

// service invalid parameters
const int32_t ERR_AD_COMMON_AD_SERVICE_INVALID_PARAMETER_ERROR = 11;

// connect ad kit fail
const int32_t ERR_AD_COMMON_AD_CONNECT_KIT_ERROR = 12;

// kit remoteobject is null
const int32_t ERR_AD_COMMON_AD_KIT_REMOTE_OBJECT_ERROR = 13;

// ad service remoteobject is null
const int32_t ERR_AD_COMMON_AD_SA_REMOTE_OBJECT_ERROR = 14;

// ad request object to json fail
const int32_t ERR_AD_COMMON_AD_REQUEST_TO_JSON_ERROR = 15;

// show ad fail
const int32_t ERR_AD_COMMON_AD_SHOW_AD_ERROR = 16;

enum AdsError : int32_t {
    ERR_SEND_OK = 0,
    PARAM_ERR = 401,
    DEVICE_ERR = 801,
    INNER_ERR = 21800001,
    REQUEST_FAIL = 21800003,
    DISPLAY_ERR = 21800004
};
} // namespace Cloud
} // namespace OHOS
#endif // ADS_ERROR_CODE_H