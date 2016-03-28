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
	
float						I2S_Period;

static int32_t	I2S_OutReady;
static int32_t	I2S_InReady;
uint32_t				I2S_OutCnt;
uint32_t				I2S_InCnt;

uint32_t	I2S_Underruns;
uint32_t	I2S_Overruns;

//
// Input data Interrupts / Interrupt Service Routines
//

//   First half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_HalfTransfer_CallBack()
{
	CYCCNT = DWT->CYCCNT;
	I2S_Period = I2S_Period * 0.99f + (CYCCNT - I2S_InPrev) * 0.01f;
	I2S_InPrev = CYCCNT;
	osMessagePut(osParams.dataInPDMMsg, DONE_FIRST, 0);
	I2S_InCnt++;
}

//   Second half of PDM input buffer was filled - ask the task to convert PDM -> PCM
void BSP_AUDIO_IN_TransferComplete_CallBack()
{
	CYCCNT = DWT->CYCCNT;
	I2S_Period = I2S_Period * 0.99f + (CYCCNT - I2S_InPrev) * 0.01f;
	I2S_InPrev = CYCCNT;
	osMessagePut(osParams.dataInPDMMsg, DONE_SECOND, 0);
	I2S_InCnt++;
}

//
//  Output data interrupts / Interrupt Service Routines
//

// Half of the  data buffer was sent out to the output device
//  populate the first half of the PCM_Out_data
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
		uint32_t	nBytes = Queue_Count_Bytes(osParams.PCM_Out_data);
		
		CYCCNT = DWT->CYCCNT;
		I2S_Period = I2S_Period * 0.99f + (CYCCNT - I2S_OutPrev) * 0.01f;		
		I2S_OutPrev = CYCCNT;

		if(I2S_OutReady) {
			if(nBytes < NUM_PCM_BYTES){
				memset(&osParams.pPCM_Out[0], 0, 2 * NUM_PCM_BYTES);
//				I2S_OutReady = 0;
				I2S_Underruns++;
			}else {
				Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[0], NUM_PCM_BYTES);
			}
		}else if( nBytes >= osParams.PCM_Out_data->Size/2) {
			I2S_OutReady = 1;
		}
		I2S_OutCnt++;
}


// Full data buffer was sent out to the output device
// Start sending the data from the beginning of the pre-loaded data
//   and populate the second half of the buffer
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
		uint32_t	nBytes = Queue_Count_Bytes(osParams.PCM_Out_data);

		CYCCNT = DWT->CYCCNT;
		I2S_Period = I2S_Period * 0.99f + (CYCCNT - I2S_OutPrev) * 0.01f;
		I2S_OutPrev = CYCCNT;

		BSP_AUDIO_OUT_ChangeBuffer((uint16_t *)osParams.pPCM_Out, 2 * NUM_PCM_BYTES);

		if(I2S_OutReady) {
			if(nBytes < NUM_PCM_BYTES){
				memset(&osParams.pPCM_Out[0], 0, 2 * NUM_PCM_BYTES);
//				I2S_OutReady = 0;
				I2S_Underruns++;
			}else {
				Queue_PopData(osParams.PCM_Out_data, &osParams.pPCM_Out[NUM_PCM_BYTES], NUM_PCM_BYTES);
			}
		}else if( nBytes >= osParams.PCM_Out_data->Size/2) {
			I2S_OutReady = 1;
		}
		I2S_OutCnt++;
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

	I2S_InCnt = I2S_OutCnt = I2S_InReady = I2S_OutReady = I2S_Underruns = I2S_Overruns = 1;
	
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
			if(Queue_Space_Bytes(osParams.PCM_In_data) < NUM_PCM_BYTES) {
//				I2S_InReady = 0;
				I2S_Overruns++;
			}else {
				Queue_PushData(osParams.PCM_In_data, pPCM, NUM_PCM_BYTES);
			}
			if(I2S_InReady) {
				// Report converted samples to the main data processing task
				osMessagePut(osParams.dataReadyMsg, (uint32_t)osParams.PCM_In_data, 0);
			}else	{
				if(Queue_Count_Bytes(osParams.PCM_In_data) >= osParams.PCM_In_data->Size/2) {
					I2S_InReady = 1;
				}
			}
		}
	}
}

