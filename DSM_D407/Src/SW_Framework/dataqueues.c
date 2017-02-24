#include "arm_math.h"
#include "arm_const_structs.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "dataqueues.h"
#include "datatasks.h"
#include "string.h"

#define MIN(a, b)  	(((a) < (b)) ? (a) : (b))
#define MAX(a, b)  	(((a) > (b)) ? (a) : (b))
#define ABS(a)		(((a) >  0) ? (a) : -(a))

//
// Definitions:
//   Data type and type_size - the type (char, short, int, fixed, float) and size (in bytes) of a single element
//	 Element - the combination of N data types, all having the same type, where N is number of channels,
// 	  so basically, an element is the unit/collection of multiple data of the same type with the same timestamp ...
//   The data types in the element can be INTERLEAVED (CH1 CH2 CH1 CH2 ...) or SEQUENTIAL (CH1 CH1 CH1 ... CH2 CH2 CH2...)

#define  Q31_F				((float)(1U<<31))
#define	 Q31_INV_F			(1.0F / Q31_F)

#define  FLOAT_TO_Q31(a) 	( (int)(((float) (a)) * Q31_F))
#define  Q31_TO_FLOAT(a) 	( ((float) (a)) * Q31_INV_F)


#define  memcpy(dst,src,nbytes)  arm_copy_q7((q7_t *)(src),(q7_t *)(dst),nbytes)

DQueue_t *Queue_Create(uint32_t nBytes, uint32_t Type)
{
	if(nBytes == 0) return 0;
	DQueue_t *pQueue = osAlloc(sizeof(DQueue_t));	if(pQueue == 0) return 0;
	pQueue->pBuffer = osAlloc(nBytes); if(pQueue->pBuffer == 0) { osFree(pQueue); return 0;}
	pQueue->Size = nBytes;
	Queue_Init(pQueue, Type);
	return pQueue;
}

void Queue_Init(DQueue_t *pQueue, uint32_t Type)
{
	// Check if the type or channel number is not specified
	// In that case use the provided Element Size field
	if((Type & DATA_SIZE_MASK) == 0)
	{
		Type = Type | ((DATA_TYPE_SIZE(Type) * DATA_TYPE_NUM_CHANNELS(Type)));
	}
	pQueue->Type = Type;
	Queue_Clear(pQueue);
	pQueue->pNext = 0;
}

uint32_t Queue_Count(DQueue_t *pQueue)
{
	int Count;
	uint32_t iPut, iGet, nSize;
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->Size;
	Count =  iPut - iGet;
	if (Count < 0) { Count += nSize; if(Count <= 0) Count += nSize;}
	else if(Count > nSize) {Count -= nSize;}
	return Count;
}

uint32_t Queue_Space(DQueue_t *pQueue)
{
	int Space;
	uint32_t iPut, iGet, nSize;
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->Size;
	Space =  iGet - iPut;
	if(Space <= 0) {Space += nSize; if(Space < 0) Space += nSize;}
	else if(Space > nSize) {Space -= nSize;}
	return Space;
}

void Queue_Clear(DQueue_t *pQueue)
{
	pQueue->isReady = pQueue->iPut = pQueue->iGet = 0;
}

uint32_t Queue_Push(DQueue_t *pQueue, void *pData, uint32_t nBytes)
{
	int diff, space, count;
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

	// Set up the isReady flag - if the number of bytes available is more than 1/2 of the buffer
	count =  iPut - iGet;
	if (count < 0) { count += nSize; if(count <= 0) count += nSize;}
	else if(count > nSize) {count -= nSize;}
	if(count >= nSize/2) pQueue->isReady = 1;

	return ret;
}

uint32_t Queue_Pop(DQueue_t *pQueue, void *pData, uint32_t nBytes)
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
	// If after this pop the queue is empty - mark it as not-ready
	if((ret < nBytes) || (ret == count)) pQueue->isReady = 0;
	return ret;
}

