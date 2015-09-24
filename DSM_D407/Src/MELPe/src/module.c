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

#include "stm32f4_discovery.h"

#if NPP
#include "npp.h"
#endif


typedef enum DataType
{
	DATA_TYPE_BITS 		= 0x0000,		// Bits, not grouped into integers
	DATA_TYPE_I8 		= 0x0000,		// 8-bit Integer, full range 0 - 255 (-128 +127)
	DATA_TYPE_Q7		= 0x0100,		// Q7 signed (-1.0 +1.0)
	DATA_TYPE_I16		= 0x0400,		// 16-bit Integer, full range (-32768 +32768)
	DATA_TYPE_Q15   	= 0x0500,		// Q15 signed, range (-1.0 +1.0)
	DATA_TYPE_I24		= 0x0800,		// 24-bit Integer, full range
	DATA_TYPE_Q23		= 0x0900,   // Q23 signed, range (-1.0 +1.0)
	DATA_TYPE_I32		= 0x0C00,		// 32-bit Integer
	DATA_TYPE_Q31   	= 0x0D00,		// Q31 signed, range (-1.0 + 1.0)
	DATA_TYPE_F32_32K	= 0x0E00,		// 32-bit Floating point, range (-32768.0  +32767.0)
	DATA_TYPE_F32		= 0x0F00,		// 32-bit Floating point, range (-1.0 +1.0)
	DATA_TYPE_MASK		= 0x0F00,
	DATA_RANGE_MASK		= 0x0100,		// Range is limited to (-1.0 +1.0)
	DATA_FP_MASK		= 0x0200,		// Floating Point representation
	DATA_TYPE_SHIFT   = 10
}DataType_t;

#define DATA_TYPE_SIZE(a)  					((((a) & DATA_TYPE_MASK) >> DATA_TYPE_SHIFT) + 1)

typedef enum DataChannels
{
	DATA_NUM_CH_NONE 	= 0x0000,
	DATA_NUM_CH_1 		= 0x0000,
	DATA_NUM_CH_2 		= 0x1000,
	DATA_NUM_CH_3 		= 0x2000,
	DATA_NUM_CH_4 		= 0x3000,
	DATA_NUM_CH_5 		= 0x4000,
	DATA_NUM_CH_6 		= 0x5000,
	DATA_NUM_CH_7 		= 0x6000,
	DATA_NUM_CH_8 		= 0x7000,
	DATA_ALT 			= 0x0000,		// If more than 1 channel, the elements are interleaved/alternating
	DATA_SEQ			= 0x8000,		// If more than 1 channel, all the elements of one channel follow all of another
	DATA_NUM_CH_MASK	= 0x7000,
	DATA_SEQ_MASK 		= 0x8000,
	DATA_NUM_CH_SHIFT = 12
} DataChannels_t;

#define DATA_TYPE_NUM_CHANNELS(a) 	((((a) & DATA_NUM_CH_MASK) >> DATA_NUM_CH_SHIFT) + 1)

typedef enum DataChannelMask
{
	DATA_CHANNEL_ANY 	= 0x0000,
	DATA_CHANNEL_1 		= 0x0001,
	DATA_CHANNEL_2 		= 0x0002,
	DATA_CHANNEL_3 		= 0x0004,
	DATA_CHANNEL_4 		= 0x0008,
	DATA_CHANNEL_5 		= 0x0010,
	DATA_CHANNEL_6 		= 0x0020,
	DATA_CHANNEL_7 		= 0x0040,
	DATA_CHANNEL_8 		= 0x0080,
	DATA_CHANNEL_ALL	= 0x00FF
} DataChannelMask_t;


typedef void *Data_Create_t(uint32_t Params);
typedef void Data_Init_t(void *pHandle);
typedef uint32_t Data_TypeSize_t(void *pHandle, uint32_t *pDataType);
typedef uint32_t Data_Process_t(void *pHandle, void *pIn, void *pOut, uint32_t *pInElements);
typedef void Data_Close_t(void *pHandle);

typedef struct DataProcessBlock {
	Data_Create_t		*Create;
	Data_Init_t			*Init;
	Data_TypeSize_t		*TypeSize;
	Data_Process_t  	*Process;
	Data_Close_t		*Close;
} DataProcessBlock_t;

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

uint32_t melpe_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	uint32_t	nFrameSize = melp_parameters->frameSize;
	
	while(*pInSamples >= nFrameSize)
	{
BSP_LED_On(LED4);
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
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		synthesis_q(melp_parameters, speech);
		arm_q15_to_float(speech, pDataOut, nFrameSize);		
BSP_LED_Off(LED5);
//		v_equ(pDataOut, pDataIn, MELP_FRAME_SIZE);		
		pDataIn = (void *) ((int32_t)pDataIn + nFrameSize * 4);
		pDataOut = (void *)((int32_t)pDataOut + nFrameSize * 4);
		*pInSamples -= nFrameSize;
		nGenerated += nFrameSize;
	}
	return nGenerated;
}

uint32_t melpe_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return melp_parameters->frameSize;
}


DataProcessBlock_t  MELPE = {melpe_create, melpe_init, melpe_data_typesize, melpe_process, melpe_close};
