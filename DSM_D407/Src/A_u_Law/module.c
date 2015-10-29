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
#include "aulaw_q15.h"
#include "aulaw_data_q15.h"
#include "dataqueues.h"
#include "stm32f4_discovery.h"


#define AU_LAW_BLOCK_SIZE   		(60)


static uint8_t dataBits[AU_LAW_BLOCK_SIZE] CCMRAM;

void *aulaw_create(uint32_t Params)
{
	return 0;
}

void aulaw_close(void *pHandle)
{
	return;
}

void aulaw_init(void *pHandle)
{
}

uint32_t alaw_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
BSP_LED_On(LED4);
		alaw_encode_q15(pHandle, dataBits, pDataIn, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		alaw_decode_q15(pHandle, pDataOut, dataBits, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED5);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	return nGenerated;
}

uint32_t alaw_encode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
BSP_LED_On(LED4);
		alaw_encode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED4);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	return nGenerated;
}

uint32_t alaw_decode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
BSP_LED_On(LED5);
		alaw_decode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED5);
		pDataIn += AU_LAW_BLOCK_SIZE;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	return nGenerated;
}

uint32_t ulaw_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
BSP_LED_On(LED4);
		ulaw_encode_q15(pHandle, dataBits, pDataIn, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		ulaw_decode_q15(pHandle, pDataOut, dataBits, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED5);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	return nGenerated;
}

uint32_t ulaw_encode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
BSP_LED_On(LED4);
		ulaw_encode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED4);
		pDataIn += AU_LAW_BLOCK_SIZE * 2;
		pDataOut += AU_LAW_BLOCK_SIZE;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	return nGenerated;
}

uint32_t ulaw_decode_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
	while(*pInSamples >= AU_LAW_BLOCK_SIZE)
	{
BSP_LED_On(LED5);
		ulaw_decode_q15(pHandle, pDataOut, pDataIn, AU_LAW_BLOCK_SIZE);
BSP_LED_Off(LED5);
		pDataIn += AU_LAW_BLOCK_SIZE;
		pDataOut += AU_LAW_BLOCK_SIZE * 2;
		*pInSamples -= AU_LAW_BLOCK_SIZE;
		nGenerated += AU_LAW_BLOCK_SIZE;
	}
	return nGenerated;
}


uint32_t aulaw_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	 return AU_LAW_BLOCK_SIZE;
}

uint32_t aulaw_encode_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2);
	 return AU_LAW_BLOCK_SIZE;
}

uint32_t aulaw_decode_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_Q7 | DATA_NUM_CH_1 | (1);
	 return AU_LAW_BLOCK_SIZE;
}

DataProcessBlock_t  ALAW = {aulaw_create, aulaw_init, aulaw_data_typesize, alaw_process, aulaw_close};
DataProcessBlock_t  ULAW = {aulaw_create, aulaw_init, aulaw_data_typesize, ulaw_process, aulaw_close};

DataProcessBlock_t  ALAW_ENC = {aulaw_create, aulaw_init, aulaw_encode_typesize, alaw_encode_process, aulaw_close};
DataProcessBlock_t  ULAW_ENC = {aulaw_create, aulaw_init, aulaw_encode_typesize, ulaw_encode_process, aulaw_close};
DataProcessBlock_t  ALAW_DEC = {aulaw_create, aulaw_init, aulaw_decode_typesize, alaw_decode_process, aulaw_close};
DataProcessBlock_t  ULAW_DEC = {aulaw_create, aulaw_init, aulaw_decode_typesize, ulaw_decode_process, aulaw_close};


//
//  Downsample 48KHz to 8 KHz and 8 KHZ to 48KHz upsample functionality modules
//
#define  DOWNSAMPLE_TAPS  		(24)
#define  UPSAMPLE_TAPS			(24)
#define  UPDOWNSAMPLE_RATIO 	(48000/8000)
#define  DOWNSAMPLE_DATA_TYPE	(DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2))
#define  DOWNSAMPLE_BLOCK_SIZE  (60)	// Divisable by 2,3,4,5,6,10,12,15,20,30

static q15_t DownSample48_8_Buff[DOWNSAMPLE_BLOCK_SIZE + DOWNSAMPLE_TAPS - 1] CCMRAM;
static q15_t DownSample48_8_Coeff[DOWNSAMPLE_TAPS] RODATA = {
-9, 406, 428, 235, -325, -1020,
-1342, -738, 1034, 3658, 6300, 7958,
 7958, 6300, 3658, 1034, -738, -1342,
-1020, -325, 235, 428, 406, -9
};

static q15_t UpSample8_48_Buff[(DOWNSAMPLE_BLOCK_SIZE + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1] CCMRAM;
static q15_t UpSample8_48_Coeff[UPSAMPLE_TAPS] RODATA = {
         147,        -945,       -1638,       -2407,
       -2700,       -2025,         -46,        3256,
        7492,       11914,       15591,       17681,
       17681,       15591,       11914,        7492,
        3256,         -46,       -2025,       -2700,
       -2407,       -1638,        -945,         147
};

static arm_fir_decimate_instance_q15 CCMRAM Dec ;

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
	arm_fir_decimate_init_q15(pHandle, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO,
			DownSample48_8_Coeff, DownSample48_8_Buff, DOWNSAMPLE_BLOCK_SIZE);
}

static uint32_t ds_48_8_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
//BSP_LED_On(LED3);
	while(*pInSamples >= DOWNSAMPLE_BLOCK_SIZE)
	{
		arm_fir_decimate_q15(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE);
		pDataIn += DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF);
		pDataOut += (DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF))/UPDOWNSAMPLE_RATIO;
		*pInSamples -= DOWNSAMPLE_BLOCK_SIZE;
		nGenerated += DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
	}
//BSP_LED_Off(LED3);
	return nGenerated;
}

static uint32_t ds_48_8_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DOWNSAMPLE_DATA_TYPE;
	 return DOWNSAMPLE_BLOCK_SIZE;
}

DataProcessBlock_t  DS_48_8_Q15 = {ds_48_8_create, ds_48_8_init, ds_48_8_typesize, ds_48_8_process, ds_48_8_close};

static arm_fir_interpolate_instance_q15 CCMRAM Int;

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
	arm_fir_interpolate_init_q15(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSample8_48_Coeff, UpSample8_48_Buff, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
}

static uint32_t us_8_48_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInSamples)
{
	uint32_t	nGenerated = 0;
//BSP_LED_On(LED3);
	while(*pInSamples >= DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO)
	{
		arm_fir_interpolate_q15(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
		pDataIn += (DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF))/UPDOWNSAMPLE_RATIO;
		pDataOut += DOWNSAMPLE_BLOCK_SIZE * (DOWNSAMPLE_DATA_TYPE & 0x00FF);
		*pInSamples -= DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
		nGenerated += DOWNSAMPLE_BLOCK_SIZE;
	}
//BSP_LED_Off(LED3);
	return nGenerated;
}

static uint32_t us_8_48_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DOWNSAMPLE_DATA_TYPE;
	 return DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO;
}

DataProcessBlock_t  US_8_48_Q15 = {us_8_48_create, us_8_48_init, us_8_48_typesize, us_8_48_process, us_8_48_close};

