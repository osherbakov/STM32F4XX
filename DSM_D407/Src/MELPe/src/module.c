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

typedef void *Data_Create_t(uint32_t Params);
typedef void Data_Init_t(void *pHandle);
typedef uint32_t Data_TypeSize_t(void *pHandle, uint32_t *pDataType);
typedef void Data_Process_t(void *pHandle, void *pIn, void *pOut, uint32_t *pInElements, uint32_t *pOutElements);
typedef void Data_Close_t(void *pHandle);

/* ====== External memory ====== */

typedef unsigned short uint16_t;
typedef signed short int16_t;

/* ========== Static Variables ========== */
static int16_t	speech[BLOCK] CCMRAM;

void *melpe_create(uint32_t Params)
{
	return 0;
}

void melpe_close(void *pHandle)
{
	return;
}

void melpe_init(void *pHandle)
{
	/* ====== Initialize MELP analysis and synthesis ====== */
	melp_parameters->rate = RATE2400;
	melp_parameters->frameSize = FRAME;
	melp_parameters->bitSize = 7;
	melp_parameters->chwordSize = 8;
	
	melp_ana_init_q(melp_parameters);
	melp_syn_init_q(melp_parameters);
}

void melpe_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	uint32_t	nFrameSize = melp_parameters->frameSize;
	
	while(*pInSamples >= nFrameSize)
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
		analysis_q(speech, melp_parameters);
		synthesis_q(melp_parameters, speech);
		arm_q15_to_float(speech, pDataOut, nFrameSize);		
		pDataIn = (void *) ((int32_t)pDataIn + nFrameSize * 4);
		pDataOut = (void *)((int32_t)pDataOut + nFrameSize * 4);
		*pInSamples -= nFrameSize;
		nGenerated += nFrameSize;
	}
	*pOutSamples = nGenerated;
}

uint32_t melpe_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return melp_parameters->frameSize;
}


DataProcessBlock_t  MELPE = {melpe_create, melpe_init, melpe_data_typesize, melpe_process, melpe_close};
