# 流量变现服务部件

### 简介

广告服务为您提供流量变现服务，极简接入，无需集成SDK，轻松实现广告的接入，帮助您解决流量变现的难题。

### 目录

```
/domain/cloud/advertising  # 流量变现服务部件业务代码
├── frameworks                         # 框架代码
│   └── js                             # 外部接口实现
│       └── napi                       # napi 外部接口实现
├── sa_profile                         # 服务配置文件
├── services                           # 服务代码
├── test                               # 测试代码
├── LICENSE                            # 证书文件
├── BUILD.gn                           # 编译入口
└── bundle.json                        # 部件描述文件
```

#### 概念说明

- SA

  SA是SystemAbility的缩写，中文名叫系统元能力，运行在服务端的进程中，接收到外部的请求后进行处理并返回处理结果，对外提供服务。

### 说明

#### 使用说明

广告SA用于接收来自媒体的广告请求，进行处理后将请求转发到广告平台，广告平台需要找到广告内容给SA响应，用于后续的广告内容展示。

##### 创建广告平台

以APP开发者为例，描述如何创建广告平台并与系统交互。

1. 创建ServiceExtensionAbility的服务端组件

   该组件是与SA交互的入口。

   ```javascript
   export default class AdsCoreService extends ServiceExtensionAbility {
     private descriptor = 'com.xxx.xxx';
   
     onCreate(want) {
       console.i(TAG, `service onCreate`);
     }
   
     onRequest(want, startId) {
       console.i(TAG, `service onRequest`);
     }
   
     onConnect(want) {
       console.i(TAG, `service onConnect, want: ${JSON.stringify(want)}`);
       return new AdsCoreServiceRpcObj(this.descriptor);
     }
   
     onDisconnect(want) {
       console.i(TAG, `service onDisconnect`);
     }
   
     onDestroy() {
       console.i(TAG, `service onDestory`);
     }
   }
   ```

2. 创建ServiceExtensionAbility服务端组件返回给SA的RPC对象

   该RPC对象用于接收SA请求并向SA发送回调数据。

   ```javascript
   import rpc from '@ohos.rpc';
   import bundleManager from '@ohos.bundle.bundleManager';
   
   /**
    * AdsCoreService返回给调用方的rpc对象，用于调用方向service发送数据
    */
   export default class AdsCoreServiceRpcObj extends rpc.RemoteObject {
     constructor(descriptor) {
       super(descriptor);
     }
   
     /**
      * 系统接口，接收SA请求
      *
      * @param code 对端发送的服务请求码。
      * @param data 携带客户端调用参数的对象，客户端调用参数顺序为Uid、RemoteObject、AdRequestParams、AdOptions、自定义采集数据，必须按照此顺序读取。
      * @param reply 写入结果的MessageSequence对象。
      * @param options 指示操作是同步还是异步。
      * @returns
      */
     async onRemoteMessageRequest(code: number, data: rpc.MessageSequence, reply: rpc.MessageSequence, options: rpc.MessageOption) {
       // code：1，代表广告请求
       console.info(`onRemoteMessageRequest, the code is: ${code}`);
   
       try {
         // 1.读取广告请求数据，约定的数据读取顺序，不可更改
         // 读取SA侧的rpc远程对象
         const replyRpcObj: rpc.IRemoteObject = data.readRemoteObject();
        const req = {
          // 广告请求参数，数据结构参考API文档的AdRequestParams
          reqParams: data.readString(),
          // 广告配置参数，数据结构参考API文档的AdOptions
          adOptions: data.readString(),
          // 自定义采集数据
          collectedData: data.readString(),
        };
        // 2.请求数据校验
        // 3.请求广告处理
        // 返回广告数据，参考API文档中的advertising.Advertisement
        const ads: Array<advertising.Advertisement> = [];
        // 4.给SA响应数据
        const respData = rpc.MessageSequence.create();
        /**
         * 业务响应码
         * CODE_SUCCESS = 200
         * CODE_INVALID_PARAMETERS = 401
         * CODE_INTERNAL_ERROR = 100001
         * CODE_INIT_CONFIG_FAILURE = 100002
         * CODE_LOAD_ADS_FAILURE = 100003
         */
        respData.writeInt(200);
        respData.writeString(JSON.stringify(ads));
        const reply = rpc.MessageSequence.create();
        replyRpcObj.sendMessageRequest(code, respData, reply, new rpc.MessageOption(1))
          .catch(e => {
            console.error(`send message from kit to caller failed, error msg: ${e.message}`);
          });
        return true;
       } catch (e) {
         console.error(`handle rpc request failed, msg: ${e.message}`);
         return false;
       }
     }
   ```

