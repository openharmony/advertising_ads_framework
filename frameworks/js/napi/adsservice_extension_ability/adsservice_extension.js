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

const hilog = globalThis.requireNapi('hilog');
const HILOG_DOMAIN_CODE = 65280;
let rpc = requireNapi('rpc');
let ExtensionAbility = requireNapi('app.ability.ExtensionAbility');
let BundleManager = requireNapi('bundle.bundleManager');

class AdsServiceExtensionAbility extends ExtensionAbility {
  onConnect(want) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdsServiceExtensionAbility', 'onConnect');
    return new AdsCoreServiceRpcObj('com.ohos.AdsCoreService', this.onLoadAd, this.onLoadAdWithMultiSlots);
  }

  onLoadAd(adParam, adOptions, respCallback) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdsServiceExtensionAbility', 'onLoadAd');
  }

  onLoadAdWithMultiSlots(adParams, adOptions, respCallback) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdsServiceExtensionAbility', 'onLoadAdWithMultiSlots');
  }

}

/**
 * AdsCoreService返回给调用方的rpc对象，用于调用方向AdsCoreService发送数据
 */
class AdsCoreServiceRpcObj extends rpc.RemoteObject {
  constructor(descriptor, onLoadAd, onLoadAdWithMultiSlots) {
    super(descriptor);
    this.onLoadAd = onLoadAd;
    this.onLoadAdWithMultiSlots = onLoadAdWithMultiSlots;
  }

  /**
   * sendMessageRequest请求的响应处理函数，在该函数里异步处理请求
   * 
   * @param code 对端发送的服务请求码。
   * @param data 携带客户端调用参数的对象，客户端调用参数顺序为RemoteObject、AdRequestParams、AdOptions。
   * @param reply 写入结果的MessageSequence对象。
   * @param options 指示操作是同步还是异步。
   */
  async onRemoteMessageRequest(code, data, reply, options) {
    hilog.info(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `onRemoteMessageRequest, the code is: ${code}`);

    try {
      const requestStartTime = Date.now();
      // 读取Uid
      const uid = this.getCallingUid();
      const packageName = await BundleManager.getBundleNameByUid(uid);
      // 读取rpc远程对象
      const replyRpcObj = data.readRemoteObject();
      // 1.读取广告请求数据
      // 约定的数据读取顺序，不可更改
      const reqData = {
        // 广告请求参数AdRequestParams
        reqParams: data.readString(),
        // 广告配置参数AdOptions
        adOptions: data.readString(),
      };
      const isMultiSlots = this.isMultiSlotsReq(code);
      // 2.数据类型转换
      // 参数校验
      if (!this.validate(isMultiSlots, reqData)) {
        hilog.info(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `the request params is invalid`);
        this.bizReqCallback(code, replyRpcObj)(RpcReqCallbackCode.CODE_INVALID_PARAMETERS, RpcReqCallbackMsg.INVALID_PARAMETERS);
		return true;
      }
      const adRequestParams = this.parseAdRequestParams(isMultiSlots, reqData, packageName, requestStartTime);
      const adOptions = this.parseAdOptions(reqData);
      // 3.请求广告业务处理
      try {
        if (isMultiSlots) {
          hilog.info(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', 'onLoadAdWithMultiSlots start');
          this.onLoadAdWithMultiSlots(adRequestParams, adOptions, this.bizAdsReqCallback(code, replyRpcObj));
        } else {
          hilog.info(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', 'onLoadAd start');
          this.onLoadAd(adRequestParams[0], adOptions, this.bizAdsReqCallback(code, replyRpcObj));
        }
      } catch (error) {
        hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `request ad failed, msg: ${error.message}`);
        this.bizReqCallback(code, replyRpcObj)(RpcReqCallbackCode.CODE_INTERNAL_ERROR, RpcReqCallbackMsg.INTERNAL_ERROR);
      }
      return true;
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `handle rpc request failed, msg: ${e.message}`);
      this.bizReqCallback(code, replyRpcObj)(RpcReqCallbackCode.CODE_INTERNAL_ERROR, RpcReqCallbackMsg.INTERNAL_ERROR);
      return false;
    }
  }

