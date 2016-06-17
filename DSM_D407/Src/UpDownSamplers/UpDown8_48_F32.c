#ifndef _MSC_VER
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#endif

#ifdef _MSC_VER
#define CCMRAM
#define RODATA
#else
#define CCMRAM __attribute__((section (".ccmram")))
#define RODATA __attribute__((section (".rodata")))
#endif

#include "arm_math.h"
#include "arm_const_structs.h"
#include "dataqueues.h"

//
//  Downsample 48KHz to 8KHz and 8KHZ to 48KHz upsample functionality modules
//
#define  DOWNSAMPLE_TAPS  		(12)
#define  UPSAMPLE_TAPS			(24)
#define  UPDOWNSAMPLE_RATIO 	(48000/8000)
#define  DOWNSAMPLE_DATA_TYPE	(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define  DOWNSAMPLE_BLOCK_SIZE  (60)	// Divisable by 2,3,4,5,6,10,12,15,20,30
#define  DOWNSAMPLE_BLOCK_BYTES (DOWNSAMPLE_BLOCK_SIZE * 4)


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

static arm_fir_decimate_instance_f32 Dec CCMRAM ;

ProfileData_t	DS48_8_P;
ProfileData_t	US8_48_P;

static void *ds_48_8_create(uint32_t Params)
{
	return &Dec;
}

static void ds_48_8_close(void *pHandle)
{
	return;
}

static void ds_48_8_init(void *pHandle)
{
	arm_fir_decimate_init_f32(pHandle, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO,
			DownSample48_8_Coeff, DownSample48_8_Buff, DOWNSAMPLE_BLOCK_SIZE);
	INIT_PROFILE(&DS48_8_P);
}

static void ds_48_8_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= DOWNSAMPLE_BLOCK_BYTES)
	{
		START_PROFILE(&DS48_8_P);
		arm_fir_decimate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t )pDataIn + DOWNSAMPLE_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t )pDataOut + DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO);
		*pInBytes -= DOWNSAMPLE_BLOCK_BYTES;
		nGenerated += DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
		STOP_PROFILE(&DS48_8_P);
	}
	*pOutBytes = nGenerated;
}


static void ds_48_8_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DOWNSAMPLE_DATA_TYPE;
	pIn->Size = DOWNSAMPLE_BLOCK_BYTES;
	
	pOut->Type = DOWNSAMPLE_DATA_TYPE;
	pOut->Size = DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
}


DataProcessBlock_t  DS_48_8 = {ds_48_8_create, ds_48_8_init, ds_48_8_info, ds_48_8_process, ds_48_8_close};

static arm_fir_interpolate_instance_f32 Int CCMRAM;

static void *us_8_48_create(uint32_t Params)
{
	return &Int;
}

static void us_8_48_close(void *pHandle)
{
	return;
}

static void us_8_48_init(void *pHandle)
{
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSample8_48_Coeff, UpSample8_48_Buff, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
	INIT_PROFILE(&US8_48_P);
}

static void us_8_48_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO)
	{
		START_PROFILE(&US8_48_P);
		arm_fir_interpolate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
		pDataIn = (void *)((uint32_t )pDataIn + DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO);
		pDataOut = (void *)((uint32_t )pDataOut + DOWNSAMPLE_BLOCK_BYTES);
		*pInBytes -= DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
		nGenerated += DOWNSAMPLE_BLOCK_BYTES;
		STOP_PROFILE(&US8_48_P);
	}
	*pOutBytes = nGenerated;
}

static void us_8_48_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DOWNSAMPLE_DATA_TYPE;
	pIn->Size = DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
	
	pOut->Type = DOWNSAMPLE_DATA_TYPE;
	pOut->Size = DOWNSAMPLE_BLOCK_BYTES;
}

DataProcessBlock_t  US_8_48 = {us_8_48_create, us_8_48_init, us_8_48_info, us_8_48_process, us_8_48_close};
