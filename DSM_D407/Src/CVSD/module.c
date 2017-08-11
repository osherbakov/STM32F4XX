#include "arm_math.h"
#include "arm_const_structs.h"
#include "cvsd_f32.h"
#include "cvsd_data_f32.h"
#include "dataqueues.h"
#include "cmsis_os.h"

#define	CVSD_DATA_TYPE			(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define CVSD_BLOCK_SIZE   		(60)
#define CVSD_BLOCK_BYTES   		(CVSD_BLOCK_SIZE * 4)

#define	CVSD_BITS_TYPE			(DATA_TYPE_BITS | DATA_NUM_CH_1 | (1))
#define CVSD_BITS_SIZE   		(60)
#define CVSD_BITS_BYTES   		(CVSD_BITS_SIZE)

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef BOOL
typedef int BOOL;
#endif

void *cvsd_ana;
void *cvsd_syn;

static CVSD_STATE_F32_t ana CCMRAM;
static CVSD_STATE_F32_t syn CCMRAM;

static uint8_t dataBits[CVSD_BLOCK_SIZE] CCMRAM;

void *cvsd_create(uint32_t Params)
{
	cvsd_ana = &ana;
	cvsd_syn = &syn;
	return 0;
}

void cvsd_close(void *pHandle)
{
	return;
}

void cvsd_open(void *pHandle, uint32_t Params)
{
	cvsd_init_f32(cvsd_ana);
	cvsd_init_f32(cvsd_syn);
}


void cvsd_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= CVSD_BLOCK_BYTES)
	{
		arm_scale_f32(pDataIn, 32767.0f, pDataIn, CVSD_BLOCK_SIZE);
		cvsd_encode_f32(cvsd_ana, dataBits, pDataIn, CVSD_BLOCK_SIZE);
		cvsd_decode_f32(cvsd_syn, pDataOut, dataBits, CVSD_BLOCK_SIZE);
		arm_scale_f32(pDataOut, 1.0f/32768.0f, pDataOut, CVSD_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + CVSD_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + CVSD_BLOCK_BYTES);
		*pInBytes -= CVSD_BLOCK_BYTES;
		nGenerated += CVSD_BLOCK_BYTES;
	}
	*pOutBytes = nGenerated;
}

void cvsd_data_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = CVSD_DATA_TYPE;
	pIn->Size = CVSD_BLOCK_BYTES;
	
	pOut->Type = CVSD_DATA_TYPE;
	pOut->Size = CVSD_BLOCK_BYTES;
}

ProcessBlock_t  CVSD = {cvsd_create, cvsd_open, cvsd_data_info, cvsd_process, cvsd_close};


//
//    CVSD  Encoder  (Samples -> Bits)
//
void *cvsd_encode_create(uint32_t Params)
{
	return osAlloc(sizeof(CVSD_STATE_F32_t));
}

void cvsd_encode_close(void *pHandle)
{
	osFree(pHandle);
	return;
}

void cvsd_encode_open(void *pHandle, uint32_t Params)
{
	cvsd_init_f32(pHandle);
}

void cvsd_encode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= CVSD_BLOCK_BYTES)
	{
		cvsd_encode_f32(pHandle, pDataOut, pDataIn, CVSD_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + CVSD_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + CVSD_BITS_BYTES);
		*pInBytes -= CVSD_BLOCK_BYTES;
		nGenerated += CVSD_BITS_BYTES;
	}
	*pOutBytes = nGenerated;
}

void cvsd_encode_data_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = CVSD_DATA_TYPE;
	pIn->Size = CVSD_BLOCK_BYTES;
	
	pOut->Type = CVSD_BITS_TYPE;
	pOut->Size = CVSD_BITS_BYTES;
}

ProcessBlock_t  CVSD_ENC = {cvsd_encode_create, cvsd_encode_open, cvsd_encode_data_info, cvsd_encode_process, cvsd_encode_close};

//
//    CVSD  Decoder  (Bits -> Samples)
//
void *cvsd_decode_create(uint32_t Params)
{
	return osAlloc(sizeof(CVSD_STATE_F32_t));
}

void cvsd_decode_close(void *pHandle)
{
	osFree(pHandle);
	return;
}

void cvsd_decode_open(void *pHandle, uint32_t Params)
{
	cvsd_init_f32(pHandle);
}

void cvsd_decode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= CVSD_BITS_BYTES)
	{
		cvsd_decode_f32(pHandle, pDataOut, pDataIn, CVSD_BITS_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + CVSD_BITS_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + CVSD_BLOCK_BYTES);
		*pInBytes -= CVSD_BITS_BYTES;
		nGenerated += CVSD_BLOCK_BYTES;
	}
	*pOutBytes = nGenerated;
}

void cvsd_decode_data_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = CVSD_BITS_TYPE;
	pIn->Size = CVSD_BITS_BYTES;
	
	pOut->Type = CVSD_DATA_TYPE;
	pOut->Size = CVSD_BLOCK_BYTES;
}

ProcessBlock_t  CVSD_DEC = {cvsd_decode_create, cvsd_decode_open, cvsd_decode_data_info, cvsd_decode_process, cvsd_decode_close};

