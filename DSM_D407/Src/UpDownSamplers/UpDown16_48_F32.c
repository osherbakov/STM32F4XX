#include "arm_math.h"
#include "arm_const_structs.h"
#include "dataqueues.h"

#ifdef _MSC_VER
#define CCMRAM
#define RODATA
#else
#define CCMRAM __attribute__((section (".ccmram"))) __attribute__((aligned(4)))
#define RODATA __attribute__((section (".rodata")))
#endif


//
//  BYPASS functionality module
//
#define  BYPASS_DATA_TYPE		(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define  BYPASS_BLOCK_SIZE  	(60)
#define  BYPASS_BLOCK_BYTES  	(BYPASS_BLOCK_SIZE * 4)

static void *bypass_create(uint32_t Params)
{
	return 0;
}

static void bypass_close(void *pHandle)
{
	return;
}

static void bypass_open(void *pHandle, uint32_t Params)
{
}

static void bypass_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t nGenerated = *pInBytes;
	memcpy(pDataOut, pDataIn, nGenerated);
	*pInBytes = 0;
	*pOutBytes = nGenerated;
}

static void bypass_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = BYPASS_DATA_TYPE;
	pIn->Size = BYPASS_BLOCK_BYTES;

	pOut->Type = BYPASS_DATA_TYPE;
	pOut->Size = BYPASS_BLOCK_BYTES;
}


ProcessBlock_t  BYPASS = {bypass_create, bypass_open, bypass_info, bypass_process, bypass_close};


//
//  Downsample 48KHz to 16 KHz and 16 KHZ to 48KHz upsample functionality modules
//
#define  DOWNSAMPLE_TAPS  		(24)
#define  UPSAMPLE_TAPS			(24)
#define  UPDOWNSAMPLE_RATIO 	(48000/16000)
#define  DOWNSAMPLE_DATA_TYPE	(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define  DOWNSAMPLE_BLOCK_SIZE  (60)	// Divisable by 2,3,4,5,6,10,12,15,20,30
#define  DOWNSAMPLE_BLOCK_BYTES (DOWNSAMPLE_BLOCK_SIZE * 4)

static float DownSample48_16_Buff[DOWNSAMPLE_BLOCK_SIZE + DOWNSAMPLE_TAPS - 1] CCMRAM;
static const float DownSample48_16_Coeff[DOWNSAMPLE_TAPS] RODATA = {
//-0.0318333953619003f, -0.0245810560882092f, 0.0154596352949739f, 0.0997937619686127f,
//0.200223222374916f, 0.268874526023865f, 0.268874526023865f, 0.200223222374916f,
//0.0997937619686127f, 0.0154596352949739f, -0.0245810560882092f, -0.0318333953619003
  -0.002015205388767,  -0.014256718305640, -0.018984229704331,  -0.025234247245753,
  -0.024783278937429,  -0.014674724627645,  0.007238951137461,   0.040131597322940,
   0.079972228665562,   0.120093117684145,   0.152725452217424,   0.171036493289764,
   0.171036493289764,   0.152725452217424,   0.120093117684145,   0.079972228665562,
   0.040131597322940,   0.007238951137461, -0.014674724627645,  -0.024783278937429,
  -0.025234247245753,  -0.018984229704331,  -0.014256718305640,  -0.002015205388767
	};


static float UpSample16_48_Buff[(DOWNSAMPLE_BLOCK_SIZE + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1] CCMRAM;
static const float UpSample16_48_Coeff[UPSAMPLE_TAPS] RODATA = {
0.00449248310178518f, -0.0288104526698589f, -0.0499703288078308f, -0.0734485313296318f,
-0.082396112382412f, -0.0617895275354385f, -0.00137842050753534f, 0.0993839502334595f,
0.228658899664879f, 0.363589704036713f, 0.475823938846588f, 0.539592683315277f,
0.539592683315277f, 0.475823938846588f, 0.363589704036713f, 0.228658899664879f,
0.0993839502334595f, -0.00137842050753534f, -0.0617895275354385f, -0.082396112382412f,
-0.0734485313296318f, -0.0499703288078308f, -0.0288104526698589f, 0.00449248310178518f};

static arm_fir_decimate_instance_f32 Dec CCMRAM;

static void *ds_48_16_create(uint32_t Params)
{
	return &Dec;
}

static void ds_48_16_close(void *pHandle)
{
	return;
}

static void ds_48_16_open(void *pHandle, uint32_t Params)
{
	arm_fir_decimate_init_f32(pHandle, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO,
			(float32_t *)DownSample48_16_Coeff, DownSample48_16_Buff, DOWNSAMPLE_BLOCK_SIZE);
}

static void ds_48_16_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= DOWNSAMPLE_BLOCK_BYTES)
	{
		arm_fir_decimate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE);
		pDataIn = (void *)((uint32_t)pDataIn + DOWNSAMPLE_BLOCK_BYTES);
		pDataOut = (void *)((uint32_t)pDataOut + DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO);
		*pInBytes -= DOWNSAMPLE_BLOCK_BYTES;
		nGenerated += DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
	}
	*pOutBytes = nGenerated;
}

static void ds_48_16_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DOWNSAMPLE_DATA_TYPE;
	pIn->Size = DOWNSAMPLE_BLOCK_BYTES;

	pOut->Type = DOWNSAMPLE_DATA_TYPE;
	pOut->Size = DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
}

ProcessBlock_t  DS_48_16 CCMRAM = {ds_48_16_create, ds_48_16_open, ds_48_16_info, ds_48_16_process, ds_48_16_close};

static arm_fir_interpolate_instance_f32 Int CCMRAM;

static void *us_16_48_create(uint32_t Params)
{
	return &Int;
}

static void us_16_48_close(void *pHandle)
{
	return;
}

static void us_16_48_open(void *pHandle, uint32_t Params)
{
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			(float32_t *)UpSample16_48_Coeff, UpSample16_48_Buff, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
}

static void us_16_48_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO)
	{
		arm_fir_interpolate_f32(pHandle, pDataIn, pDataOut, DOWNSAMPLE_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
		pDataIn = (void *)((uint32_t)pDataIn + DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO);
		pDataOut = (void *)((uint32_t)pDataOut + DOWNSAMPLE_BLOCK_BYTES);
		*pInBytes -= DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;
		nGenerated += DOWNSAMPLE_BLOCK_BYTES;
	}
	*pOutBytes = nGenerated;
}

static void us_16_48_info(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = DOWNSAMPLE_DATA_TYPE;
	pIn->Size = DOWNSAMPLE_BLOCK_BYTES/UPDOWNSAMPLE_RATIO;

	pOut->Type = DOWNSAMPLE_DATA_TYPE;
	pOut->Size = DOWNSAMPLE_BLOCK_BYTES;
}

ProcessBlock_t  US_16_48 CCMRAM = {us_16_48_create, us_16_48_open, us_16_48_info, us_16_48_process, us_16_48_close};

