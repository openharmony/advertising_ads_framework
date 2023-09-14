# Advertising Service Component

## Introduction

The advertising service enables you to implement ad access without SDK integration, helping you easily monetize your traffic.

## Directory Structure

```
/domain/cloud/advertising # Service code of the advertising service component
├── frameworks                         # Framework code
│   └── js                             # External JS API implementation
│       └── napi                       # External native API implementation
├── sa_profile                         # Service configuration profile
├── services                           # Service code
├── test                               # Test code
├── LICENSE                            # License file
├── BUILD.gn                           # Build entry
└── bundle.json                        # Component description file
```

### Concepts

- SA

  SA, short for SystemAbility, is a ipc entity that runs in the server process. After receiving an external request, SA processes the request and returns the processing result, thereby implementing service provisioning for external systems.


## How to Use

The advertising SA receives an ad request, processes the request, and forwards the request to the ad platform. The ad platform finds the best ad content and sends it to the SA for display.

### Creating an Ad Platform

The following steps walk you (application developer) through on how to create an ad platform and interact with the system.

1. Create a server component that implements the ServiceExtensionAbility.

   This component is the entry for SA interaction.

   ```javascript
   export default class AdsCoreService extends ServiceExtensionAbility {
     private descriptor = 'com.xxx.xxx';
   
     onCreate(want) {
       HiAdLog.i(TAG, `service onCreate`);
       HGlobalVariableManager.set("hmscoreFramework", "coreServiceAbilityContext", this.context);
     }
   
     onRequest(want, startId) {
       HiAdLog.i(TAG, `service onRequest`);
     }
   
     onConnect(want) {
       HiAdLog.i(TAG, `service onConnect, want: ${JSON.stringify(want)}`);
       return new AdsCoreServiceRpcObj(this.descriptor);
     }
   
     onDisconnect(want) {
       HiAdLog.i(TAG, `service onDisconnect`);
     }
   
     onDestroy() {
       HiAdLog.i(TAG, `service onDestory`);
     }
   }
   ```

2. Create an RPC object, which will be returned by the ServiceExtensionAbility server component to the SA.

   This RPC object is used to receive requests from the SA and send callback data to the SA.

   ```javascript
   import rpc from '@ohos.rpc';
   import bundleManager from '@ohos.bundle.bundleManager';
   
   /**
    * RPC object returned by AdsCoreService to the caller, which is used by the caller to send data to AdsCoreService.
    */
   export default class AdsCoreServiceRpcObj extends rpc.RemoteObject {
     constructor(descriptor) {
       super(descriptor);
     }
   
     /**
      * System API for receiving requests from the SA
      *
      * @param code Service request code sent by the peer end.
      * @param data Object that carries the parameters called by the client. The parameters are called in the following sequence: Uid, RemoteObject, AdRequestParams, AdOptions, and custom collection data. The parameters must be read in the same sequence.
      * @param reply MessageSequence object to which the result is written.
      * @param options Whether the operation is synchronous or asynchronous.
      * @returns
      */
     async onRemoteMessageRequest(code: number, data: rpc.MessageSequence, reply: rpc.MessageSequence, options: rpc.MessageOption) {
       // code: 1, indicating an ad request.
       console.info(`onRemoteMessageRequest, the code is: ${code}`);
   
       try {
         // 1. Read the ad request data in the specified data read sequence.
         // Read the RPC remote object on the SA side.
         const replyRpcObj: rpc.IRemoteObject = data.readRemoteObject();
        const req = {
          // Read ad request parameters. For details about the data structure, see AdRequestParams in the API document.
          reqParams: data.readString(),
          // Read ad configuration parameters. For details about the data structure, see AdOptions in the API document.
          adOptions: data.readString(),
          // Read custom collection data.
          collectedData: data.readString(),
        };
        // 2. Request data validation.
        // 3. Request ad processing.
        // Return the ad data. For details, see advertising.Advertisement in the API document.
        const ads: Array<advertising.Advertisement> = [];
        // 4. Send a response to the SA.
        const respData = rpc.MessageSequence.create();
        /**
         * The following service response codes are possible:
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

3. Configure application information.

   Modify the **ad_service_config.json** file in **services/advertising_manager/resource**.

   **providerBundleName**: bundle name of the application, which corresponds to **bundleName** in **app.json5** under **AppScope**.

   **providerAbilityName**: name of the server component that implements the ServiceExtensionAbility. This component is used to interact with the SA. It is **AdsCoreService** in the preceding example.

   **providerUIAbilityName**: name of the component that implements the UIAbility. This component is used to display full-screen ads.

   **providerUEAAbilityName**: name of the component that implements the UIExtensionAbility. This component is used to display non-full-screen ads.

4. Publish a full-screen ad.

   You can use the common event capability provided by the system to send the interactive operations of the full-screen ad to the application.

```javascript
import commonEvent from '@ohos.commonEventManager';

