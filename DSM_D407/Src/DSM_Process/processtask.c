#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"
#include "dataqueues.h"

#include "usb_device.h"
#include "usbd_audio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"


extern DataProcessBlock_t  MELP;
extern DataProcessBlock_t  MELPE;
extern DataProcessBlock_t  CVSD;
extern DataProcessBlock_t  CODEC;
extern DataProcessBlock_t  ALAW;
extern DataProcessBlock_t  ULAW;

extern DataProcessBlock_t  BYPASS;

extern DataProcessBlock_t  US_16_48;
extern DataProcessBlock_t  DS_48_16;
extern DataProcessBlock_t  US_8_48;
extern DataProcessBlock_t  DS_48_8;
extern DataProcessBlock_t  US_8_48_Q15;
extern DataProcessBlock_t  DS_48_8_Q15;

//
//  Task to handle all incoming data
//

DataProcessBlock_t  *pProcModule = 	&ALAW;
DataProcessBlock_t  *pDecModule = 	&DS_48_8;
DataProcessBlock_t  *pIntModule = 	&US_8_48;

void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQ;

	uint8_t 	*pAudio;
	float		*pAudioIn;
	float		*pAudioOut;
	void		*pProcModuleState;
	void		*pDecState;
	void		*pIntState;

	uint32_t	Type;
	uint32_t 	nSamplesInQueue, nSamplesModuleNeeds, nSamplesModuleGenerated;

	// Allocate static data buffers
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);
	pAudioIn = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));
	pAudioOut = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));

	// Create the processing modules
	pProcModuleState = pProcModule->Create(0);
	pDecState = pDecModule->Create(0);
	pIntState = pIntModule->Create(0);

	// Initialize processing modules
	pProcModule->Init(pProcModuleState);
	pDecModule->Init(pDecState);
	pIntModule->Init(pIntState);

	while(1)
	{
		event = osMessageGet(osParams.dataReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Message came that some valid Input Data is present
		{
			pDataQ = (DQueue_t *) event.value.p;

			// First, downsample, if neccessary, the received signal
			nSamplesInQueue = Queue_Count_Elems(pDataQ);
			nSamplesModuleNeeds = pDecModule->TypeSize(pDecState, &Type);
			if(nSamplesInQueue >= nSamplesModuleNeeds)
			{
				Queue_PopData(pDataQ, pAudio, nSamplesModuleNeeds * pDataQ->ElemSize);
				// Convert data from the Queue-provided type to the Processing-Module-required type
				DataConvert(pAudioIn, Type, DATA_CHANNEL_ALL, pAudio, pDataQ->Type, DATA_CHANNEL_ANY, nSamplesModuleNeeds);
				//   Call data processing
				pDecModule->Process(pDecState, pAudioIn, pAudioOut, &nSamplesModuleNeeds, &nSamplesModuleGenerated);
				// Convert data from the Processing-Module-provided type to the HW Queue type
				DataConvert(pAudio, osParams.DownSample_data->Type, DATA_CHANNEL_1 , pAudioOut, Type, DATA_CHANNEL_1, nSamplesModuleGenerated);
				// Place the processed data into the queue for the next module to process
				Queue_PushData(osParams.DownSample_data, pAudio, nSamplesModuleGenerated * osParams.DownSample_data->ElemSize);

				nSamplesInQueue = Queue_Count_Elems(pDataQ);
				nSamplesModuleNeeds = pDecModule->TypeSize(pDecState, &Type);
			}

			// Second, do the data processing
			nSamplesInQueue = Queue_Count_Elems(osParams.DownSample_data);
			nSamplesModuleNeeds = pProcModule->TypeSize(pProcModuleState, &Type);
			if(nSamplesInQueue >= nSamplesModuleNeeds)
			{
				Queue_PopData(osParams.DownSample_data, pAudio, nSamplesModuleNeeds * osParams.DownSample_data->ElemSize);
				// Convert data from the Queue-provided type to the Processing-Module-required type
				DataConvert(pAudioIn, Type, DATA_CHANNEL_1, pAudio, osParams.DownSample_data->Type, DATA_CHANNEL_1, nSamplesModuleNeeds);
				//   Call data processing
				 pProcModule->Process(pProcModuleState, pAudioIn, pAudioOut, &nSamplesModuleNeeds, &nSamplesModuleGenerated);
				// Convert data from the Processing-Module-provided type to the HW Queue type
				DataConvert(pAudio, osParams.UpSample_data->Type, DATA_CHANNEL_1 , pAudioOut, Type, DATA_CHANNEL_1, nSamplesModuleGenerated);
				// Place the processed data into the queue for the next module to process
				Queue_PushData(osParams.UpSample_data, pAudio, nSamplesModuleGenerated * osParams.UpSample_data->ElemSize);

				nSamplesInQueue = Queue_Count_Elems(osParams.DownSample_data);
				nSamplesModuleNeeds = pProcModule->TypeSize(pProcModuleState, &Type);
			}

			// Third, upsample and distribute to the output channels
			nSamplesInQueue = Queue_Count_Elems(osParams.UpSample_data);
			nSamplesModuleNeeds = pIntModule->TypeSize(pIntState, &Type);
			if(nSamplesInQueue >= nSamplesModuleNeeds)
			{
				Queue_PopData(osParams.UpSample_data, pAudio, nSamplesModuleNeeds * osParams.UpSample_data->ElemSize);
				// Convert data from the Queue-provided type to the Processing-Module-required type
				DataConvert(pAudioIn, Type, DATA_CHANNEL_1, pAudio, osParams.UpSample_data->Type, DATA_CHANNEL_1, nSamplesModuleNeeds);
				//   Call data processing
				pIntModule->Process(pIntState, pAudioIn, pAudioOut, &nSamplesModuleNeeds, &nSamplesModuleGenerated);
				// Convert data from the Processing-Module-provided type to the HW Queue type
				DataConvert(pAudio, osParams.PCM_Out_data->Type, DATA_CHANNEL_ALL , pAudioOut, Type, DATA_CHANNEL_ANY, nSamplesModuleGenerated);
				//   Distribute output data to all output data sinks (USB, I2S, etc)
				Queue_PushData(osParams.PCM_Out_data, pAudio, nSamplesModuleGenerated * osParams.PCM_Out_data->ElemSize);
				Queue_PushData(osParams.USB_In_data, pAudio, nSamplesModuleGenerated * osParams.USB_In_data->ElemSize);
				nSamplesInQueue = Queue_Count_Elems(osParams.UpSample_data);
				nSamplesModuleNeeds = pIntModule->TypeSize(pIntState, &Type);
			}
			// Check if we have to start playing audio thru external codec
			if(osParams.bStartPlay  && ( Queue_Count_Bytes(osParams.PCM_Out_data) >= (osParams.PCM_Out_data->nSize /2) ))
			{
				Queue_PopData(osParams.PCM_Out_data, osParams.pPCM_Out, NUM_PCM_BYTES);
				BSP_AUDIO_OUT_Play((uint16_t *)osParams.pPCM_Out, NUM_PCM_BYTES);
				osParams.bStartPlay = 0;
			}
		}
	}
}

