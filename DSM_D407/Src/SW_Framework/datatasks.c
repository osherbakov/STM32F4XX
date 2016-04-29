#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"

#include "usb_device.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"
#include "pdm_filter.h"
#include "usbd_audio_if.h"

#include "spi.h"
#include "NRF24L01func.h"

uint8_t		*pPCM0;
uint8_t		*pPCM1;
uint8_t		*pInputBuffer;

extern void InData(void *pHandle, uint32_t nSamples);
extern void OutData(void *pHandle, uint32_t nSamples);

//
// Input data Interrupts / Interrupt Service Routines
//

//   First half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_HalfTransfer_CallBack()
{
	if( osParams.audioInMode == AUDIO_MODE_IN_MIC) {	// We are in PDM microphone IN mode
			pInputBuffer = &osParams.pPDM_In[0];
InData(osParams.pRSIn, NUM_PCM_SAMPLES);
			// Call BSP-provided function to convert PDM data from the microphone to normal PCM data
			BSP_AUDIO_IN_PDMToPCM((uint16_t *)pInputBuffer, (uint16_t *)pPCM0);
			Queue_PushData(osParams.PCM_In_data, pPCM0, NUM_PCM_BYTES);
			// Report converted samples to the main data processing task
			osMessagePut(osParams.dataInReadyMsg, (uint32_t)osParams.PCM_In_data, 0);
	}
}

//   Second half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_TransferComplete_CallBack()
{
	if( osParams.audioInMode == AUDIO_MODE_IN_MIC) {	// We are in PDM microphone IN mode
			pInputBuffer = &osParams.pPDM_In[NUM_PDM_BYTES];
InData(osParams.pRSIn, NUM_PCM_SAMPLES);
			// Call BSP-provided function to convert PDM data from the microphone to normal PCM data
			BSP_AUDIO_IN_PDMToPCM((uint16_t *)pInputBuffer, (uint16_t *)pPCM1);
			Queue_PushData(osParams.PCM_In_data, pPCM1, NUM_PCM_BYTES);
			// Report converted samples to the main data processing task
			osMessagePut(osParams.dataInReadyMsg, (uint32_t)osParams.PCM_In_data, 0);
	}
}

//
//  Output data interrupts / Interrupt Service Routines
//

// Half of the  data buffer was sent out to the output device
//  populate the first half of the PCM_Out_data
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
		uint32_t	nBytes; 

OutData(osParams.pRSOut, NUM_PCM_SAMPLES);

		nBytes = Queue_Count_Bytes(osParams.PCM_Out_data);
		if(nBytes < NUM_PCM_BYTES){
			memset(&osParams.pPCM_Out[0], 0, 2 * NUM_PCM_BYTES);
		}else {
			Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[0], NUM_PCM_BYTES);
		}
}


// Full data buffer was sent out to the output device
// Start sending the data from the beginning of the pre-loaded data
//   and populate the second half of the buffer
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
		uint32_t	nBytes;

OutData(osParams.pRSOut, NUM_PCM_SAMPLES);
	
		nBytes = Queue_Count_Bytes(osParams.PCM_Out_data);
		if(nBytes < NUM_PCM_BYTES){
				memset(&osParams.pPCM_Out[0], 0, 2 * NUM_PCM_BYTES);
		}else {
			Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[NUM_PCM_BYTES], NUM_PCM_BYTES);
		}
}


//
//  Task to handle incoming PDM data (Mono) and convert it to PCM samples (Stereo)
//

void StartDataInPDMTask(void const * argument)
{
	// Allocate the storage for PDM data (double-buffered) and resulting storage for PCM
	osParams.pPDM_In  = (uint8_t *)osAlloc(NUM_PDM_BYTES * 2);
	pPCM0 = (uint8_t *)osAlloc(NUM_PCM_BYTES);
	pPCM1 = (uint8_t *)osAlloc(NUM_PCM_BYTES);

	// Start collecting PDM data (double-buffered) into alocated buffer with circular DMA 
	BSP_AUDIO_IN_Record((uint16_t *)osParams.pPDM_In, 2 * NUM_PDM_BYTES);
	
	while(1)
	{	// Wait for the message (sent by ISR) that the buffer is filled and ready to be processed
    osDelay(1000);
	}
}

