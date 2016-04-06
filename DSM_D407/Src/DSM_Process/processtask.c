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
extern DataProcessBlock_t  RATESYNC;

extern DataProcessBlock_t  US_16_48;
extern DataProcessBlock_t  DS_48_16;
extern DataProcessBlock_t  US_8_48;
extern DataProcessBlock_t  DS_48_8;
extern DataProcessBlock_t  US_8_48_Q15;
extern DataProcessBlock_t  DS_48_8_Q15;



//
//  RATE_SYNC functionality module
//
#define  RATESYNC_DATA_TYPE			(DATA_TYPE_I16 | DATA_NUM_CH_2 | (4))
#define  RATESYNC_BLOCK_SIZE  	(SAMPLE_FREQ_KHZ)

typedef struct RateSyncData {
		uint32_t 	DataInPrev;
		uint32_t 	DataOutPrev;
		uint32_t 	DataInDiff;
		uint32_t 	DataOutDiff;
		int32_t		Diff;
		int32_t		SampleDiff;
		int32_t		BlockDiff;
}RateSyncData_t;

RateSyncData_t	rs;

void *ratesync_create(uint32_t Params)
{
	return &rs;
}

void ratesync_close(void *pHandle)
{
	return;
}

void ratesync_init(void *pHandle)
{
	rs.SampleDiff = 168000/48;
	rs.DataOutDiff = rs.DataInDiff = rs.BlockDiff = 168000;
	rs.Diff = rs.DataInPrev = rs.DataOutPrev = 0;
}

void InBlock() {
	if(rs.DataInPrev) {
	}
}

void ratesync_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
		RateSyncData_t	*pRS = (RateSyncData_t	*) pHandle;
	
		if(pRS->Diff >= pRS->SampleDiff ) {				// More data samples that we need - remove one
				memcpy(pDataOut, pDataIn, (*pInSamples - 1)* 4);
				*pOutSamples = *pInSamples - 1;
				pRS->Diff -= pRS->SampleDiff;
		}else if (pRS->Diff <= -pRS->SampleDiff) {	// We are lacking samples - add extra one
				memcpy(pDataOut, pDataIn, *pInSamples * 4);
				memcpy(pDataOut + *pInSamples * 4, pDataIn + (*pInSamples -1) * 4, 4);
				*pOutSamples = *pInSamples + 1;
				pRS->Diff += pRS->SampleDiff;
		}else	{
				memcpy(pDataOut, pDataIn, *pInSamples * 4);
				*pOutSamples = *pInSamples;
		}
		*pInSamples = 0;
}

uint32_t ratesync_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = RATESYNC_DATA_TYPE;
	 return RATESYNC_BLOCK_SIZE;
}

DataProcessBlock_t  RATESYNC = {ratesync_create, ratesync_init, ratesync_data_typesize, ratesync_process, ratesync_close};


//
//  Task to handle all incoming data
//

DataProcessBlock_t  *pProcModule = 	&BYPASS;
DataProcessBlock_t  *pDecModule = 	&BYPASS;
DataProcessBlock_t  *pIntModule = 	&BYPASS;
DataProcessBlock_t  *pSyncModule = 	&RATESYNC;

uint32_t	PROC_Underruns;
uint32_t	PROC_Overruns;


void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQ;

	uint8_t 	*pAudio;
	float			*pAudioIn;
	float			*pAudioOut;
	void			*pProcModuleState;
	void			*pDecState;
	void			*pIntState;
	void			*pSyncState;

	int32_t		DoProcessing;
	uint32_t	Type;
	uint32_t 	nSamplesInQueue, nSamplesModuleNeeds, nSamplesModuleGenerated;
	uint32_t	nBytes, nBytesIn, nBytesOut;

	
	// Allocate static data buffers
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);
	pAudioIn = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));
	pAudioOut = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));

	osParams.pPCM_Out = (uint8_t *)osAlloc(NUM_PCM_BYTES * 2);

	// Create the processing modules
	pProcModuleState = pProcModule->Create(0);
	pDecState = pDecModule->Create(0);
	pIntState = pIntModule->Create(0);
	pSyncState = pSyncModule->Create(0);

	// Initialize processing modules
	pProcModule->Init(pProcModuleState);
	pDecModule->Init(pDecState);
	pIntModule->Init(pIntState);
	pSyncModule->Init(pSyncState);

	PROC_Underruns = PROC_Overruns = 0;

	while(1)
	{
		// Check if we have to start playing audio thru external codec
		if(osParams.bStartPlay)
		{
			BSP_AUDIO_OUT_Play((uint16_t *)osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			osParams.bStartPlay = 0;
		}
		event = osMessageGet(osParams.dataInReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Message came that some valid Input Data is present
		{
			pDataQ = (DQueue_t *) event.value.p;

			nBytes = Queue_Count_Bytes(pDataQ);
			nBytesIn = pSyncModule->TypeSize(pSyncState, &Type);
			if(nBytes >= nBytesIn) {
				while(nBytes >= nBytesIn) {
					Queue_PopData(pDataQ, pAudio, nBytesIn);
					pSyncModule->Process(pSyncState, pAudio, pAudio, &nBytesIn, &nBytesOut);
				
					if( Queue_Space_Bytes(osParams.PCM_Out_data) >= nBytesOut) {
						Queue_PushData(osParams.PCM_Out_data, pAudio, nBytesOut);
					} else {
						PROC_Overruns++;
					}
					nBytes = Queue_Count_Bytes(pDataQ);
					nBytesIn = pSyncModule->TypeSize(pSyncState, &Type);
				}
			}else {
					PROC_Underruns++;
			}

#if 0
			do {
				DoProcessing = 0;
				// First, downsample, if neccessary, the received signal
				nSamplesInQueue = Queue_Count_Elems(pDataQ);
				nSamplesModuleNeeds = pDecModule->TypeSize(pDecState, &Type);
				if(nSamplesInQueue >= nSamplesModuleNeeds)
				{
					Queue_PopData(pDataQ, pAudio, nSamplesModuleNeeds * pDataQ->Data.ElemSize);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudioIn, Type, DATA_CHANNEL_ALL, pAudio, pDataQ->Data.Type, DATA_CHANNEL_ANY, nSamplesModuleNeeds);
					//   Call data processing
					pDecModule->Process(pDecState, pAudioIn, pAudioOut, &nSamplesModuleNeeds, &nSamplesModuleGenerated);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio, osParams.DownSample_data->Data.Type, DATA_CHANNEL_1 , pAudioOut, Type, DATA_CHANNEL_1, nSamplesModuleGenerated);
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
#endif
		}
	}
}

