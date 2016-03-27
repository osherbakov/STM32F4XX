/**
  ******************************************************************************
  * @file           : usbd_audio_if.c
  * @author         : MCD Application Team
  * @version        : V1.1.0
  * @date           : 19-March-2012
  * @brief          : Generic media access Layer.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio_if.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "datatasks.h"

/**
  * @}
  */ 

/* Private variables ---------------------------------------------------------*/
/* USB handler declaration */
/* Handle for USB Full Speed IP */
extern USBD_HandleTypeDef hUsbDeviceFS;

/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
  * @{
  */
static int8_t  AUDIO_Init         (uint32_t  AudioFreq);
static int8_t  AUDIO_DeInit       (void);
static int8_t  AUDIO_AudioCmd     (void *pBuff, uint32_t nbytes, uint8_t cmd);
static int8_t  AUDIO_VolumeCtl    (uint8_t vol);
static int8_t  AUDIO_MuteCtl      (uint8_t cmd);
static int8_t  AUDIO_PeriodicTC   (uint8_t cmd);
static int8_t  AUDIO_GetState     (void);

static uint32_t	USB_Prev;
static uint32_t	CYCCNT;
	
float						USB_Period;

static int32_t	USB_OutReady;	
static int32_t	USB_InReady;	

uint32_t	USB_Underruns;
uint32_t	USB_Overruns;


USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops_FS = 
{
  AUDIO_Init,
  AUDIO_DeInit,
  AUDIO_AudioCmd,
  AUDIO_VolumeCtl,
  AUDIO_MuteCtl,
  AUDIO_PeriodicTC,
  AUDIO_GetState,
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  AUDIO_Init
  *         Initializes the AUDIO media low layer over USB FS IP
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_Init(uint32_t  AudioFreq)
{
	USB_Period = SystemCoreClock/1000.0f;
	USB_Underruns = USB_Overruns = USB_OutReady = USB_InReady = 0;
  return (USBD_OK);
}

/**
  * @brief  AUDIO_DeInit
  *         DeInitializes the AUDIO media low layer
  * @param  options: Reserved for future use
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_DeInit()
{
  return (USBD_OK);
}

/**
  * @brief  AUDIO_AudioCmd
  *         Handles AUDIO command.
  * @param  pbuf: Pointer to buffer of data to be sent/received
  * @param  cmd: Command opcode
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_AudioCmd (void *pBuff, uint32_t nbytes, uint8_t cmd)
{

  switch(cmd)
  {
		case AUDIO_1MS_SYNC:
			CYCCNT = DWT->CYCCNT;
			USB_Period = 0.99f * USB_Period + 0.01f * (CYCCNT - USB_Prev);
			USB_Prev = CYCCNT;
		
		break;
		
		case AUDIO_DATA_IN:		// Callback called by USBD stack to get INPUT data INTO the Host
				if(Queue_Count_Bytes(osParams.USB_In_data) < nbytes) {
					USB_InReady = 0;
					USB_Underruns++;
				}
				if(USB_InReady) {
					Queue_PopData(osParams.USB_In_data,  pBuff, nbytes);
				}else{
					memset(pBuff, 0, nbytes);
					if(Queue_Count_Bytes(osParams.USB_In_data) >= osParams.USB_In_data->Size/2) {
						USB_InReady = 1;
					}
				}
			break;

		case AUDIO_DATA_OUT:	// Callback called by USBD stack when it receives OUTPUT data from the Host
			if(osParams.audioinMode == AUDIO_MODE_IN_USB)
			{
				// Place data into the queue and report to the main data processing task that data had arrived
				if(Queue_Space_Bytes(osParams.USB_Out_data) < nbytes)
				{
					USB_OutReady = 0;
					USB_Overruns++;
				}else {
					Queue_PushData(osParams.USB_Out_data,  pBuff, nbytes);
				}
				if(USB_OutReady) {
					osMessagePut(osParams.dataReadyMsg, (uint32_t)osParams.USB_Out_data, 0);
				}else	{
					if(Queue_Count_Bytes(osParams.USB_Out_data) >= osParams.USB_Out_data->Size/2) {
						USB_OutReady = 1;
					}
				}
			}
			break;
  }
  return (USBD_OK);
}

/**
  * @brief  AUDIO_VolumeCtl   
  *         Controls AUDIO Volume.
  * @param  vol: volume level (0..100)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_VolumeCtl (uint8_t vol)
{
  BSP_AUDIO_OUT_SetVolume(vol);
  return (USBD_OK);
}

/**
  * @brief  AUDIO_MuteCtl
  *         Controls AUDIO Mute.   
  * @param  cmd: command opcode
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_MuteCtl (uint8_t cmd)
{
  BSP_AUDIO_OUT_SetMute(cmd);
  return (USBD_OK);
}

/**
  * @brief  AUDIO_Periodic              
  * @param  cmd: Command opcode
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_PeriodicTC (uint8_t cmd)
{
  return (USBD_OK);
}

/**
  * @brief  AUDIO_GetState
  *         Gets AUDIO State.  
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_GetState (void)
{
  return (USBD_OK);
}
