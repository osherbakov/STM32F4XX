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
		if(hFF->bStartPlay  && ( Queue_Count_Bytes(hFF->PCM_Out_data) >= (hFF->PCM_Out_data->nSize /2) ))
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
		Queue_PushData(hFF->USB_In_data, pData, nBytes);
	}
}

extern DataProcessBlock_t  MELP;
extern DataProcessBlock_t  CVSD;
extern DataProcessBlock_t  CODEC2;
extern DataProcessBlock_t  BYPASS;


//
//  Task to handle all incoming data
//

DataProcessBlock_t  *pModule = &CODEC2;

void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQ;

	uint8_t 	*pAudio;
	float		*pAudioIn;
	float		*pAudioOut;
	void		*pModuleState;
	
	uint32_t	Type, nSamplesQueue, nSamplesModule;
	
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);	
	pAudioIn = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));	
	pAudioOut = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));	

	pModuleState = pModule->Create(0);
	pModule->Init(pModuleState);
	
	while(1)
	{	
		event = osMessageGet(osParams.dataReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Message came that some valid Input Data is present
		{
			pDataQ = (DQueue_t *) event.value.p;
			// - Figure out what type of data is it
			// - If there is enough data accumulated - place it in a buffer and 
			//      call the appropriate processing function
			nSamplesQueue = Queue_Count_Elems(pDataQ);
			nSamplesModule = pModule->TypeSize(&osParams, &Type); 
			
			while(nSamplesQueue >= nSamplesModule)
			{
				Queue_PopData(pDataQ, pAudio, nSamplesModule * pDataQ->ElemSize);
				
				// Convert data from the Queue-provided type to the Processing-Module-required type
				DataConvert(pAudioIn, Type, DATA_CHANNEL_ALL, pAudio, pDataQ->Type, DATA_CHANNEL_ANY, nSamplesModule);

				//   Call data processing
				pModule->Process(pModuleState, pAudioIn, pAudioOut, nSamplesModule);
				
				// Convert data from the Processing-Module-provided type to the HW Queue type
				DataConvert(pAudio, osParams.PCM_Out_data->Type, DATA_CHANNEL_ALL , pAudioOut, Type, DATA_CHANNEL_ANY, nSamplesModule);

				//   Distribute output data to all output data sinks (USB, I2S, etc)
				Data_Distribute(&osParams, pAudio, nSamplesModule * (osParams.PCM_Out_data->ElemSize));
				nSamplesQueue = Queue_Count_Elems(pDataQ);
			}
		}	
	}
}

