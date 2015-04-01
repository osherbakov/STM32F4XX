/**
  ******************************************************************************
  * @file    usbd_audio.c
  * @author  MCD Application Team
  * @version V2.3.0
  * @date    04-November-2014
  * @brief   This file provides the Audio core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                AUDIO Class  Description
  *          ===================================================================
 *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
  *           Audio Devices V1.0 Mar 18, 98".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Standard AC Interface Descriptor management
  *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
  *             - 1 Audio Streaming Endpoint
  *             - 1 Audio Terminal Input (1 channel)
  *             - Audio Class-Specific AC Interfaces
  *             - Audio Class-Specific AS Interfaces
  *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
  *             - Audio Feature Unit (limited to Mute control)
  *             - Audio Synchronization type: Asynchronous
  *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
  *          The current audio class version supports the following audio features:
  *             - Pulse Coded Modulation (PCM) format
  *             - sampling rate: 48KHz.
  *             - Bit resolution: 16
  *             - Number of channels: 2
  *             - No volume control
  *             - Mute/Unmute capability
  *             - Asynchronous Endpoints
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_AUDIO
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_AUDIO_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Macros
  * @{
  */
#define AUDIO_SAMPLE_FREQ(frq)      (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2 * 2)/1000) & 0xFF), \
                                       (uint8_t)((((frq * 2 * 2)/1000) >> 8) & 0xFF)

/**
  * @}
  */




/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
  * @{
  */


static uint8_t  USBD_AUDIO_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

static uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

static uint8_t  *USBD_AUDIO_GetCfgDesc (uint16_t *length);

static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length);

