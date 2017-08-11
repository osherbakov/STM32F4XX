#include "arm_math.h"
#include "arm_const_structs.h"


#include "codec2.h"
#include "sine.h"
#include "CMSIS/mat.h"
#include "dataqueues.h"
#include "cmsis_os.h"

extern void BSP_LED_On(int LED);
extern void BSP_LED_Off(int LED);
#define LED5 2
#define LED6 3

extern int ENCODER, DECODER;

#define 	CODEC2_BUFF_SIZE (320)
#define 	CODEC2_BUFF_BYTES (CODEC2_BUFF_SIZE * sizeof(float))

static float    speech[CODEC2_BUFF_SIZE] CCMRAM;
static unsigned char bits[64] CCMRAM;

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
}

void codec2_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	uint32_t	frame_size =  codec2_samples_per_frame(pHandle);
	uint32_t	frame_bytes =  frame_size * sizeof(float);
	while(*pInBytes >= frame_bytes)
	{
		arm_scale_f32(pDataIn, 32767.0f, speech, frame_size);
		BSP_LED_On(LED5); ENCODER = 1;
		codec2_encode(pHandle, bits, speech);
		BSP_LED_Off(LED5); ENCODER = 0;
		BSP_LED_On(LED6); DECODER = 1;
		codec2_decode(pHandle, speech, bits);
		BSP_LED_Off(LED6); DECODER = 0;
		arm_scale_f32(speech, 1.0f/32768.0f, pDataOut, frame_size);

		pDataIn = (void *)((uint32_t)pDataIn + frame_bytes);
		pDataOut =  (void *)((uint32_t)pDataOut + frame_bytes);
		*pInBytes -= frame_bytes;
		nGenerated += frame_bytes;		
	}
	*pOutBytes = nGenerated;
}

void codec2_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	uint32_t	frame_size =  codec2_samples_per_frame(pHandle);
	uint32_t	frame_bytes =  frame_size * sizeof(float);

	pIn->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | sizeof(float);
	pIn->Size = frame_bytes;
	
	pOut->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | sizeof(float);
	pOut->Size = frame_bytes;
}


ProcessBlock_t  CODEC = {codec2_create, codec2_open, codec2_info, codec2_process, codec2_deinit};

