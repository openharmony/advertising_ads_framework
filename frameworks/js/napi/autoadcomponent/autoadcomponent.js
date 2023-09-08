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

const HILOG_DOMAIN_CODE = 65280;
const READ_FILE_BUFFER_SIZE = 4096;

class AutoAdComponent extends ViewPU {
  constructor(e, t, i, o = -1) {
    super(e, i, o);
    this.context = getContext(this);
    this.want = void 0;
    this.__adChangeStatus = new ObservedPropertySimplePU(0, this, 'adChangeStatus');
    this.adsCount = void 0;
    this.intervalId = void 0;
    this.refreshTime = void 0;
    this.loader = void 0;
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
    void 0 !== e.refreshTime && (this.refreshTime = e.refreshTime);
    void 0 !== e.loader && (this.loader = e.loader);
    void 0 !== e.adParam && (this.adParam = e.adParam);
    void 0 !== e.adOptions && (this.adOptions = e.adOptions);
    void 0 !== e.displayOptions && (this.displayOptions = e.displayOptions);
    void 0 !== e.interactionListener && (this.interactionListener = e.interactionListener);
  }

  updateStateVars(e) {}

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
      fs.readSync(t.fd, i);
      fs.closeSync(t);
      let o = String.fromCharCode(...new Uint8Array(i));
      o = o.replace(/[\r\n\t\"]/g, '').replace(/\s*/g, '').replace(/\[|\]/g, '');
      e = this.toMap(o);
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'file succeed');
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AutoAdComponent', `open file failed with error:${e.code}, message:${e.message}`);
    }
    e || hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'get config json failed');
    return e;
  }

  setWant(e) {
    let t = this.getConfigJsonData();
    this.want = {
      bundleName: null == t ? void 0 : t.providerBundleName,
      abilityName: null == t ? void 0 : t.providerUEAAbilityName,
      parameters: {
        ads: e,
        displayOptions: this.displayOptions
      }
    };
  }

  loadAd() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'start load advertising.');
    let e = {
      onAdLoadFailure: (e, t) => {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `request ad errorCode is: ${e}, errorMsg is: ${t}`);
      },
      onAdLoadSuccess: e => {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'request ad success!');
        this.setWant(e);
        this.adsCount = e.length;
        if (this.adsCount > 0) {
          hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `advertising size is: ${this.adsCount}!`);
          this.adChangeStatus++;
        } else {
          hilog.warn(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'advertising size is 0!');
        }
      }
    };
    this.loader.loadAd(this.adParam, this.adOptions, e);
  }

  autoRefresh() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'start auto refresh advertising.');
    this.adsCount = 0;
    this.intervalId = setInterval((() => {
      this.adsCount--;
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `Auto refresh, adsCount is : ${this.adsCount},
        refreshTime is: ${this.refreshTime} ms.`);
      this.adsCount <= 0 && this.loadAd();
    }), this.refreshTime);
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `intervalId is: ${this.intervalId}.`);
  }

  initRefreshTime() {
    if (!(this.displayOptions && this.displayOptions.refreshTime &&
        'number' === typeof this.displayOptions.refreshTime && this.displayOptions.refreshTime > 0)) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent',
        `Invalid input refreshTime, refreshTime is： ${this.refreshTime}.`);
      return !1;
    }
    this.displayOptions.refreshTime < MIN_REFRESH_TIME ? this.refreshTime = MIN_REFRESH_TIME :
      this.displayOptions.refreshTime > MAX_REFRESH_TIME ? this.refreshTime = MAX_REFRESH_TIME :
      this.refreshTime = this.displayOptions.refreshTime;
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `refreshTime is： ${this.refreshTime} ms.`);
    return !0;
  }

  aboutToAppear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'aboutToAppear.');
    this.loader = new advertising.AdLoader(this.context);
    let e = this.initRefreshTime();
    this.loadAd();
    if (e) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'Auto refresh advertising.');
      this.autoRefresh();
    } else {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'Load advertising once.');
    }
  }

  aboutToDisappear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'aboutToDisappear.');
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `intervalId is: ${this.intervalId}.`);
    if (null !== this.intervalId) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'stop cycle display advertising.');
      clearInterval(this.intervalId);
    }
  }

  initialRender() {
    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      Row.create();
      Row.height('100%');
      t || Row.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      Column.create();
      Column.width('100%');
      t || Column.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      If.create();
      this.adChangeStatus ? this.ifElseBranchUpdateFunction(0, (() => {
        this.observeComponentCreation(((e, t) => {
          ViewStackProcessor.StartGetAccessRecordingFor(e);
          UIExtensionComponent.create(this.want);
          UIExtensionComponent.width('100%');
          UIExtensionComponent.height('100%');
          UIExtensionComponent.onReceive((e => {
            hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `${JSON.stringify(e)}`);
            this.interactionListener.onStatusChanged(e.status, e.ad, e.data);
          }));
          t || UIExtensionComponent.pop();
          ViewStackProcessor.StopGetAccessRecording();
        }));
      })) : If.branchId(1);
      t || If.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    If.pop();
    Column.pop();
    Row.pop();
  }

  rerender() {
    this.updateDirtyElements();
  }
}

export default { AutoAdComponent };