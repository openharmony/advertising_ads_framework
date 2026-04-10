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
const INIT = 0;
const IMP = 1;
const CLICK = 2;
const CLOSE = 3;
const CODE_SUCCESS = 200;
const SET_DEATH_CALLBACK = 7;
const REGISTERE = 1;
const UNREGISTERE = 0;
const AdMainCounterMap = new Map();

const AdConnectionManager = {
  connection: -1,
  remoteObj: null,
  connectCount: 0,
  reset() {
    this.connection = -1;
    this.remoteObj = null;
    this.connectCount = 0;
  }
};

class AdFailCallbackStub extends rpc.RemoteObject {
  static DESCRIPTOR = 'AdFailCallbackDescriptor';
  constructor(component) {
    super(AdFailCallbackStub.DESCRIPTOR);
    this.component = component;
  }

  async onRemoteMessageRequest(code, data, reply, options) {
    await this.component.handleUIFail();
    return true;
  }
}

class AdStateCallbackStub extends rpc.RemoteObject {
  static DESCRIPTOR = 'AdStateCallbackDescriptor';
  constructor(component) {
    super(AdStateCallbackStub.DESCRIPTOR);
    this.component = component;
  }

  async onRemoteMessageRequest(code, data, reply, options) {
    const interfaceToken = data.readInterfaceToken();
    if (interfaceToken !== AdStateCallbackStub.DESCRIPTOR) {
      hilog.error(HILOG_DOMAIN_CODE, `onRemoteMessageRequest interfaceToken check error.`);
      return false;
    }
    const jsonStr = data.readString();
    const params = JSON.parse(jsonStr);
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `jsonStr: ${jsonStr}, params: ${params}`);
    const status = params?.status || '';
    const msg = params?.msg || '';
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `status: ${status}, msg: ${msg}`);
    this.component.interactionListener?.onStatusChanged(status, this.component.ads[0], msg);
    return true;
  }
}

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
    this.isAdRenderer = false;
    this.context = getContext(this);
    this.eventUniqueId = '';
    this.uniqueId = '';
    this.uiExtProxy = null;
    this.isVisibleBeforeRemoteReady = false;
    this.currentRatioBeforeRemoteReady = 0;
    this.__showComponent = new ObservedPropertySimplePU(false, this, 'showComponent');
    this.__Behavior = new ObservedPropertySimplePU(HitTestMode.Default, this, 'Behavior');
    this.__uecHeight = new ObservedPropertySimplePU('100%', this, 'uecHeight');
    this.adRenderer = this.Component;
    this.__rollPlayState = new SynchedPropertySimpleOneWayPU(p1.rollPlayState, this, 'rollPlayState');
    this.AdFailCallbackStub = null;
    this.failCallbackStub = null;
    this.AdStateCallbackStub = null;
    this.stateCallbackStub = null;
    this.isRegisterFailCallback = false;
    this.isReconnecting = false;
    this.eventQueue = [];
    this.setInitiallyProvidedValue(p1);
    this.declareWatch('rollPlayState', this.onRollPlayStateChange);
  }

  setInitiallyProvidedValue(p1) {
    const properties = [
      'ads',
      'displayOptions',
      'interactionListener',
      'want',
      'map',
      'ratios',
      'isAdRenderer',
      'context',
      'eventUniqueId',
      'uniqueId',
      'uiExtProxy',
      'isVisibleBeforeRemoteReady',
      'currentRatioBeforeRemoteReady',
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

    if (p1.rollPlayState === undefined) {
      this.__rollPlayState.set(2);
    }
  }

  updateStateVars(p1) {
    this.__rollPlayState.reset(p1.rollPlayState);
  }

  purgeVariableDependenciesOnElmtId(o1) {
    this.__showComponent.purgeDependencyOnElmtId(o1);
    this.__Behavior.purgeDependencyOnElmtId(o1);
    this.__uecHeight.purgeDependencyOnElmtId(o1);
    this.__rollPlayState.purgeDependencyOnElmtId(o1);
  }

  aboutToBeDeleted() {
    this.__showComponent.aboutToBeDeleted();
    this.__Behavior.aboutToBeDeleted();
    this.__uecHeight.aboutToBeDeleted();
    this.__rollPlayState.aboutToBeDeleted();
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

  get rollPlayState() {
    return this.__rollPlayState.get();
  }

  set rollPlayState(n1) {
    this.__rollPlayState.set(n1);
  }

  onRollPlayStateChange() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `onRollPlayStateChange: ${this.rollPlayState}.`);
    if (this.uiExtProxy) {
      this.uiExtProxy.send({
        'type': 'rollPlayControlData',
        'rollPlayState': this.rollPlayState,
      });
    }
  }

  getRatios() {
    let t;
    if (((t = this.ads[0]) === null || t === void 0 ? void 0 : t.adType) === 3) {
      let s;
      const u = (s = this.ads[0].minEffectiveShowRatio) !== null && s !== void 0 ? s : DEFAULT_MIN_SHOW_RATIO;
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent minEffectiveShowRatio:${u / 100}`);
      let k1 = [];
      k1.push(u / 100);
      for (let m1 = 0; m1 <= 100; m1 += 5) {
        k1.push(m1 / 100);
      }
      const l1 = Array.from(new Set(k1));
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent ratios: ${l1}`);
      return l1;
    } else {
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent ratios: ${[0, 1]}`);
      return [0, 1];
    }
  }

  initIds() {
    let t;
    this.eventUniqueId = util.generateRandomUUID(true);
    this.uniqueId = (t = this.ads[0]) === null || t === void 0 ? void 0 : t.uniqueId;
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `eventUniqueId:${this.eventUniqueId}, uniqueId:${this.uniqueId}`);
  }

  getMillis() {
    return Date.now().toString();
  }

  getAdEventFields() {
    return {
      adUniqueId: '',
      eventUniqueId: '',
      abilityName: '',
      options: '',
      ratio: '',
      visible: '',
      xAxis: '',
      yAxis: '',
      event: ''
    };
  }

  createRpcData(u, w = 0, visible = false, event = null) {
    const remoteObj = AdConnectionManager.remoteObj;
    if (!remoteObj || !remoteObj.getDescriptor) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', 'createRpcData failed');
      return null;
    }

    let y = rpc.MessageSequence.create();
    y.writeInterfaceToken(AdConnectionManager.remoteObj?.getDescriptor());
    y.writeInt(u);

    const adUniqueId = this.uniqueId || '';
    const eventUniqueId = this.eventUniqueId || '';
    const abilityName = this.context?.abilityInfo?.abilityName || '';

    const fields = this.getAdEventFields();
    fields.adUniqueId = this.uniqueId || '';
    fields.eventUniqueId = this.eventUniqueId || '';
    fields.abilityName = abilityName || '';
    switch (u) {
      case INIT:
        fields.options = JSON.stringify(this.displayOptions);
        break;
      case IMP:
        fields.ratio = w.toString();
        fields.visible = visible ? 'true' : 'false';
        break;
      case CLICK:
        fields.xAxis = event.tapLocation?.x;
        fields.yAxis = event.tapLocation?.y;
        fields.abilityName = abilityName || '';
        fields.event = JSON.stringify(event);
        break;
      case CLOSE:
        break;
      default:
        hilog.warn(HILOG_DOMAIN_CODE, 'AdComponent', `unknown: ${u}`);
        break;
    }
    const params = {};
    Object.keys(fields).forEach(key => {
      if (fields[key] !== undefined && fields[key] !== '') {
        params[key] = fields[key];
      }
    });
    const jsonStr = JSON.stringify(params);
    y.writeString(jsonStr);
    if (u === INIT) {
      if (!this.stateCallbackStub) {
        this.stateCallbackStub = new AdStateCallbackStub(this);
      }
      y.writeRemoteObject(this.stateCallbackStub);
    }
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `send json params: ${jsonStr}`);
    return y;
  }

  async sendDataRequest(u, ratio = 0, visible = false, event = null) {
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent sendDataRequest: ${u}`);
    const y = this.createRpcData(u, ratio, visible, event);
    if (!AdConnectionManager.remoteObj || !y) {
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'AdComponent reconnet');
      this.eventQueue.push({ u, ratio, visible, event });
      if (!this.isReconnecting) {
        await this.reconnectServiceExtAbility();
      }
      return;
    }
    let g1 = new rpc.MessageOption();
    let h1 = rpc.MessageSequence.create();
    try {
      const i1 = await AdConnectionManager.remoteObj.sendMessageRequest(CUSTOMIZED_RENDERING, y, h1, g1);
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent sendRequest. result: ${JSON.stringify(i1)}`);
      let j1 = i1.reply.readInt();
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'AdComponent rpc reply code is : ' + j1);
      if (j1 === -1 || (u === CLOSE && j1 === CODE_SUCCESS)) {
        hilog.warn(HILOG_DOMAIN_CODE, 'AdComponent', `reply code is: ${j1}, event type code is : ${u}.`);
        this.disconnectServiceExtAbility();
      }
      if (u === INIT) {
        this.showComponent = true;
      }
    } catch (j) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent',
        `AdComponent sendRequest error. code: ${j.code}, message : ${j.message}`);
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', 'AdComponent reconnet');
      this.eventQueue.push({ u, ratio, visible, event });
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent isReconnecting: ${this.isReconnecting}`);
      if (!this.isReconnecting) {
        await this.reconnectServiceExtAbility();
      }
    } finally {
      if (h1) {
        h1.reclaim();
      }
    }
  }

  async handleUIFail() {
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent handleUIFail.`);
    let t;
    (t = this.interactionListener) === null || t === void 0 ? void 0 :
      t.onStatusChanged('onAdClose', this.ads[0], 'adUiProcessDied');
    await this.unregisterFailCallback();
    this.disconnectServiceExtAbility();
  }

  async registerFailCallback() {
    if (!AdConnectionManager.remoteObj || this.isRegisterFailCallback) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent registerFailCallback remoteObj is null.`);
      return;
    }
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `registerFailCallback: ${AdConnectionManager.connection}`);
    if (!this.failCallbackStub) {
      this.failCallbackStub = new AdFailCallbackStub(this);
    }
    let data = rpc.MessageSequence.create();
    data.writeInterfaceToken(AdConnectionManager.remoteObj?.getDescriptor());
    data.writeInt(REGISTERE);
    data.writeString(this.uniqueId);
    data.writeRemoteObject(this.failCallbackStub);
    let reply = rpc.MessageSequence.create();
    let option = new rpc.MessageOption();
    try {
      const result = await AdConnectionManager.remoteObj?.sendMessageRequest(SET_DEATH_CALLBACK, data, reply, option);
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent registerFailCallback result: ${JSON.stringify(result)}`);
      this.isRegisterFailCallback = true;
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent',
        `AdComponent registerFailCallback error. code: ${e.code}, message: ${e.message}`);
    } finally {
      if (reply) {
        reply.reclaim();
      }
    }
  }

  async unregisterFailCallback() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `unregisterFailCallback this.isRegisterFailCallback: ${this.isRegisterFailCallback}`);
    if (!AdConnectionManager.remoteObj || !this.isRegisterFailCallback) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent unregisterFailCallback remoteObj is null.`);
      return;
    }
    let data = rpc.MessageSequence.create();
    data.writeInterfaceToken(AdConnectionManager.remoteObj?.getDescriptor());
    data.writeInt(UNREGISTERE);
    data.writeString(this.uniqueId);
    data.writeRemoteObject(this.failCallbackStub);
    let reply = rpc.MessageSequence.create();
    let option = new rpc.MessageOption();
    try {
      const result = await AdConnectionManager.remoteObj?.sendMessageRequest(SET_DEATH_CALLBACK, data, reply, option);
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent unregisterFailCallback result: ${JSON.stringify(result)}`);
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent',
        `AdComponent unregisterFailCallback error. code: ${e.code}, message: ${e.message}`);
    } finally {
      if (reply) {
        reply.reclaim();
      }
      this.isRegisterFailCallback = false;
      this.failCallbackStub = null;
    }
  }

  aboutToAppear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent aboutToAppear.`);
    this.ratios = this.getRatios();
    this.getConfigJsonData();
    this.getSuggestedCompHeight();
    this.registerWindowObserver();
  }

  async aboutToDisappear() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent aboutToDisappear.`);
    this.clearMainAdCounter();
    this.unregisterWindowObserver();
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent aboutToDisappear connection:${AdConnectionManager.connection}`);
    if (this.isAdRenderer) {
      await this.sendDataRequest(CLOSE);
    } else {
      await this.unregisterFailCallback();
    }
    this.disconnectServiceExtAbility();
    this.component = null;
    this.stateCallbackStub = null;
  }

  initAdRender() {
    this.initIds();
    if (this.checkMainAdDuplicate()) {
      this.showComponent = false;
      return;
    }
    this.isAdRenderer = this.ads[0]?.canSelfRendering;
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `isAdRenderer:${this.isAdRenderer}, canSelfRendering:${this.ads[0]?.canSelfRendering}`);
    if (!this.isAdRenderer) {
      this.showComponent = true;
    }
    this.connectServiceExtAbility();
  }

  checkMainAdDuplicate() {
    const adUniqueId = this.uniqueId;
    const type = this.displayOptions?.type;
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `checkMainAdDuplicate type: ${type}, adUniqueId: ${adUniqueId}`);
    if (!type || !adUniqueId) {
      return false;
    }
    if (type !== 'main') {
      return false;
    }
    let count = AdMainCounterMap.get(adUniqueId) || 0;
    count++;
    AdMainCounterMap.set(adUniqueId, count);
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `main adUniqueId: ${adUniqueId}, count: ${count}`);
    if (count > 1) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `main ad duplicate, trigger onAdFail: ${adUniqueId}`);
      let t;
      (t = this.interactionListener) === null || t === void 0 ? void 0 :
        t.onStatusChanged('onAdFail', this.ads[0], 'multi main ad');
      return true;
    }
    return false;
  }

  clearMainAdCounter() {
    const uniqueId = this.uniqueId;
    const type = this.displayOptions?.type;
    if (type === 'main' && uniqueId) {
      AdMainCounterMap.delete(uniqueId);
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `clear uniqueId: ${uniqueId}`);
    }
  }

  executeRenderModeTask() {
    if (this.isAdRenderer) {
      this.Behavior = HitTestMode.Default;
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `type:${this.displayOptions?.type}, Behavior:${this.Behavior}`);
      this.sendDataRequest(INIT);
    } else {
      if (!this.isRegisterFailCallback) {
        this.registerFailCallback();
      }
    }
  }

  connectServiceExtAbility() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent connectServiceExtAbility:${AdConnectionManager.connection}, 
      ${AdConnectionManager.connectCount}`);
    if (AdConnectionManager.connection !== -1 || AdConnectionManager.connectCount > 0) {
      AdConnectionManager.connectCount++;
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'already connected, skip duplicate connect');
      this.executeRenderModeTask();
      return;
    }
    let a1 = {
      bundleName: this.map.get('providerBundleName'),
      abilityName: this.map.get('providerApiAbilityName'),
    };
    AdConnectionManager.connection = this.context?.connectServiceExtensionAbility(a1, {
      onConnect: (b1, c1) => {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onConnect success.`);
        if (c1 === null) {
          hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onConnect remote is null.`);
          return;
        }
        AdConnectionManager.remoteObj = c1;
        AdConnectionManager.connectCount = 1;
        this.executeRenderModeTask();
        hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `connectCount after increment: ${AdConnectionManager.connectCount}`);
      },
      onDisconnect: () => {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onDisconnect`);
        AdConnectionManager.connection = -1;
      },
      onFailed: () => {
        hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onFailed`);
        AdConnectionManager.connection = -1;
      }
    });
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent connectServiceExtAbility result: ${AdConnectionManager.connection}`);
  }


  async reconnectServiceExtAbility() {
    if (this.isReconnecting) {
      return;
    }
    this.isReconnecting = true;
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'reconnectForResendEvent');
    let a1 = {
      bundleName: this.map.get('providerBundleName'),
      abilityName: this.map.get('providerApiAbilityName'),
    };
    AdConnectionManager.connection = this.context?.connectServiceExtensionAbility(a1, {
      onConnect: async (b1, c1) => {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent reconnect success.`);
        if (c1 === null) {
          hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onConnect remote is null.`);
          return;
        }
        AdConnectionManager.remoteObj = c1;
        AdConnectionManager.connectCount++;
        await this.sendDataRequest(INIT);
        while (this.eventQueue.length > 0) {
          const event = this.eventQueue.shift();
          await this.sendDataRequest(
            event.u,
            event.ratio,
            event.visible,
            event.event
          );
          hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `sendDataRequest pendingEvent: ${event.u}`);
        }
        this.isReconnecting = false;
        hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `sendDataRequest pendingEvent done`);
      },
      onDisconnect: () => {
        hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onDisconnect`);
        AdConnectionManager.connection = -1;
        this.isReconnecting = false;
        this.eventQueue = [];
      },
      onFailed: (code) => {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onFailed`);
        AdConnectionManager.connection = -1;
        this.isReconnecting = false;
        this.eventQueue = [];
        let t;
        (t = this.interactionListener) === null || t === void 0 ? void 0 :
          t.onStatusChanged('onAdFail', this.ads[0], 'reconnect api service failed');
      }
    });
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent reconnectServiceExtAbility result: ${AdConnectionManager.connection}`);
  }


  disconnectServiceExtAbility() {
    hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent disconnectServiceExtAbility`);
    if (AdConnectionManager.connection === -1) {
      AdConnectionManager.reset();
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'already disconnect');
      return;
    }

    AdConnectionManager.connectCount = Math.max(0, AdConnectionManager.connectCount - 1);
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `connectCount after decrement: ${AdConnectionManager.connectCount}`);

    this.context?.disconnectServiceExtensionAbility(AdConnectionManager.connection).then(() => {
      hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent disconnectServiceExtAbility success.`);
    }).catch((f1) => {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent',
        `AdComponent disconnectAbility failed. code: ${f1.code}, message : ${f1.message}`);
    }).finally(() => {
      if (AdConnectionManager.connectCount <= 0) {
        AdConnectionManager.reset();
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'connectCount=0, reset connection manager');
      }
      AdConnectionManager.connection === -1
      this.uiExtProxy = null;
      this.isRegisterFailCallback = false;
      this.failCallbackStub = null;
    });
  }

  Component(x = null) {
    this.observeComponentCreation2((r, s) => {
      UIExtensionComponent.create(this.want);
      UIExtensionComponent.onReceive((y) => {
        let t;
        let z = y;
        if (z.status !== undefined) {
          (t = this.interactionListener) === null || t === void 0 ? void 0 : t.onStatusChanged(z.status, z.ad, z.data);
        }
        if (z.adPageHeight !== undefined) {
          this.uecHeight = z.adPageHeight;
        }
      });
      UIExtensionComponent.onRemoteReady((v) => {
        this.uiExtProxy = v;
        v.on('asyncReceiverRegister', (p) => {
          p.send({
            'type': 'uecOnVisibleData',
            'isVisible': this.isVisibleBeforeRemoteReady,
            'currentRatio': this.currentRatioBeforeRemoteReady,
          });
          p.send({
            'type': 'rollPlayControlData',
            'rollPlayState': this.rollPlayState,
          });
          this.initWindowInfo(p);
        });
      });
      UIExtensionComponent.onVisibleAreaChange(this.ratios, (v, r) => {
        if (this.uiExtProxy) {
          this.uiExtProxy.send({
            'type': 'uecOnVisibleData',
            'isVisible': v,
            'currentRatio': r,
          });
        } else {
          this.isVisibleBeforeRemoteReady = v;
          this.currentRatioBeforeRemoteReady = r;
        }
      });
      UIExtensionComponent.width('100%');
      UIExtensionComponent.height(this.uecHeight);
    }, UIExtensionComponent);
  }

  onAreaClick(clickEvent) {
    let t;
    hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onClick, type:${this.displayOptions?.isClickable}`);
    if (this.displayOptions?.isClickable && this.isAdRenderer) {
      const u = CLICK;
      this.sendDataRequest(u, 0, false, clickEvent);
    }
  }

  onColumnStateChange() {
    Column.create();
    Column.width('100%');
    Column.hitTestBehavior(this.Behavior);
    Column.onVisibleAreaChange(this.ratios, (v, w) => {
      const type = this.displayOptions?.type;
      if ((type === 'main') && this.isAdRenderer) {
        hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent isVisible is :${v}, currentRatio is :${w}, type:${type}`);
        const u = IMP;
        this.sendDataRequest(IMP, w, v);
      }
    });
    globalThis.Gesture.create(GesturePriority.Parallel);
    TapGesture.create();
    TapGesture.onAction((event) => {
      hilog.debug(HILOG_DOMAIN_CODE, 'AdComponent', `AdComponent onClick, type:${this.displayOptions?.isClickable}`);
      if (this.displayOptions?.isClickable && this.isAdRenderer) {
        const u = CLICK;
        this.sendDataRequest(u, 0, false, event);
      }
    });
    TapGesture.pop();
    globalThis.Gesture.pop();
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
    try {
      if (this.ads[0].suggestedCompHeight !== undefined) {
        this.uecHeight = this.ads[0].suggestedCompHeight;
      }
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `Get suggestedHeight error, code: ${e.code}, msg: ${e.message}`);
    }
  }

  initWindowInfo(proxy) {
    try {
      const win = this.context?.windowStage.getMainWindowSync();
      proxy?.send({
        'type': 'windowStatusChange',
        'windowStatusType': win.getWindowStatus(),
      });
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `Failed to obtain window status. Code is ${e?.code}, message is ${e?.message}`);
    }
  }

  registerWindowObserver() {
    try {
      const win = this.context?.windowStage.getMainWindowSync();
      win.on('windowStatusChange', (windowStatusType) => {
        this.uiExtProxy?.send({
          'type': 'windowStatusChange',
          'windowStatusType': windowStatusType,
        });
      });
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `Failed to observe windowStatusChange. Code is ${e?.code}, message is ${e?.message}`);
    }
  }

  unregisterWindowObserver() {
    try {
      const win = this.context?.windowStage.getMainWindowSync();
      win.off('windowStatusChange');
    } catch (e) {
      hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `Failed to off observe windowStatusChange. Code is ${e?.code}, message is ${e?.message}`);
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