void DataConvert(void *pSrc, uint32_t SrcType, uint32_t SrcChMask,
					void *pDst, uint32_t DstType, uint32_t DstChMask,
						uint32_t *pnSrcBytes, uint32_t *pnDstBytes)
{
	int  srcStep, dstStep;
	int  srcSize, dstSize;
	int  srcChan, dstChan;
	int  srcOffset[8], dstOffset[8];
	int  srcCntr, dstCntr;
	int	 srcAllMask, dstAllMask;

	int	 srcIdx, dstIdx;
	int  data;
	float fdata;
	unsigned int nElements, nGeneratedBytes;
	int  bSameType, bNonFloat, dataShift, bToFloat;
	void *pS, *pD;


	srcStep = DATA_ELEM_SIZE(SrcType); 		// Step size to get the next element
	dstStep = DATA_ELEM_SIZE(DstType);

	srcSize = DATA_TYPE_SIZE(SrcType);	// Size of one datatype(1,2,3,4 bytes)
	dstSize = DATA_TYPE_SIZE(DstType);

	srcChan = DATA_TYPE_NUM_CHANNELS(SrcType);
	dstChan = DATA_TYPE_NUM_CHANNELS(DstType);

	srcAllMask = ((1 << srcChan) - 1);
	dstAllMask = ((1 << dstChan) - 1);

	SrcChMask &= srcAllMask;
	DstChMask &= dstAllMask;

	nElements = (*pnSrcBytes / srcStep);
	nGeneratedBytes = nElements * dstStep;

	bSameType = (SrcType & DATA_TYPE_MASK) == (DstType & DATA_TYPE_MASK) ? 1 : 0;
	bNonFloat = ((SrcType & DATA_FP_MASK) == 0 ) && ((DstType & DATA_FP_MASK) == 0) ? 1 : 0;
	bToFloat = ((DstType & DATA_FP_MASK) != 0) ? 1 : 0;

	// Check for invalid conditions
	if( nElements  == 0 ){
		*pnDstBytes = 0;
		return;
	}
	// Some special cases when we have only a single channel for both Src and Dst
	if( (srcChan == dstChan) && (dstChan == 1) && (SrcChMask == DstChMask) && SrcChMask){

		if((DstType & DATA_TYPE_MASK) == DATA_TYPE_F32) {
			switch( SrcType & DATA_TYPE_MASK)
			{
				case DATA_TYPE_Q7:
					arm_q7_to_float(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q15:
					arm_q15_to_float(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q31:
					arm_q31_to_float(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_F32:
					arm_copy_f32(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes; return;
				default:
					break;
			}
		}
		if((DstType & DATA_TYPE_MASK) == DATA_TYPE_Q31) {
			switch( SrcType & DATA_TYPE_MASK)
			{
				case DATA_TYPE_Q7:
					arm_q7_to_q31(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q15:
					arm_q15_to_q31(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q31:
					arm_copy_q31(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_F32:
					arm_float_to_q31(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes; return;
				default:
					break;
			}
		}
		if((DstType & DATA_TYPE_MASK) == DATA_TYPE_Q15) {
			switch( SrcType & DATA_TYPE_MASK)
			{
				case DATA_TYPE_Q7:
					arm_q7_to_q15(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q15:
					arm_copy_q15(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q31:
					arm_q31_to_q15(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_F32:
					arm_float_to_q15(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes; return;
				default:
					break;
			}
		}
		if((DstType & DATA_TYPE_MASK) == DATA_TYPE_Q7) {
			switch( SrcType & DATA_TYPE_MASK)
			{
				case DATA_TYPE_Q7:
					arm_copy_q7(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q15:
					arm_q15_to_q7(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_Q31:
					arm_q31_to_q7(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes;	return;
				case DATA_TYPE_F32:
					arm_float_to_q7(pSrc, pDst, nElements);
					*pnSrcBytes = 0;*pnDstBytes =  nGeneratedBytes; return;
				default:
					break;
			}
		}
	}

	if((DstType == SrcType) && (DstChMask == SrcChMask)){
		memcpy(pDst, pSrc, *pnSrcBytes);
		*pnDstBytes =  *pnSrcBytes;
		*pnSrcBytes = 0;
		return;
	}

	// Prepare the shift amount for the data transferred
	dataShift = 8 * (dstSize - srcSize);

	// Check and create the proper mask for the destination, populate the offsets array
	DstChMask = (DstChMask == DATA_CHANNEL_ANY)? DATA_CHANNEL_ALL : DstChMask;
	DstChMask &= dstAllMask;	// Limit the mask to the ACTUAL number of channels
	for(dstCntr = 0, dstIdx = 0; dstIdx < dstChan; dstIdx++)
	{
		if(DstChMask & 0x01) dstOffset[dstCntr++] = dstIdx * dstSize;
		DstChMask >>= 1;
	}

	// There is a difference between DATA_CHANNEL_ANY and DATA_CHANNEL_ALL -
	//  when moving data from buffers with different number of channels,
	//  DATA_CHANNEL_ANY in Source will populate AABBCCDDEEFF from ABCDEF buffer, and ABCDEF out of AABBCCDDEEFF
	//  DATA_CHANNEL_ALL in Source will populate ABCDEF  from ABCDEF buffer, and AABBCCDDEEFF out of AABBCCDDEEFF
	//    i.e nElements will be consumed in all cases
	if(SrcChMask == DATA_CHANNEL_ANY)
	{
		srcCntr = dstCntr;
		for(srcIdx = 0; srcIdx < srcCntr; srcIdx++)
		{
			srcOffset[srcIdx] = 0;
		}
	}else	{
		SrcChMask &= srcAllMask;	// Limit the mask to the ACTUAL number of channels
		for(srcCntr = 0, srcIdx = 0; srcIdx < srcChan; srcIdx++)
		{
			if(SrcChMask & 0x01) srcOffset[srcCntr++] = srcIdx * srcSize;
			SrcChMask >>= 1;
		}
	}

	srcIdx = 0;
	while(nElements > 0)
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
					* ((float *) pD) = fdata;
				}else
				{
					fdata = *((float *) pS);
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
				srcIdx = 0;
				pSrc = (void *)((uint32_t)pSrc + srcStep);
				nElements--;
			}
		}
		pDst = (void *)((uint32_t)pDst + dstStep);
	}
	*pnSrcBytes = 0;
	*pnDstBytes =  nGeneratedBytes;
}

