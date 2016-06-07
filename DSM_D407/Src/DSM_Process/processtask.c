#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"
#include "dataqueues.h"

#include "usb_device.h"
#include "usbd_audio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"


#define ABS(a)			(((a) >  0) ? (a) : -(a))

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
extern DataProcessBlock_t  RATESYNC_S;


int32_t	DATA_In, DATA_Out, DATA_InOut, DATA_Total;

void InData(uint32_t nSamples) {
	DATA_In += nSamples;
	DATA_InOut = DATA_In - DATA_Out;
}


void OutData(uint32_t nSamples) {
	DATA_Out += nSamples;
	DATA_InOut = DATA_In - DATA_Out;
}

//
//  Task to handle all incoming data
//

DataProcessBlock_t  *pProcModule = 	&BYPASS;
DataProcessBlock_t  *pDecModule = 	&BYPASS;
DataProcessBlock_t  *pIntModule = 	&BYPASS;
DataProcessBlock_t  *pSyncModule = 	&RATESYNC_S;

void			*pRSIn;
void			*pRSOut;

typedef struct RateSyncData {
		int32_t		DeltaIn;			// The calculated Input difference between samples (CPU-tied)
		int32_t		DeltaOut;			// The calculated Output difference between samples (CPU-tied)
		uint32_t	Delay;				// The delay amount (how Output samples are delayed relative to Input)
		int32_t		AddRemoveCnt;	// The counter of Added (positive) or Removed (negative) samples 
		float		States[3];		// States memory
}RateSyncData_t;

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
	DataPort_t	DataIn;

	uint32_t 	nBytesIn, nBytesModuleNeeds, nBytesModuleGenerated;
	uint32_t 	nSamplesModuleNeeds;

	
	// Allocate static data buffers
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);
	pAudioIn = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));
	pAudioOut = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));


	DATA_Total = DATA_InOut = DATA_In = DATA_Out = 0;

	// Create the processing modules
	pProcModuleState = pProcModule->Create(0);
	pDecState = pDecModule->Create(0);
	pIntState = pIntModule->Create(0);
	pRSIn = pSyncModule->Create(0);
	pRSOut = pSyncModule->Create(0);

	// Initialize processing modules
	pProcModule->Init(pProcModuleState);
	pDecModule->Init(pDecState);
	pIntModule->Init(pIntState);
	pSyncModule->Init(pRSIn);
	pSyncModule->Init(pRSOut);
	
	((RateSyncData_t *)pRSIn)->DeltaIn = 1;
	((RateSyncData_t *)pRSIn)->DeltaOut = 1;
	((RateSyncData_t *)pRSOut)->DeltaIn = 1;
	((RateSyncData_t *)pRSOut)->DeltaOut = 1;

	while(1)
	{
		// Check if we have to start playing audio thru external codec when we accumulate more than 1/2 of the buffer
		if(osParams.bStartPlay && (Queue_Count(osParams.PCM_Out_data) >= osParams.PCM_Out_data->Size/2))
		{
			Queue_Pop(osParams.PCM_Out_data, osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			BSP_AUDIO_OUT_Play((uint16_t *)osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			osParams.bStartPlay = 0;
		}
	
		// Wait for the next IN Audio data packet to arrive (either from I2S MIC, or from the USB)
		event = osMessageGet(osParams.dataInReadyMsg, osWaitForever);
		// Check if we have to start processing, or wait for more samples to accumulate (up to 1/2 of the buffer)
		pDataQ = (DQueue_t *) event.value.p;
		if((osParams.ProcessingState == WAITING_FOR_BUFF) && (Queue_Count(pDataQ) >= pDataQ->Size/2)) 
		{
			osParams.ProcessingState = RUNNING;
		}
		if( osParams.ProcessingState == RUNNING ) // Message came that some valid Input Data is present
		{
#if 1
			nBytesIn = Queue_Count(pDataQ);
			pSyncModule->Ready(pRSIn, &DataIn);
			nBytesModuleNeeds = DataIn.Size;
			if(nBytesIn >= nBytesModuleNeeds) 
			{
				Queue_Pop(pDataQ, pAudioIn, nBytesModuleNeeds);
				pSyncModule->Process(pRSIn, pAudioIn, pAudioOut, &nBytesModuleNeeds, &nBytesModuleGenerated);

				nBytesIn = nBytesModuleGenerated;
				pSyncModule->Process(pRSOut, pAudioOut, pAudioIn, &nBytesIn, &nBytesModuleGenerated);
				Queue_Push(osParams.PCM_Out_data, pAudioIn, nBytesModuleGenerated);
				DATA_Total = Queue_Count(osParams.PCM_Out_data) + Queue_Count(pDataQ);
			}
#else			
			do {
				// First, downsample, if neccessary, the received signal
				nBytesIn = Queue_Count(pDataQ);
				pDecModule->Ready(pDecState, &DataIn); 
				nBytesModuleNeeds = DataIn.Size;
				nSamplesModuleNeeds = nBytesModuleNeeds/DataIn.ElemSize;
				if(nBytesIn >= nBytesModuleNeeds)
				{
					Queue_Pop(pDataQ, pAudio, nBytesModuleNeeds);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudioIn, DataIn.Type, DATA_CHANNEL_ALL, pAudio, pDataQ->Type, DATA_CHANNEL_ANY, nSamplesModuleNeeds);
					//   Call data processing
					pDecModule->Process(pDecState, pAudioIn, pAudioOut, &nBytesModuleNeeds, &nBytesModuleGenerated);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio, osParams.DownSample_data->Type, DATA_CHANNEL_1 , pAudioOut, DataIn.Type, DATA_CHANNEL_1, nBytesModuleGenerated/DataIn.ElemSize);
					// Place the processed data into the queue for the next module to process
					Queue_Push(osParams.DownSample_data, pAudio, nBytesModuleGenerated);
				}

				// Second, do the data processing
				nBytesIn = Queue_Count(osParams.DownSample_data);
				pProcModule->Ready(pProcModuleState, &DataIn);
				nBytesModuleNeeds = DataIn.Size;
				nSamplesModuleNeeds = nBytesModuleNeeds/DataIn.ElemSize;
				if(nBytesIn >= nBytesModuleNeeds)
				{
					Queue_Pop(osParams.DownSample_data, pAudio, nBytesModuleNeeds);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudioIn, DataIn.Type, DATA_CHANNEL_1, pAudio, osParams.DownSample_data->Type, DATA_CHANNEL_1, nSamplesModuleNeeds);
					//   Call data processing
					pProcModule->Process(pProcModuleState, pAudioIn, pAudioOut, &nBytesModuleNeeds, &nBytesModuleGenerated);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio, osParams.UpSample_data->Type, DATA_CHANNEL_1 , pAudioOut, DataIn.Type, DATA_CHANNEL_1, nBytesModuleGenerated/DataIn.ElemSize);

					// Place the processed data into the queue for the next module to process
					Queue_Push(osParams.UpSample_data, pAudio, nBytesModuleGenerated);
				}

				// Third, upsample and distribute to the output channels
				nBytesIn = Queue_Count(osParams.UpSample_data);
				pIntModule->Ready(pIntState, &DataIn);
				nBytesModuleNeeds = DataIn.Size;
				nSamplesModuleNeeds = nBytesModuleNeeds/DataIn.ElemSize;
				if(nBytesIn >= nBytesModuleNeeds)
				{
					Queue_Pop(osParams.UpSample_data, pAudio, nBytesModuleNeeds);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudioIn, DataIn.Type, DATA_CHANNEL_1, pAudio, osParams.UpSample_data->Type, DATA_CHANNEL_1, nSamplesModuleNeeds);
					//   Call data processing
					pIntModule->Process(pIntState, pAudioIn, pAudioOut, &nBytesModuleNeeds, &nBytesModuleGenerated);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio, osParams.PCM_Out_data->Type, DATA_CHANNEL_ALL , pAudioOut, DataIn.Type, DATA_CHANNEL_ANY, nBytesModuleGenerated/DataIn.ElemSize);

					//   Distribute output data to all output data sinks (USB, I2S, etc)
					Queue_Push(osParams.PCM_Out_data, pAudio, nBytesModuleGenerated);
					Queue_Push(osParams.USB_In_data, pAudio, nBytesModuleGenerated);
				}
			}while(0);
#endif			
		}
	}
}

