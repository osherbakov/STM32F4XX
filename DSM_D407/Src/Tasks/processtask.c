#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"
#include "dataqueues.h"

//#include "usb_device.h"
//#include "usbd_audio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"

#ifdef _MSC_VER
#define CCMRAM
#define RODATA
#else
#define CCMRAM __attribute__((section (".ccmram"))) __attribute__((aligned(4)))
#define RODATA __attribute__((section (".rodata")))
#endif


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

//DataProcessBlock_t	BYPASS;
//DataProcessBlock_t	RATESYNC_S;

DataProcessBlock_t  *pDecModule = 	&BYPASS;
DataProcessBlock_t  *pProcModule = 	&BYPASS;
DataProcessBlock_t  *pIntModule = 	&BYPASS;


DataProcessBlock_t  *pSyncModule = 	&RATESYNC_S;



// Allocate static ping-pong data buffers
static float	sAudio0[2 * MAX_AUDIO_SAMPLES] CCMRAM;
static float	sAudio1[2 * MAX_AUDIO_SAMPLES] CCMRAM;

static void		*pAudio0 = sAudio0;
static void		*pAudio1 = sAudio1;

int  DoProcessing(DQueue_t *pDataQIn, DataProcessBlock_t  *pModule, void *pModuleState, DQueue_t *pDataQOut)
{
	DataPort_t	DataIn, DataOut;

	uint32_t 	nBytesIn, nBytesNeeded, nBytesGenerated;
	uint32_t 	nElemsIn, nElemsNeeded;
	uint32_t	srcChMask;

	int			DoMoreProcessing = 0;

	// Get the info anout processing Module - number of channels, data format for In and Out
	pModule->Info(pModuleState, &DataIn, &DataOut);

	// How many elements are in the queue
	nElemsIn = Queue_Count(pDataQIn)/DATA_ELEM_SIZE(pDataQIn->Type);
	// How many Elements we will need
	nElemsNeeded = DataIn.Size/DATA_ELEM_SIZE(DataIn.Type);
	if(nElemsIn >= nElemsNeeded)
	{
		// How many bytes we have to pop
		nBytesNeeded = nElemsNeeded * DATA_ELEM_SIZE(pDataQIn->Type);
		Queue_Pop(pDataQIn, pAudio0, nBytesNeeded);

		// Convert data from the Queue-provided type to the Processing-Module-required type  In->Out
		srcChMask = (DATA_TYPE_NUM_CHANNELS(pDataQIn->Type) == DATA_TYPE_NUM_CHANNELS(DataIn.Type)) ? DATA_CHANNEL_ALL : DATA_CHANNEL_ANY;
		DataConvert(pAudio0, pDataQIn->Type, srcChMask, pAudio1, DataIn.Type, DATA_CHANNEL_ALL, &nBytesNeeded, &nBytesGenerated);
		//   Call data processing
		pModule->Process(pModuleState, pAudio1, pAudio0, &nBytesGenerated, &nBytesIn);
		DoMoreProcessing = nBytesIn;

		while(pDataQOut)
		{
			nBytesIn = DoMoreProcessing;
			// Convert data from the Processing-Module-provided type to the HW Queue type
			srcChMask = (DATA_TYPE_NUM_CHANNELS(pDataQOut->Type) == DATA_TYPE_NUM_CHANNELS(DataOut.Type)) ? DATA_CHANNEL_ALL : DATA_CHANNEL_ANY;
			DataConvert(pAudio0, DataOut.Type, srcChMask, pAudio1, pDataQOut->Type, DATA_CHANNEL_ALL, &nBytesIn, &nBytesGenerated);
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

