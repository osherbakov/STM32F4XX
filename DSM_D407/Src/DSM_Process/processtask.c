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


int32_t	DATA_In, DATA_Out, DATA_InOut;

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

DataProcessBlock_t  *pDecModule = 	&DS_48_16;
DataProcessBlock_t  *pProcModule = 	&CVSD;
DataProcessBlock_t  *pIntModule = 	&US_16_48;


DataProcessBlock_t  *pSyncModule = 	&RATESYNC_S;

void			*pRSIn;
void			*pRSOut;

void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQIn, *pDataQOut;

	uint8_t 	*pAudio;
	float		*pAudioIn;
	float		*pAudioOut;
	void		*pProcModuleState;
	void		*pDecState;
	void		*pIntState;
	DataPort_t	DataIn, DataOut;

	uint32_t 	nBytesIn, nBytesNeeded, nBytesGenerated;
	uint32_t 	nElemsIn, nElemsNeeded;
	
	int			DoProcessing;

	
	// Allocate static data buffers
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);
	pAudioIn = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));
	pAudioOut = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));


	DATA_InOut = DATA_In = DATA_Out = 0;

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
	
	while(1)
	{
		// Check if we have to start playing audio thru external codec when we accumulate more than 1/2 of the buffer
		if(osParams.bStartPlay && (Queue_Count(osParams.PCM_OutQ) >= osParams.PCM_OutQ->Size/2))
		{
			Queue_Pop(osParams.PCM_OutQ, osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			BSP_AUDIO_OUT_Play((uint16_t *)osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			osParams.bStartPlay = 0;
		}
	
		// Wait for the next IN Audio data packet to arrive (either from I2S MIC, or from the USB)
		event = osMessageGet(osParams.dataInReadyMsg, osWaitForever);
		// Check if we have to start processing, or wait for more samples to accumulate (up to 1/2 of the buffer)
		pDataQIn = (DQueue_t *) event.value.p;
		if((osParams.ProcessingState == WAITING_FOR_BUFF) && (Queue_Count(pDataQIn) >= pDataQIn->Size/2)) 
		{
			osParams.ProcessingState = RUNNING;
		}
		if( osParams.ProcessingState == RUNNING ) // Message came that some valid Input Data is present
		{
#if 0
			pDataQIn = (DQueue_t *) event.value.p;
			pDataQOut = osParams.PCM_OutQ;
			nBytesIn = Queue_Count(pDataQ);
			pSyncModule->Info(pRSIn, &DataIn, &DataOut);
			nBytesNeeded = DataIn.Size;
			if(nBytesIn >= nBytesNeeded)
			{
				Queue_Pop(pDataQ, pAudioIn, nBytesNeeded);
				pSyncModule->Process(pRSIn, pAudioIn, pAudioOut, &nBytesNeeded, &nBytesGenerated);

				nBytesIn = nBytesGenerated;
				pSyncModule->Process(pRSOut, pAudioOut, pAudioIn, &nBytesIn, &nBytesGenerated);
				Queue_Push(pDataQOut, pAudioIn, nBytesGenerated);
			}
#else			
			do {
				DoProcessing = 0;
				// First, downsample, if neccessary, the received signal
				pDataQIn = (DQueue_t *) event.value.p;
				pDataQOut = osParams.DownSampleQ;
				nElemsIn = Queue_Count(pDataQIn)/pDataQIn->ElemSize;
				// Find out how many Elements we will need
				pDecModule->Info(pDecState, &DataIn, &DataOut); 
				nElemsNeeded = DataIn.Size/DataIn.ElemSize;
				if(nElemsIn >= nElemsNeeded)
				{
					nBytesNeeded = nElemsNeeded * pDataQIn->ElemSize;
					Queue_Pop(pDataQIn, pAudio, nBytesNeeded);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudio, pDataQIn->Type, DATA_CHANNEL_1, pAudioIn, DataIn.Type, DATA_CHANNEL_1, &nBytesNeeded, &nBytesGenerated);
					//   Call data processing
					pDecModule->Process(pDecState, pAudioIn, pAudioOut, &nBytesGenerated, &nBytesIn);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudioOut, DataOut.Type, DATA_CHANNEL_1, pAudio, pDataQOut->Type, DATA_CHANNEL_1, &nBytesIn, &nBytesGenerated);
					// Place the processed data into the queue for the next module to process
					Queue_Push(pDataQOut, pAudio, nBytesGenerated);
					DoProcessing = 1;
				}

				// Second, do the data processing
				pDataQIn = osParams.DownSampleQ;
				pDataQOut = osParams.UpSampleQ;
				
				nElemsIn = Queue_Count(pDataQIn)/pDataQIn->ElemSize;
				pProcModule->Info(pProcModuleState, &DataIn, &DataOut);
				nElemsNeeded = DataIn.Size/DataIn.ElemSize;
				if(nElemsIn >= nElemsNeeded)
				{
					nBytesNeeded = nElemsNeeded * pDataQIn->ElemSize;
					Queue_Pop(pDataQIn, pAudio, nBytesNeeded);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudio, pDataQIn->Type, DATA_CHANNEL_1, pAudioIn, DataIn.Type, DATA_CHANNEL_1, &nBytesNeeded, &nBytesGenerated);
					//   Call data processing
					pProcModule->Process(pProcModuleState, pAudioIn, pAudioOut, &nBytesGenerated, &nBytesIn);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudioOut, DataOut.Type, DATA_CHANNEL_1, pAudio, pDataQOut->Type, DATA_CHANNEL_1, &nBytesIn, &nBytesGenerated);
					// Place the processed data into the queue for the next module to process
					Queue_Push(pDataQOut, pAudio, nBytesGenerated);
					DoProcessing = 1;
				}

				// Third, upsample and distribute to the output channels
				pDataQIn = osParams.UpSampleQ;
				nElemsIn = Queue_Count(pDataQIn)/pDataQIn->ElemSize;
				pIntModule->Info(pIntState, &DataIn, &DataOut);
				nElemsNeeded = DataIn.Size/DataIn.ElemSize;
				if(nBytesIn >= nBytesNeeded)
				{
					nBytesNeeded = nElemsNeeded * pDataQIn->ElemSize;
					Queue_Pop(pDataQIn, pAudio, nBytesNeeded);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudio, pDataQIn->Type, DATA_CHANNEL_1, pAudioIn, DataIn.Type, DATA_CHANNEL_1, &nBytesNeeded, &nBytesGenerated);
					//   Call data processing
					pIntModule->Process(pIntState, pAudioIn, pAudioOut, &nBytesGenerated, &nBytesIn);
					nBytesNeeded =  nBytesIn;
					
					//   Distribute output data to all output data sinks (USB, I2S, etc)
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudioOut, DataOut.Type, DATA_CHANNEL_ANY, pAudio, osParams.PCM_OutQ->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
					Queue_Push(osParams.PCM_OutQ, pAudio, nBytesGenerated);

					nBytesIn = nBytesNeeded;
					DataConvert(pAudioOut, DataOut.Type, DATA_CHANNEL_ANY, pAudio, osParams.USB_InQ->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
					Queue_Push(osParams.USB_InQ, pAudio, nBytesGenerated);
					DoProcessing = 1;
				}

			}while(0);
#endif			
		}
	}
}

