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


//
//  For test purposes the Data_Process Function just copies input data buffer to output buffer
//
uint32_t Data_Process(void *pHandle, void *pAudioIn, void *pAudioOut, uint32_t nElements)
{

}


