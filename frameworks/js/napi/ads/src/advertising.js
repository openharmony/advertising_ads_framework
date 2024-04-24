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
const fs = globalThis.requireNapi('file.fs');
const configPolicy = globalThis.requireNapi('configPolicy');
const rpc = globalThis.requireNapi('rpc');

const HILOG_DOMAIN_CODE = 65280;
const READ_FILE_BUFFER_SIZE = 4096;
const JS_BRIDGE_RPC_CODE = 1;
const ADS_SERVICE_CONFIG_EXT_FILE = 'etc/advertising/ads_framework/ad_service_config_ext.json';
const ADS_SERVICE_CONFIG_FILE = 'etc/advertising/ads_framework/ad_service_config.json';

const AdsError = {
  ERR_SEND_OK: 0,
  PARAM_ERR: 401,
  INNER_ERR: 21800001,
  REQUEST_FAIL: 21800003
};

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
    this.loader.loadAd(adParams, adOptions, {
      onAdLoadFailure: listener?.onAdLoadFailure,
      onAdLoadSuccess: onSingleSlotAdLoadSuccessProxy(listener),
    });
  }
}

function onAdLoadSuccessProxy(callBackListener) {
  return (multiSlotsAds) => {
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', 'on receive loading-ad resp');
    try {
      const resultMap = new Map();
      const adsObj = JSON.parse(multiSlotsAds);
      Object.keys(adsObj)?.forEach(key => {
        resultMap.set(key, adsObj[key]);
      });
      callBackListener?.onAdLoadSuccess(resultMap);
    } catch (error) {
      listener?.onAdLoadFailure(AdsError.REQUEST_FAIL, error.message);
    }
  };
}

function onSingleSlotAdLoadSuccessProxy(callBackListener) {
  return (ads) => {
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', 'on receive loading single slot ad resp');
    try {
      const adsArray = JSON.parse(ads);
      callBackListener?.onAdLoadSuccess(adsArray);
    } catch (error) {
      listener?.onAdLoadFailure(AdsError.REQUEST_FAIL, error.message);
    }
  };
}

function getConfigJsonData() {
  let map = null;
  let path = '';
  try {
    path = configPolicy.getOneCfgFileSync(ADS_SERVICE_CONFIG_EXT_FILE);
    if (path === null || path === '') {
      hilog.warn(HILOG_DOMAIN_CODE, 'advertising', 'get ext config file failed');
      path = configPolicy.getOneCfgFileSync(ADS_SERVICE_CONFIG_FILE);
      if (path === null || path === '') {
        hilog.warn(HILOG_DOMAIN_CODE, 'advertising', 'get config file failed');
        return map;
      }
    }

    const file = fs.openSync(path);
    const buf = new ArrayBuffer(READ_FILE_BUFFER_SIZE);
    let readLen = fs.readSync(file.fd, buf);
    fs.closeSync(file);
    let fileContent = String.fromCharCode(...new Uint8Array(buf.slice(0, readLen)));
    fileContent = fileContent.replace(/[\r\n\t\"]/g, '').replace(/\s*/g, '').replace(/\[|\]/g, '');
    hilog.info(HILOG_DOMAIN_CODE, 'advertising', `read file succeed`);
    map = toMap(fileContent);
    if (!map) {
      hilog.info(HILOG_DOMAIN_CODE, 'advertising', `get config json failed`);
    }
    return map;
  } catch (e) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `get config error, code:${e.code}, message:${e.message}`);
  }
  return map;
}

function toMap(str) {
  str = str.replace(/[{}]/g, '');
  const arr = str.split(',');
  const map = {};
  for (let index = 0; index < arr.length; index++) {
    const item = arr[index];
    const i = item.indexOf(':');
    if (i > -1) {
      const key = item.substring(0, i);
      const value = item.substring(i + 1);
      map[key] = value;
    }
  }
  return map;
}

class AdsJsClientRpcObj extends rpc.RemoteObject {
  constructor(descriptor, callback) {
    super(descriptor);
    this.callback = callback;
  }

