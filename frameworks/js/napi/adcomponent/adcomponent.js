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

let fs = globalThis.requireNapi('file.fs');
let hilog = globalThis.requireNapi('hilog');
let configPolicy = globalThis.requireNapi('configPolicy');
let LightWeightMap = globalThis.requireNapi('util.LightWeightMap');
let rpc = globalThis.requireNapi('rpc');
let util = globalThis.requireNapi('util');
const READ_FILE_BUFFER_SIZE = 4096;
const HILOG_DOMAIN_CODE = 65280;
const DEFAULT_MIN_SHOW_RATIO = 50;
const CUSTOMIZED_RENDERING = 3;
const CLICK = 0;
const IMP = 1;
const CLOSE = 2;
const CODE_SUCCESS = 200;

class AdComponent extends ViewPU {
  constructor(x, p1, q1, r = -1, r1 = undefined, s1) {
    super(x, q1, r, s1);
    if (typeof r1 === 'function') {
      this.paramsGenerator_ = r1;
    }
    this.ads = [];
    this.displayOptions = {};
    this.interactionListener = null;
    this.want = null;
    this.map = new LightWeightMap();
    this.ratios = [];
    this.remoteObj = null;
    this.connection = 0;
    this.isAdRenderer = false;
    this.context = getContext(this);
    this.minEffectiveShowRatio = 1;
    this.eventUniqueId = '';
    this.uniqueId = '';
    this.uiExtProxy = null;
    this.__showComponent = new ObservedPropertySimplePU(false, this, 'showComponent');
    this.__Behavior = new ObservedPropertySimplePU(HitTestMode.Default, this, 'Behavior');
    this.__uecHeight = new ObservedPropertySimplePU('100%', this, 'uecHeight');
    this.adRenderer = this.Component;
    this.setInitiallyProvidedValue(p1);
  }

  setInitiallyProvidedValue(p1) {
    const properties = [
      'ads',
      'displayOptions',
      'interactionListener',
      'want',
      'map',
      'ratios',
      'remoteObj',
      'connection',
      'isAdRenderer',
      'context',
      'minEffectiveShowRatio',
      'eventUniqueId',
      'uniqueId',
      'uiExtProxy',
      'showComponent',
      'Behavior',
      'uecHeight',
      'adRenderer',
    ];

    properties.forEach(prop => {
      if (p1[prop] !== undefined) {
        this[prop] = p1[prop];
      }
    });
  }

  updateStateVars(p1) {
  }

  purgeVariableDependenciesOnElmtId(o1) {
    this.__showComponent.purgeDependencyOnElmtId(o1);
    this.__Behavior.purgeDependencyOnElmtId(o1);
    this.__uecHeight.purgeDependencyOnElmtId(o1);
  }

  aboutToBeDeleted() {
    this.__showComponent.aboutToBeDeleted();
    this.__Behavior.aboutToBeDeleted();
    this.__uecHeight.aboutToBeDeleted();
    SubscriberManager.Get().delete(this.id__());
    this.aboutToBeDeletedInternal();
  }

  get showComponent() {
    return this.__showComponent.get();
  }

  set showComponent(n1) {
    this.__showComponent.set(n1);
  }

  get Behavior() {
    return this.__Behavior.get();
  }

  set Behavior(n1) {
    this.__Behavior.set(n1);
  }

  get uecHeight() {
    return this.__uecHeight.get();
  }

  set uecHeight(n1) {
    this.__uecHeight.set(n1);
  }