static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_EP0_RxReady (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_SOF (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetMin(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetMax(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetRes(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Variables
  * @{
  */

USBD_ClassTypeDef  USBD_AUDIO =
{
  USBD_AUDIO_Init,
  USBD_AUDIO_DeInit,
  USBD_AUDIO_Setup,
  USBD_AUDIO_EP0_TxReady,
  USBD_AUDIO_EP0_RxReady,
  USBD_AUDIO_DataIn,
  USBD_AUDIO_DataOut,
  USBD_AUDIO_SOF,
  USBD_AUDIO_IsoINIncomplete,
  USBD_AUDIO_IsoOutIncomplete,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetDeviceQualifierDesc,
};

/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[100 + 147 + 9 + 9] __ALIGN_END =
{
  /* Configuration 1 */
  0x09,                                 /* bLength */
  USB_DESC_TYPE_CONFIGURATION,          /* bDescriptorType */
  LOBYTE((100 + 147 + 9 + 9)),                /* wTotalLength */
  HIBYTE((100 + 147 + 9 + 9)),
  0x03,                                 /* bNumInterfaces */
  0x01,                                 /* bConfigurationValue */
  0x00,                                 /* iConfiguration */
  0xC0,                                 /* bmAttributes  BUS Powered*/
  0x32,                                 /* bMaxPower = 100 mA*/
  /* 09 byte*/

  /* USB Headset Standard interface descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  0x00,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */
  0x00,                                 /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

//-------------------- Start Len calculation -------------------
  /* USB Headset Class-specific AC Interface Descriptor */
  0x0A,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */
  0x00,          /* 1.00 */             /* bcdADC */
  0x01,
  100,                                  /* wTotalLength = 100 */
  0x00,
  0x02,                                 /* bInCollection */
  0x01,                                 /* baInterfaceNr */
  0x02,                                 /* baInterfaceNr */
  /* 10 byte*/

  /* USB Headset Input Terminal 1 Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  0x01,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType USB_Streaming   0x0101 */
  0x01,
  0x00,                                 /* bAssocTerminal */
  0x02,                                 /* bNrChannels */
  0x03,                                 /* wChannelConfig 0x0003  LF + RF */
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* iTerminal */
  /* 12 byte*/

  /* USB Headset MIXER Unit 8 Descriptor */
  0x0D,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_MIXER_UNIT,             /* bDescriptorSubtype */
  0x08,                                 /* bUnitID */
  0x02,                                  /* bNrInPins */
  0x01,                                 /* baSourceID1 */
  0x07,                                 /* baSourceID1 */
  0x02,                                 /* bNrChannels */
  0x03,                                 /* wChannelConfig 0x0003  LF + RF */
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* bmControls */
  0x00,                                 /* iMixer */
  /* 13 byte*/

  /* USB Headset Feature Unit 2 Descriptor */
  0x0D,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  0x02,                                 /* bUnitID */
  0x08,                                 /* bSourceID */
  0x02,                                 /* bControlSize */
  0x01, 0x00,                           /* bmaControls(0) */
  0x02, 0x00,                           /* bmaControls(1) */
  0x02, 0x00,                           /* bmaControls(3) */
  0x00,                                 /* iFeature */
  /* 13 byte*/

  /*USB Headset Output Terminal 3 Descriptor */
  0x09,      /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  0x03,                                 /* bTerminalID */
  0x02,                                 /* wTerminalType  0x0302 - Headphone Speaker*/
  0x03,
  0x00,                                 /* bAssocTerminal */
  0x02,                                 /* bSourceID */
  0x00,                                 /* iTerminal */
  /* 09 byte*/


  /* USB Headset Input Terminal 4 Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  0x04,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType MIC  0x0201 */
  0x02,
  0x00,                                 /* bAssocTerminal */
  0x01,                                 /* bNrChannels */
  0x04,                                 /* wChannelConfig 0x0004  CF */
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* iTerminal */
  /* 12 byte*/


  /* USB Headset Feature Unit 5 Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  0x05,                                 /* bUnitID */
  0x04,                                 /* bSourceID */
  0x02,                                 /* bControlSize */
  0x01, 0x00,                           /* bmaControls(0) */
  0x02, 0x00,                           /* bmaControls(1) */
  0x00,                                 /* iTerminal */
  /* 11 byte*/

  /*USB Headset Output Terminal 6 Descriptor */
  0x09,      /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  0x06,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType  0x0101 - USB Stream*/
  0x01,
  0x00,                                 /* bAssocTerminal */
  0x05,                                 /* bSourceID */
  0x00,                                 /* iTerminal */
  /* 09 byte*/

  /* USB Headset Feature Unit 7 Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  0x07,                                 /* bUnitID */
  0x04,                                 /* bSourceID */
  0x02,                                 /* bControlSize */
  0x01, 0x00,                           /* bmaControls(0) */
  0x02, 0x00,                           /* bmaControls(1) */
  0x00,                                 /* iTerminal */
  /* 11 byte*/

//-------------------- End Len calculation -------------------

  /* USB Headset OUT Interface Descriptor - Audio Streaming Zero Bandwidth */
  /* Interface 1, Alternate Setting 0                                             */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Headset OUT Interface Descriptor - Audio Streaming Operational */
  /* Interface 1, Alternate Setting 1                                           */
  AUDIO_INTERFACE_DESC_SIZE,  /* bLength */
  USB_DESC_TYPE_INTERFACE,        /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x01,                                 /* bAlternateSetting */
  0x01,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Headset OUT Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  0x01,                                 /* bTerminalLink */
  0x00,                                 /* bDelay */
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001*/
  0x00,
  /* 07 byte*/

  /* USB Headset OUT Audio Type I Format Interface Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                /* bFormatType */
  0x02,                                 /* bNrChannels */
  0x02,                                 /* bSubFrameSize :  2 Bytes per frame (16bits) */
  16,                                   /* bBitResolution (16-bits per sample) */
  0x01,                                 /* bSamFreqType only one frequency supported */
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),         /* Audio sampling frequency coded on 3 bytes */
  /* 11 byte*/

  /* Endpoint 1 - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_OUT_EP,                         /* bEndpointAddress 1 out endpoint*/
  USBD_EP_TYPE_ISOC,                    /* bmAttributes */
  AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),    /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  0x01,                                 /* bInterval */
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,0x00,                            /* wLockDelay */
  /* 07 byte*/

//----------------- IN Interface ----

  /* USB Headset IN Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
  /* Interface 2, Alternate Setting 0                                             */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  0x02,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Headset IN Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Interface 2, Alternate Setting 1     - MONO                                   */
  AUDIO_INTERFACE_DESC_SIZE,  /* bLength */
  USB_DESC_TYPE_INTERFACE,        /* bDescriptorType */
  0x02,                                 /* bInterfaceNumber */
  0x01,                                 /* bAlternateSetting */
  0x01,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Headset IN Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  0x06,                                 /* bTerminalLink */
  0x00,                                 /* bDelay */
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001*/
  0x00,
  /* 07 byte*/

  /* USB Headset IN Audio Type I Format Interface Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                /* bFormatType */
  0x01,                                 /* bNrChannels */
  0x04,                                 /* bSubFrameSize :  4 Bytes per frame (32bits) */
  32,                                   /* bBitResolution (32-bits per sample) */
  0x01,                                 /* bSamFreqType only one frequency supported */
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),         /* Audio sampling frequency coded on 3 bytes */
  /* 11 byte*/

  /* Endpoint 1 - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_IN_EP,                          /* bEndpointAddress 1 in endpoint*/
  USBD_EP_TYPE_ISOC,                    /* bmAttributes */
  AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),    /* wMaxPacketSize in Bytes (Freq(Samples)*1(Stereo)*2(HalfWord)) */
  0x01,                                 /* bInterval */
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00,
  /* 07 byte*/


  /* USB Headset IN Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Interface 2, Alternate Setting 2     - STEREO                                   */
  AUDIO_INTERFACE_DESC_SIZE,  /* bLength */
  USB_DESC_TYPE_INTERFACE,        /* bDescriptorType */
  0x02,                                 /* bInterfaceNumber */
  0x02,                                 /* bAlternateSetting */
  0x01,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Headset IN Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  0x06,                                 /* bTerminalLink */
  0x00,                                 /* bDelay */
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001*/
  0x00,
  /* 07 byte*/

  /* USB Headset IN Audio Type I Format Interface Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                /* bFormatType */
  0x02,                                 /* bNrChannels */
  0x02,                                 /* bSubFrameSize :  2 Bytes per frame (16bits) */
  16,                                   /* bBitResolution (32-bits per sample) */
  0x01,                                 /* bSamFreqType only one frequency supported */
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),         /* Audio sampling frequency coded on 3 bytes */
  /* 11 byte*/

  /* Endpoint 1 - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_IN_EP,                          /* bEndpointAddress 1 in endpoint*/
  USBD_EP_TYPE_ISOC,                    /* bmAttributes */
  AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),    /* wMaxPacketSize in Bytes (Freq(Samples)*1(Stereo)*2(HalfWord)) */
  0x01,                                 /* bInterval */
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00,
  /* 07 byte*/

} ;

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END=
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,

};

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Functions
  * @{
  */

/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx)
{
  USBD_AUDIO_HandleTypeDef   *haudio;

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev,
                 AUDIO_OUT_EP,
                 USBD_EP_TYPE_ISOC,
                 AUDIO_OUT_PACKET);

  /* Open EP IN */
  USBD_LL_OpenEP(pdev,
                 AUDIO_IN_EP,
                 USBD_EP_TYPE_ISOC,
                 AUDIO_OUT_PACKET);

  /* Allocate Audio structure */
  pdev->pClassData = USBD_malloc(sizeof (USBD_AUDIO_HandleTypeDef));

  if(pdev->pClassData == NULL)
  {
    return USBD_FAIL;
  }
  else
  {
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    haudio->out_alt_setting = 0;
		haudio->out_start_play = 1;
    haudio->out_wr_ptr = 0;

    /* Initialize the Audio output Hardware layer */
    if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->Init(USBD_AUDIO_FREQ, AUDIO_DEFAULT_VOLUME, 0) != USBD_OK)
    {
      return USBD_FAIL;
    }

    /* Prepare Out endpoint to receive 1st packet */
    USBD_LL_PrepareReceive(pdev,
                           AUDIO_OUT_EP,
                           haudio->out_buffer,
                           AUDIO_OUT_PACKET);
  }
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Init
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev,
              AUDIO_OUT_EP);
  /* Close EP IN */
  USBD_LL_CloseEP(pdev,
              AUDIO_IN_EP);
	
  /* DeInit  physical Interface components */
  if(pdev->pClassData != NULL)
  {
   ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->DeInit(0);
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint8_t ret = USBD_OK;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {
    case AUDIO_REQ_GET_CUR:
      AUDIO_REQ_GetCurrent(pdev, req);
      break;

    case AUDIO_REQ_GET_MIN:
      AUDIO_REQ_GetMin(pdev, req);
      break;

    case AUDIO_REQ_GET_MAX:
      AUDIO_REQ_GetMax(pdev, req);
      break;
		
    case AUDIO_REQ_GET_RES:
      AUDIO_REQ_GetRes(pdev, req);
      break;

    case AUDIO_REQ_SET_CUR:
      AUDIO_REQ_SetCurrent(pdev, req);
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR:
      if( (req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_AUDIO_CfgDesc + 18;
        len = MIN(USB_AUDIO_DESC_SIZ , req->wLength);


        USBD_CtlSendData (pdev,
                          pbuf,
                          len);
      }
      break;

    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)haudio->out_alt_setting,
                        1);
      break;

    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES)
      {
        haudio->out_alt_setting = (uint8_t)(req->wValue);
      }
      else
      {
        /* Call the error management function (command will be NAC'ed */
        USBD_CtlError (pdev, req);
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
    }
  }
  return ret;
}


