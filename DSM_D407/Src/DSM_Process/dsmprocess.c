#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "datatasks.h"

#include <string.h>
#define __FAST_MATH__
#include <math.h>

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

#ifndef M_PI
#define M_PI           3.14159265358979323846f
#endif

typedef enum {
	CH_LEFT = 0,
	CH_RIGHT = 1,
	I_LEFT = 0,
	V_LEFT = 1,
	I_RIGHT = 2,
	V_RIGHT = 3,

	CH_MAX = 2,
	IV_MAX = 4
} Channels_t;


//
//  Sin quadrature generator structure
//
typedef struct SinGen
{
	float Amplitude;
	float TwoCosDelta;
	float S1;
	float S2;
} SinGen_t;

//
// Initialize the Quadrature SIN generator 
//
void Gen_Init(SinGen_t* pState, float SamplingFreq, float GenerateFreq, float Amplitude)
{
		float PhaseStep = 2 * M_PI * GenerateFreq / SamplingFreq;
		pState->Amplitude = Amplitude;
		pState->S1 = 0;
		pState->S2 = arm_sin_f32(PhaseStep);
		pState->TwoCosDelta = 2 * arm_cos_f32(PhaseStep);
}

static int FirstCall = 1;

//
// generate the next sample of the SIN Quadrature generator
//
float Gen_Sin(SinGen_t* pState)
{
	float S1 = pState->S1;
	float S2 = pState->S2;
	pState->S1 = S2;
	pState->S2 = pState->TwoCosDelta * S2 - S1;
	return pState->Amplitude * S1;
}

static SinGen_t SinBlk;

void v_Gen_Sin(SinGen_t* pState, float *pBuff, uint32_t nSamples)
{
	if(FirstCall)
	{
		Gen_Init(pState, 48000.0f, 1333.0f, 0.99f);
		FirstCall = 0;
	}	
	for(int i = 0; i < nSamples; i++)
	{
		float val = Gen_Sin(&SinBlk);
		for(int j = 0; j < CH_MAX; j++)
		{
				pBuff[i] = val;
		}
	}
}

extern void melp_process(float *pDataIn, float *pDataOut, uint32_t nSamples);
extern void cvsd_process(float *pDataIn, float *pDataOut, uint32_t nSamples);

//
//  For test purposes the FF Function just copies input data buffer to output buffer
//
void FF_Process(void *dsmHandle, float *pAudioIn[CH_MAX], float *pAudioOut[CH_MAX], uint32_t nSamples)
{
	//	memcpy(pAudioOut[CH_LEFT], pAudioIn[CH_LEFT], nSamples * sizeof(float));
	//  memcpy(pAudioOut[CH_RIGHT], pAudioIn[CH_RIGHT], nSamples * sizeof(float)); 	
	cvsd_process(pAudioIn[CH_RIGHT], pAudioOut[CH_RIGHT], nSamples);
//	melp_process(pAudioIn[CH_RIGHT], pAudioOut[CH_RIGHT], nSamples);
	memcpy(pAudioOut[CH_LEFT], pAudioOut[CH_RIGHT], nSamples * sizeof(float));
}

void FB_Process(void *dsmHandle, float *pIVIn[IV_MAX], uint32_t nSamples)
{
}