  private bizAdsReqCallback(code, replyRpcObj) {
    return (respData) => {
      let hasAds = false;
      let respCode = RpcReqCallbackCode.CODE_SUCCESS;
      let respMsg = RpcReqCallbackMsg.SUCCESS;
      respData.forEach((value, key, map) => {
        if (value.length > 0) {
          hasAds = true;
        }
      })
      let respAdsData = respData;
      if (!hasAds) {
        respCode = RpcReqCallbackCode.CODE_LOAD_ADS_FAILURE;
        respMsg = RpcReqCallbackMsg.LOAD_ADS_FAILURE;
      } else {
        const isMultiSlots = this.isMultiSlotsReq(code);
        if (!isMultiSlots) {
          respAdsData = respData.values().next().value;
        }
        respMsg = toJSON(respAdsData);
        hilog.debug(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `respMsg: ${respMsg}`);
      }
      this.sendMsgReq(code, replyRpcObj, respCode, respMsg);
    }
  }

  // 给NAPI响应数据
  private bizReqCallback(code, replyRpcObj) {
    return (respCode, respMsg) => {
      this.sendMsgReq(code, replyRpcObj, respCode, respMsg);
    }
  }

  private sendMsgReq(code, replyRpcObj, respCode, respMsg) {
    const respData = rpc.MessageSequence.create();
    /**
     * 业务响应码
     * CODE_SUCCESS = 200,
     * CODE_INVALID_PARAMETERS = 401,
     * CODE_INTERNAL_ERROR = 100001,
     * CODE_LOAD_ADS_FAILURE = 100003,
     */
    respData.writeInt(respCode);
    // 业务响应内容
    respData.writeString(respMsg);
    const reply = rpc.MessageSequence.create();
    replyRpcObj.sendMessageRequest(code, respData, reply, new rpc.MessageOption(1))
      .catch(e => {
        hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `send message from kit to caller failed, error msg: ${e.message}`);
      })
      .finally(() => {
        respData.reclaim();
        reply.reclaim();
      });
  }

  private validate(isMultiSlots, reqData) {
    if (isMultiSlots) {
      return this.validateMultiSlotsReq(reqData);
    }
    return this.validateSingleSlotsReq(reqData);
  }

  private validateMultiSlotsReq(reqData) {
    let adRequestParams;
    try {
      adRequestParams = JSON.parse(reqData.reqParams);
    } catch (error) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `adRequestParams is not a json string`);
      return false;
    }
    // array is not empty
    if (adRequestParams.length === 0) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `adRequestParams array is empty`);
      return false;
    }
    // adId is required
    if (adRequestParams.some((adReqParam) => { adReqParam.adId && adReqParam.adId.trim().length === 0 })) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `All ad ids are required`);
      return false;
    }
    // check adType, adWidth and adHeight
    return this.isValidAdType(adRequestParams[0].adType) &&
      this.isValidAdSize(adRequestParams[0].adWidth) && this.isValidAdSize(adRequestParams[0].adHeight);
  }

  private validateSingleSlotsReq(reqData) {
    let adRequestParams;
    try {
      adRequestParams = JSON.parse(reqData.reqParams);
    } catch (error) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `adRequestParams is not a json string`);
      return false;
    }
    // adId is required
    if (!adRequestParams.adId) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `adId is required`);
      return false;
    }
    // check adType, adWidth and adHeight
    return this.isValidAdType(adRequestParams.adType) &&
      this.isValidAdSize(adRequestParams.adWidth) && this.isValidAdSize(adRequestParams.adHeight);
  }

  private isValidAdType(adType) {
    const adTypeArray = [
      8, // 横幅广告
      1, // 开屏广告
      3, // 普通原生广告
      60, // 前贴片广告
      7, // 激励广告
      9, // 应用图标下载广告
      12, // 插屏广告
      18 // 大屏开屏
    ];
    if (!isUndefined(adType) && adType !== -1 && adTypeArray.indexOf(adType) === -1) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `adType ${adType} is invalid`);
      return false;
    }
    return true;
  }

  private isValidAdSize(adSize) {
    if (adSize == null) {
      return true;
    }
    if (isNumber(adSize) && adSize > 0) {
      return true;
    }
    return false;
  }

  private parseAdRequestParams(isMultiSlots, reqData, packageName, requestStartTime) {
    let adRequestParams;
    if (isMultiSlots) {
      adRequestParams = JSON.parse(reqData.reqParams);
    } else {
      adRequestParams = [JSON.parse(reqData.reqParams)];
    }
    adRequestParams.forEach((adRequestParam) => {
      adRequestParam.packageName = packageName;
      adRequestParam.requestStartTime = requestStartTime;
    });
    return adRequestParams;
  }

  private parseAdOptions(reqData) {
    let adOptions;
    try {
      adOptions = JSON.parse(reqData.adOptions);
      adOptions.tagForChildProtection = this.getTagForChildProtection(adOptions.tagForChildProtection);
      adOptions.adContentClassification = this.getAdContentClassification(adOptions.adContentClassification);
      adOptions.nonPersonalizedAd = this.getNonPersonalizedAd(adOptions.nonPersonalizedAd);
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `parse adOptions string exception`);
      adOptions = {
        tagForChildProtection: null,
        adContentClassification: null,
        nonPersonalizedAd: null,
      };
    }

    return adOptions;
  }

  private isMultiSlotsReq(code) {
    return code === RpcReqCode.CODE_REQ_MULTI_SLOTS_ADS;
  }

  private getTagForChildProtection(tagForChildProtection) {
    if (tagForChildProtection !== TagForChild.TAG_FOR_CHILD_PROTECTION_UNSPECIFIED &&
      tagForChildProtection !== TagForChild.TAG_FOR_CHILD_PROTECTION_FALSE &&
      tagForChildProtection !== TagForChild.TAG_FOR_CHILD_PROTECTION_TRUE) {
      hilog.warn(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `invalid tagForChildProtection: ${tagForChildProtection}`);
      return null;
    }

    return tagForChildProtection;
  }

  private getAdContentClassification(adContentClassification) {
    if (adContentClassification !== ContentClassification.AD_CONTENT_CLASSIFICATION_W &&
      adContentClassification !== ContentClassification.AD_CONTENT_CLASSIFICATION_PI &&
      adContentClassification !== ContentClassification.AD_CONTENT_CLASSIFICATION_J &&
      adContentClassification !== ContentClassification.AD_CONTENT_CLASSIFICATION_A) {
      hilog.warn(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `invalid adContentClassification: ${adContentClassification}`);
      return null;
    }

    return adContentClassification;
  }

  private getNonPersonalizedAd(nonPersonalizedAd) {
    if (nonPersonalizedAd !== NonPersonalizedAd.ALLOW_NON_PERSONALIZED &&
      nonPersonalizedAd !== NonPersonalizedAd.ALLOW_ALL) {
      hilog.warn(HILOG_DOMAIN_CODE, 'AdsCoreServiceRpcObj', `invalid nonPersonalizedAd: ${nonPersonalizedAd}`);
      return null;
    }

    return nonPersonalizedAd;
  }
}