  onRemoteMessageRequest(code, data, reply, options) {
    hilog.info(HILOG_DOMAIN_CODE, 'advertising', `onRemoteMessageRequest enter`);
    if (code !== JS_BRIDGE_RPC_CODE) {
      hilog.error(HILOG_DOMAIN_CODE, 'advertising', `onRemoteMessageRequest code error`);
      return false;
    }
    try {
      const readstr = data.readString();
      this.callback(readstr);
      return true;
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'advertising', `handle rpc error, code:${e.code}, message:${e.message}`);
      return false;
    }
  }
}

class AdsJsBridge {
  constructor(context) {
    this.context = context;
  }

  invokeAsync(method, arg, callback) {
    hilog.info(HILOG_DOMAIN_CODE, 'advertising', `invokeAsync enter`);
    if (method === null || arg === null || callback === null || (typeof callback !== 'function')) {
      hilog.error(HILOG_DOMAIN_CODE, 'advertising', `invokeAsync parameter error`);
      return;
    }

    let options = {
      onConnect(elementName, remote) {
        try {
          // 拼接发送给服务端的数据
          let data = rpc.MessageSequence.create();
          data.writeRemoteObject(new AdsJsClientRpcObj('com.ohos.AdsJsClientRpcObj', callback));
          data.writeString(method);
          data.writeString(arg);
          const reply = rpc.MessageSequence.create();
          const option = new rpc.MessageOption();

          remote.sendMessageRequest(JS_BRIDGE_RPC_CODE, data, reply, option)
            .catch((e) => {
              hilog.error(HILOG_DOMAIN_CODE, 'advertising', `sendMessageRequest error, code:${e.code}, message:${e.message}`);
            })
            .finally(() => {
              data.reclaim();
              reply.reclaim();
            });
        } catch (e) {
          hilog.error(HILOG_DOMAIN_CODE, 'advertising', `onConnect error, code:${e.code}, message:${e.message}`);
        }
      },
      onDisconnect() {
      },
      onFailed() {
      }
    };

    try {
      const map = getConfigJsonData();
      const bundleName = map?.providerBundleName;
      const abilityName = map?.providerJSAbilityName;
      if (!bundleName || !abilityName) {
        hilog.error(HILOG_DOMAIN_CODE, 'advertising', `bundleName or abilityName is null`);
        callback(null);
        return;
      }
      const want = {
        bundleName: bundleName,
        abilityName: abilityName
      };

      this.context.connectServiceExtensionAbility(want, options);
    } catch (e) {
      callback(null);
      hilog.error(HILOG_DOMAIN_CODE, 'advertising', `invokeAsync error, code:${e.code}, message:${e.message}`);
    }
  }
}

function registerWebAdInterface(controller, context) {
  hilog.info(HILOG_DOMAIN_CODE, 'advertising', `registerWebAdInterface enter`);
  if (controller === null || context === null) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `parameter controller or context is null`);
    throw {
      code: 401,
      message: 'Invalid input parameter, controller or context is null.'
    };
  }
  try {
    const adsJsBridge = new AdsJsBridge(context);
    controller.registerJavaScriptProxy(adsJsBridge, '_OHAdsJsBridge', ['invokeAsync']);
    controller.refresh();
  } catch (e) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `registerWebAdInterface error, code:${e.code}, message:${e.message}`);
    throw {
      code: 21800001,
      message: 'System internal error.'
    };
  }
}

export default {
  // 注意：C/C++实现的NAPI模块中的接口如果需要对外暴露，都需要按这种形式来编写
  AdLoader: AdLoader,
  showAd: advertising.showAd,
  getAdRequestBody: advertising.getAdRequestBody,
  Advertisement: advertising.Advertisement,
  AdRequestParams: advertising.AdRequestParams,
  AdOptions: advertising.AdOptions,
  AdDisplayOptions: advertising.AdDisplayOptions,
  AdInteractionListener: advertising.AdInteractionListener,
  AdLoadListener: advertising.AdLoadListener,
  MultiSlotsAdLoadListener: advertising.MultiSlotsAdLoadListener,
  registerWebAdInterface: registerWebAdInterface,
};