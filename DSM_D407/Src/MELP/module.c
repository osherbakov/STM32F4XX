/*  compiler include files  */
#include "mat.h"
#include "melp.h"
#include "dsp_sub.h"
#include "melp_sub.h"
#include "dataqueues.h"

/* ====== External memory ====== */

typedef unsigned short uint16_t;
typedef signed short int16_t;

int		mode;
int		rate;

#define MELP_FRAME_SIZE  	(180)
#define MELP_FRAME_BYTES  	(MELP_FRAME_SIZE * 4)

/* ========== Static Variables ========== */
static float		speech[MELP_FRAME_SIZE] CCMRAM;
static unsigned char chan_buffer[NUM_CH_BITS];
struct melp_param	melp_ana_par CCMRAM;                 /* melp analysis parameters */
struct melp_param	melp_syn_par CCMRAM;                 /* melp synthesis parameters */

void *melp_create(uint32_t Params)
{
	return 0;
}

void melp_close(void *pHandle)
{
	return;
}

void melp_init(void *pHandle)
{
	/* ====== Initialize MELP analysis and synthesis ====== */
	melp_ana_init(&melp_ana_par);
	melp_syn_init(&melp_syn_par);
}


void melp_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	
	while(*pInBytes >= MELP_FRAME_BYTES )
	{
		arm_scale_f32(pDataIn, 32767.0f, speech, MELP_FRAME_SIZE);
		melp_ana(speech, &melp_ana_par, chan_buffer);
		melp_syn(&melp_syn_par, speech, chan_buffer);
		arm_scale_f32(speech, 1.0f/32768.0f, pDataOut, MELP_FRAME_SIZE);		
		pDataIn = (void *)( (uint32_t)pDataIn + MELP_FRAME_BYTES );
		pDataOut = (void *)((uint32_t)pDataOut + MELP_FRAME_BYTES );
		*pInBytes -= MELP_FRAME_BYTES ;
		nGenerated += MELP_FRAME_BYTES ;
	}
	*pOutBytes =  nGenerated;
}

void melp_data_ready(void *pHandle, DataPort_t *pInData)
{
	pInData->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pInData->Size = MELP_FRAME_BYTES ;
}

void melp_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pIn->Size = MELP_FRAME_BYTES ;
	
	pOut->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pOut->Size = MELP_FRAME_BYTES ;
}

DataProcessBlock_t  MELP = {melp_create, melp_init, melp_info, melp_data_ready, melp_process, melp_close};