3. 配置应用信息

   修改services/advertising_manager/resource/ad_service_config.json文件。

   providerBundleName：应用包名。AppScope下app.json5中的bundleName。

   providerAbilityName：实现了ServiceExtensionAbility的服务端组件名称，该组件用于和SA交互。如上示例中的AdsCoreService

   providerUIAbilityName：实现了UIAbility的组件名称，该组件用于展示全屏广告。

   providerUEAAbilityName：实现了UIExtensionAbility的组件名称，该组件用于展示非全屏广告。

4. 全屏广告发布事件
   通过系统提供的公共事件能力将全屏广告的互动操作发送到APP
```javascript
import commonEvent from '@ohos.commonEventManager';

// 发布公共事件
commonEvent.publish("event", (err) => {
  if (err.code) {
    console.error("[CommonEvent]PublishCallBack err=" + JSON.stringify(err))
  } else {
    console.info("[CommonEvent]Publish")
  }
})
```

##### 请求广告

请求广告需要创建一个AdLoader对象，通过AdLoader的loadAd方法请求广告。然后通过AdLoadListener回调来监听广告的加载状态。

```javascript
import advertising from '@ohos.advertising';
import common from '@ohos.app.ability.common';

try {
  const context = getContext(this) as common.UIAbilityContext;
  const adRequestParam = {
    adType: 3, // 广告类型：如原生广告
    adId: "test", // 测试广告位ID
  };
  const adOption = {
    adContentClassification: 'A', //设置广告内容分级上限
  };
  // 广告请求回调监听
  const adLoaderListener = {
    // 广告请求失败回调
    onAdLoadFailure: (errorCode: number, errorMsg: string) => {
      console.info(`request ad errorCode is: ${errorCode}, errorMsg is: ${errorMsg}`);
    },
    // 广告请求成功回调
    onAdLoadSuccess: (ads: Array<advertising.Advertisement>) => {
      console.info(`request ad success!`);
      // 保存请求到的广告内容用于展示
      const ads = ads;
    }
  };
  // 创建AdLoader广告对象
  const adLoader = new advertising.AdLoader(context);
  // 调用广告请求接口
  console.info(`request ad!`);
  adLoader.loadAd(adReqParams, adOptions, adLoaderListener);
} catch (err) {
  console.error(`load ad catch error: ${err.code} ${err.message}`);
}
```

##### 展示全屏广告

