
#ifndef _MSC_VER
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#include "cmsis_os.h"
#endif

#include "arm_math.h"
#include "arm_const_structs.h"


#include "codec2.h"
#include "sine.h"
#include "mat.h"
#include "dataqueues.h"


static struct CODEC2 *p_codec;
static int  frame_size;

#define 	CODEC2_BUFF_SIZE (320)

static float    speech[CODEC2_BUFF_SIZE] CCMRAM;
static unsigned char bits[64] CCMRAM;

void *codec2_create(uint32_t Params)
{
	int mem_req = codec2_state_memory_req();
	p_codec = osAlloc(mem_req);
	return p_codec;
}

void codec2_deinit(void *pHandle)
{
	codec2_close(p_codec);
	osFree(p_codec);
	return;
}

void codec2_initialize(void *pHandle)
{
	codec2_init(p_codec, CODEC2_MODE_1200);
	frame_size = codec2_samples_per_frame(p_codec); 
}

void codec2_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= frame_size)
	{
		arm_scale_f32(pDataIn, 32767.0f, speech, frame_size);
		codec2_encode(p_codec, bits, speech);
		codec2_decode(p_codec, speech, bits);
		arm_scale_f32(speech, 1.0f/32768.0f, pDataOut, frame_size);
		pDataIn = (void *)((uint32_t)pDataIn + frame_size * 4);
		pDataOut =  (void *)((uint32_t)pDataOut + frame_size * 4);
		*pInSamples -= frame_size;
		nGenerated += frame_size;		
	}
	*pOutSamples = nGenerated;
}

void codec2_data_ready(void *pHandle, DataPort_t *pInData)
{
	pInData->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pInData->Size = frame_size;
}

void codec2_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pIn->Size = frame_size;
	
	pOut->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pOut->Size = frame_size;
}


DataProcessBlock_t  CODEC = {codec2_create, codec2_initialize, codec2_info, codec2_data_ready, codec2_process, codec2_deinit};

