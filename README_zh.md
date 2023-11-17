# 广告服务框架部件

## 简介

广告服务框架指导OEM厂商搭建广告平台，从而为媒体提供广告服务；同时指导媒体使用广告服务框架开放接口，无需集成SDK，轻松实现广告的接入。

## 目录

```
/domain/advertising/advertising  # 广告服务框架部件业务代码
├── common                             # 公共引用
├── frameworks                         # 框架代码
│   └── js                             # 外部接口实现
│       └── napi                       # napi 外部接口实现
├── interfaces                         # 接口文件
├── LICENSE                            # 证书文件
├── BUILD.gn                           # 编译入口
└── bundle.json                        # 部件描述文件
```

## 使用说明

广告服务框架用于接收来自媒体的广告请求，进行处理后将请求转发到广告平台，广告平台需要找到广告内容给广告服务框架响应，用于后续的广告内容展示。

### 创建广告平台

以OEM厂商为例，描述如何创建广告平台。

1. 创建AdsServiceExtensionAbility的服务端组件

   该组件是与广告服务框架交互的入口。

   ```javascript
   import AdsServiceExtensionAbility from '@ohos.advertising.AdsServiceExtensionAbility';
   import { RespCallback } from '@ohos.advertising.AdsServiceExtensionAbility';
   import advertising from '@ohos.advertising';
   
   /**
    * AdsExtensionAbility继承AdsServiceExtensionAbility类，实现onLoadAd和onLoadAdWithMultiSlots方法，获取广告内容并向广告服务框架返回广告数据。
    * 想要继承AdsServiceExtensionAbility类必须是系统应用，并申请ohos.permission.GET_BUNDLE_INFO_PRIVILEGED权限，
    * 同时module.json5中对应的extensionAbility的type属性对应的值需要设置为adsService。
    */
   export default class AdsExtensionAbility extends AdsServiceExtensionAbility {
     /**
      * 请求广告接口，接受媒体单广告位广告请求
      *
      * @param adParam 媒体发送的单广告位广告请求参数
      * @param adOptions 媒体发送的请求广告配置
      * @param respCallback 广告请求结果回调方法
      */
     onLoadAd(adParam: advertising.AdRequestParms, adOptions: advertising.AdOptions, respCallback: RespCallback) {
       // 返回的广告数据
       const ads: Array<advertising.Advertisement> = [];
       ads.push({adType: 7, uniqueId: '111111', rewardVerifyConfig: null, rewarded: false, clicked: false});
       // 广告位ID
       const slot: string = 'test1';
       const resMap: Map<string, Array<advertising.Advertisement>> = new Map();
       // 回调添加广告位对应的广告信息
       respMap.set(slot, ads);
       // 当resMap为空的时候，会触发广告请求失败
       respCallback(respMap);
     }
   
     /**
      * 请求广告接口，接受媒体多广告位广告请求
      *
      * @param adParam 媒体发送的多广告位广告请求参数
      * @param adOptions 媒体发送的请求广告配置
      * @param respCallback 广告请求结果回调方法
      */
     onLoadAdWithMultiSlots(adParams: advertising.AdRequestParms[], adOptions: advertising.AdOptions, respCallback: RespCallback) {
       // 返回的广告数据
       const ads1: Array<advertising.Advertisement> = [];
       ads1.push({adType: 7, uniqueId: '111111', rewardVerifyConfig: null, rewarded: false, clicked: false});
       ads1.push({adType: 7, uniqueId: '222222', rewardVerifyConfig: null, rewarded: false, clicked: false});
       // 广告位ID
       const slot1: string = 'test1';
       // 返回的广告数据
       const ads2: Array<advertising.Advertisement> = [];
       ads2.push({adType: 7, uniqueId: '333333', rewardVerifyConfig: null, rewarded: false, clicked: false});
       ads2.push({adType: 7, uniqueId: '444444', rewardVerifyConfig: null, rewarded: false, clicked: false});
       // 广告位ID
       const slot2: string = 'test2';
       const resMap: Map<string, Array<advertising.Advertisement>> = new Map();
       // 回调添加广告位对应的广告信息
       respMap.set(slot1, ads1);
       respMap.set(slot2, ads2);
       // 当resMap为空的时候，会触发广告请求失败
       respCallback(respMap);
     }
   }
   ```

2. 配置应用信息

   修改frameworks/js/napi/ads/resource/ad_service_config.json文件。

   providerBundleName：应用包名。AppScope下app.json5中的bundleName。

   providerAbilityName：实现了ServiceExtensionAbility的服务端组件名称，该组件用于和广告服务框架交互。如上示例中的AdsExtensionAbility。

   providerUIAbilityName：实现了UIAbility的组件名称，该组件用于展示全屏广告。

   providerUEAAbilityName：实现了UIExtensionAbility的组件名称，该组件用于展示非全屏广告。

3. 全屏广告发布事件
   通过系统提供的公共事件能力将全屏广告的互动操作发送到APP
```javascript
import commonEvent from '@ohos.commonEventManager';

// 设置发布公共事件的通知
const options: Record<string, Object> = {
  'code': 1,
  'parameters': {
    // 广告页面变化状态
    'reward_ad_status': 'onAdRewarded',
    // 设置onAdRewarded状态对应的奖励信息
    'reward_ad_data': {
      rewardType: 'coin', // 奖励物品的名称
      rewardAmount: 10 // 奖励物品的数量
    }
  }
}

// 发布公共事件，事件ID可自定义
commonEvent.publish("com.company.pps.action.PPS_REWARD_STATUS_CHANGED", options, (err) => {
  if (err.code) {
    console.error("[CommonEvent]PublishCallBack err=" + JSON.stringify(err))
  } else {
    console.info("[CommonEvent]Publish")
  }
})
```

### 请求广告

以媒体为例，描述如何使用广告服务框架开放接口。

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

### 展示全屏广告

以媒体为例，描述如何使用广告服务框架开放接口。

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

### 展示非全屏广告

以媒体为例，描述如何使用广告服务框架开放接口。

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

## 相关仓
