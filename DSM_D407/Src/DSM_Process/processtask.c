#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "datatasks.h"
#include "dataqueues.h"

#include "usb_device.h"
#include "usbd_audio.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio.h"

#define  Q31_F						((float)(1<<31))
#define	 Q31_INV_F				(1.0F / Q31_F)

#define  FLOAT_TO_Q31(a) 	( (int)(((float) (a)) * Q31_F))
#define  Q31_TO_FLOAT(a) 	( ((float) (a)) * Q31_INV_F)


void Data_Distribute(osObjects_t *hFF, void *pData, uint32_t nBytes)
{
	if( hFF->audiooutMode & AUDIO_MODE_OUT_I2S)
	{
		// Move the input audio samples into I2S output queue
		Queue_PushData(hFF->PCM_Out_data, pData, nBytes);
		
		// Check if we have to start playing audio thru external codec
		if(hFF->bStartPlay  && ( Queue_Count_Bytes(hFF->PCM_Out_data) >= (hFF->PCM_Out_data->nSize /2) ))
		{
			Queue_PopData(hFF->PCM_Out_data, hFF->pPCM_Out, NUM_PCM_BYTES);
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
		Queue_PushData(osParams.USB_In_data, pData, nBytes);
	}
}

extern DataProcessBlock_t  Melp;
extern DataProcessBlock_t  CVSD;
extern DataProcessBlock_t  CODEC2;

DataProcessBlock_t  *pModule;


void DataConvert(void *pDst, uint32_t DstType, uint32_t DstChMask, void *pSrc, uint32_t SrcType, uint32_t SrcChMask, uint32_t nSrcElements)
{
	int  srcStep, dstStep;
	int  srcSize, dstSize;
	int  srcChan, dstChan;
	int  srcOffset[8], dstOffset[8];
	int  srcCntr, dstCntr;
	int  data, srcIdx, dstIdx;
	int  bSameType, bNonFloat, dataShift, bToFloat;
	int  dataConv;
	
	void *pS, *pD;
	
	if((DstType == SrcType) && (DstChMask == SrcChMask))
	{
		memcpy(pDst, pSrc, nSrcElements * (SrcType & 0x00FF));
		return;
	}		
  srcStep = (SrcType & 0x00FF); 			// Step size to get the next element
	dstStep = (DstType & 0x00FF);
	srcSize = DATA_TYPE_SIZE(SrcType);	// Size of one datatype(1,2,3,4 bytes)
	dstSize = DATA_TYPE_SIZE(DstType);
	srcChan = DATA_TYPE_NUM_CHANNELS(SrcType);
	dstChan = DATA_TYPE_NUM_CHANNELS(DstType);
	bSameType = (SrcType & DATA_TYPE_MASK) == (DstType & DATA_TYPE_MASK) ? 1 : 0;
	bNonFloat = ((SrcType & DATA_FP_MASK) == 0 ) && ((DstType & DATA_FP_MASK) == 0) ? 1 : 0;
	bToFloat = ((DstType & DATA_FP_MASK) == 0) ? 1 : 0;
	dataConv = ((DstType & DATA_RANGE_MASK) == 0) ? 32768 : 1;
	dataShift = 8 * (dstSize - srcSize);
	
	// Check and create the proper mask for the destination, populate the offsets array
	DstChMask = (DstChMask == DATA_CHANNEL_ANY)? DATA_CHANNEL_ALL : DstChMask;
	DstChMask &= ((1 << dstChan) - 1);
	for(dstCntr = 0, dstIdx = 0; dstIdx < dstChan; dstIdx++)
	{
		if(DstChMask & 0x01) dstOffset[dstCntr++] = dstIdx * dstSize;
		DstChMask >>= 1;
	}

	// There is a difference between DATA_CHANNEL_ANY and DATA_CHANNEL_ALL - 
	//  when moving data from buffers with different number of channels,
	//  DATA_CHANNEL_ANY in Source will populate AABBCCDDEEFF from ABCDEF buffer, and ABCDEF out of AABBCCDDEEFF
	//  DATA_CHANNEL_ALL in Source will populate ABCDEF  from ABCDEF buffer, and AABBCCDDEEFF out of AABBCCDDEEFF 
	//    i.e nSrcElements will be consumed in all cases
	if(SrcChMask == DATA_CHANNEL_ANY)
	{
		srcCntr = dstCntr;
		for(srcIdx = 0; srcIdx < srcCntr; dstIdx++)
		{
			srcOffset[srcIdx] = 0;
		}
	}else
	{
		SrcChMask = (SrcChMask == DATA_CHANNEL_ANY)? DATA_CHANNEL_ALL : SrcChMask;
		SrcChMask &= ((1 << srcChan) - 1);
		for(srcCntr = 0, srcIdx = 0; srcIdx < srcChan; srcIdx++)
		{
			if(SrcChMask & 0x01) srcOffset[srcCntr++] = srcIdx * srcSize;
			SrcChMask >>= 1;
		}
	}
	
	srcIdx = 0;
	while(1)
	{
		for (dstIdx = 0; dstIdx < dstCntr; dstIdx++)
		{
			// 1. Adjust the SRC pointer to point to the next data type in the element
			pS = (void *) (((uint32_t)pSrc) + srcOffset[srcIdx]); 
			// 2. Get the data into the register with proper alignmenrt
			if(srcSize==1) data = *(int8_t *)pS; else if(srcSize==2)data = *(int16_t *)pS; else data = *(int32_t *)pS;
			// 3. Convert the data from one type to another
			if( !bSameType) 
			{	// Special cases - Integer to Integer conversion (No FP)
				//  or Q7, Q15, Q31 into another Integer/Q format
				if( bNonFloat )
				{
					data = (dataShift >= 0) ? data << dataShift : data >> -dataShift;
				}else if(bToFloat)
				{
					data = (dataShift >= 0) ? data << dataShift : data >> -dataShift;
					*((float *)(&data)) = Q31_TO_FLOAT(data);
				}else
				{
					data = FLOAT_TO_Q31(data);
					data = data >> -dataShift;
				}
			}
			// 4. Adjust the DST pointer to point to the next data type in the element
			pD = (void *) (((uint32_t)pDst) + dstOffset[dstIdx]); 
			// 5. Save the data with proper alignmenrt
			if(dstSize==1) *(int8_t *)pD = data; else if(dstSize==2)*(int16_t *)pD = data; else *(int32_t *)pD = data;
			
			// 6. Adjust the Src index, and check for exit condition
			srcIdx++;
			if(srcIdx >= srcCntr)
			{
				nSrcElements--;
				if(nSrcElements == 0) return;
				srcIdx = 0;
				pSrc = (void *)((uint32_t)pSrc + srcStep);
			}
		}
		pDst = (void *)((uint32_t)pDst + dstStep);
	}
}

//
//  Task to handle all incoming data
//
void StartDataProcessTask(void const * argument)
{
	osEvent		event;
	DQueue_t 	*pDataQ;

	uint8_t 	*pAudio;
	float		*pAudioIn;
	float		*pAudioOut;
	
	uint32_t	Type, nSamplesQueue, nSamplesModule;
	
	pAudio   = osAlloc(MAX_AUDIO_SIZE_BYTES);	
	pAudioIn = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));	
	pAudioOut = osAlloc(MAX_AUDIO_SAMPLES * sizeof(float));	

	while(1)
	{	
		event = osMessageGet(osParams.dataReadyMsg, osWaitForever);
		if( event.status == osEventMessage  ) // Message came that some valid Input Data is present
		{
			pDataQ = (DQueue_t *) event.value.p;
			// - Figure out what type of data is it
			// - If there is enough data accumulated - place it in a buffer and 
			//      call the appropriate processing function
			nSamplesQueue = Queue_Count_Elems(pDataQ);
			nSamplesModule = pModule->TypeSize(&osParams, &Type); 
			
			while(nSamplesQueue >= nSamplesModule)
			{
				Queue_PopData(pDataQ, pAudio, nSamplesModule * pDataQ->ElemSize);
				
				// Convert data from the Queue-provided type to the Processing-Module-required type
				DataConvert(pAudioIn, Type, DATA_CHANNEL_1, pAudio, pDataQ->Type, DATA_CHANNEL_1, nSamplesModule);
				//   Call data processing
				pModule->Process(&osParams, pAudioIn, pAudioOut, nSamplesModule);
				
				// Convert data from the Processing-Module-provided type to the HW Queue type
				DataConvert(pAudio, pDataQ->Type, DATA_CHANNEL_1 | DATA_CHANNEL_2 , pAudioOut, Type, DATA_CHANNEL_ANY, nSamplesModule);

				//   Distribute output data to all output data sinks (USB, I2S, etc)
				Data_Distribute(&osParams, pAudio, nSamplesModule);
				nSamplesQueue = Queue_Count_Elems(pDataQ);
			}
		}	
	}
}

