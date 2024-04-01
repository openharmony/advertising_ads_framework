/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "ad_json_util.h"

namespace OHOS {
namespace Cloud {
bool AdJsonUtil::IsValid(cJSON *item)
{
    return (item != nullptr) && !cJSON_IsInvalid(item);
}

std::string AdJsonUtil::ToString(cJSON *item)
{
    std::string result;
    if (!item) {
        return result;
    }
    char *value = cJSON_PrintUnformatted(item);
    if (value != nullptr) {
        result = value;
        cJSON_free(value);
    }
    return result;
}
} // namespace Cloud
} // namespace OHOS