const COMMA = ',';

/**
 * 业务响应码
 */
const RpcReqCallbackCode = {
  CODE_SUCCESS: 200,
  CODE_INVALID_PARAMETERS: 401,
  CODE_INTERNAL_ERROR: 100001,
  CODE_INIT_CONFIG_FAILURE: 100002,
  CODE_LOAD_ADS_FAILURE: 100003,
}

const RpcReqCallbackMsg = {
  SUCCESS: '',
  INVALID_PARAMETERS: 'Invalid adRequestParams',
  INTERNAL_ERROR: 'biz service internal error',
  LOAD_ADS_FAILURE: 'Load ads failure',
}

/**
 * RPC请求的code值枚举
 */
const RpcReqCode = {
  /**
   * 请求单个广告位
   */
  CODE_REQ_ADS: 1,
  /**
   * 请求多个广告位
   */
  CODE_REQ_MULTI_SLOTS_ADS: 2,
}

/**
 * 面向儿童的设置
 */
const TagForChild = {
  /**
   * 不确定是否根据 COPPA 的规定来处理广告请求
   */
  TAG_FOR_CHILD_PROTECTION_UNSPECIFIED: -1,

  /**
   * 不根据 COPPA 的规定来处理广告请求
   */
  TAG_FOR_CHILD_PROTECTION_FALSE: 0,

  /**
   * 根据 COPPA 的规定来处理广告请求
   */
  TAG_FOR_CHILD_PROTECTION_TRUE: 1,
}

/**
 * 广告类型：非个性化广告、个性化广告和非个性化广告
 *
 */
const NonPersonalizedAd = {
  /**
   * 1：只请求非个性化广告
   */
  ALLOW_NON_PERSONALIZED: 1,

  /**
   * 0：请求个性化广告和非个性化广告（默认）
   */
  ALLOW_ALL: 0,
}

