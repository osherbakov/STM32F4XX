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

#define CVSD_BLOCK_SIZE   		(180)

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


void cvsd_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= CVSD_BLOCK_SIZE)
	{
		arm_scale_f32(pDataIn, 32767.0f, pDataIn, CVSD_BLOCK_SIZE);
		cvsd_encode_f32(cvsd_ana, dataBits, pDataIn, CVSD_BLOCK_SIZE);
		cvsd_decode_f32(cvsd_syn, pDataOut, dataBits, CVSD_BLOCK_SIZE);
		arm_scale_f32(pDataOut, 1.0f/32768.0f, pDataOut, CVSD_BLOCK_SIZE);
		pDataIn += CVSD_BLOCK_SIZE * 4;
		pDataOut += CVSD_BLOCK_SIZE * 4;
		*pInSamples -= CVSD_BLOCK_SIZE;
		nGenerated += CVSD_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}

uint32_t cvsd_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return CVSD_BLOCK_SIZE;
}

DataProcessBlock_t  CVSD = {cvsd_create, cvsd_init, cvsd_data_typesize, cvsd_process, cvsd_close};
