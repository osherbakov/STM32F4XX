#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "dataqueues.h"
#include "string.h"

#define MIN(a, b)  	(((a) < (b)) ? (a) : (b))
#define MAX(a, b)  	(((a) > (b)) ? (a) : (b))
#define ABS(a)		(((a) >  0) ? (a) : -(a))


//
// Definitions:
//   Data type and type_size - the type (char, short, int, fixed, float) and size (in bytes) of a single element
//	 Element - the combination of N data types, as having the same type, where N is number of channels,
// 	  so basically, an element is the unit/collection of multiple data of the same type with the same timestamp ...
//   The data types in the element can be INTERLEAVED (CH1 CH2 CH1 CH2 ...) or SEQUENTIAL (CH1 CH1 CH1 ... CH2 CH2 CH2...)

#define  Q31_F				((float)(1U<<31))
#define	 Q31_INV_F			(1.0F / Q31_F)

#define  FLOAT_TO_Q31(a) 	( (int)(((float) (a)) * Q31_F))
#define  Q31_TO_FLOAT(a) 	( ((float) (a)) * Q31_INV_F)


DQueue_t *Queue_Create(uint32_t nBytes, uint32_t Type)
{
	if(nBytes == 0) return 0;
	DQueue_t *pQ = osAlloc(sizeof(DQueue_t));	if(pQ == 0) return 0;
	pQ->pBuffer = osAlloc(nBytes); if(pQ->pBuffer == 0) { osFree(pQ); return 0;}
	pQ->Size = nBytes;
	if((Type & (DATA_TYPE_MASK | DATA_CH_MASK )) == 0){
		pQ->Type = Type;
	}else{
		pQ->ElemType = Type >> 8;
		pQ->ElemSize = DATA_TYPE_ELEM_SIZE(Type);
	}
	// The final sanity check - Element size cannot be 0!!!
	if(pQ->ElemSize == 0) pQ->Type = 1;
	Queue_Clear(pQ);
	return pQ;
}

void Queue_Init(DQueue_t *pQueue, uint32_t Type)
{
	if((Type & (DATA_TYPE_MASK | DATA_CH_MASK )) == 0){
		pQueue->Type = Type;
	}else{
		pQueue->ElemType = Type >> 8;
		pQueue->ElemSize = DATA_TYPE_ELEM_SIZE(Type);
	}
	// The final sanity check - Element size cannot be 0!!!
	if(pQueue->ElemSize == 0) pQueue->Type = 1;
	Queue_Clear(pQueue);
}

uint32_t Queue_Count_Bytes(DQueue_t *pQueue)
{
	int Count;
	uint32_t iPut, iGet, nSize;
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->Size;
	Count =  iPut - iGet;
	if (Count < 0) { Count += nSize; if(Count <= 0) Count += nSize;}
	else if(Count > nSize) {Count -= nSize;}
	return Count;
}
uint32_t Queue_Count_Elems(DQueue_t *pQueue)
{
	uint32_t Count, ElemSize;
	
	Count = Queue_Count_Bytes(pQueue);
	ElemSize = pQueue->ElemSize;
	return Count/ElemSize;
}


uint32_t Queue_Space_Bytes(DQueue_t *pQueue)
{
	int Space;
	uint32_t iPut, iGet, nSize;
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->Size;
	Space =  iGet - iPut;
	if(Space <= 0) {Space += nSize; if(Space < 0) Space += nSize;}	
	else if(Space > nSize) {Space -= nSize;}
	return Space;
}

uint32_t Queue_Space_Elems(DQueue_t *pQueue)
{
	uint32_t Space, ElemSize;
	
	Space = Queue_Space_Bytes(pQueue);
	ElemSize = pQueue->ElemSize;
	return Space/ElemSize;
}

void Queue_Clear(DQueue_t *pQueue)
{
	pQueue->iPut = pQueue->iGet = 0;
}

uint32_t Queue_PushData(DQueue_t *pQueue, void *pData, uint32_t nBytes)
{
	int diff, space;
	uint32_t iPut, iGet, nSize;
	uint32_t n_bytes, n_copy, ret;
	
	// Prefetch all parameters to avoid race conditions */
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->Size; 
	space = iGet - iPut; 
	// Check if the buffer is already full - return 0 as number of bytes consumed */
	if((ABS(space) == nSize) || (nBytes == 0)) return 0;

	// Calculate the available space
	if(space <= 0) {space += nSize; if(space < 0) space += nSize;} 
	else if(space > nSize) {space -= nSize;}
		
	if(iPut >= nSize) iPut -= nSize;
	diff = nSize - iPut;
	
	ret = n_bytes = MIN(nBytes, space);	// so many bytes will be copied
	n_copy = MIN(diff, n_bytes);	// so many bytes will be placed starting from iPut
	memcpy(&pQueue->pBuffer[iPut], pData, n_copy);
	n_bytes -= n_copy;
	if(n_bytes > 0)
	{
		memcpy(pQueue->pBuffer, (void *)((uint32_t)pData + n_copy), n_bytes);
	}
	// Adjust the iPut pointer - if full, it must point exactly nSize from iGet
	if(ret == space) {iPut = (iGet >= nSize) ? (iGet - nSize) : (iGet + nSize);}
	else {iPut += ret; if(iPut >= nSize) iPut -= nSize;}
	pQueue->iPut = iPut;
	return ret;
}