  getRatios() {
    let t;
    if (((t = this.ads[0]) === null || t === void 0 ? void 0 : t.adType) === 3) {
      let k1 = [];
      for (let m1 = 0; m1 <= 100; m1 += 5) {
        k1.push(m1 / 100);
      }
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent',
        `AdComponent minEffectiveShowRatio:${this.minEffectiveShowRatio / 100}`);
      k1.push(this.minEffectiveShowRatio / 100);
      const l1 = k1.filter((m1, d, k1) => {
        return k1.indexOf(m1, 0) === d;
      });
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent ratios: ${l1}`);
      return l1;
    } else {
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent ratios: ${[0, 1]}`);
      return [0, 1];
    }
  }

  createRpcData(u, w) {
    let y = rpc.MessageSequence.create();
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `remote descriptor: ${this.remoteObj?.getDescriptor()}`);
    y.writeInterfaceToken(this.remoteObj?.getDescriptor());
    y.writeInt(u);
    y.writeString(this.eventUniqueId);
    y.writeString(this.uniqueId);
    if (w !== null && w !== undefined) {
      y.writeString(this.getMillis());
      y.writeFloat(w);
    }
    return y;
  }

  initIds() {
    let t;
    this.eventUniqueId = util.generateRandomUUID(true);
    this.uniqueId = (t = this.ads[0]) === null || t === void 0 ? void 0 : t.uniqueId;
  }

  getMillis() {
    return Date.now().toString();
  }

  getMillisStr() {
    return Date.now().toString();
  }

  async sendDataRequest(u, w) {
    let t;
    const y = this.createRpcData(u, w);
    if (this.remoteObj === null) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onConnect failed.`);
      return;
    }
    let g1 = new rpc.MessageOption();
    let h1 = rpc.MessageSequence.create();
    await ((t = this.remoteObj) === null || t === void 0 ? void 0 :
    t.sendMessageRequest(CUSTOMIZED_RENDERING, y, h1, g1).then((i1) => {
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent sendRequest. result: ${JSON.stringify(i1)}`);
      let j1 = i1.reply.readInt();
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'AdComponent rpc reply code is : ' + j1);
      if (j1 === -1 || (u === CLOSE && j1 === CODE_SUCCESS)) {
        hilog.warn(HILOG_DOMAIN_CODE, 'AdComponent', `reply code is: ${j1}, event type code is : ${u}.`);
        this.remoteObj = null;
        this.disconnectServiceExtAbility();
      }
    }).catch((j) => {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent',
        `AdComponent sendRequest error. code: ${j.code}, message : ${j.message}`);
    }));
  }

  aboutToAppear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent aboutToAppear.`);
    this.ratios = this.getRatios();
    this.getConfigJsonData();
    this.getSuggestedCompHeight();
  }

  async aboutToDisappear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent aboutToDisappear.`);
    if (this.connection) {
      await this.sendDataRequest(CLOSE, 0);
    }
  }

  disconnectServiceExtAbility() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent disconnectServiceExtAbility`);
    this.context.disconnectServiceExtensionAbility(this.connection).then(() => {
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent disconnectServiceExtAbility success.`);
    }).catch((f1) => {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent',
        `AdComponent disconnectAbility failed. code: ${f1.code}, message : ${f1.message}`);
    });
  }

  initImpressionCondition() {
    let t;
    let d1;
    this.minEffectiveShowRatio =
      (d1 = (t = this.ads[0]) === null || t === void 0 ? void 0 : t.minEffectiveShowRatio) !== null &&
        d1 !== void 0 ? d1 : DEFAULT_MIN_SHOW_RATIO;
  }

  initAdRender() {
    let t;
    let d1;
    let e1;
    if (((t = this.ads[0]) === null || t === void 0 ? void 0 : t.adType) === 3 &&
      ((d1 = this.ads[0]) === null || d1 === void 0 ? void 0 : d1.creativeType) !== 99 &&
      ((e1 = this.ads[0]) === null || e1 === void 0 ? void 0 : e1.canSelfRendering)) {
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent initAdRender self rendering.`);
      this.isAdRenderer = true;
      this.Behavior = HitTestMode.Block;
      this.initIds();
      this.initImpressionCondition();
      this.connectServiceExtAbility();
    } else {
      this.showComponent = true;
    }
  }

  connectServiceExtAbility() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent connectServiceExtAbility`);
    let a1 = {
      bundleName: this.map.get('providerBundleName'),
      abilityName: this.map.get('providerApiAbilityName'),
    };
    this.connection = this.context.connectServiceExtensionAbility(a1, {
      onConnect: (b1, c1) => {
        let t;
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onConnect success.`);
        if (c1 === null) {
          hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onConnect remote is null.`);
          return;
        }
        this.remoteObj = c1;
        this.showComponent = true;
        (t = this.interactionListener) === null || t === void 0 ? void 0 :
        t.onStatusChanged('onConnect', this.ads[0], 'onAdRenderer connect api service.');
      },
      onDisconnect: () => {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onDisconnect`);
      },
      onFailed: () => {
        hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onFailed`);
      }
    });
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent connectServiceExtAbility result: ${this.connection}`);
  }

  Component(x = null) {
    this.observeComponentCreation2((r, s) => {
      UIExtensionComponent.create(this.want);
      UIExtensionComponent.onReceive((y) => {
        let t;
        let z = y;
        (t = this.interactionListener) === null || t === void 0 ? void 0 : t.onStatusChanged(z.status, z.ad, z.data);
        if (z.adPageHeight !== undefined) {
          this.uecHeight = z.adPageHeight;
        }
      });
      UIExtensionComponent.onRemoteReady((v) => {
        this.uiExtProxy = v;
      });
      UIExtensionComponent.width('100%');
      UIExtensionComponent.height(this.uecHeight);
    }, UIExtensionComponent);
  }

  onAreaClick() {
    let t;
    if (this.isAdRenderer) {
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'AdComponent onClick');
      const u = CLICK;
      this.sendDataRequest(u);
      (t = this.interactionListener) === null || t === void 0 ? void 0 :
      t.onStatusChanged('onAdClick', this.ads[0], 'onAdRenderer click');
    }
  }

  onColumnStateChange() {
    Column.create();
    Column.width('100%');
    Column.hitTestBehavior(this.Behavior);
    Column.onVisibleAreaChange(this.ratios, (v, w) => {
      if (this.isAdRenderer) {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent isVisible is :${v}, currentRatio is :${w}`);
        const u = IMP;
        this.sendDataRequest(u, w);
      }
      var p;
      (p = this.uiExtProxy) === null || p === void 0 ? void 0 : p.send({
        'isVisible': v,
        'currentRatio': w,
      });
    });
    Column.onClick(() => this.onAreaClick());
  }

  updateView() {
    this.ifElseBranchUpdateFunction(0, () => {
      this.observeComponentCreation2((r, s) => this.onColumnStateChange(), Column);
      this.adRenderer.bind(this)();
      Column.pop();
    });
  }

  initialRender() {
    this.observeComponentCreation2((r, s) => {
      Row.create();
      Row.height(this.uecHeight);
    }, Row);
    this.observeComponentCreation2((r, s) => {
      If.create();
      if (this.showComponent) {
        this.updateView();
      } else {
        this.ifElseBranchUpdateFunction(1, () => {
        });
      }
    }, If);
    If.pop();
    Row.pop();
  }

  getConfigPath() {
    let i = '';
    i = configPolicy.getOneCfgFileSync('etc/advertising/ads_framework/ad_service_config_ext.json');
    if (i === null || i === '') {
      hilog.warn(HILOG_DOMAIN_CODE, 'AdComponent', 'get ext config fail');
      i = configPolicy.getOneCfgFileSync('etc/advertising/ads_framework/ad_service_config.json');
    }
    if (i === null || i === '') {
      hilog.warn(HILOG_DOMAIN_CODE, 'AdComponent', 'get config fail');
      this.setWant();
      return '';
    }
    return i;
  }

  getConfigJsonData() {
    const i = this.getConfigPath();
    if (i === '') {
      return;
    }
    try {
      const k = fs.openSync(i);
      const l = new ArrayBuffer(READ_FILE_BUFFER_SIZE);
      let m = fs.readSync(k.fd, l);
      fs.closeSync(k);
      let n = String.fromCharCode(...new Uint8Array(l.slice(0, m)));
      const b = new RegExp('\[\\r\\n\\t\\\"\]', 'g');
      const o = new RegExp('\\s*', 'g');
      const p = new RegExp('\\\[\|\\\]', 'g');
      n = n.replace(b, '').replace(o, '').replace(p, '');
      this.toMap(n);
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `file succeed`);
      if (!this.map) {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `get config json failed`);
      }
      this.setWant();
    } catch (j) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `open file failed with error:${j.code}, message:${j.message}`);
      this.setWant();
    }
  }

  getSuggestedCompHeight() {
    if (this.ads[0].suggestedCompHeight !== undefined) {
      this.uecHeight = this.ads[0].suggestedCompHeight;
    }
  }

  setWant() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `setWant map ${this.map.toString()}`);
    if (this.map) {
      this.want = {
        bundleName: this.map.get('providerBundleName'),
        abilityName: this.map.get('providerUEAAbilityName'),
        parameters: {
          ads: this.ads,
          displayOptions: this.displayOptions,
          'ability.want.params.uiExtensionType': 'ads'
        }
      };
    }
    this.initAdRender();
  }

  toMap(a) {
    const b = new RegExp('\[\{\}\]', 'g');
    a = a.replace(b, '');
    const c = a.split(',');
    for (let d = 0; d < c.length; d++) {
      const e = c[d];
      const f = e.indexOf(':');
      if (f > -1) {
        const g = e.substring(0, f);
        const h = e.substring(f + 1);
        this.map.set(g, h);
      }
    }
  }

  rerender() {
    this.updateDirtyElements();
  }
}

export default {
  AdComponent
};