#ifndef _MSC_VER
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#endif

#include "arm_math.h"
#include "arm_const_structs.h"
#include "aulaw_q15.h"
#include "aulaw_data_q15.h"
#include "dataqueues.h"


#define AU_LAW_BLOCK_SIZE   		(60)
#define AU_LAW_BLOCK_BYTES			(AU_LAW_BLOCK_SIZE * 2)   		

static uint8_t dataBits[AU_LAW_BLOCK_SIZE] CCMRAM;

void *aulaw_create(uint32_t Params)
{
	return 0;
}

void aulaw_close(void *pHandle)
{
	return;
}

void aulaw_init(void *pHandle)
{
}

void alaw_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= AU_LAW_BLOCK_BYTES)
	{
		alaw_encode_q15(pHandle, dataBits, pDataIn, AU_LAW_BLOCK_SIZE);
		alaw_decode_q15(pHandle, pDataOut, dataBits, AU_LAW_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + AU_LAW_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + AU_LAW_BLOCK_BYTES);
		*pInBytes -= AU_LAW_BLOCK_BYTES;
		nGenerated += AU_LAW_BLOCK_BYTES;
	}
	*pOutBytes = nGenerated;
}

void alaw_encode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= AU_LAW_BLOCK_BYTES)
	{
		alaw_encode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + AU_LAW_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + AU_LAW_BLOCK_SIZE);
		*pInBytes -= AU_LAW_BLOCK_BYTES;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutBytes = nGenerated;
}

void alaw_decode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= AU_LAW_BLOCK_SIZE)
	{
		alaw_decode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + AU_LAW_BLOCK_SIZE);
		pDataOut = (void *)((uint32_t)pDataOut + AU_LAW_BLOCK_BYTES);
		*pInBytes -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_BYTES;
	}
	*pOutBytes = nGenerated;
}

void ulaw_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= AU_LAW_BLOCK_BYTES)
	{
		ulaw_encode_q15(pHandle, dataBits, pDataIn, AU_LAW_BLOCK_SIZE);
		ulaw_decode_q15(pHandle, pDataOut, dataBits, AU_LAW_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + AU_LAW_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + AU_LAW_BLOCK_BYTES);
		*pInBytes -= AU_LAW_BLOCK_BYTES;
		nGenerated += AU_LAW_BLOCK_BYTES;
	}
	*pOutBytes = nGenerated;
}

void ulaw_encode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= AU_LAW_BLOCK_BYTES)
	{
		ulaw_encode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + AU_LAW_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + AU_LAW_BLOCK_SIZE);
		*pInBytes -= AU_LAW_BLOCK_BYTES;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutBytes = nGenerated;
}

void ulaw_decode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= AU_LAW_BLOCK_SIZE)
	{
		ulaw_decode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + AU_LAW_BLOCK_SIZE);
		pDataOut = (void *)((uint32_t)pDataOut + AU_LAW_BLOCK_BYTES);
		*pInBytes -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_BYTES;
	}
	*pOutBytes = nGenerated;
}


void aulaw_data_ready(void *pHandle, DataPort_t *pInData)
{
	pInData->Type = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	pInData->Size = AU_LAW_BLOCK_BYTES;
}

void aulaw_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	pIn->Size = AU_LAW_BLOCK_BYTES;
	
	pOut->Type = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	pOut->Size = AU_LAW_BLOCK_BYTES;
}

void aulaw_encode_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	pIn->Size = AU_LAW_BLOCK_BYTES;
	
	pOut->Type = DATA_TYPE_BITS | DATA_NUM_CH_1 | (1);
	pOut->Size = AU_LAW_BLOCK_SIZE;
}

void aulaw_decode_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DATA_TYPE_BITS | DATA_NUM_CH_1 | (1);
	pIn->Size = AU_LAW_BLOCK_SIZE;
	
	pOut->Type = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	pOut->Size = AU_LAW_BLOCK_BYTES;
}

DataProcessBlock_t  ALAW = {aulaw_create, aulaw_init, aulaw_info, aulaw_data_ready, alaw_process, aulaw_close};
DataProcessBlock_t  ULAW = {aulaw_create, aulaw_init, aulaw_info, aulaw_data_ready, ulaw_process, aulaw_close};

DataProcessBlock_t  ALAW_ENC = {aulaw_create, aulaw_init, aulaw_encode_info, aulaw_data_ready, alaw_encode_process, aulaw_close};
DataProcessBlock_t  ULAW_ENC = {aulaw_create, aulaw_init, aulaw_encode_info, aulaw_data_ready, ulaw_encode_process, aulaw_close};
DataProcessBlock_t  ALAW_DEC = {aulaw_create, aulaw_init, aulaw_decode_info, aulaw_data_ready, alaw_decode_process, aulaw_close};
DataProcessBlock_t  ULAW_DEC = {aulaw_create, aulaw_init, aulaw_decode_info, aulaw_data_ready, ulaw_decode_process, aulaw_close};
