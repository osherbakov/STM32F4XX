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
#include "cvsd_f32.h"
#include "cvsd_data_f32.h"
#include "dataqueues.h"

#define	CVSD_DATA_TYPE			(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define CVSD_BLOCK_SIZE   		(180)
#define CVSD_BLOCK_BYTES   		(CVSD_BLOCK_SIZE * 4)

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

void cvsd_init(void *pHandle)
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

void cvsd_data_ready(void *pHandle, DataPort_t *pInData)
{
	pInData->Type = CVSD_DATA_TYPE;
	pInData->Size = CVSD_BLOCK_BYTES;
}

void cvsd_data_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = CVSD_DATA_TYPE;
	pIn->Size = CVSD_BLOCK_BYTES;
	
	pOut->Type = CVSD_DATA_TYPE;
	pOut->Size = CVSD_BLOCK_BYTES;
}


DataProcessBlock_t  CVSD = {cvsd_create, cvsd_init, cvsd_data_info, cvsd_data_ready, cvsd_process, cvsd_close};
