#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "dataqueues.h"
#include "string.h"

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define ABS(a)		 (((a) >   0) ? (a) : -(a))


DQueue_t *Queue_Create(uint32_t nBytes, uint32_t Type)
{
	if(nBytes == 0) return 0;
	DQueue_t *pQ = osAlloc(sizeof(DQueue_t));	if(pQ == 0) return 0;
	pQ->pBuffer = osAlloc(nBytes); if(pQ->pBuffer == 0) { osFree(pQ); return 0;}
	pQ->nSize = nBytes;
	if((Type & (DATA_TYPE_MASK | DATA_CH_MASK )) == 0){
		pQ->Type = Type;
	}else{
		pQ->ElemType = Type >> 8;
		pQ->ElemSize = DATA_ELEM_SIZE(Type);
	}
	Queue_Clear(pQ);
	return pQ;
}

void Queue_Init(DQueue_t *pQueue, uint32_t Type)
{
	if((Type & (DATA_TYPE_MASK | DATA_CH_MASK )) == 0){
		pQueue->Type = Type;
	}else{
		pQueue->ElemType = Type >> 8;
		pQueue->ElemSize = DATA_ELEM_SIZE(Type);
	}
	Queue_Clear(pQueue);
}

uint32_t Queue_Count(DQueue_t *pQueue)
{
	int Count;
	uint32_t iPut, iGet, nSize;
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->nSize;
	Count =  iPut - iGet;
	if (Count < 0) { Count += nSize; if(Count <= 0) Count += nSize;}
	else if(Count > nSize) {Count -= nSize;}
	return Count;
}

uint32_t Queue_Space(DQueue_t *pQueue)
{
	int Space;
	uint32_t iPut, iGet, nSize;
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->nSize;
	Space =  iGet - iPut;
	if(Space > nSize) {Space -= nSize;}
	else if(Space <= 0) {Space += nSize; if(Space < 0) Space += nSize;}
	return Space;
}

void Queue_Clear(DQueue_t *pQueue)
{
	pQueue->iPut = pQueue->iGet = 0;
}

uint32_t Queue_PushData(DQueue_t *pQueue, uint8_t *pData, uint32_t nBytes)
{
	int diff, space;
	uint32_t iPut, iGet, nSize;
	uint32_t n_bytes, n_copy, ret;
	
	// Prefetch all parameters to avoid race conditions */
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->nSize; 
	space = iGet - iPut; 
	// Check if the buffer is already full - return 0 as number of bytes consumed */
	if((ABS(space) == nSize) || (nBytes == 0)) return 0;

	// Calculate the available space
	if(space > nSize) {space -= nSize;}
	else if(space <= 0) {space += nSize; if(space < 0) space += nSize;} 
	
	if(iPut >= nSize) iPut -= nSize;
	diff = nSize - iPut;
	
	ret = n_bytes = MIN(nBytes, space);	// so many bytes will be copied
	n_copy = MIN(diff, n_bytes);	// so many bytes will be placed starting from iPut
	memcpy(&pQueue->pBuffer[iPut], pData, n_copy);
	n_bytes -= n_copy;
	if(n_bytes > 0)
	{
		memcpy(pQueue->pBuffer, pData + n_copy, n_bytes);
	}
	// Adjust the iPut pointer - if full, it must point exactly nSize from iGet
	if(ret == space) {iPut = (iGet >= nSize) ? (iGet - nSize) : (iGet + nSize);}
	else {iPut += ret; if(iPut >= nSize) iPut -= nSize;}
	pQueue->iPut = iPut;
	return ret;
}

uint32_t Queue_PopData(DQueue_t *pQueue, uint8_t *pData, uint32_t nBytes)
{
	int diff, count;
	uint32_t iPut, iGet, nSize;
	uint32_t n_bytes, n_copy, ret;
	
	// Prefetch all parameters to avoid race conditions */
	iPut = pQueue->iPut; iGet = pQueue->iGet; nSize = pQueue->nSize; 

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
		memcpy(pData + n_copy, pQueue->pBuffer, n_bytes);
	}
	
	// Adjust the get pointer .... and save it
	if(ret == count) {iGet = iPut;}	
	else{ iGet += ret; if(iGet >= nSize) iGet -= nSize;}
	pQueue->iGet = iGet;
	return ret;
}