/**
 * 广告内容分级
 *
 */
const ContentClassification = {
  /**
   * 适合幼儿及以上年龄段观众的内容
   */
  AD_CONTENT_CLASSIFICATION_W: 'W',

  /**
   * 适合少儿及以上年龄段观众的内容
   */
  AD_CONTENT_CLASSIFICATION_PI: 'PI',

  /**
   * 适合青少年及以上年龄段观众的内容
   */
  AD_CONTENT_CLASSIFICATION_J: 'J',

  /**
   * 仅适合成年观众的内容
   */
  AD_CONTENT_CLASSIFICATION_A: 'A',
}

function toJSON(data) {
  if (isUndefined(data)) {
    return '';
  }
  if (isString(data)) {
    const value = data.toString();
    return JSON.stringify(value);
  }

  if (isPlain(data)) {
    return JSON.stringify(data);
  }

  if (isPrimitiveOrBoxed(data)) {
    return data + '';
  }

  if (data instanceof Array) {
    return arrayToJson(data);
  }

  if (data instanceof Map) {
    return mapToJson(data);
  }
  if (typeof data === 'object') {
    return objectToJson(data);
  }
}

function mapToJson(data) {
  if (data.size <= 0) {
    return "{}";
  }
  let jsonStr = '';
  jsonStr += '{';
  data.forEach((value, key) => {
    let jsonValue = toJSON(value);
    if (jsonValue) {
      jsonStr += "\""
      jsonStr += key
      jsonStr += "\":"
      jsonStr += jsonValue
      jsonStr += COMMA;
    }
  });
  jsonStr = formatJsonStr(jsonStr);
  jsonStr += '}';
  return jsonStr;
}

function objectToJson(data) {
  const fields = Object.keys(data);
  if (!fields || fields.length == 0) {
    return '';
  }
  let jsonStr = '';
  jsonStr += '{';
  const length = fields.length;
  for (let i = 0; i < length; i++) {
    const field = fields[i];
    const value = data[field];
    if (isUndefined(value) || isFunction(value)) {
      continue;
    }
    let jsonValue = toJSON(value);

    if (jsonValue) {
      jsonStr += "\"";
      jsonStr += field;
      jsonStr += "\":";
      jsonStr += jsonValue;
      if (i < length - 1) {
        jsonStr += COMMA;
      }
    }
  }
  jsonStr = formatJsonStr(jsonStr);
  jsonStr += '}';
  return jsonStr;
}

function arrayToJson(data) {
  if (data.length <= 0) {
    return "[]";
  }
  let jsonStr = '';
  jsonStr += '[';

  let jsonValue;
  let length = data.length;
  for (let i = 0; i < length; i++) {
    if (isUndefined(data[i])) {
      continue;
    }
    jsonValue = toJSON(data[i]);
    if (isUndefined(jsonValue)) {
      continue;
    }
    jsonStr += jsonValue;
    jsonStr += COMMA;

  }
  jsonStr = formatJsonStr(jsonStr);
  jsonStr += ']';
  return jsonStr;
}

function formatJsonStr(jsonStr) {
  const length = jsonStr.length;
  if (length > 0 && jsonStr.charAt(length - 1) == COMMA) {
    // 删除最后一个“,”
    jsonStr = jsonStr.slice(0, length - 1);
  }
  return jsonStr;
}

function isFunction(v) {
  if (typeof v === "function") {
    return true;
  } else {
    return false;
  }
}

function isUndefined(obj) {
  return obj === null || obj === undefined;
}

function isPlain(obj) {
  if (typeof obj !== "object" || obj === null) return false;

  let proto = obj;
  while (Object.getPrototypeOf(proto) !== null) {
    proto = Object.getPrototypeOf(proto);
  }

  return Object.getPrototypeOf(obj) === proto;
}

function isString(obj) {
  return typeof obj === "string";
}

function isNumber(obj) {
  return Number.isInteger(obj);
}

function isPrimitiveOrBoxed(obj) {
  if (typeof obj !== "object" && !isUndefined(obj)) {
    return true;
  }
  const toString = Object.prototype.toString;
  const primitives = [
    "string",
    "number",
    "bigint",
    "boolean",
    "null",
    "undefined",
    "symbol",
  ]
  let ret = toString.call(obj).slice(8, -1).toLowerCase();
  return primitives.indexOf(ret) > -1;
}

export default AdsServiceExtensionAbility;