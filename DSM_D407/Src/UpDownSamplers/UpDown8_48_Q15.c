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
//  Downsample 48KHz to 8 KHz and 8 KHZ to 48KHz upsample functionality modules
//
#define  DOWNSAMPLE_TAPS  		(24)
#define  UPSAMPLE_TAPS			(24)
#define  UPDOWNSAMPLE_RATIO 	(48000/8000)
#define  DOWNSAMPLE_DATA_TYPE	(DATA_TYPE_Q15 | DATA_NUM_CH_1 | (2))
#define  DOWNSAMPLE_BLOCK_SIZE  (60)	// Divisable by 2,3,4,5,6,10,12,15,20,30
#define  DOWNSAMPLE_BLOCK_BYTES (DOWNSAMPLE_BLOCK_SIZE * 2)	

static q15_t DownSample48_8_Buff[DOWNSAMPLE_BLOCK_SIZE + DOWNSAMPLE_TAPS - 1] CCMRAM;
static q15_t DownSample48_8_Coeff[DOWNSAMPLE_TAPS] RODATA = {
-18, 812, 856, 470, -650, -2040,
-2684, -1476, 2068, 7316, 12600, 15916,
 15916, 12600, 7316, 2068, -1476, -2684,
-2040, -650, 470, 856, 812, -18
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

static arm_fir_decimate_instance_q15 Dec CCMRAM;

static void *ds_48_8_create(uint32_t Params)
{
	return &Dec;
}

static void ds_48_8_close(void *pHandle)
{
	return;
}

static void ds_48_8_open(void *pHandle, uint32_t Params)
{
	arm_fir_decimate_init_q15(pHandle, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO,
			DownSample48_8_Coeff, DownSample48_8_Buff, DOWNSAMPLE_BLOCK_SIZE);
}

static void ds_48_8_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= DOWNSAMPLE_BLOCK_BYTES)
	{
		arm_fir_decimate_q15(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + DOWNSAMPLE_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO);
		*pInBytes -= DOWNSAMPLE_BLOCK_BYTES;
		nGenerated += DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
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


DataProcessBlock_t  DS_48_8_Q15 CCMRAM = {ds_48_8_create, ds_48_8_open, ds_48_8_info, ds_48_8_process, ds_48_8_close};

static arm_fir_interpolate_instance_q15 Int CCMRAM;

static void *us_8_48_create(uint32_t Params)
{
	return &Int;
}

static void us_8_48_close(void *pHandle)
{
	return;
}

static void us_8_48_open(void *pHandle, uint32_t Params)
{
	arm_fir_interpolate_init_q15(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSample8_48_Coeff, UpSample8_48_Buff, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
}

static void us_8_48_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO)
	{
		arm_fir_interpolate_q15(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
		pDataIn = (void *)((uint32_t)pDataIn + DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO);
		pDataOut = (void *)((uint32_t)pDataOut + DOWNSAMPLE_BLOCK_BYTES);
		*pInBytes -= DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
		nGenerated += DOWNSAMPLE_BLOCK_BYTES;
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


DataProcessBlock_t  US_8_48_Q15 CCMRAM = {us_8_48_create, us_8_48_open, us_8_48_info, us_8_48_process, us_8_48_close};

