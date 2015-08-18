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


//
//  Task to handle all incoming data
//
void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQ;

	uint8_t		*pAudio;
	float			*pAudioIn;
	float			*pAudioOut;
	uint32_t	nBytes, Type;
	
	pAudio 	= osAlloc(MAX_AUDIO_SIZE_BYTES);

	pAudioIn = osAlloc(MAX_AUDIO_SIZE_BYTES * sizeof(float));	
	pAudioOut = osAlloc(MAX_AUDIO_SIZE_BYTES * sizeof(float));


	while(1)
	{	
		event = osMessageGet(osParams.dataReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Valid Input Data is present
		{
			pDataQ = (DQueue_t *) event.value.p;
			// - Figure out what type of data is it
			// - If there is enough data accumulated - place in appropriate buffer and 
			//   call the appropriate processing function
			nBytes = pModule->TypeSize(NULL, &Type);
			while(Queue_Count(pDataQ) >= nBytes)
			{
				Queue_PopData(pDataQ, pAudio, nBytes);
				//   Call data processing
				pModule->Process(NULL, pAudioIn, pAudioOut, nBytes);
				//   Distribute output data to all output data sinks (USB, I2S, etc)
				Data_Distribute(&osParams, pAudio, nBytes);
			}
		}	
	}
}

