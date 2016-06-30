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


// Allocate static ping-pong data buffers
static float	sAudio0[2 * MAX_AUDIO_SAMPLES] CCMRAM;
static float	sAudio1[2 * MAX_AUDIO_SAMPLES] CCMRAM;

static void		*pAudio0 = sAudio0;
static void		*pAudio1 = sAudio1;



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


int  DoProcessing(DQueue_t *pDataQIn, DataProcessBlock_t  *pModule, void *pModuleState, DQueue_t *pDataQOut) 
{
	DataPort_t	DataIn, DataOut;

	uint32_t 	nBytesIn, nBytesNeeded, nBytesGenerated;
	uint32_t 	nElemsIn, nElemsNeeded;
	
	int			DoMoreProcessing = 0;
	
	nElemsIn = Queue_Count(pDataQIn)/pDataQIn->ElemSize;
	// Find out how many Elements we will need
	pModule->Info(pModuleState, &DataIn, &DataOut); 
	nElemsNeeded = DataIn.Size/DataIn.ElemSize;
	if(nElemsIn >= nElemsNeeded)
	{
		nBytesNeeded = nElemsNeeded * pDataQIn->ElemSize;
		Queue_Pop(pDataQIn, pAudio0, nBytesNeeded);
		// Convert data from the Queue-provided type to the Processing-Module-required type  In->Out
		DataConvert(pAudio0, pDataQIn->Type, DATA_CHANNEL_1, pAudio1, DataIn.Type, DATA_CHANNEL_1, &nBytesNeeded, &nBytesGenerated);
		//   Call data processing     
		pModule->Process(pModuleState, pAudio1, pAudio0, &nBytesGenerated, &nBytesIn);
		DoMoreProcessing = nBytesIn;

//		while(pDataQOut) 
		{
			nBytesIn = DoMoreProcessing;
			// Convert data from the Processing-Module-provided type to the HW Queue type
			DataConvert(pAudio0, DataOut.Type, DATA_CHANNEL_1, pAudio1, pDataQOut->Type, DATA_CHANNEL_1, &nBytesIn, &nBytesGenerated);
			// Place the processed data into the queue for the next module to process
			Queue_Push(pDataQOut, pAudio1, nBytesGenerated);
			pDataQOut = pDataQOut->pNext;
		}
	}
	return DoMoreProcessing;
}

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
				
				// Rate-sync the input 48Ksamples/sec signal
				DoMoreProcessing += DoProcessing((DQueue_t *) event.value.p, pSyncModule, pRSyncState, osParams.RateSyncQ);

				// Downsample, if neccessary, the received signal
#if 0	
				DoMoreProcessing += DoProcessing(osParams.RateSyncQ, pDecModule, pDecState, osParams.DownSampleQ);
#else				
				pDataQIn = osParams.RateSyncQ;
				pDataQOut = osParams.DownSampleQ;

				nElemsIn = Queue_Count(pDataQIn)/pDataQIn->ElemSize;
				// Find out how many Elements we will need
				pDecModule->Info(pDecState, &DataIn, &DataOut); 
				nElemsNeeded = DataIn.Size/DataIn.ElemSize;
				if(nElemsIn >= nElemsNeeded)
				{
					nBytesNeeded = nElemsNeeded * pDataQIn->ElemSize;
					Queue_Pop(pDataQIn, pAudio0, nBytesNeeded);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudio0, pDataQIn->Type, DATA_CHANNEL_1, pAudio1, DataIn.Type, DATA_CHANNEL_1, &nBytesNeeded, &nBytesGenerated);
					//   Call data processing
					pDecModule->Process(pDecState, pAudio1, pAudio0, &nBytesGenerated, &nBytesIn);
					// Convert data from the Processing-Module-provided type to the HW Queue type
					DataConvert(pAudio0, DataOut.Type, DATA_CHANNEL_1, pAudio1, pDataQOut->Type, DATA_CHANNEL_1, &nBytesIn, &nBytesGenerated);
					// Place the processed data into the queue for the next module to process
					Queue_Push(pDataQOut, pAudio1, nBytesGenerated);
					DoMoreProcessing = 1;
				}
#endif
				// Do the data processing
				DoMoreProcessing += DoProcessing(osParams.DownSampleQ, pProcModule, pProcModuleState, osParams.UpSampleQ);

				// Upsample and distribute to the output channels
#if 1
				pIntModule->Info(pIntState, &DataIn, &DataOut);
				nBytesIn = DoProcessing(osParams.UpSampleQ, pIntModule, pIntState, osParams.USB_InQ);
				DataConvert(pAudio0, DataOut.Type, DATA_CHANNEL_ANY, pAudio1, osParams.PCM_OutQ->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
				Queue_Push(osParams.PCM_OutQ, pAudio1, nBytesGenerated);
#else				
				pDataQIn = osParams.UpSampleQ;
				pDataQOut = osParams.PCM_OutQ;

				nElemsIn = Queue_Count(pDataQIn)/pDataQIn->ElemSize;
				pIntModule->Info(pIntState, &DataIn, &DataOut);
				nElemsNeeded = DataIn.Size/DataIn.ElemSize;
				if(nBytesIn >= nBytesNeeded)
				{
					nBytesNeeded = nElemsNeeded * pDataQIn->ElemSize;
					Queue_Pop(pDataQIn, pAudio0, nBytesNeeded);
					// Convert data from the Queue-provided type to the Processing-Module-required type
					DataConvert(pAudio0, pDataQIn->Type, DATA_CHANNEL_1, pAudio1, DataIn.Type, DATA_CHANNEL_1, &nBytesNeeded, &nBytesGenerated);
					//   Call data processing
					pIntModule->Process(pIntState, pAudio1, pAudio0, &nBytesGenerated, &nBytesNeeded);
					
					//   Distribute output data to all output data sinks (USB, I2S, etc)
					// Convert data from the Processing-Module-provided type to the HW Queue type
					nBytesIn = nBytesNeeded;
					DataConvert(pAudio0, DataOut.Type, DATA_CHANNEL_ANY, pAudio1, osParams.PCM_OutQ->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
					Queue_Push(osParams.PCM_OutQ, pAudio1, nBytesGenerated);

					nBytesIn = nBytesNeeded;
					DataConvert(pAudio0, DataOut.Type, DATA_CHANNEL_ANY, pAudio1, osParams.USB_InQ->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
					Queue_Push(osParams.USB_InQ, pAudio1, nBytesGenerated);
					DoMoreProcessing = 1;
				}
#endif				

			}while(DoMoreProcessing);
		}
	}
}

