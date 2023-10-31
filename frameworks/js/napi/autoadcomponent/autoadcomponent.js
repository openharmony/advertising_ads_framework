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

let advertising = globalThis.requireNapi('advertising');
let fs = globalThis.requireNapi('file.fs');
let hilog = globalThis.requireNapi('hilog');

const MAX_REFRESH_TIME = 12e4;
const MIN_REFRESH_TIME = 3e4;
const AD_SIZE_OFFSET = 1;
const AD_LOAD_ONCE_SIZE = 1;
const HILOG_DOMAIN_CODE = 65280;
const READ_FILE_BUFFER_SIZE = 4096;

class AutoAdComponent extends ViewPU {
  constructor(e, t, i, o = -1) {
    super(e, i, o);
    this.context = getContext(this);
    this.want = void 0;
    this.__adChangeStatus = new ObservedPropertySimplePU(0, this, 'adChangeStatus');
    this.adsCount = 0;
    this.intervalId = void 0;
    this.freshInterval = void 0;
    this.refreshTime = void 0;
    this.loader = void 0;
    this.isAutoRefresh = void 0;
    this.remoteProxy = void 0;
    this.alreadyLoadAd = !1;
    this.adParam = void 0;
    this.adOptions = void 0;
    this.displayOptions = void 0;
    this.interactionListener = void 0;
    this.setInitiallyProvidedValue(t);
  }

  setInitiallyProvidedValue(e) {
    void 0 !== e.context && (this.context = e.context);
    void 0 !== e.want && (this.want = e.want);
    void 0 !== e.adChangeStatus && (this.adChangeStatus = e.adChangeStatus);
    void 0 !== e.adsCount && (this.adsCount = e.adsCount);
    void 0 !== e.intervalId && (this.intervalId = e.intervalId);
    void 0 !== e.freshInterval && (this.freshInterval = e.freshInterval);
    void 0 !== e.refreshTime && (this.refreshTime = e.refreshTime);
    void 0 !== e.loader && (this.loader = e.loader);
    void 0 !== e.isAutoRefresh && (this.isAutoRefresh = e.isAutoRefresh);
    void 0 !== e.remoteProxy && (this.remoteProxy = e.remoteProxy);
    void 0 !== e.alreadyLoadAd && (this.alreadyLoadAd = e.alreadyLoadAd);
    void 0 !== e.adParam && (this.adParam = e.adParam);
    void 0 !== e.adOptions && (this.adOptions = e.adOptions);
    void 0 !== e.displayOptions && (this.displayOptions = e.displayOptions);
    void 0 !== e.interactionListener && (this.interactionListener = e.interactionListener);
  }

  updateStateVars(e) {
  }

  purgeVariableDependenciesOnElmtId(e) {
    this.__adChangeStatus.purgeDependencyOnElmtId(e);
  }

  aboutToBeDeleted() {
    this.__adChangeStatus.aboutToBeDeleted();
    SubscriberManager.Get().delete(this.id__());
    this.aboutToBeDeletedInternal();
  }

  get adChangeStatus() {
    return this.__adChangeStatus.get();
  }

  set adChangeStatus(e) {
    this.__adChangeStatus.set(e);
  }

  toMap(e) {
    const t = (e = e.replace(/[{}]/g, '')).split(',');
    const i = {};
    for (let e = 0; e < t.length; e++) {
      const o = t[e];
      const s = o.indexOf(':');
      if (s > -1) {
        const e = o.substring(0, s);
        const t = o.substring(s + 1);
        i[e] = t;
      }
    }
    return i;
  }

  getConfigJsonData() {
    let e = null;
    try {
      const t = fs.openSync('/system/etc/cloud/advertising/ad_service_config.json');
      const i = new ArrayBuffer(READ_FILE_BUFFER_SIZE);
      let o = fs.readSync(t.fd, i);
      fs.closeSync(t);
      let s = String.fromCharCode(...new Uint8Array(i.slice(0, o)));
      s = s.replace(/[\r\n\t\"]/g, '').replace(/\s*/g, '').replace(/\[|\]/g, '');
      e = this.toMap(s);
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'file succeed');
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AutoAdComponent', `open file failed with error:${e.code}, message:${e.message}`);
    }
    e || hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'get config json failed');
    return e;
  }