/**
  * @brief  USBD_AUDIO_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_AUDIO_GetCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_AUDIO_CfgDesc);
  return USBD_AUDIO_CfgDesc;
}

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{
  if (epnum == AUDIO_IN_EP)
  {
	}
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_EP0_RxReady (USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR)
  {/* In this driver, to simplify code, only SET_CUR request is managed */

//    if (haudio->control.unit == AUDIO_OUT_STREAMING_CTRL)
    {
     ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->MuteCtl(haudio->control.data[0]);
      haudio->control.cmd = 0;
      haudio->control.len = 0;
    }
  }

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev)
{
  /* Only OUT control data are processed */
  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_SOF (USBD_HandleTypeDef *pdev)
{
  return USBD_OK;
}


/**
  * @brief  USBD_AUDIO_Sync
  *         Plays the next portion of the received samples, adjusts the sampling rate
  * @param  pdev: device instance
  * @retval status
  */
void  USBD_AUDIO_Sync (USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  int32_t 	USBToDACOffset;
	int32_t	 	AdjustValue;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
	
	if(	haudio->out_start_play )
	{  // It's time to start next audio frame, but no data had arrived - stop playing/processing
		//   and make sure that on next data arrival from USB we will issue AUDIO_CMD_START command
		((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->AudioCmd(0, 0, AUDIO_CMD_STOP);
		haudio->out_wr_ptr = 0;
	}else
	{
		// Simple rate adjustment logic - when we start playing newly received USB samples,
		//  our write pointer should be exactly in a middle of the buffer.
		//  if not - play one more or one less sample to catch up with it
		USBToDACOffset =  haudio->out_wr_ptr - (AUDIO_TOTAL_BUF_SIZE/2);
		AdjustValue = 0;
		if(USBToDACOffset != 0)
		{
			AdjustValue = (USBToDACOffset > 0) ? AUDIO_OUT_ADJ_SIZE : -AUDIO_OUT_ADJ_SIZE;
		}
		((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->AudioCmd(&haudio->out_buffer[0],
																												 AUDIO_TOTAL_BUF_SIZE - AdjustValue,
																												 AUDIO_CMD_PLAY);
		haudio->out_start_play = 1;   // Set the flag to start playing anew if there is no new data
	}
}
/**
  * @brief  USBD_AUDIO_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  if (epnum == AUDIO_OUT_EP)
  {
    /* Increment the pointer and wrap it around if it goes beyond the buffer*/
    haudio->out_wr_ptr += AUDIO_OUT_PACKET;
    if (haudio->out_wr_ptr >= AUDIO_TOTAL_BUF_SIZE)
    {
      haudio->out_wr_ptr = 0;
			haudio->out_start_play = 0;
		}
    /* Prepare Out endpoint to receive next audio packet */
    USBD_LL_PrepareReceive(pdev,
                           AUDIO_OUT_EP,
                           &haudio->out_buffer[haudio->out_wr_ptr],
                           AUDIO_OUT_PACKET);

    if ( haudio->out_start_play && (haudio->out_wr_ptr >= AUDIO_TOTAL_BUF_SIZE/2))
    { // When the first half of the buffer is filled and we are not playing it yet, 
			// then start playing it
			((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->AudioCmd(&haudio->out_buffer[0],
																													 AUDIO_TOTAL_BUF_SIZE,
																													 AUDIO_CMD_START);
    }
  }
  return USBD_OK;
}

/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  USBD_memset(haudio->control.data, 0, 64);
  /* Send the current mute state */
  USBD_CtlSendData (pdev,
                    haudio->control.data,
                    req->wLength);
}

/**
  * @brief  AUDIO_Req_GetMin
  *         Handles the GET_MIN Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetMin(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  USBD_memset(haudio->control.data, 0, 64);
	haudio->control.data[0] = 0x01;
	haudio->control.data[1] = 0x80;
  /* Send the Min Volume */
  USBD_CtlSendData (pdev,
                    haudio->control.data,
                    req->wLength);
}

/**
  * @brief  AUDIO_Req_GetMax
  *         Handles the GET_MAX Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetMax(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  USBD_memset(haudio->control.data, 0, 64);
  /* Send the current Max Volume */
	haudio->control.data[0] = 0x00;
	haudio->control.data[1] = 0x06;
  USBD_CtlSendData (pdev,
                    haudio->control.data,
                    req->wLength);
}

/**
  * @brief  AUDIO_Req_GetRes
  *         Handles the GET_RES Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetRes(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  USBD_memset(haudio->control.data, 0, 64);
  /* Send the current Resolution */
	haudio->control.data[0] = 0x00;
	haudio->control.data[1] = 0x01;
  USBD_CtlSendData (pdev,
                    haudio->control.data,
                    req->wLength);
}


/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  if (req->wLength)
  {
    /* Prepare the reception of the buffer over EP0 */
    USBD_CtlPrepareRx (pdev,
                       haudio->control.data,
                       req->wLength);

    haudio->control.cmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
    haudio->control.len = req->wLength;          /* Set the request data length */
    haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
  }
}


/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_AUDIO_DeviceQualifierDesc);
  return USBD_AUDIO_DeviceQualifierDesc;
}

/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t  USBD_AUDIO_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                        USBD_AUDIO_ItfTypeDef *fops)
{
  if(fops != NULL)
  {
    pdev->pUserData= fops;
  }
  return 0;
}
/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
