
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
static int  frame_bytes;

#define 	CODEC2_BUFF_SIZE (320)
#define 	CODEC2_BUFF_BYTES (CODEC2_BUFF_SIZE * 4)

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
	frame_bytes = frame_size * 4;
}

void codec2_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= frame_bytes)
	{
		arm_scale_f32(pDataIn, 32767.0f, speech, frame_size);
		codec2_encode(p_codec, bits, speech);
		codec2_decode(p_codec, speech, bits);
		arm_scale_f32(speech, 1.0f/32768.0f, pDataOut, frame_size);
		pDataIn = (void *)((uint32_t)pDataIn + frame_bytes);
		pDataOut =  (void *)((uint32_t)pDataOut + frame_bytes);
		*pInBytes -= frame_bytes;
		nGenerated += frame_bytes;		
	}
	*pOutBytes = nGenerated;
}

void codec2_data_ready(void *pHandle, DataPort_t *pInData)
{
	pInData->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pInData->Size = frame_bytes;
}

void codec2_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pIn->Size = CODEC2_BUFF_BYTES;
	
	pOut->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pOut->Size = CODEC2_BUFF_BYTES;
}


DataProcessBlock_t  CODEC = {codec2_create, codec2_initialize, codec2_info, codec2_data_ready, codec2_process, codec2_deinit};

