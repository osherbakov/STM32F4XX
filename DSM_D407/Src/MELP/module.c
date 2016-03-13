/*  compiler include files  */
#include "mat.h"

#include "melp.h"
#include "dsp_sub.h"
#include "melp_sub.h"
#include "stm32f4_discovery.h"
#include "dataqueues.h"

/* ====== External memory ====== */

typedef unsigned short uint16_t;
typedef signed short int16_t;

int		mode;
int		rate;

#define MELP_FRAME_SIZE  (180)

/* ========== Static Variables ========== */
static float		speech[MELP_FRAME_SIZE] CCMRAM;
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


void melp_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	
	while(*pInSamples >= MELP_FRAME_SIZE)
	{
BSP_LED_On(LED4);
		arm_scale_f32(pDataIn, 32767.0f, speech, MELP_FRAME_SIZE);
		melp_ana(speech, &melp_ana_par);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		melp_syn(&melp_syn_par, speech);
		arm_scale_f32(speech, 1.0f/32768.0f, pDataOut, MELP_FRAME_SIZE);		
BSP_LED_Off(LED5);
//		v_equ(pDataOut, pDataIn, MELP_FRAME_SIZE);		
		pDataIn += MELP_FRAME_SIZE * 4;
		pDataOut += MELP_FRAME_SIZE * 4;
		*pInSamples -= MELP_FRAME_SIZE;
		nGenerated += MELP_FRAME_SIZE;
	}
	*pOutSamples =  nGenerated;
}

uint32_t melp_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return MELP_FRAME_SIZE;
}


DataProcessBlock_t  MELP = {melp_create, melp_init, melp_data_typesize, melp_process, melp_close};


//
//  Downsample 48KHz to 8KHz and 8KHZ to 48KHz upsample functionality modules
//
#define  DOWNSAMPLE_TAPS  		(12)
#define  UPSAMPLE_TAPS			(24)
#define  UPDOWNSAMPLE_RATIO 	(48000/8000)
#define  DOWNSAMPLE_DATA_TYPE	(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define  DOWNSAMPLE_BLOCK_SIZE  (60)	// Divisable by 2,3,4,5,6,10,12,15,20,30


static float DownSample48_8_Buff[DOWNSAMPLE_BLOCK_SIZE + DOWNSAMPLE_TAPS - 1] CCMRAM;
static float DownSample48_8_Coeff[DOWNSAMPLE_TAPS] RODATA = {
-0.0163654778152704f, -0.0205210950225592f, 0.00911782402545214f, 0.0889585390686989f,
0.195298701524735f, 0.272262066602707f, 0.272262066602707f, 0.195298701524735f,
0.0889585390686989f, 0.00911782402545214f, -0.0205210950225592f, -0.0163654778152704f
};


static float UpSample8_48_Buff[(DOWNSAMPLE_BLOCK_SIZE + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1] CCMRAM;
static float UpSample8_48_Coeff[UPSAMPLE_TAPS] RODATA = {
0.0420790389180183f, 0.0118295839056373f, -0.0317823477089405f, -0.10541670024395f,
-0.180645510554314f, -0.209222942590714f, -0.140164494514465f, 0.0557445883750916f,
0.365344613790512f, 0.727912843227386f, 1.05075252056122f, 1.24106538295746f,
1.24106538295746f, 1.05075252056122f, 0.727912843227386f, 0.365344613790512f,
0.0557445883750916f, -0.140164494514465f, -0.209222942590714f, -0.180645510554314f,
-0.10541670024395f, -0.0317823477089405f, 0.0118295839056373f, 0.0420790389180183f
};

static arm_fir_decimate_instance_f32 CCMRAM Dec ;

void *ds_48_8_create(uint32_t Params)
{
	return &Dec;
}

void ds_48_8_close(void *pHandle)
{
	return;
}

void ds_48_8_init(void *pHandle)
{
	arm_fir_decimate_init_f32(pHandle, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO,
			DownSample48_8_Coeff, DownSample48_8_Buff, DOWNSAMPLE_BLOCK_SIZE);
}

void ds_48_8_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= DOWNSAMPLE_BLOCK_SIZE)
	{
		arm_fir_decimate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE);
		pDataIn += DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF);
		pDataOut += (DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF))/UPDOWNSAMPLE_RATIO;
		*pInSamples -= DOWNSAMPLE_BLOCK_SIZE;
		nGenerated += DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
	}
	*pOutSamples = nGenerated;
}

uint32_t ds_48_8_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DOWNSAMPLE_DATA_TYPE;
	 return DOWNSAMPLE_BLOCK_SIZE;
}

DataProcessBlock_t  DS_48_8 = {ds_48_8_create, ds_48_8_init, ds_48_8_typesize, ds_48_8_process, ds_48_8_close};

static arm_fir_interpolate_instance_f32 CCMRAM Int;

void *us_8_48_create(uint32_t Params)
{
	return &Int;
}

void us_8_48_close(void *pHandle)
{
	return;
}

void us_8_48_init(void *pHandle)
{
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSample8_48_Coeff, UpSample8_48_Buff, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
}

void us_8_48_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples, uint32_t *pOutSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO)
	{
		arm_fir_interpolate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
		pDataIn += (DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF))/UPDOWNSAMPLE_RATIO;
		pDataOut += DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF);
		*pInSamples -= DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
		nGenerated += DOWNSAMPLE_BLOCK_SIZE;
	}
	*pOutSamples = nGenerated;
}

uint32_t us_8_48_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DOWNSAMPLE_DATA_TYPE;
	 return DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
}

DataProcessBlock_t  US_8_48 = {us_8_48_create, us_8_48_init, us_8_48_typesize, us_8_48_process, us_8_48_close};
