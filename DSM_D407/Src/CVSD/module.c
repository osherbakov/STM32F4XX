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
#include "stm32f4_discovery.h"


#define v_equ(v1,v2,n) 			arm_copy_f32((float32_t *)v2, (float32_t *)v1, n)

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

static CVSD_STATE_F32_t ana;
static CVSD_STATE_F32_t syn;

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


uint32_t cvsd_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= CVSD_BLOCK_SIZE)
	{
BSP_LED_On(LED4);
		arm_scale_f32(pDataIn, 32767.0f, pDataIn, CVSD_BLOCK_SIZE);
		cvsd_encode_f32(cvsd_ana, dataBits, pDataIn, CVSD_BLOCK_SIZE);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		cvsd_decode_f32(cvsd_syn, pDataOut, dataBits, CVSD_BLOCK_SIZE);
		arm_scale_f32(pDataOut, 1.0f/32768.0f, pDataOut, CVSD_BLOCK_SIZE);
BSP_LED_Off(LED5);
		pDataIn += CVSD_BLOCK_SIZE * 4;
		pDataOut += CVSD_BLOCK_SIZE * 4;
		*pInSamples -= CVSD_BLOCK_SIZE;
		nGenerated += CVSD_BLOCK_SIZE;
	}
	return nGenerated;
}

uint32_t cvsd_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return CVSD_BLOCK_SIZE;
}

DataProcessBlock_t  CVSD = {cvsd_create, cvsd_init, cvsd_data_typesize, cvsd_process, cvsd_close};

//
//  BYPASS functionality module
//
#define  BYPASS_DATA_TYPE		(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define  BYPASS_BLOCK_SIZE  	(60)

void *bypass_create(uint32_t Params)
{
	return 0;
}

void bypass_close(void *pHandle)
{
	return;
}

void bypass_init(void *pHandle)
{
}

uint32_t bypass_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t nGenerated = *pInSamples;
BSP_LED_On(LED5);
	v_equ(pDataOut, pDataIn, *pInSamples );
BSP_LED_Off(LED5);
	*pInSamples = 0;
	return nGenerated;
}

uint32_t bypass_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = BYPASS_DATA_TYPE;
	 return BYPASS_BLOCK_SIZE;
}

DataProcessBlock_t  BYPASS = {bypass_create, bypass_init, bypass_data_typesize, bypass_process, bypass_close};


//
//  Downsample 48KHz to 16 KHz and 16 KHZ to 48KHz upsample functionality modules
//
#define  DOWNSAMPLE_TAPS  		(12)
#define  UPSAMPLE_TAPS			(24)
#define  UPDOWNSAMPLE_RATIO 	(48000/16000)
#define  DOWNSAMPLE_DATA_TYPE	(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define  DOWNSAMPLE_BLOCK_SIZE  (60)	// Divisable by 2,3,4,5,6,10,12,15,20,30

static float DownSample48_16_Buff[DOWNSAMPLE_BLOCK_SIZE + DOWNSAMPLE_TAPS - 1] CCMRAM;
static float DownSample48_16_Coeff[DOWNSAMPLE_TAPS] RODATA = {
-0.0318333953619003f, -0.0245810560882092f, 0.0154596352949739f, 0.0997937619686127f,
0.200223222374916f, 0.268874526023865f, 0.268874526023865f, 0.200223222374916f,
0.0997937619686127f, 0.0154596352949739f, -0.0245810560882092f, -0.0318333953619003};


static float UpSample16_48_Buff[(DOWNSAMPLE_BLOCK_SIZE + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1] CCMRAM;
static float UpSample16_48_Coeff[UPSAMPLE_TAPS] RODATA = {
0.00449248310178518f, -0.0288104526698589f, -0.0499703288078308f, -0.0734485313296318f,
-0.082396112382412f, -0.0617895275354385f, -0.00137842050753534f, 0.0993839502334595f,
0.228658899664879f, 0.363589704036713f, 0.475823938846588f, 0.539592683315277f,
0.539592683315277f, 0.475823938846588f, 0.363589704036713f, 0.228658899664879f,
0.0993839502334595f, -0.00137842050753534f, -0.0617895275354385f, -0.082396112382412f,
-0.0734485313296318f, -0.0499703288078308f, -0.0288104526698589f, 0.00449248310178518f};

static arm_fir_decimate_instance_f32 CCMRAM Dec ;

void *ds_48_16_create(uint32_t Params)
{
	return &Dec;
}

void ds_48_16_close(void *pHandle)
{
	return;
}

void ds_48_16_init(void *pHandle)
{
	arm_fir_decimate_init_f32(pHandle, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO,
			DownSample48_16_Coeff, DownSample48_16_Buff, DOWNSAMPLE_BLOCK_SIZE);
}

uint32_t ds_48_16_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
//BSP_LED_On(LED3);
	while(*pInSamples >= DOWNSAMPLE_BLOCK_SIZE)
	{
		arm_fir_decimate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE);
		pDataIn += DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF);
		pDataOut += (DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF))/UPDOWNSAMPLE_RATIO;
		*pInSamples -= DOWNSAMPLE_BLOCK_SIZE;
		nGenerated += DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
	}
//BSP_LED_Off(LED3);
	return nGenerated;
}

uint32_t ds_48_16_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DOWNSAMPLE_DATA_TYPE;
	 return DOWNSAMPLE_BLOCK_SIZE;
}

DataProcessBlock_t  DS_48_16 = {ds_48_16_create, ds_48_16_init, ds_48_16_typesize, ds_48_16_process, ds_48_16_close};

static arm_fir_interpolate_instance_f32 CCMRAM Int;

void *us_16_48_create(uint32_t Params)
{
	return &Int;
}

void us_16_48_close(void *pHandle)
{
	return;
}

void us_16_48_init(void *pHandle)
{
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSample16_48_Coeff, UpSample16_48_Buff, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
}

uint32_t us_16_48_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
//BSP_LED_On(LED3);
	while(*pInSamples >= DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO)
	{
		arm_fir_interpolate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
		pDataIn += (DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF))/UPDOWNSAMPLE_RATIO;
		pDataOut += DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF);
		*pInSamples -= DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
		nGenerated += DOWNSAMPLE_BLOCK_SIZE;
	}
//BSP_LED_Off(LED3);
	return nGenerated;
}

uint32_t us_16_48_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DOWNSAMPLE_DATA_TYPE;
	 return DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
}

DataProcessBlock_t  US_16_48 = {us_16_48_create, us_16_48_init, us_16_48_typesize, us_16_48_process, us_16_48_close};

