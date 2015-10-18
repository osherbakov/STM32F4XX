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
#include "NRF24L01.h"

//
// Input data Interrupts / Interrupt Service Routines
//
//   First half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_HalfTransfer_CallBack()
{
		if(osParams.audioinMode == AUDIO_MODE_IN_MIC)
		{
			osMessagePut(osParams.dataInPDMMsg, DONE_FIRST, 0);
		}
}
//   Second half of PDM input buffer was filled - if that is the first buffer, then start playback. 
//  Ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_TransferComplete_CallBack()
{
	if(osParams.audioinMode == AUDIO_MODE_IN_MIC)
	{
		osMessagePut(osParams.dataInPDMMsg, DONE_SECOND, 0);
	}
}

//
//  Output data interrupts / Interrupt Service Routines
//


// Half of the  data buffer was sent out to the output device
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
	if (osParams.audiooutMode & AUDIO_MODE_OUT_I2S)
	{
		Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[0], NUM_PCM_BYTES);
	}
}


// Full data buffer was sent out to the output device
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
	if (osParams.audiooutMode & AUDIO_MODE_OUT_I2S)
	{
		BSP_AUDIO_OUT_ChangeBuffer((uint16_t *)osParams.pPCM_Out, NUM_PCM_BYTES * 2);
		Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[NUM_PCM_BYTES], NUM_PCM_BYTES);
	}
}


//
//  Task to handle incoming PDM data (Mono) and convert it to PCM samples (Stereo)
//

uint8_t	 SPI_Tx[16] = {1,2,3,4,5,6,7,8,9,10,0x55, 0xF9, 0xAF, 0x12, 0x55, 0xAA};
uint8_t	 SPI_Rx[16];

void StartDataInPDMTask(void const * argument)
{
	osEvent		event;
	uint8_t		*pPCM;
	uint8_t		*pInputBuffer;
	
	// Allocate the storage for PDM data (double-buffered) and resulting storage for PCM
	osParams.pPDM_In  = (uint8_t *)osAlloc(NUM_PDM_BYTES * 2);
	osParams.pPCM_Out = (uint8_t *)osAlloc(NUM_PCM_BYTES * 2);

	// Allocate Temporary buffer
	pPCM = (uint8_t *)osAlloc(NUM_PCM_BYTES);
	
	// Start collecting PDM data (double-buffered) into alocated buffer with circular DMA 
	BSP_AUDIO_IN_Record((uint16_t *)osParams.pPDM_In, (NUM_PDM_BYTES * 2));
	
	while(1)
	{	// Wait for the message (sent by ISR) that the buffer is filled and ready to be processed
		event = osMessageGet(osParams.dataInPDMMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Valid Data is present in the queue
		{
			pInputBuffer = &osParams.pPDM_In[NUM_PDM_BYTES * event.value.v];
			// Call BSP-provided function to convert PDM data from the microphone to normal PCM data
BSP_LED_On(LED6);
			BSP_AUDIO_IN_PDMToPCM((uint16_t *)pInputBuffer, (uint16_t *)pPCM);
			Queue_PushData(osParams.PCM_In_data, pPCM, NUM_PCM_BYTES);
BSP_LED_Off(LED6);
			

			RF24_Init();
//		NRF24L01_CE(1);
//		delayMicroseconds(123);
//		NRF24L01_CE(0);
//		delayMicroseconds(330);
//		NRF24L01_CE(1);
//		delayMicroseconds(40);
//		NRF24L01_CE(0);
//		delayMicroseconds(500);
//		NRF24L01_CE(0);
			
//		NRF24L01_Write(0x22, SPI_Tx,16);
//		NRF24L01_Write(0x66, SPI_Tx,16);
			// Report converted samples to the main data processing task
			osMessagePut(osParams.dataReadyMsg, (uint32_t)osParams.PCM_In_data, 0);
		}
	}
}

