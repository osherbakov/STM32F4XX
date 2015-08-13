#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"

#include "usb_device.h"
#include "usbd_audio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"


void FF_Distribute(osObjects_t *hFF, void *pData, uint32_t nBytes)
{
	if( hFF->audiooutMode & AUDIO_MODE_OUT_I2S)
	{
		// Move the input audio samples into I2S output queue
		Queue_PushData(&hFF->PCM_Out_data, pData, nBytes);
		
		// Check if we have to start playing audio thru external codec
		if(hFF->bStartPlay  && ( Queue_Count(&hFF->PCM_Out_data) >= (hFF->PCM_Out_data.nSize /2) ))
		{
			Queue_PopData(&hFF->PCM_Out_data, hFF->pPCM_Out, NUM_PCM_BYTES);
			BSP_AUDIO_OUT_Play((uint16_t *)hFF->pPCM_Out, NUM_PCM_BYTES);					
			hFF->bStartPlay = 0;
		}
	}
	//if( hFF->audiooutMode & AUDIO_MODE_OUT_USB)
	{
		// Move the input audio samples into USB output queue
		// Note: Do not worry why it is called USB_In - in USB world
		//    everything that receives data from the HOST (PC) is always called OUT,
		//      and everything that sends data to the PC (HOST) is called/prefixed IN!!!!
		Queue_PushData(&osParams.USB_In_data, pData, nBytes);
	}
}

void Convert_S16_to_F32(int16_t *pS16Data, float *pF32Data, uint32_t S16Offset, uint32_t S16Stride, uint32_t nElems)
{
		pS16Data = (int16_t *) (((uint32_t)pS16Data)  + S16Offset);
		while(nElems--)
		{
			 *pF32Data++ = ((float)*pS16Data);
			 pS16Data = (int16_t *) (((uint32_t)pS16Data)  + S16Stride);
		}
}

void Convert_F32_to_S16(float *pF32Data, int16_t *pS16Data, uint32_t S16Offset, uint32_t S16Stride, uint32_t nElems)
{
		pS16Data = (int16_t *) (((uint32_t)pS16Data)  + S16Offset);
		while(nElems--)
		{
			 *pS16Data = (int16_t) (*pF32Data++);
			 pS16Data = (int16_t *) (((uint32_t)pS16Data)  + S16Stride);
		}
}

//
//  Task to handle all incoming data
//
void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	BuffBlk_t *pDataQ;

	uint8_t		*pAudio;
	float			*pAudioInLeft, *pAudioInRight; 
	float			*pAudioOutLeft, *pAudioOutRight; 


	float			*AudioInF32[2];
	float			*AudioOutF32[2];
	
	pAudio 	= osAlloc(AUDIO_SIZE_BYTES);

	pAudioInLeft = osAlloc(AUDIO_BLOCK_SAMPLES * sizeof(float));
	pAudioInRight = osAlloc(AUDIO_BLOCK_SAMPLES * sizeof(float));
	pAudioOutLeft = osAlloc(AUDIO_BLOCK_SAMPLES * sizeof(float));
	pAudioOutRight = osAlloc(AUDIO_BLOCK_SAMPLES * sizeof(float));
	AudioInF32[0] = pAudioInLeft;
	AudioInF32[1] = pAudioInRight;
	AudioOutF32[0] = pAudioOutLeft;
	AudioOutF32[1] = pAudioOutRight;

	while(1)
	{	
		event = osMessageGet(osParams.dataReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Valid Input Data is present
		{
			pDataQ = (BuffBlk_t *) event.value.p;
			// - Figure out what type of data is it
			// - If there is enough data accumulated - place in appropriate buffer and 
			//   call the appropriate processing function
			if(pDataQ->Type == AUDIO_STEREO_Q15)
			{
				while(Queue_Count(pDataQ) >= AUDIO_SIZE_BYTES)
				{
					Queue_PopData(pDataQ, pAudio, AUDIO_SIZE_BYTES);
					
					//   Call Feed Forward data processing
					Convert_S16_to_F32((int16_t *)pAudio, pAudioInLeft, 0, 4, AUDIO_BLOCK_SAMPLES);
					Convert_S16_to_F32((int16_t *)pAudio, pAudioInRight, 2, 4, AUDIO_BLOCK_SAMPLES);
					Data_Process(&osParams, AudioInF32, AudioOutF32, AUDIO_BLOCK_SAMPLES);
					//   Distribute output data to all output data sinks (USB, I2S, etc)
					Convert_F32_to_S16(pAudioOutLeft, (int16_t *)pAudio, 0, 4, AUDIO_BLOCK_SAMPLES);
					Convert_F32_to_S16(pAudioOutRight, (int16_t *)pAudio, 2, 4, AUDIO_BLOCK_SAMPLES);
					FF_Distribute(&osParams, pAudio, AUDIO_SIZE_BYTES);
				}
			}
		}	
	}
}

