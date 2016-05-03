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

void alaw_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
		alaw_encode_q15(pHandle, dataBits, pDataIn, AU_LAW_BLOCK_SIZE);
		alaw_decode_q15(pHandle, pDataOut, dataBits, AU_LAW_BLOCK_SIZE);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}

void alaw_encode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
		alaw_encode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}

void alaw_decode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
		alaw_decode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn += AU_LAW_BLOCK_SIZE;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}

void ulaw_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
		ulaw_encode_q15(pHandle, dataBits, pDataIn, AU_LAW_BLOCK_SIZE);
		ulaw_decode_q15(pHandle, pDataOut, dataBits, AU_LAW_BLOCK_SIZE);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}

void ulaw_encode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
		ulaw_encode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}

void ulaw_decode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
		ulaw_decode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
		pDataIn += AU_LAW_BLOCK_SIZE;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}


uint32_t aulaw_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	 return AU_LAW_BLOCK_SIZE;
}

uint32_t aulaw_encode_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	 return AU_LAW_BLOCK_SIZE;
}

uint32_t aulaw_decode_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_Q7 | DATA_NUM_CH_1 | (1);
	 return AU_LAW_BLOCK_SIZE;
}

DataProcessBlock_t  ALAW = {aulaw_create, aulaw_init, aulaw_data_typesize, alaw_process, aulaw_close};
DataProcessBlock_t  ULAW = {aulaw_create, aulaw_init, aulaw_data_typesize, ulaw_process, aulaw_close};

DataProcessBlock_t  ALAW_ENC = {aulaw_create, aulaw_init, aulaw_encode_typesize, alaw_encode_process, aulaw_close};
DataProcessBlock_t  ULAW_ENC = {aulaw_create, aulaw_init, aulaw_encode_typesize, ulaw_encode_process, aulaw_close};
DataProcessBlock_t  ALAW_DEC = {aulaw_create, aulaw_init, aulaw_decode_typesize, alaw_decode_process, aulaw_close};
DataProcessBlock_t  ULAW_DEC = {aulaw_create, aulaw_init, aulaw_decode_typesize, ulaw_decode_process, aulaw_close};
