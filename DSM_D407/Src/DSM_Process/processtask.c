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

DataProcessBlock_t  *pDecModule = 	&DS_48_8;
DataProcessBlock_t  *pProcModule = 	&ULAW;
DataProcessBlock_t  *pIntModule = 	&US_8_48;


DataProcessBlock_t  *pSyncModule = 	&RATESYNC_S;

void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQIn;

	void		*pProcModuleState;
	void		*pDecState;
	void		*pIntState;
	void		*pRSyncState;

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
		pDataQIn = (DQueue_t *) event.value.p;

		// Check if we have to start processing, or wait for more samples to accumulate (up to 1/2 of the buffer)
		if( pDataQIn->isReady ) // Message came that some valid Input Data is present
		{
			do {
				DoMoreProcessing = 0;
				// Rate-sync the input 48Ksamples/sec signal
				DoMoreProcessing += DoProcessing(pDataQIn, pSyncModule, pRSyncState, osParams.RateSyncQ);
				// Downsample, if neccessary, the received signal
				DoMoreProcessing += DoProcessing(osParams.RateSyncQ, pDecModule, pDecState, osParams.DownSampleQ);
				// Do the data processing
				DoMoreProcessing += DoProcessing(osParams.DownSampleQ, pProcModule, pProcModuleState, osParams.UpSampleQ);
				// Upsample and distribute to the output channels
				DoMoreProcessing += DoProcessing(osParams.UpSampleQ, pIntModule, pIntState, osParams.PCM_OutQ);
			}while(DoMoreProcessing);
		}
	}
}

