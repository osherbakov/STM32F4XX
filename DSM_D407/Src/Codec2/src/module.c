#define ARM_MATH_CM4
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1

#include <stdint.h>
#include "cmsis_os.h"

#include "arm_math.h"
#include "arm_const_structs.h"


#include "codec2.h"
#include "sine.h"
#include "mat.h"
#include "dataqueues.h"


#define 	CODEC2_BUFF_SIZE (320)
#define 	CODEC2_BUFF_BYTES (CODEC2_BUFF_SIZE * 4)

static float    speech[CODEC2_BUFF_SIZE] CCMRAM;
static unsigned char bits[64] CCMRAM;

ProfileData_t	CODEC_P;

void *codec2_create(uint32_t Params)
{
	int mem_req = codec2_state_memory_req();
	return osAlloc(mem_req);
}

void codec2_deinit(void *pHandle)
{
	codec2_close(pHandle);
	osFree(pHandle);
	return;
}

void codec2_open(void *pHandle, uint32_t Params)
{
	codec2_init(pHandle, CODEC2_MODE_3200);
	INIT_PROFILE(&CODEC_P);
}

void codec2_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	uint32_t	frame_size =  codec2_samples_per_frame(pHandle);
	uint32_t	frame_bytes =  frame_size * 4;
	while(*pInBytes >= frame_bytes)
	{
		START_PROFILE(&CODEC_P);
		arm_scale_f32(pDataIn, 32767.0f, speech, frame_size);
		codec2_encode(pHandle, bits, speech);
		codec2_decode(pHandle, speech, bits);
		arm_scale_f32(speech, 1.0f/32768.0f, pDataOut, frame_size);

		pDataIn = (void *)((uint32_t)pDataIn + frame_bytes);
		pDataOut =  (void *)((uint32_t)pDataOut + frame_bytes);
		*pInBytes -= frame_bytes;
		nGenerated += frame_bytes;		
		STOP_PROFILE(&CODEC_P);
	}
	*pOutBytes = nGenerated;
}

void codec2_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	uint32_t	frame_size =  codec2_samples_per_frame(pHandle);
	uint32_t	frame_bytes =  frame_size * 4;

	pIn->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pIn->Size = frame_bytes;
	
	pOut->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pOut->Size = frame_bytes;
}


DataProcessBlock_t  CODEC = {codec2_create, codec2_open, codec2_info, codec2_process, codec2_deinit};

