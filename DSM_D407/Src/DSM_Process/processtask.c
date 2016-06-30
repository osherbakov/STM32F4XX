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


// Allocate static data buffers
uint8_t 	sAudio[MAX_AUDIO_SIZE_BYTES] CCMRAM;
float		sAudioIn[MAX_AUDIO_SAMPLES] CCMRAM;
float		sAudioOut[MAX_AUDIO_SAMPLES] CCMRAM;

uint8_t 	*pAudio   = sAudio;
float		*pAudioIn = sAudioIn;
float		*pAudioOut = sAudioOut;



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

DataProcessBlock_t  *pDecModule = 	&DS_48_8;
DataProcessBlock_t  *pProcModule = 	&MELP;
DataProcessBlock_t  *pIntModule = 	&US_8_48;


DataProcessBlock_t  *pSyncModule = 	&RATESYNC_S;


void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQIn, *pDataQOut;

	void		*pProcModuleState;
	void		*pDecState;
	void		*pIntState;
	void		*pRSyncState;
	
	DataPort_t	DataIn, DataOut;

	uint32_t 	nBytesIn, nBytesNeeded, nBytesGenerated;
	uint32_t 	nElemsIn, nElemsNeeded;
	
	int			DoMoreProcessing;


	DATA_InOut = DATA_In = DATA_Out = 0;

	// Create the processing modules
	pProcModuleState = pProcModule->Create(0);
	pDecState = pDecModule->Create(0);
	pIntState = pIntModule->Create(0);
	pRSyncState = pSyncModule->Create(0);

	// Initialize processing modules
	pProcModule->Open(pProcModuleState, 0);
	pDecModule->Open(pDecState, 0);
	pIntModule->Open(pIntState, 0);
	pSyncModule->Open(pRSyncState, 0);
	
	while(1)
	{
		// Check if we have to start playing audio thru external codec when we accumulate more than 1/2 of the buffer
		if(osParams.bStartPlay && osParams.PCM_OutQ->isReady)
		{
			Queue_Pop(osParams.PCM_OutQ, osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			BSP_AUDIO_OUT_Play((uint16_t *)osParams.pPCM_Out, 2 * NUM_PCM_BYTES);
			osParams.bStartPlay = 0;
		}
	
		// Wait for the next IN Audio data packet to arrive (either from I2S MIC, or from the USB)
		event = osMessageGet(osParams.dataInReadyMsg, osWaitForever);
		// Check if we have to start processing, or wait for more samples to accumulate (up to 1/2 of the buffer)
		pDataQIn = (DQueue_t *) event.value.p;
		if( pDataQIn->isReady ) // Message came that some valid Input Data is present
		{
			do {
				DoMoreProcessing = 0;
				
				pDataQIn = (DQueue_t *) event.value.p;
				pDataQOut = osParams.RateSyncQ;
				nBytesIn = Queue_Count(pDataQIn);
				pSyncModule->Info(pRSyncState, &DataIn, &DataOut);
				nBytesNeeded = DataIn.Size;
				if(nBytesIn >= nBytesNeeded)
				{
					Queue_Pop(pDataQIn, pAudioIn, nBytesNeeded);
					nBytesIn = nBytesNeeded;
					pSyncModule->Process(pRSyncState, pAudioIn, pAudioOut, &nBytesIn, &nBytesGenerated);
					Queue_Push(pDataQOut, pAudioOut, nBytesGenerated);
					DoMoreProcessing = 1;
				}
				
				// First, downsample, if neccessary, the received signal
				pDataQIn = osParams.RateSyncQ;
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
					DoMoreProcessing = 1;
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
					DoMoreProcessing = 1;
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
					pIntModule->Process(pIntState, pAudioIn, pAudioOut, &nBytesGenerated, &nBytesNeeded);
					
					//   Distribute output data to all output data sinks (USB, I2S, etc)
					// Convert data from the Processing-Module-provided type to the HW Queue type
					nBytesIn = nBytesNeeded;
					DataConvert(pAudioOut, DataOut.Type, DATA_CHANNEL_ANY, pAudio, osParams.PCM_OutQ->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
					Queue_Push(osParams.PCM_OutQ, pAudio, nBytesGenerated);

					nBytesIn = nBytesNeeded;
					DataConvert(pAudioOut, DataOut.Type, DATA_CHANNEL_ANY, pAudio, osParams.USB_InQ->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
					Queue_Push(osParams.USB_InQ, pAudio, nBytesGenerated);
					DoMoreProcessing = 1;
				}

			}while(DoMoreProcessing);
		}
	}
}

