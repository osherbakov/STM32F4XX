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


uint32_t	ProcUnderrun, ProcOverrun;

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

	int32_t		DoProcessing;
	uint32_t 	nBytesIn, nBytesOut, nBytesModuleNeeds, nBytesModuleGenerated;

	
	// Allocate static data buffers
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);
	pAudioIn = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));
	pAudioOut = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));


	DATA_Total = DATA_InOut = DATA_In = DATA_Out = 0;
	ProcUnderrun =  ProcOverrun = 0;

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
		if(osParams.bStartPlay && (Queue_Count_Bytes(osParams.PCM_Out_data) >= osParams.PCM_Out_data->Size/2))
		{
			Queue_PopData(osParams.PCM_Out_data, osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			BSP_AUDIO_OUT_Play((uint16_t *)osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			osParams.bStartPlay = 0;
		}
		// Check if we have to start processing, or wait for more samples to accumulate (up to 1/2 of the buffer)
		pDataQ = (DQueue_t *) event.value.p;
		if((osParams.ProcessingState == WAITING_FOR_BUFF) && (Queue_Count_Bytes(pDataQ) >= pDataQ->Size/2)) 
		{
			osParams.ProcessingState = RUNNING;
		}
		
		// Wait for the next IN Audio data packet to arrive (either from I2S MIC, or from the USB)
		event = osMessageGet(osParams.dataInReadyMsg, osWaitForever);
		if( (event.status == osEventMessage) && (osParams.ProcessingState == RUNNING) ) // Message came that some valid Input Data is present
		{
#if 0			
			nBytesIn = Queue_Count_Bytes(pDataQ);
			pSyncModule->Ready(pRSIn, &DataIn);
			nBytesModuleNeeds = DataIn.Size;
			if(nBytesIn >= nBytesModuleNeeds) 
			{
				Queue_PopData(pDataQ, pAudioIn, nBytesModuleNeeds);
				nBytesIn = nBytesModuleNeeds;
				pSyncModule->Process(pRSIn, pAudioIn, pAudioOut, &nBytesIn, &nBytesModuleGenerated);

				nBytesModuleNeeds = nBytesIn = nBytesModuleGenerated;
				pSyncModule->Process(pRSOut, pAudioOut, pAudioIn, &nBytesIn, &nBytesModuleGenerated);
				
				if(Queue_Space_Bytes(osParams.PCM_Out_data) < nBytesModuleGenerated) {
					ProcOverrun++;
				}
				Queue_PushData(osParams.PCM_Out_data, pAudioIn, nBytesModuleGenerated);
				DATA_Total = Queue_Count_Elems(osParams.PCM_Out_data) + Queue_Count_Elems(pDataQ);
			}
#endif			
			do {
				DoProcessing = 0;
				// First, downsample, if neccessary, the received signal
				nBytesIn = Queue_Count_Bytes(pDataQ);
				pDecModule->Ready(pDecState, &DataIn); nBytesModuleNeeds = DataIn.Size;
				if(nBytesIn >= nBytesModuleNeeds)
				{
					Queue_PopData(pDataQ, pAudio, nBytesModuleNeeds);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvertOut(pAudioIn, DataIn.Type, DATA_CHANNEL_ALL, pAudio, pDataQ->Type, DATA_CHANNEL_ANY, nBytesModuleNeeds);
					//   Call data processing
					pDecModule->Process(pDecState, pAudioIn, pAudioOut, &nSamplesModuleNeeds, &nSamplesModuleGenerated);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio, osParams.DownSample_data->Type, DATA_CHANNEL_1 , pAudioOut, Type, DATA_CHANNEL_1, nSamplesModuleGenerated);
					// Place the processed data into the queue for the next module to process
					if(Queue_Space_Bytes(osParams.DownSample_data) < nSamplesModuleGenerated * osParams.DownSample_data->Data.ElemSize) {
						PROC_Overruns++;
					}else {
						Queue_PushData(osParams.DownSample_data, pAudio, nSamplesModuleGenerated * osParams.DownSample_data->Data.ElemSize);
						DoProcessing = 1;
					}
				}

				// Second, do the data processing
				nSamplesInQueue = Queue_Count_Elems(osParams.DownSample_data);
				nSamplesModuleNeeds = pProcModule->TypeSize(pProcModuleState, &Type);
				if(nSamplesInQueue >= nSamplesModuleNeeds)
				{
					Queue_PopData(osParams.DownSample_data, pAudio, nSamplesModuleNeeds * osParams.DownSample_data->Data.ElemSize);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudioIn, Type, DATA_CHANNEL_1, pAudio, osParams.DownSample_data->Data.Type, DATA_CHANNEL_1, nSamplesModuleNeeds);
					//   Call data processing
					pProcModule->Process(pProcModuleState, pAudioIn, pAudioOut, &nSamplesModuleNeeds, &nSamplesModuleGenerated);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio, osParams.UpSample_data->Data.Type, DATA_CHANNEL_1 , pAudioOut, Type, DATA_CHANNEL_1, nSamplesModuleGenerated);

					// Place the processed data into the queue for the next module to process
					if(Queue_Space_Bytes(osParams.UpSample_data) < nSamplesModuleGenerated * osParams.UpSample_data->Data.ElemSize) {
						PROC_Overruns++;
					}else {
						Queue_PushData(osParams.UpSample_data, pAudio, nSamplesModuleGenerated * osParams.UpSample_data->Data.ElemSize);
						DoProcessing = 1;
					}
				}

				// Third, upsample and distribute to the output channels
				nSamplesInQueue = Queue_Count_Elems(osParams.UpSample_data);
				nSamplesModuleNeeds = pIntModule->TypeSize(pIntState, &Type);
				if(nSamplesInQueue >= nSamplesModuleNeeds)
				{
					Queue_PopData(osParams.UpSample_data, pAudio, nSamplesModuleNeeds * osParams.UpSample_data->Data.ElemSize);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudioIn, Type, DATA_CHANNEL_1, pAudio, osParams.UpSample_data->Data.Type, DATA_CHANNEL_1, nSamplesModuleNeeds);
					//   Call data processing
					pIntModule->Process(pIntState, pAudioIn, pAudioOut, &nSamplesModuleNeeds, &nSamplesModuleGenerated);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio, osParams.PCM_Out_data->Data.Type, DATA_CHANNEL_ALL , pAudioOut, Type, DATA_CHANNEL_ANY, nSamplesModuleGenerated);

					//   Distribute output data to all output data sinks (USB, I2S, etc)
					if(Queue_Space_Bytes(osParams.PCM_Out_data) < nSamplesModuleGenerated * osParams.PCM_Out_data->Data.ElemSize) {
						PROC_Overruns++;
					}else {
						Queue_PushData(osParams.PCM_Out_data, pAudio, nSamplesModuleGenerated * osParams.PCM_Out_data->Data.ElemSize);
						DoProcessing = 1;
					}
					if(Queue_Space_Bytes(osParams.USB_In_data) < nSamplesModuleGenerated * osParams.USB_In_data->Data.ElemSize)  {
						PROC_Overruns++;
					} else {
						Queue_PushData(osParams.USB_In_data, pAudio, nSamplesModuleGenerated * osParams.USB_In_data->Data.ElemSize);
						DoProcessing = 1;
					}
				}
			}while(0);
			}
		}
	}
}

