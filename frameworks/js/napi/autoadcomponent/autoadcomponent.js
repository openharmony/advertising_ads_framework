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

let configPolicy = globalThis.requireNapi('configPolicy');

const MAX_REFRESH_TIME = 12e4;

const MIN_REFRESH_TIME = 3e4;

const HILOG_DOMAIN_CODE = 65280;
const READ_FILE_BUFFER_SIZE = 4096;

class AutoAdComponent extends ViewPU {
  constructor(t, o, i, e = -1) {
    super(t, i, e);
    this.context = getContext(this);
    this.want = void 0;
    this.__showComponent = new ObservedPropertySimplePU(!1, this, 'showComponent');
    this.timeoutId = void 0;
    this.refreshTime = void 0;
    this.loader = void 0;
    this.isAutoRefresh = void 0;
    this.remoteProxy = void 0;
    this.adParam = void 0;
    this.adOptions = void 0;
    this.displayOptions = void 0;
    this.interactionListener = void 0;
    this.ads = void 0;
    this.isFirstLoad = !0;
    this.isTaskRunning = !1;
    this.setInitiallyProvidedValue(o);
  }
  setInitiallyProvidedValue(t) {
    void 0 !== t.context && (this.context = t.context);
    void 0 !== t.want && (this.want = t.want);
    void 0 !== t.showComponent && (this.showComponent = t.showComponent);
    void 0 !== t.timeoutId && (this.timeoutId = t.timeoutId);
    void 0 !== t.refreshTime && (this.refreshTime = t.refreshTime);
    void 0 !== t.loader && (this.loader = t.loader);
    void 0 !== t.isAutoRefresh && (this.isAutoRefresh = t.isAutoRefresh);
    void 0 !== t.remoteProxy && (this.remoteProxy = t.remoteProxy);
    void 0 !== t.adParam && (this.adParam = t.adParam);
    void 0 !== t.adOptions && (this.adOptions = t.adOptions);
    void 0 !== t.displayOptions && (this.displayOptions = t.displayOptions);
    void 0 !== t.interactionListener && (this.interactionListener = t.interactionListener);
    void 0 !== t.ads && (this.ads = t.ads);
    void 0 !== t.isFirstLoad && (this.isFirstLoad = t.isFirstLoad);
    void 0 !== t.isTaskRunning && (this.isTaskRunning = t.isTaskRunning);
  }
  updateStateVars(t) {
  }
  purgeVariableDependenciesOnElmtId(t) {
    this.__showComponent.purgeDependencyOnElmtId(t);
  }
  aboutToBeDeleted() {
    this.__showComponent.aboutToBeDeleted();
    SubscriberManager.Get().delete(this.id__());
    this.aboutToBeDeletedInternal();
  }
  get showComponent() {
    return this.__showComponent.get();
  }
  set showComponent(t) {
    this.__showComponent.set(t);
  }
  toMap(t) {
    const o = (t = t.replace(/[{}]/g, '')).split(',');
    const i = {};
    for (let t = 0; t < o.length; t++) {
      const e = o[t];
      const s = e.indexOf(':');
      if (s > -1) {
        const t = e.substring(0, s);
        const o = e.substring(s + 1);
        i[t] = o;
      }
    }
    return i;
  }
  getConfigJsonData() {
    let t = null;
    configPolicy.getOneCfgFile('etc/advertising/ads_framework/ad_service_config.json', ((o, i) => {
      if (null == o) {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'value is ' + i);
        try {
          const o = fs.openSync(i);
          const e = new ArrayBuffer(READ_FILE_BUFFER_SIZE);
          let s = fs.readSync(o.fd, e);
          fs.closeSync(o);
          let n = String.fromCharCode(...new Uint8Array(e.slice(0, s)));
          n = n.replace(/[\r\n\t\"]/g, '').replace(/\s*/g, '').replace(/\[|\]/g, '');
          t = this.toMap(n);
          hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'file succeed');
          t || hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'get config json failed');
          this.setWant(t);
        } catch (o) {
          hilog.error(HILOG_DOMAIN_CODE, 'AutoAdComponent',
            `open file failed with error:${o.code}, message:${o.message}`);
          this.setWant(t);
        }
      } else {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'error occurs ' + o);
        this.setWant(t);
      }
    }))
  }
  setWant(t) {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `setWant map ${t}`);
    t && (this.want = {
      bundleName: null == t ? void 0 : t.providerBundleName,
      abilityName: null == t ? void 0 : t.providerUEAAbilityName,
      parameters: {
        ads: this.ads,
        displayOptions: this.displayOptions,
        "ability.want.params.uiExtensionType": 'ads'
      }
    })
  }
  loadAd() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'start load advertising.');
    let t = {
      onAdLoadFailure: (t, o) => {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `request ad errorCode is: ${t}, errorMsg is: ${o}`);
        this.interactionListener.onStatusChanged('onAdFail', null, t.toString())
      },
      onAdLoadSuccess: t => {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'request ad success!');
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `adArray is : ${JSON.stringify(t)}`);
        this.interactionListener.onStatusChanged('onAdLoad', null, null);
        if (this.isFirstLoad) {
          this.ads = t;
          this.showComponent = !0;
          this.isFirstLoad = !1;
          this.getConfigJsonData();
        } else if (this.isTaskRunning) {
          hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'send data.');
          this.remoteProxy.send({
            ads: t
          })
        }
      }
    };
    this.loader.loadAd(this.adParam, this.adOptions, t);
    this.refreshAd()
  }
  initRefreshTime() {
    if (!(this.displayOptions && this.displayOptions.refreshTime &&
      typeof this.displayOptions.refreshTime === 'number' && this.displayOptions.refreshTime > 0)) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `Invalid input refreshTime, refreshTime is: ${this.refreshTime}.`);
      return !1;
    }
    this.displayOptions.refreshTime < MIN_REFRESH_TIME ? this.refreshTime = MIN_REFRESH_TIME :
      this.displayOptions.refreshTime > MAX_REFRESH_TIME ? this.refreshTime = MAX_REFRESH_TIME :
        this.refreshTime = this.displayOptions.refreshTime;
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `refreshTime is: ${this.refreshTime} ms.`);
    return !0;
  }
  async refreshAd() {
    if (this.isAutoRefresh) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'start next task.');
      this.isTaskRunning = !0;
      this.timeoutId = setTimeout((() => {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `run task, timeoutId:${this.timeoutId}.`);
        this.loadAd()
      }), this.refreshTime);
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `start next task timeoutId:${this.timeoutId}.`)
    }
  }
  aboutToAppear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'aboutToAppear.');
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `displayOptions:${JSON.stringify(this.displayOptions)}.`);
    this.loader = new advertising.AdLoader(this.context);
    this.isAutoRefresh = this.initRefreshTime();
    this.loadAd()
  }
  aboutToDisappear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'aboutToDisappear.');
    hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `timeoutId is: ${this.timeoutId}.`);
    if (null != this.timeoutId) {
      hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `stop refresh task, timeoutId:${this.timeoutId}.`);
      clearTimeout(this.timeoutId);
      this.isTaskRunning = !1
    }
  }
  initialRender() {
    this.observeComponentCreation(((t, o) => {
      ViewStackProcessor.StartGetAccessRecordingFor(t);
      Row.create();
      Row.height('100%');
      Row.onVisibleAreaChange([0, 1], ((t, o) => {
        hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `isVisible:${t}, currentRatio:${o}`);
        if (t && o >= 1) {
          hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'component visible');
          this.isTaskRunning || this.refreshAd()
        }
        if (!t && o <= 0) {
          hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'component invisible');
          hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `stop task, timeoutId:${this.timeoutId}.`);
          clearTimeout(this.timeoutId);
          this.isTaskRunning = !1
        }
      }));
      o || Row.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    this.observeComponentCreation(((t, o) => {
      ViewStackProcessor.StartGetAccessRecordingFor(t);
      Column.create();
      Column.width("100%");
      o || Column.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));
    this.observeComponentCreation(((t, o) => {
      ViewStackProcessor.StartGetAccessRecordingFor(t);
      If.create();
      this.showComponent ? this.ifElseBranchUpdateFunction(0, (() => {
        this.observeComponentCreation(((t, o) => {
          ViewStackProcessor.StartGetAccessRecordingFor(t);
          UIExtensionComponent.create(this.want);
          UIExtensionComponent.width('100%');
          UIExtensionComponent.height('100%');
          UIExtensionComponent.onRemoteReady((t => {
            this.remoteProxy = t;
            hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', 'remote proxy ready.');
          }));
          UIExtensionComponent.onReceive((t => {
            hilog.info(HILOG_DOMAIN_CODE, 'AutoAdComponent', `${JSON.stringify(t)}`);
            this.interactionListener.onStatusChanged(t.status, t.ad, t.data);
          }));
          o || UIExtensionComponent.pop();
          ViewStackProcessor.StopGetAccessRecording();
        }))
      })) : If.branchId(1);
      o || If.pop();
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