// Publish a common event.
commonEvent.publish("event", (err) => {
  if (err.code) {
    console.error("[CommonEvent]PublishCallBack err=" + JSON.stringify(err))
  } else {
    console.info("[CommonEvent]Publish")
  }
})
```

### Requesting an Ad

To request an ad, you must create an **AdLoader** object and call its **loadAd** method to initiate the request. You also need to use the **AdLoadListener** callback function to listen for the ad loading status.

```javascript
import advertising from '@ohos.advertising';
import common from '@ohos.app.ability.common';

try {
  const context = getContext(this) as common.UIAbilityContext;
  const adRequestParam = {
    adType: 3, // Ad type, for example, native ad.
    adId: "test", // Ad slot ID.
  };
  const adOption = {
    adContentClassification: 'A', // Set the maximum ad content rating.
  };
  // Listen for the ad loading status.
  const adLoaderListener = {
    // Called when an ad request fails.
    onAdLoadFailure: (errorCode: number, errorMsg: string) => {
      console.info(`request ad errorCode is: ${errorCode}, errorMsg is: ${errorMsg}`);
    },
    // Called when an ad request is successful.
    onAdLoadSuccess: (ads: Array<advertising.Advertisement>) => {
      console.info(`request ad success!`);
      // Save the requested ad content for display.
      const ads = ads;
    }
  };
  // Create an AdLoader object.
  const adLoader = new advertising.AdLoader(context);
  // Call the loadAd method.
  console.info(`request ad!`);
  adLoader.loadAd(adReqParams, adOptions, adLoaderListener);
} catch (err) {
  console.error(`load ad catch error: ${err.code} ${err.message}`);
}
```

### Showing a Full-Screen Ad

1. Subscribe to the event.

   Subscribe to the com.company.pps.action.PPS_REWARD_STATUS_CHANGED event to listen for changes on the reward ad page and receive reward information. The subscription must be initiated each time before an ad is displayed.

   After receiving the common event, use **reward_ad_status** and **reward_ad_data** as keys in the **parameters** parameter of [CommonEventData](https://docs.openharmony.cn/pages/v4.0/en/application-dev/reference/apis/js-apis-inner-commonEvent-commonEventData.md/) to obtain changes on the reward ad page and obtain reward information, respectively. You can use the **rewardType** and **rewardAmount** attributes to obtain the name of a reward and its quantity.

   ```javascript
   import commonEvent from '@ohos.commonEventManager';
   
   const KEY_REWARD_DATA = "reward_ad_data";
   const KEY_REWARD_STATUS = "reward_ad_status";
   
   export class RewardAdStatusHandler {
     // Used to save the created subscriber object for subsequent subscription and unsubscription.
     private subscriber;
       
     // Subscription method, which must be called each time before an ad is displayed.
     public registerPPSReceiver(): void {
       if (this.subscriber) {
         this.unRegisterPPSReceiver();
       }
       // Subscriber information.
       const subscribeInfo = {
         events: ["com.company.pps.action.PPS_REWARD_STATUS_CHANGED"],
       };
       // Callback for subscriber creation.
       commonEvent.createSubscriber(subscribeInfo, (err, commonEventSubscriber) => {
         if (err) {
           console.error(`createSubscriber error, ${JSON.stringify(err)}`);
           return;
         }
         console.debug(`createSubscriber success`);
         this.subscriber = commonEventSubscriber;
         // Callback for common event subscription.
         if (!this.subscriber) {
           console.warn(`need create subscriber`);
           return;
         }
         commonEvent.subscribe(this.subscriber, (err, commonEventData) => {
           if (err) {
             console.error(`subscribe error, ${JSON.stringify(err)}`);
           } else {
             // The subscriber receives the common event.
             console.debug(`subscribe data, ${JSON.stringify(commonEventData)}`);
             // Obtain the changes on the reward ad page.
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
                 // Obtain reward information.
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
   
     // Unsubscribe from the event.
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
     
     // Ad page changes
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

2. Show an ad.

   Call the **showAd** method to show an ad.

```javascript
import advertising from '@ohos.advertising';
import common from '@ohos.app.ability.common';

@Entry
@Component
export struct ShowAd {
  private context = getContext(this) as common.UIAbilityContext;
  // Requested ad content.
  private ad: advertising.Advertisement = void 0;
  // Ad display parameters.
  private adDisplayOptions: advertising.AdDisplayOptions = {
	  // Whether to mute the ad. By default, the ad is not muted.
	  mute: false,
	}

  build() {
	Column() {
      Button ("Show Ad")
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
			// Show a full-screen ad, such as a reward ad.
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

### Showing a Non-Full-Screen Ad

Use the AdComponent component to show a non-full-screen ad.

```javascript
import advertising from '@ohos.advertising';
import { AdComponent } from '@ohos.cloud.AdComponent';

@Entry
@Component
export struct ShowAd {
  // Requested ad content.
  private ads: Array[advertising.Advertisement] = [];
  // Ad display parameters.
  private adDisplayOptions: advertising.AdDisplayOptions = {
    // Whether to mute the ad. By default, the ad is not muted.
    mute: false,
  }

  build() {
    Column() {
      // Show a non-full-screen ad, such as a native ad.
      AdComponent({ ads: this.ads, displayOptions: this.adDisplayOptions,
        interactionListener: {
          // Ad status change callback.
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

## Repositories Involved
