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
const abilityAccessCtrl = globalThis.requireNapi('abilityAccessCtrl');
const identifier = globalThis.requireNapi('identifier.oaid');

const HILOG_DOMAIN_CODE = 65280;
const READ_FILE_BUFFER_SIZE = 4096;
const RPC_STRING_LENGTH = 32768;
const PARSE_RESP_LENGTH_LIMIT = 52428800;
const JS_BRIDGE_RPC_CODE = 1;
const PARSE_RESP_RPC_CODE = 2;
const CODE_SUCCESS = 200;
const CODE_DEVICE_NOT_SUPPORT = 801;
const ILLEGAL_ARGUMENT_INPUT = -111111;
const ADS_SERVICE_CONFIG_EXT_FILE = 'etc/advertising/ads_framework/ad_service_config_ext.json';
const ADS_SERVICE_CONFIG_FILE = 'etc/advertising/ads_framework/ad_service_config.json';
const ADS_JS_BRIDGE = '_OHAdsJsBridge';

const AdsError = {
  ERR_SEND_OK: 0,
  PARAM_ERR: 401,
  INNER_ERR: 21800001,
  REQUEST_FAIL: 21800003,
  PARSE_RESPONSE_ERROR: 21800005
};

class AdLoader {
  constructor(context) {
    this.context = context;
    this.loader = new advertising.AdLoader(context);
  }

  loadAdWithMultiSlots(adParams, adOptions, listener) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `loadAdWithMultiSlots begin`);
    this.checkParamAndLoad(true, adParams, adOptions, listener);
  }

  loadAd(adParams, adOptions, listener) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `loadAd begin`);
    this.checkParamAndLoad(false, adParams, adOptions, listener);
  }

  checkParamAndLoad(isMultiSlots, adParams, adOptions, listener) {
    processParamsNull(adParams, adOptions, listener, isMultiSlots ? 'loadAdWithMultiSlots' : 'loadAd');
    hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `start to load ad`);
    // 校验传入值
    if (!Number.isInteger(adOptions.nonPersonalizedAd)) {
      adOptions.nonPersonalizedAd = ILLEGAL_ARGUMENT_INPUT;
      hilog.warn(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `value of nonPer is empty or invalid`);
    }
    getOaid(this.context).then(
      (oaid) => {
        adParams.oaid = oaid;
        hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `getOaid success load ad begin`);
        this.loaderLoad(isMultiSlots, adParams, adOptions, listener);
      },
      (fail) => {
        adParams.oaid = '';
        hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `getOaid failed code:${fail.code}, msg:${fail.msg}`);
        this.loaderLoad(isMultiSlots, adParams, adOptions, listener);
      });
  }

  loaderLoad(isMultiSlots, adParams, adOptions, listener) {
    isMultiSlots ?
      this.loader.loadAdWithMultiSlots(adParams, adOptions, {
        onAdLoadFailure: listener?.onAdLoadFailure,
        onAdLoadSuccess: onAdLoadSuccessProxy(listener),
      }) :
      this.loader.loadAd(adParams, adOptions, {
        onAdLoadFailure: listener?.onAdLoadFailure,
        onAdLoadSuccess: onSingleSlotAdLoadSuccessProxy(listener),
      });
  }
}

async function getOaid(context) {
  const atManager = abilityAccessCtrl.createAtManager();
  return new Promise(async (resolve, reject) => {
    try {
      const data = await atManager.requestPermissionsFromUser(context, ['ohos.permission.APP_TRACKING_CONSENT']);
      data.authResults[0] === 0 ?
        identifier.getOAID((err, oaid) => {
          hilog.info(HILOG_DOMAIN_CODE, 'AdLoaderProxy', 'request permission success and start getOAID');
          err.code ? reject({ code: err.code, msg: err.message }) : resolve(oaid);
        }) :
        reject({
          msg: `user rejected`
        });
    } catch (err) {
      reject({
        code: err.code,
        msg: err.message
      });
    }
  });
}

