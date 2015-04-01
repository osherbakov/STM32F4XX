#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"

#include "usb_device.h"

#include "stm32f429i_discovery.h"
#include "usbd_audio_if.h"
#include "datatasks.h"



uint8_t *pPDM_Buffer;			// Global pointer to the PDM data buffer
uint16_t *pPCM_Samples;		// Global pointer to the output PCM data buffer
int bStartPDMPlay;				// Global flag to start play PCM Samples in sync with Input PDM or USB Data

osMessageQId dataInPDMMsg;	// Queue that is signaled when there is data in PDM buffer
osMessageQId dataInUSBMsg;	// Queue that is signaled when there is data in Audio USB buffer
osMessageQId dataInI2SMsg;	// Queue that is signaled when there is data in I2S buffer
osMessageQId dataOutI2SMsg;	// Queue that is signaled when the data in I2S buffer was sent out

//
// Input data Interrupts
//
//   First half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_HalfTransfer_CallBack()
{
		if(audioMode == AUDIO_MODE_MIC)
		{
			osMessagePut(dataInPDMMsg, (uint32_t)&pPDM_Buffer[0], 0);
		}
}


//   Second half of PDM input buffer was filled - start playback and
//  ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_TransferComplete_CallBack()
{
		if(audioMode == AUDIO_MODE_MIC)
		{
			if(bStartPDMPlay)
			{
//				BSP_AUDIO_OUT_Play(&pPCM_Samples[NUM_PCM_SAMPLES * NUM_CHANNELS], (NUM_PCM_SAMPLES * NUM_CHANNELS * AUDIODATA_SIZE));
				bStartPDMPlay = 0;
			}
			osMessagePut(dataInPDMMsg, (uint32_t)&pPDM_Buffer[NUM_PDM_BYTES], 0);
		}
}

//
//  Output data interrupts
//
// Full data buffer was sent out to the output device
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
		if(audioMode == AUDIO_MODE_USB)
		{
			USBD_AUDIO_Sync(&hUsbDeviceHS);
		}else if (audioMode == AUDIO_MODE_MIC)
		{
// 			BSP_AUDIO_OUT_ChangeBuffer(&pPCM_Samples[0], NUM_PCM_SAMPLES * NUM_CHANNELS * NUM_BUFFERS * AUDIODATA_SIZE);
		}
		osMessagePut(dataOutI2SMsg, (uint32_t)&pPDM_Buffer[0], 0);
}


//
//  Task to handle incoming PDM data
//
void StartDataInPDMTask(void const * argument)
{
	osEvent		event;
	uint8_t		*pInputBuffer;
	uint16_t	*pOutputBuffer;

	// Allocate the storage for PDM data
	pPDM_Buffer = (uint8_t *)osAlloc(NUM_PDM_BYTES * NUM_BUFFERS);

	// Start collecting PDM data into allocated buffer with circular DMA
//	BSP_AUDIO_IN_Record((uint16_t *)&pPDM_Buffer[0], (NUM_PDM_BYTES * NUM_BUFFERS));

	while(1)
	{	// Wait for the message (sent by ISR) that there is enough data to be processed
		event = osMessageGet(dataInPDMMsg, osWaitForever);
		pInputBuffer = event.value.p;
		if( (event.status == osEventMessage ) && pInputBuffer && pPCM_Samples) // Valid Data is present
		{
			pOutputBuffer = (pInputBuffer == pPDM_Buffer) ?
												&pPCM_Samples[0] : 		&pPCM_Samples[NUM_PCM_SAMPLES * NUM_CHANNELS];
//			BSP_AUDIO_IN_PDMToPCM((uint16_t *)pInputBuffer, pOutputBuffer);
		}
	}
}

//
// Task to handle output data
//
void StartDataOutI2STask(void const * argument)
{
	osEvent		event;
	uint16_t	*pOutputBuffer;

	// Allocate the storage for PCM Stereo data
	pPCM_Samples = (uint16_t *)osAlloc(NUM_PCM_SAMPLES * NUM_BUFFERS * NUM_CHANNELS * AUDIODATA_SIZE);
	while(1)
	{	// Wait for the message (sent by ISR) that there is enough data to be sent out
		event = osMessageGet(dataOutI2SMsg, osWaitForever);
		if( event.status == osEventMessage )
		{
		}
	}
}

void StartDataProcessTask(void const * argument)
{
}



