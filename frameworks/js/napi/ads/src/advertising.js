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
let bundleManager = requireNapi("bundle.bundleManager");
let fs = globalThis.requireNapi('file.fs');
let configPolicy = globalThis.requireNapi('configPolicy');
const hilog = globalThis.requireNapi('hilog');
const READ_FILE_BUFFER_SIZE = 4096;
const HILOG_DOMAIN_CODE = 65280;
const INTERSTITIAL_AD_TYPE = 12;

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

function showAdProxy(ad, adOptions, context) {
  hilog.info(HILOG_DOMAIN_CODE, 'showAdProxy', 'start to show ad');

  if (ad?.adType === null || ad?.uniqueId === null) {
    throw {
      code: 401,
      message: 'Invalid input parameter.'
    };
  }

  if (adOptions === null) {
    throw {
      code: 401,
      message: 'Invalid input parameter.'
    };
  }

  if (ad.adType === INTERSTITIAL_AD_TYPE && !ad.isFullScreen) {
    hilog.info(HILOG_DOMAIN_CODE, 'showAdProxy', 'into interstitial_ad_halfscreen');
    getConfigJsonDataAndJump(ad, adOptions, context);
  } else {
    advertising.showAd(ad, adOptions, context);
  }
}

function getConfigJsonDataAndJump(ad, adOptions, context) {
  configPolicy.getOneCfgFile('etc/advertising/ads_framework/ad_service_config.json', ((t, o) => {
    if (null != t) {
      hilog.warn(HILOG_DOMAIN_CODE, 'showAdProxy', 'error occurs ' + t);
    }
    try {
      const t = fs.openSync(o);
      const i = new ArrayBuffer(READ_FILE_BUFFER_SIZE);
      let s = fs.readSync(t.fd, i);
      fs.closeSync(t);
      let n = String.fromCharCode(...new Uint8Array(i.slice(0, s)));
      n = n.replace(/[\r\n\t\"]/g, '').replace(/\s*/g, '').replace(/\[|\]/g, '');
      let e = toMap(n);
      hilog.info(HILOG_DOMAIN_CODE, 'showAdProxy', 'file succeed');
      e || hilog.info(HILOG_DOMAIN_CODE, 'showAdProxy', 'get config json failed');
      let bundleFLags = bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT;
      let bundleInfo = bundleManager.getBundleInfoForSelfSync(bundleFLags);
      hilog.info(HILOG_DOMAIN_CODE, 'showAdProxy', `get calling bundlename is: ${bundleInfo.name}`);
      let want = {
        bundleName: null == e ? void 0 : e.providerBundleName,
        abilityName: null == e ? void 0 : e.providerUEAAbilityName,
        parameters: {
          ads: ad,
          displayOptions: adOptions,
          bundleName: bundleInfo.name,
          'ability.want.params.uiExtensionType': 'ads'
        }
      };
      context.requestModalUIExtension(want);
    } catch (t) {
      hilog.error(HILOG_DOMAIN_CODE, 'showAdProxy', `open file failed with error:${t.code}, message:${t.message}`);
    }
  }));
}

function toMap(e) {
  const t = (e = e.replace(/[{}]/g, '')).split(',');
  const o = {};
  for (let e = 0; e < t.length; e++) {
    const s = t[e];
    const i = s.indexOf(':');
    if (i > -1) {
      const e = s.substring(0, i);
      const t = s.substring(i + 1);
      o[e] = t;
    }
  }
  return o;
}

export default {
  // 注意：C/C++实现的NAPI模块中的接口如果需要对外暴露，都需要按这种形式来编写
  AdLoader: AdLoader,
  showAd: showAdProxy,
  Advertisement: advertising.Advertisement,
  AdRequestParams: advertising.AdRequestParams,
  AdOptions: advertising.AdOptions,
  AdDisplayOptions: advertising.AdDisplayOptions,
  AdInteractionListener: advertising.AdInteractionListener,
  AdLoadListener: advertising.AdLoadListener,
  MultiSlotsAdLoadListener: advertising.MultiSlotsAdLoadListener,
};