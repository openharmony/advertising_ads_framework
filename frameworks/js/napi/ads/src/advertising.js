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

// 首先需要通过requireInternal函数加载本模块
const advertising = requireInternal('advertising');
const hilog = globalThis.requireNapi('hilog');
const HILOG_DOMAIN_CODE = 65280;
class AdLoader {
  constructor(context) {
    this.loader = new advertising.AdLoader(context);
  }

  loadAdWithMultiSlots(adParams, adOptions, listener) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', 'start to load ad with multi-slots');
    this.loader.loadAdWithMultiSlots(adParams, adOptions, {
      onAdLoadFailure: listener?.onAdLoadFailure,
      onAdLoadSuccess: onAdLoadSuccessProxy(listener),
    });
  }

  loadAd(adParams, adOptions, listener) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', 'start to load ad');
    this.loader.loadAd(adParams, adOptions, listener);
  }
}

function onAdLoadSuccessProxy(callBackListener) {
  return (multiSlotsAds) => {
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', 'on receive loading-ad resp');
    const resultMap = new Map();
    Object.keys(multiSlotsAds)?.forEach(key => {
      resultMap.set(key, multiSlotsAds[key]);
    });
    callBackListener.onAdLoadSuccess(resultMap);
  };
}

export default {
  // 注意：C/C++实现的NAPI模块中的接口如果需要对外暴露，都需要按这种形式来编写
  AdLoader: AdLoader,
  showAd: advertising.showAd,
  Advertisement: advertising.Advertisement,
  AdRequestParams: advertising.AdRequestParams,
  AdOptions: advertising.AdOptions,
  AdDisplayOptions: advertising.AdDisplayOptions,
  AdInteractionListener: advertising.AdInteractionListener,
  AdLoadListener: advertising.AdLoadListener,
  MultiSlotsAdLoadListener: advertising.MultiSlotsAdLoadListener,
};