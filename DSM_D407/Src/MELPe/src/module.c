/*  compiler include files  */
#include "sc1200.h"
#include "mat_lib.h"
#include "global.h"
#include "macro.h"
#include "mathhalf.h"
#include "dsp_sub.h"
#include "melp_sub.h"
#include "constant.h"
#include "math_lib.h"
#include "math.h"

#include "dataqueues.h"

#if NPP
#include "npp.h"
#endif


/* ====== External memory ====== */
extern void BSP_LED_On(int LED);
extern void BSP_LED_Off(int LED);
#define LED5 2
#define LED6 3

extern int ENCODER, DECODER;

/* ========== Static Variables ========== */
static int16_t			speech[BLOCK] CCMRAM;
static unsigned char 	chan_buffer[NUM_CH_BITS]  CCMRAM;

void *melpe_create(uint32_t Params)
{
	return 0;
}

void melpe_close(void *pHandle)
{
	return;
}

void melpe_open(void *pHandle, uint32_t Params)
{
	/* ====== Initialize MELP analysis and synthesis ====== */
	melp_parameters->rate = RATE2400;
	melp_parameters->frameSize = FRAME;
	
//	melp_parameters->rate = RATE1200;
//	melp_parameters->frameSize = BLOCK;
	
	melp_ana_init_q(melp_parameters);
	melp_syn_init_q(melp_parameters);
}

void melpe_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	uint32_t	nFrameSize = melp_parameters->frameSize;
	uint32_t	nFrameBytes = nFrameSize * 4;
	
	while(*pInBytes >= nFrameBytes)
	{
		arm_float_to_q15(pDataIn, speech, nFrameSize);
#if NPP
		if (melp_parameters->rate == RATE1200){
			npp(melp_parameters, speech, speech);
			npp(melp_parameters, &(speech[FRAME]), &(speech[FRAME]));
			npp(melp_parameters, &(speech[2*FRAME]), &(speech[2*FRAME]));
		} else
			npp(melp_parameters, speech, speech);
#endif
		BSP_LED_On(LED5); ENCODER = 1;
		analysis_q(speech, melp_parameters, chan_buffer);
		BSP_LED_Off(LED5); ENCODER = 0;
		BSP_LED_On(LED6); DECODER = 1;
		synthesis_q(melp_parameters, speech, chan_buffer);
		BSP_LED_Off(LED6); DECODER = 0;
		arm_q15_to_float(speech, pDataOut, nFrameSize);		
		pDataIn = (void *) ((int32_t)pDataIn + nFrameBytes);
		pDataOut = (void *)((int32_t)pDataOut + nFrameBytes);
		*pInBytes -= nFrameBytes;
		nGenerated += nFrameBytes;
	}
	*pOutBytes = nGenerated;
}

void melpe_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pIn->Size = melp_parameters->frameSize * 4;
	
	pOut->Type = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	pOut->Size = melp_parameters->frameSize * 4;
}


DataProcessBlock_t  MELPE = {melpe_create, melpe_open, melpe_info, melpe_process, melpe_close};