uint32_t Queue_PopData(DQueue_t *pQueue, void *pData, uint32_t nBytes)
{
	int diff, count;
	uint32_t iPut, iGet, nSize;
	uint32_t n_bytes, n_copy, ret;
	
	// Prefetch all parameters to avoid race conditions */
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->Size; 

	// Check if the buffer is empty - return 0 as number of bytes produced */
	count =  iPut - iGet;
	if((count == 0) || (nBytes == 0)) return 0;

	// Calculate the number of available bytes
	if (count < 0) { count += nSize; if(count <= 0) count += nSize;}
	else if(count > nSize) {count -= nSize;}
	
	if(iGet >= nSize) iGet -= nSize;	
	diff = nSize - iGet;

	ret = n_bytes = MIN(nBytes, count);	// so many bytes will be copied
	n_copy = MIN(diff, n_bytes);	// so many bytes will be read starting from iGet
	memcpy(pData, &pQueue->pBuffer[iGet], n_copy);
	n_bytes -= n_copy;
	if(n_bytes > 0)
	{
		memcpy((void *)((uint32_t)pData + n_copy), pQueue->pBuffer, n_bytes);
	}
	
	// Adjust the get pointer .... and save it
	if(ret == count) {iGet = iPut;}	
	else{ iGet += ret; if(iGet >= nSize) iGet -= nSize;}
	pQueue->iGet = iGet;
	return ret;
}

void DataConvert(void *pDst, uint32_t DstType, uint32_t DstChMask, void *pSrc, uint32_t SrcType, uint32_t SrcChMask, uint32_t nSrcElements)
{
	int  srcStep, dstStep;
	int  srcSize, dstSize;
	int  srcChan, dstChan;
	int  srcOffset[8], dstOffset[8];
	int  srcCntr, dstCntr;
	int  data;
	int	 srcIdx, dstIdx;
	float fdata, scale;
	int  bSameType, bNonFloat, dataShift, bToFloat;
	
	void *pS, *pD;
	
	if((DstType == SrcType) && (DstChMask == SrcChMask))
	{
		memcpy(pDst, pSrc, nSrcElements * (SrcType & 0x00FF));
		return;
	}		
	srcStep = (SrcType & 0x00FF); 		// Step size to get the next element
	dstStep = (DstType & 0x00FF);
	
	srcSize = DATA_TYPE_SIZE(SrcType);	// Size of one datatype(1,2,3,4 bytes)
	dstSize = DATA_TYPE_SIZE(DstType);
	
	srcChan = DATA_TYPE_NUM_CHANNELS(SrcType);
	dstChan = DATA_TYPE_NUM_CHANNELS(DstType);
	
	bSameType = (SrcType & DATA_TYPE_MASK) == (DstType & DATA_TYPE_MASK) ? 1 : 0;
	bNonFloat = ((SrcType & DATA_FP_MASK) == 0 ) && ((DstType & DATA_FP_MASK) == 0) ? 1 : 0;
	bToFloat = ((DstType & DATA_FP_MASK) != 0) ? 1 : 0;
	dataShift = 8 * (dstSize - srcSize);
	scale = (SrcType & DATA_RANGE_MASK) == (DstType & DATA_RANGE_MASK) ? 1.0F :
					(SrcType & DATA_RANGE_MASK) && !(DstType & DATA_RANGE_MASK) ? 32768.0F :
					1.0F/32768.0F;
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
		for(srcIdx = 0; srcIdx < srcCntr; srcIdx++)
		{
			srcOffset[srcIdx] = 0;
		}
	}else	{
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
			// 2. Adjust the DST pointer to point to the next data type in the element
			pD = (void *) (((uint32_t)pDst) + dstOffset[dstIdx]); 			
			// 3. Get the data and convert from one type to another
			if( !bSameType) 
			{	// Special cases - Integer to Integer conversion (No FP)
				//  or Q7, Q15, Q31 into another Integer/Q format
				if( bNonFloat )
				{
					if(srcSize==1) data = *(int8_t *)pS; else if(srcSize==2)data = *(int16_t *)pS; else data = *(int32_t *)pS;					
					data = (dataShift >= 0) ? data << dataShift : data >> -dataShift;
				  if(dstSize==1) *(int8_t *)pD = data; else if(dstSize==2)*(int16_t *)pD = data; else *(int32_t *)pD = data;				
				}else if(bToFloat)
				{
					if(srcSize==1) data = *(int8_t *)pS; else if(srcSize==2)data = *(int16_t *)pS; else data = *(int32_t *)pS;					
					data = data << dataShift;
					fdata = Q31_TO_FLOAT(data);
					*(float *) pD = fdata * scale;
				}else
				{
					fdata = (*(float *) pS) * scale;
					data = FLOAT_TO_Q31(fdata);
					data = data >> -dataShift;
				  if(dstSize==1) *(int8_t *)pD = data; else if(dstSize==2)*(int16_t *)pD = data; else *(int32_t *)pD = data;					
				}
			}else
			{
				if(srcSize==1) data = *(int8_t *)pS; else if(srcSize==2)data = *(int16_t *)pS; else data = *(int32_t *)pS;
				if(dstSize==1) *(int8_t *)pD = data; else if(dstSize==2)*(int16_t *)pD = data; else *(int32_t *)pD = data;				
			}

			// 4. Adjust the Src index, and check for exit condition
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

