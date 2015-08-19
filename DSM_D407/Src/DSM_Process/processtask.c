#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"
#include "dataqueues.h"

#include "usb_device.h"
#include "usbd_audio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"


void Data_Distribute(osObjects_t *hFF, void *pData, uint32_t nBytes)
{
	if( hFF->audiooutMode & AUDIO_MODE_OUT_I2S)
	{
		// Move the input audio samples into I2S output queue
		Queue_PushData(hFF->PCM_Out_data, pData, nBytes);
		
		// Check if we have to start playing audio thru external codec
		if(hFF->bStartPlay  && ( Queue_Count(hFF->PCM_Out_data) >= (hFF->PCM_Out_data->nSize /2) ))
		{
			Queue_PopData(hFF->PCM_Out_data, hFF->pPCM_Out, NUM_PCM_BYTES);
			BSP_AUDIO_OUT_Play((uint16_t *)hFF->pPCM_Out, NUM_PCM_BYTES);					
			hFF->bStartPlay = 0;
		}
	}
	//if( hFF->audiooutMode & AUDIO_MODE_OUT_USB)
	{
		// Move the input audio samples into USB output queue
		// Note: Do not worry why it is called USB_In - in USB world
		//    everything that receives data from the HOST (PC) is always called OUT,
		//      and everything that sends data to the PC (HOST) is called/prefixed IN!!!!
		Queue_PushData(osParams.USB_In_data, pData, nBytes);
	}
}

extern DataProcessBlock_t  Melp;
extern DataProcessBlock_t  CVSD;
extern DataProcessBlock_t  CODEC2;

DataProcessBlock_t  *pModule;


void DataConvert(void *pDest, uint32_t TypeDest, void *pSrc, uint32_t TypeSrc, uint32_t nBytes)
{
	if(TypeDest == TypeSrc)
	{
		memcpy(pDest, pSrc, nBytes);
		return;
	}		
	
	
}

//
//  Task to handle all incoming data
//
void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQ;

	uint8_t 	*pAudio;
	float		*pAudioIn_f32;
	float		*pAudioOut_f32;
	
	uint32_t	nBytesQueue, nBytesModule, Type, nSamplesQueue, nSamplesBlock;
	
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);	
	pAudioIn_f32 = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));	
	pAudioOut_f32 = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));	

	while(1)
	{	
		event = osMessageGet(osParams.dataReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Valid Input Data is present
		{
			pDataQ = (DQueue_t *) event.value.p;
			// - Figure out what type of data is it
			// - If there is enough data accumulated - place in a buffer and 
			//   call the appropriate processing function
			nBytesQueue = Queue_Count(pDataQ); nSamplesQueue = NUM_SAMPLES(nBytesQueue, pDataQ->Type);
			nBytesModule = pModule->TypeSize(NULL, &Type); nSamplesBlock = NUM_SAMPLES(nBytesModule, Type);
			while(nSamplesQueue >= nSamplesBlock)
			{
				Queue_PopData(pDataQ, pAudio, nBytesQueue);
				//   Call data processing
				DataConvert(pAudioIn_f32, Type, pAudio, pDataQ->Type, nBytesQueue);
				pModule->Process(NULL, pAudioIn_f32, pAudioOut_f32, nBytesModule);
				//   Distribute output data to all output data sinks (USB, I2S, etc)
				Data_Distribute(&osParams, pAudio, nBytesModule);
			}
		}	
	}
}

