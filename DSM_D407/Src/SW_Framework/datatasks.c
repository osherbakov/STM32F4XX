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

static uint32_t	I2S_InPrev;
static uint32_t	I2S_OutPrev;
static uint32_t	CYCCNT;
	
uint32_t	I2S_InPeriod;
uint32_t	I2S_OutPeriod;


//
// Input data Interrupts / Interrupt Service Routines
//

//   First half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_HalfTransfer_CallBack()
{
	CYCCNT = DWT->CYCCNT;
	I2S_InPeriod = (CYCCNT - I2S_InPrev);
	I2S_InPrev = CYCCNT;
	osMessagePut(osParams.dataInPDMMsg, DONE_FIRST, 0);
}

//   Second half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_TransferComplete_CallBack()
{
	CYCCNT = DWT->CYCCNT;
	I2S_InPeriod = (CYCCNT - I2S_InPrev);
	I2S_InPrev = CYCCNT;
	osMessagePut(osParams.dataInPDMMsg, DONE_SECOND, 0);
}

//
//  Output data interrupts / Interrupt Service Routines
//

// Half of the  data buffer was sent out to the output device
//  populate the first half of the PCM_Out_data
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
	if (osParams.audiooutMode & AUDIO_MODE_OUT_I2S)
	{
		CYCCNT = DWT->CYCCNT;
		I2S_OutPeriod = (CYCCNT - I2S_OutPrev);
		I2S_OutPrev = CYCCNT;
		Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[0], NUM_PCM_BYTES);
	}
}


// Full data buffer was sent out to the output device
// Start sending the data from the beginning of the pre-loaded data
//   and populate the second half of the buffer
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
	if (osParams.audiooutMode & AUDIO_MODE_OUT_I2S)	// We are in PCM output mode
	{
		CYCCNT = DWT->CYCCNT;
		I2S_OutPeriod = (CYCCNT - I2S_OutPrev);
		I2S_OutPrev = CYCCNT;
		BSP_AUDIO_OUT_ChangeBuffer((uint16_t *)osParams.pPCM_Out, NUM_PCM_BYTES * 2);
		Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[NUM_PCM_BYTES], NUM_PCM_BYTES);
	}
}


//
//  Task to handle incoming PDM data (Mono) and convert it to PCM samples (Stereo)
//

void StartDataInPDMTask(void const * argument)
{
	osEvent		event;
	uint8_t		*pPCM;
	uint8_t		*pInputBuffer;
	int32_t		numBytes;
	
	// Allocate the storage for PDM data (double-buffered) and resulting storage for PCM
	osParams.pPDM_In  = (uint8_t *)osAlloc(NUM_PDM_BYTES * 2);
	pPCM = (uint8_t *)osAlloc(NUM_PCM_BYTES);
	
	
	// Start collecting PDM data (double-buffered) into alocated buffer with circular DMA 
	BSP_AUDIO_IN_Record((uint16_t *)osParams.pPDM_In, (NUM_PDM_BYTES * 2));
		
	while(1)
	{	// Wait for the message (sent by ISR) that the buffer is filled and ready to be processed
		event = osMessageGet(osParams.dataInPDMMsg, osWaitForever);
		if( 	(osParams.audioinMode == AUDIO_MODE_IN_MIC) &&	// We are in PDM microphone IN mode
						(event.status == osEventMessage)  ) // Valid Data is present in the queue
		{
			numBytes =  (event.value.v == DONE_FIRST ? 0 : 1) * NUM_PDM_BYTES;
			pInputBuffer = &osParams.pPDM_In[numBytes];
			// Call BSP-provided function to convert PDM data from the microphone to normal PCM data
			BSP_AUDIO_IN_PDMToPCM((uint16_t *)pInputBuffer, (uint16_t *)pPCM);
			Queue_PushData(osParams.PCM_In_data, pPCM, NUM_PCM_BYTES);
			// Report converted samples to the main data processing task
			osMessagePut(osParams.dataReadyMsg, (uint32_t)osParams.PCM_In_data, 0);
		}
	}
}