function processParamsNull(adParams, adOptions, listener, methodName) {
  if (!adParams) {
    hilog.error(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `${methodName} param error. code 401 , message adParam is null`);
    throw {
      code: AdsError.PARAM_ERR,
      message: 'Invalid input parameter. AdParams is null.'
    };
  }
  if (!adOptions) {
    hilog.error(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `${methodName} param error. code 401 , message adOptions is null`);
    throw {
      code: AdsError.PARAM_ERR,
      message: 'Invalid input parameter. AdOptions is null.'
    };
  }
  if (!listener) {
    hilog.error(HILOG_DOMAIN_CODE, 'AdLoaderProxy', `${methodName} LoadAd param error. code 401 , message listener is null`);
    throw {
      code: AdsError.PARAM_ERR,
      message: 'Invalid input parameter. Listener is null.'
    };
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
      callBackListener?.onAdLoadFailure(AdsError.REQUEST_FAIL, error.message);
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
      callBackListener?.onAdLoadFailure(AdsError.REQUEST_FAIL, error.message);
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
      const buffer = data.readStringArray();
      const readstr = buffer.join('');
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

    if (arg.length > PARSE_RESP_LENGTH_LIMIT) {
      hilog.error(HILOG_DOMAIN_CODE, 'advertising', `invokeAsync parameter arg is too long`);
      return;
    }

    try {
      const map = getConfigJsonData();
      const bundleName = map?.providerBundleName;
      const abilityName = map?.providerJSAbilityName;
      if (!bundleName || !abilityName) {
        hilog.error(HILOG_DOMAIN_CODE, 'advertising', `bundleName or abilityName is null`);
        callback(null);
        return;
      }

      const options = createJsbOptions(method, arg, callback);
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

function createJsbOptions(method, arg, callback) {
  return {
    onConnect(elementName, remote) {
      sendJsbRpcMessage(method, arg, callback, remote);
    },
    onDisconnect() {
    },
    onFailed() {
    }
  };
}

function sendJsbRpcMessage(method, arg, callback, remote) {
  try {
    // 拼接发送给服务端的数据
    let data = rpc.MessageSequence.create();
    hilog.info(HILOG_DOMAIN_CODE, 'advertising', `remote descriptor:${remote.getDescriptor()}`);
    data.writeInterfaceToken(remote.getDescriptor());
    data.writeRemoteObject(new AdsJsClientRpcObj('com.ohos.AdsJsClientRpcObj', callback));
    data.writeString(method);
    data.writeStringArray(splitString((arg), RPC_STRING_LENGTH));
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
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `sendJsbRpcMessage error, code:${e.code}, message:${e.message}`);
  }
}

function registerWebAdInterface(controller, context, needRefresh) {
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
    controller.registerJavaScriptProxy(adsJsBridge, ADS_JS_BRIDGE, ['invokeAsync']);
    if (isUndefined(needRefresh) || needRefresh) {
      hilog.info(HILOG_DOMAIN_CODE, 'advertising', `registerWebAdInterface refresh`);
      controller.refresh();
    }
  } catch (e) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `registerWebAdInterface error, code:${e.code}, message:${e.message}`);
    throw {
      code: 21800001,
      message: 'operation javascriptRegister error'
    };
  }
}

function deleteWebAdInterface(controller, needRefresh) {
  hilog.info(HILOG_DOMAIN_CODE, 'advertising', `deleteWebAdInterface enter`);
  if (controller === null) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `parameter controlleris null`);
    throw {
      code: 401,
      message: 'Invalid input parameter, controller is null.'
    };
  }
  try {
    controller.deleteJavaScriptRegister(ADS_JS_BRIDGE);
    if (isUndefined(needRefresh) || needRefresh) {
      hilog.info(HILOG_DOMAIN_CODE, 'advertising', `deleteWebAdInterface refresh`);
      controller.refresh();
    }
  } catch (e) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `deleteWebAdInterface error, code:${e.code}, message:${e.message}`);
    throw {
      code: 21800001,
      message: 'operation javascriptRegister error'
    };
  }
}

function isUndefined(obj) {
  return obj === undefined || obj === null;
}

class ParseAdResponseRpcObj extends rpc.RemoteObject {
  constructor(descriptor, listener) {
    super(descriptor);
    this.listener = listener;
  }

  onRemoteMessageRequest(code, data, reply, options) {
    hilog.info(HILOG_DOMAIN_CODE, 'advertising', `onRemoteMessageRequest enter`);
    if (code !== PARSE_RESP_RPC_CODE) {
      hilog.error(HILOG_DOMAIN_CODE, 'advertising', `onRemoteMessageRequest code error`);
      return false;
    }
    try {
      const respCode = data.readInt();
      const respData = data.readString();
      if (respCode === CODE_SUCCESS) {
        this.listener?.onAdLoadSuccess(new Map(Object.entries(JSON.parse(respData))));
      } else if (respCode === CODE_DEVICE_NOT_SUPPORT) {
        this.listener?.onAdLoadFailure(respCode, 'Device not supported');
      } else if (respCode === AdsError.PARSE_RESPONSE_ERROR) {
        this.listener?.onAdLoadFailure(respCode, 'Failed to parse the ad response.');
      } else {
        this.listener?.onAdLoadFailure(respCode, respData);
      }
      return true;
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'advertising', `handle rpc error, code:${e.code}, message:${e.message}`);
      return false;
    }
  }
}