  setWant() {
    let e = this.getConfigJsonData();
    this.want = {
      bundleName: null == e ? void 0 : e.providerBundleName,
      abilityName: null == e ? void 0 : e.providerUEAAbilityName,
      parameters: { displayOptions: this.displayOptions, 'ability.want.params.uiExtensionType': 'ads' }
    }
  }

  pullUpPage(e, t) {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `send ${e} advertisings to UI Extension Ability.`);
    let i = 0;
    this.remoteProxy.send({ adArray: t[i] });
    e > AD_SIZE_OFFSET && (this.freshInterval = setInterval((() => {
      i++;
      this.remoteProxy.send({ adArray: t[i] });
      if (i == e - AD_SIZE_OFFSET) {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `clearInterval ${this.freshInterval}.`);
        clearInterval(this.freshInterval);
      }
    }), this.refreshTime));
  }

  loadAd(e) {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'start load advertising.');
    let t = { onAdLoadFailure: (e, t) => {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `request ad errorCode is: ${e}, errorMsg is: ${t}`);
    }, onAdLoadSuccess: t => {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'request ad success!');
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `adArray is : ${JSON.stringify(t)}`);
      this.adsCount = t.length;
      if (this.adsCount > 0 && e) {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `advertising size is: ${this.adsCount}!`);
        this.pullUpPage(this.adsCount, t)
      } else if (this.adsCount > 0 && !e) {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'is not auto refresh!');
        this.pullUpPage(AD_LOAD_ONCE_SIZE, t)
      } else hilog.warn(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'advertising size is 0!');
    } };
    this.loader.loadAd(this.adParam, this.adOptions, t);
  }

  autoRefresh() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'start auto refresh advertising.');
    this.intervalId = setInterval((() => {
      this.adsCount--;
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `Auto refresh, adsCount is : ${this.adsCount}, refreshTime is:
        ${this.refreshTime} ms.`);
      this.adsCount <= 0 && this.loadAd(!0);
    }), this.refreshTime);
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `intervalId is: ${this.intervalId}.`);
  }

  initRefreshTime() {
    if (!(this.displayOptions && this.displayOptions.refreshTime && typeof this.displayOptions.refreshTime == 'number'
      && this.displayOptions.refreshTime > 0)) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `Invalid input refreshTime, refreshTime is:
        ${this.refreshTime}.`);
      return !1;
    }
    this.displayOptions.refreshTime < MIN_REFRESH_TIME ? this.refreshTime = MIN_REFRESH_TIME : 
      this.displayOptions.refreshTime > MAX_REFRESH_TIME ? this.refreshTime = MAX_REFRESH_TIME : 
      this.refreshTime = this.displayOptions.refreshTime;
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `refreshTime is: ${this.refreshTime} ms.`);
    return !0;
  }

  aboutToAppear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'aboutToAppear.');
    this.setWant();
    this.loader = new advertising.AdLoader(this.context);
    this.isAutoRefresh = this.initRefreshTime();
  }

  aboutToDisappear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'aboutToDisappear.');
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `intervalId is: ${this.intervalId}.`);
    if (null !== this.intervalId) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'stop cycle display advertising.');
      clearInterval(this.intervalId);
    }
    null !== this.freshInterval && clearInterval(this.freshInterval);
  }

  initialRender() {
    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      Row.create();
      Row.height("100%");
      t || Row.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      Column.create();
      Column.width("100%");
      t || Column.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      UIExtensionComponent.create(this.want);
      UIExtensionComponent.width("100%");
      UIExtensionComponent.height("100%");
      UIExtensionComponent.onRemoteReady((e => {
        this.remoteProxy = e;
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'remote proxy ready.');
        if (!this.alreadyLoadAd) {
          this.alreadyLoadAd = !0;
          if (this.isAutoRefresh) {
            this.loadAd(!0);
            hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'Auto refresh advertising.');
            this.autoRefresh();
          } else {
            this.loadAd(!1);
            hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'Load advertising once.');
          }
        }
      }));
      UIExtensionComponent.onReceive((e => {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `${JSON.stringify(e)}`);
        this.interactionListener.onStatusChanged(e.status, e.ad, e.data);
      }));
      t || UIExtensionComponent.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    Column.pop();
    Row.pop();
  }

  rerender() {
    this.updateDirtyElements();
  }
}

export default { AutoAdComponent };
