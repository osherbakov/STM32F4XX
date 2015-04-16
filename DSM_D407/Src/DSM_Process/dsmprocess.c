#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "datatasks.h"

#include <string.h>
#define __FAST_MATH__
#include <math.h>

#ifndef M_PI
#define M_PI           3.14159265358979323846f
#endif

extern void melp_main(void);
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
		pState->S2 = sinf(PhaseStep);
		pState->TwoCosDelta = 2 * cosf(PhaseStep);
}

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


static int FirstCall = 1;
static SinGen_t SinBlk;
//
//  For test purposes the Fake FF Function just copies input data buffer to output buffer
//
void FF_Process(void *dsmHandle, float *pAudioIn[CH_MAX], float *pAudioOut[CH_MAX], uint32_t nSamples)
{
	memcpy(pAudioOut[CH_LEFT], pAudioIn[CH_LEFT], nSamples * sizeof(float));
	memcpy(pAudioOut[CH_RIGHT], pAudioIn[CH_RIGHT], nSamples * sizeof(float));

	if(FirstCall)
	{
		Gen_Init(&SinBlk, 48000.0f, 1333.0f, 0.99f);
		FirstCall = 0;
	}	
	for(int i = 0; i < nSamples; i++)
	{
		float val = Gen_Sin(&SinBlk);
		for(int j = 0; j < CH_MAX; j++)
		{
				pAudioOut[j][i] = val;
		}
	}
//     melp_main();
}

void FB_Process(void *dsmHandle, float *pIVIn[IV_MAX], uint32_t nSamples)
{
}