1. 事件订阅

   开发者需要在App中订阅com.company.pps.action.PPS_REWARD_STATUS_CHANGED事件来监听激励广告页面变化并接收奖励信息。订阅需要在每次展示广告前调用 。

   订阅者接收到公共事件后，可以从[CommonEventData](https://docs.openharmony.cn/pages/v4.0/zh-cn/application-dev/reference/apis/js-apis-inner-commonEvent-commonEventData.md/)的parameters参数中使用"reward_ad_status"作为key值获取激励广告页面变化状态，使用"reward_ad_data"作为key值获取奖励信息，属性rewardType用来获取奖励物品的名称，rewardAmount用来获取奖励物品的数量。

   ```javascript
   import commonEvent from '@ohos.commonEventManager';
   
   const KEY_REWARD_DATA = "reward_ad_data";
   const KEY_REWARD_STATUS = "reward_ad_status";
   
   export class RewardAdStatusHandler {
     // 用于保存创建成功的订阅者对象，后续使用其完成订阅及退订的动作
     private subscriber;
       
     // 订阅方法，需要在每次展示广告前调用
     public registerPPSReceiver(): void {
       if (this.subscriber) {
         this.unRegisterPPSReceiver();
       }
       // 订阅者信息
       const subscribeInfo = {
         events: ["com.company.pps.action.PPS_REWARD_STATUS_CHANGED"],
       };
       // 创建订阅者回调
       commonEvent.createSubscriber(subscribeInfo, (err, commonEventSubscriber) => {
         if (err) {
           console.error(`createSubscriber error, ${JSON.stringify(err)}`);
           return;
         }
         console.debug(`createSubscriber success`);
         this.subscriber = commonEventSubscriber;
         // 订阅公共事件回调
         if (!this.subscriber) {
           console.warn(`need create subscriber`);
           return;
         }
         commonEvent.subscribe(this.subscriber, (err, commonEventData) => {
           if (err) {
             console.error(`subscribe error, ${JSON.stringify(err)}`);
           } else {
             // 订阅者成功接收到公共事件
             console.debug(`subscribe data, ${JSON.stringify(commonEventData)}`);
             // 获取激励广告页面变化状态
             const status: string = commonEventData.parameters[KEY_REWARD_STATUS];
             switch (status) {
               case AdStatus.AD_OPEN:
                 console.info(`onAdOpen`);
                 break;
               case AdStatus.AD_CLICKED:
                 console.info(`onAdClick`);
                 break;
               case AdStatus.AD_CLOSED:
                 console.info(`onAdClose`);
                 this.unRegisterPPSReceiver();
                 break;
               case AdStatus.AD_REWARDED:
                 // 获取奖励信息
                 const rewardData = commonEventData.parameters[KEY_REWARD_DATA];
                 const rewardType = rewardData?.rewardType;
                 const rewardAmount = rewardData?.rewardAmount;
                 console.info(`onAdReward, rewardType: ${rewardType}, rewardAmount: ${rewardAmount}`);
                 break;
               case AdStatus.AD_VIDEO_START:
                 console.info(`onAdVideoStart`);
                 break;
               case AdStatus.AD_COMPLETED:
                 console.info(`onAdCompleted`);
                 break;
               default:
                 break;
             }
           }
         });
       });
     }
   
     // 取消订阅
     public unRegisterPPSReceiver(): void {
       commonEvent.unsubscribe(this.subscriber, (err) => {
         if (err) {
           console.error(`unsubscribe error, ${JSON.stringify(err)}`);
         } else {
           console.info(`unsubscribe success`);
           this.subscriber = null;
         }
       });
     }
     
     // 广告页面变化状态
     enum AdStatus {
       AD_OPEN = "onAdOpen",
       AD_CLICKED = "onAdClick",
       AD_CLOSED = "onAdClose",
       AD_REWARDED = "onAdReward",
       AD_VIDEO_START = "onVideoPlayBegin",
       AD_COMPLETED = "onVideoPlayEnd",
     }
   }
   ```

2. 展示广告

   调用showAd方法来展示广告。

```javascript
import advertising from '@ohos.advertising';
import common from '@ohos.app.ability.common';

@Entry
@Component
export struct ShowAd {
  private context = getContext(this) as common.UIAbilityContext;
  // 请求到的广告内容
  private ad: advertising.Advertisement = void 0;
  // 广告展示参数
  private adDisplayOptions: advertising.AdDisplayOptions = {
	  // 是否静音，默认不静音
	  mute: false,
	}

  build() {
	Column() {
      Button("展示广告")
		.backgroundColor('#d3d4d6')
		.fontSize(20)
		.fontColor('#000')
		.fontWeight(FontWeight.Normal)
		.align(Alignment.Center)
		.type(ButtonType.Capsule)
		.width('90%')
		.height(40)
		.margin({ top: 10, bottom: 5 })
		.onClick(() => {
	      try {
			// 展示全屏广告，如激励广告
			advertising.showAd(this.ad, this.adDisplayOptions, this.context);
	      } catch (err) {
			hilog.error(0x0000, 'testTag', 'show ad catch error: %{public}d %{public}s', err.code, err.message);
	      }
		});
	}
	.width('100%')
	.height('100%')
  }
}
```

##### 展示非全屏广告

在您的页面中使用AdComponent组件展示非全屏广告。

```javascript
import advertising from '@ohos.advertising';
import { AdComponent } from '@ohos.cloud.AdComponent';

@Entry
@Component
export struct ShowAd {
  // 请求到的广告内容
  private ads: Array[advertising.Advertisement] = [];
  // 广告展示参数
  private adDisplayOptions: advertising.AdDisplayOptions = {
    // 是否静音，默认不静音
    mute: false,
  }

  build() {
    Column() {
      // AdComponent组件用于展示非全屏广告，如原生广告
      AdComponent({ ads: this.ads, displayOptions: this.adDisplayOptions,
        interactionListener: {
          // 广告状态变化回调
          onStatusChanged: (status: string, ad: advertising.Advertisement, data: string) => {
            switch(status) {
              case AdStatus.AD_OPEN:
                console.info(`onAdOpen`);
                break;
              case AdStatus.AD_CLICKED:
                console.info(`onAdClick`);
                break;
              case AdStatus.AD_CLOSED:
                console.info(`onAdClose`);
                break;
            }
          }
		}
	  })
      .width('100%')
      .height('100%')
    }.width('100%').height('100%')
  }
}
```

### 相关仓