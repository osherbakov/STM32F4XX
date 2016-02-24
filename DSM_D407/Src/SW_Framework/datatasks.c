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
#include "NRF24L01func.h"


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

static uint8_t	 _Tx[32] = {1,2,3,4,5,6,7,8,9,10,0x55, 0xF9, 0xAF, 0x12, 0x55, 0xAA};
static uint8_t	 _Rx[32];
static uint8_t	 TxAddress[] = "1Node";
static uint8_t	 TxChannel = 0;
static int		 RxMode = 1;

static void StartRF24(void)
{
	RF24_Init();
	RF24_setAddressWidth(5);
	RF24_setDynamicPayload(0);
	RF24_setAckPayload(0);
	RF24_setAutoAckAll(0);
	RF24_setDataRate(RF24_250KBPS);
	RF24_openReadingPipe(0, TxAddress, 16);	
	RF24_openWritingPipe(TxAddress);		

	if (RxMode) 
		RF24_startListening();
	else 
		RF24_stopListening();		
			
}

void ProcessRF24(void)
{
	if(RxMode == 0){
		//			RF24_stopListeningFast();		
		// RF24_setChannel(TxChannel++);
		RF24_writeFast(_Tx, 16);
		//			RxMode = 1;
	}else{
		//			RF24_startListeningFast();
		//			RxMode = 0;
	}		
}

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
	
	StartRF24();
	
	
	// Start collecting PDM data (double-buffered) into alocated buffer with circular DMA 
	BSP_AUDIO_IN_Record((uint16_t *)osParams.pPDM_In, (NUM_PDM_BYTES * 2));
		
	while(1)
	{	// Wait for the message (sent by ISR) that the buffer is filled and ready to be processed
		event = osMessageGet(osParams.dataInPDMMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Valid Data is present in the queue
		{
			pInputBuffer = &osParams.pPDM_In[NUM_PDM_BYTES * event.value.v];
			// Call BSP-provided function to convert PDM data from the microphone to normal PCM data
			BSP_AUDIO_IN_PDMToPCM((uint16_t *)pInputBuffer, (uint16_t *)pPCM);
			Queue_PushData(osParams.PCM_In_data, pPCM, NUM_PCM_BYTES);
			// Report converted samples to the main data processing task
			osMessagePut(osParams.dataReadyMsg, (uint32_t)osParams.PCM_In_data, 0);
			
			ProcessRF24();
		}
	}
}

