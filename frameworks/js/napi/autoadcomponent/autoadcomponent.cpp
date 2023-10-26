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
 
#include "native_engine/native_engine.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

/*
 * The modules that need to be developed need to reference the two variables.
 * The xx.js file is compiled into xx_abc.o, which contains the two variables.
 * The variable naming rule is _binary_{MiddleName}_start.
 * In this example, MiddleName is defined as xx_abc,
 * which can be customized but must match the target name of gen_js_obj in BUILD.gn.
 */
extern const char _binary_autoadcomponent_abc_start[];
extern const char _binary_autoadcomponent_abc_end[];

// function name: NAPI_{ModuleName}_GetABCCode
extern "C" __attribute__((visibility("default"))) void NAPI_advertising_AutoAdComponent_GetABCCode(
    const char** buf, int* bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_autoadcomponent_abc_start;
    }
 
    if (bufLen != nullptr) {
        *bufLen = _binary_autoadcomponent_abc_end - _binary_autoadcomponent_abc_start;
    }
}

/*
 * Module define
 */
static napi_module autoAdComponentModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_modname = "advertising.AutoAdComponent",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};
/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void AutoAdComponentRegisterModule(void)
{
    napi_module_register(&autoAdComponentModule);
}