async function getAdRequestBody(adParams, adOptions) {
  if (!adParams || !adOptions) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `Parameter verification failed, code: ${AdsError.PARAM_ERR}`);
    throw {
      code: AdsError.PARAM_ERR,
      message: 'Mandatory parameters are left unspecified.'
    };
  }
  if (!Array.isArray(adParams) || (typeof adOptions !== 'object')) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `Parameter verification failed, code: ${AdsError.PARAM_ERR}`);
    throw {
        code: AdsError.PARAM_ERR,
        message: 'Incorrect parameter types.'
    };
  }
  // 校验字段
  if (!Number.isInteger(adOptions.nonPersonalizedAd)) {
    adOptions.nonPersonalizedAd = ILLEGAL_ARGUMENT_INPUT;
    hilog.warn(HILOG_DOMAIN_CODE, 'AdLoaderProxy', 'value of nonPer is empty or invalids of RequestBody');
  }
  return advertising.getAdRequestBody(adParams, adOptions);
}

function parseAdResponse(adResponse, listener, context) {
  hilog.info(HILOG_DOMAIN_CODE, 'advertising', `parseAdResponse enter`);
  validateParams(adResponse, listener, context);

  try {
    const options = createConnectionOptions(listener, adResponse);
    const { bundleName, abilityName } = getConfigData();
    const want = { bundleName, abilityName };

    context.connectServiceExtensionAbility(want, options);
  } catch (e) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `parseAdResponse error, code:${e.code}, message:${e.message}`);
    throw {
      code: AdsError.INNER_ERR,
      message: 'System internal error.'
    };
  }
}

function validateParams(adResponse, listener, context) {
  if (adResponse === null || listener === null || context === null) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `The parameters cannot be empty, error code 401.`);
    throw {
      code: AdsError.PARAM_ERR,
      message: 'Invalid input parameter. The parameters cannot be empty.'
    };
  }
  if (adResponse.length > PARSE_RESP_LENGTH_LIMIT) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `The parameter adResponse is too long.`);
    throw {
      code: AdsError.PARAM_ERR,
      message: 'Invalid input parameter. The parameter adResponse is too long.'
    };
  }
}

function createConnectionOptions(listener, adResponse) {
  return {
    onConnect(elementName, remote) {
      sendRPCMessage(listener, adResponse, remote);
    },
    onDisconnect() {
    },
    onFailed() {
      throw {
        code: AdsError.PARAM_ERR,
        message: 'RPC connection failed.'
      };
    }
  };
}

function sendRPCMessage(listener, adResponse, remote) {
  try {
    let data = rpc.MessageSequence.create();
    hilog.info(HILOG_DOMAIN_CODE, 'advertising', `remote descriptor:${remote.getDescriptor()}`);
    data.writeInterfaceToken(remote.getDescriptor());
    data.writeRemoteObject(new ParseAdResponseRpcObj('com.ohos.ParseAdResponseRpcObj', listener));
    data.writeStringArray(splitString(adResponse, RPC_STRING_LENGTH));

    const reply = rpc.MessageSequence.create();
    const option = new rpc.MessageOption();

    remote.sendMessageRequest(PARSE_RESP_RPC_CODE, data, reply, option)
      .catch((e) => {
        hilog.error(HILOG_DOMAIN_CODE, 'advertising', `sendMessageRequest error, code:${e.code}, message:${e.message}`);
        throw {
          code: AdsError.INNER_ERR,
          message: 'An error occurred during RPC send message request.'
        };
      })
      .finally(() => {
        data.reclaim();
        reply.reclaim();
      });
  } catch (e) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `onConnect error, code:${e.code}, message:${e.message}`);
    throw {
      code: AdsError.INNER_ERR,
      message: 'An error occurred during RPC connection interaction.'
    };
  }
}

function getConfigData() {
  const map = getConfigJsonData();
  const bundleName = map?.providerBundleName;
  const abilityName = map?.providerApiAbilityName;
  if (!bundleName || !abilityName) {
    hilog.error(HILOG_DOMAIN_CODE, 'advertising', `bundleName or abilityName is null`);
    throw {
      code: AdsError.INNER_ERR,
      message: 'System internal error.'
    };
  }
  return { bundleName, abilityName };
}

function splitString(str, length) {
  let result = [];
  for (let i = 0; i < str.length; i += length) {
    result.push(str.slice(i, i + length));
  }
  return result;
}

export default {
  // 注意：C/C++实现的NAPI模块中的接口如果需要对外暴露，都需要按这种形式来编写
  AdLoader: AdLoader,
  showAd: advertising.showAd,
  getAdRequestBody: getAdRequestBody,
  parseAdResponse: parseAdResponse,
  Advertisement: advertising.Advertisement,
  AdRequestParams: advertising.AdRequestParams,
  AdOptions: advertising.AdOptions,
  AdDisplayOptions: advertising.AdDisplayOptions,
  AdInteractionListener: advertising.AdInteractionListener,
  AdLoadListener: advertising.AdLoadListener,
  MultiSlotsAdLoadListener: advertising.MultiSlotsAdLoadListener,
  registerWebAdInterface: registerWebAdInterface,
  deleteWebAdInterface: deleteWebAdInterface,
};