#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"

#include "usb_device.h"
#include "usbd_audio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"


//
//  For test purposes the Fake FF Function just copies all input data to output queues
//

void FF_Process(osObjects_t *hFF, void *pIn, void *pOut, uint32_t nBytes)
{
  	if( hFF->audiooutMode & AUDIO_MODE_OUT_I2S)
		{
			// Move the input audio samples into I2S output queue
			Queue_PushData(&hFF->PCM_Out_data, pIn, nBytes);
			
			// Check if we have to start playing audio thru external codec
			if(hFF->bStartPlay  && ( Queue_Count(&hFF->PCM_Out_data) >= (hFF->PCM_Out_data.nSize /2) ))
			{
				Queue_PopData(&hFF->PCM_Out_data, hFF->pPCM_Out, NUM_PCM_BYTES);
				BSP_AUDIO_OUT_Play((uint16_t *)hFF->pPCM_Out, NUM_PCM_BYTES);					
				hFF->bStartPlay = 0;
			}
		}
		if( hFF->audiooutMode & AUDIO_MODE_OUT_USB)
		{
			// Move the input audio samples into USB output queue
			// Note: Do not worry why it is called USB_In - in USB world
			//    everything that receives data from the HOST (PC) is always called OUT,
			//      and everything that sends data to the PC (HOST) is called/prefixed IN!!!!
			Queue_PushData(&osParams.USB_In_data, pIn, nBytes);
		}
}
	

//
//  Task to handle all incoming data
//
void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	BuffBlk_t *pDataQ;

	uint8_t		*pAudioIn, *pAudioOut, *pIV;
	
	pAudioIn 	= osAlloc(DSM_AUDIO_SIZE_BYTES);
	pAudioOut = osAlloc(DSM_AUDIO_SIZE_BYTES);
	pIV 			= osAlloc(DSM_IV_SIZE_BYTES);
	
	while(1)
	{	
		event = osMessageGet(osParams.dataReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Valid Input Data is present
		{
			pDataQ = (BuffBlk_t *) event.value.p;
			// - Figure out what type of data is it
			// - If there is enough data accumulated - place in appropriate buffer and 
			//   call the appropriate processing function
			if(pDataQ->Type == AUDIO_STEREO_Q15)
			{
				while(Queue_Count(pDataQ) >= DSM_AUDIO_SIZE_BYTES)
				{
					Queue_PopData(pDataQ, pAudioIn, DSM_AUDIO_SIZE_BYTES);
					//
					//   Call Feed Forward data processing
					//   DO_FF_PROCESSING(hFF, pAudioIn, pAudioOut, DSM_AUDIO_SIZE_BYTES);
					//
					FF_Process(&osParams, pAudioIn, pAudioOut, DSM_AUDIO_SIZE_BYTES);
				}
			}else if(pDataQ->Type == IV_MONO_Q15 )
			{
				while(Queue_Count(pDataQ) >= DSM_IV_SIZE_BYTES)
				{
					Queue_PopData(pDataQ, pIV, DSM_IV_SIZE_BYTES);
					//
					//   Call Feed Back data processing
					//   DO_FB_PROCESSING(hFB, pIV, DSM_IV_SIZE_SAMPLES);
					// 
					}
			}
		}	
	}
}

