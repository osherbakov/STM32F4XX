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

#include "cmsis_os.h"

//
//  RATE_SYNC functionality module
//
#define  RATESYNC_BLOCK_SIZE  		(48)
//----------------MONO Version---------------
#define	 RATESYNC_ELEM_SIZE_M		(2)
#define	 RATESYNC_ELEM_STRIDE_M		(1)
#define  RATESYNC_DATA_TYPE_M		(DATA_TYPE_I16 | DATA_NUM_CH_1 | (RATESYNC_ELEM_SIZE_M))
#define  RATESYNC_BLOCK_BYTES_M  	(RATESYNC_BLOCK_SIZE * RATESYNC_ELEM_SIZE_M)
//----------------STEREO Version---------------
#define	 RATESYNC_ELEM_SIZE_S		(4)
#define	 RATESYNC_ELEM_STRIDE_S		(2)
#define  RATESYNC_DATA_TYPE_S		(DATA_TYPE_I16 | DATA_NUM_CH_2 | (RATESYNC_ELEM_SIZE_S))
#define  RATESYNC_BLOCK_BYTES_S  	(RATESYNC_BLOCK_SIZE * RATESYNC_ELEM_SIZE_S)



//
// Input data resync module. The input sampling rate is reported by calling InClock.
//  The output data rate is hard linked to the CPU clock and is equal to SAMPLE_FREQ
//
typedef struct RateSyncData {
		int32_t		DeltaIn;		// The calculated Input difference between samples (CPU-tied)
		int32_t		DeltaOut;		// The calculated Output difference between samples (CPU-tied)
		uint32_t	Delay;			// The delay amount (how Output samples are delayed relative to Input)
		int32_t		AddRemoveCnt;	// The counter of Added (positive) or Removed (negative) samples 
		float		States[3];		// States memory
}RateSyncData_t;

void *ratesync_create(uint32_t Params)
{
	return osAlloc(sizeof(RateSyncData_t));
}

void ratesync_close(void *pHandle)
{
	osFree(pHandle);
	return;
}

void ratesync_init(void *pHandle)
{
		RateSyncData_t	*pRS = (RateSyncData_t	*) pHandle;
		pRS->DeltaIn = SystemCoreClock/1000;		// How many clock ticks in 1 ms for Input samples
		pRS->DeltaOut = SystemCoreClock/1000;		// How many clock ticks in 1 ms for Output samples
		pRS->AddRemoveCnt = 0;
}

void calc_coeff(float *pOutput, float Delay, uint32_t nCoeff)
{
	int32_t		diff;
	uint32_t	n, k;
	float		result;
	for(n = 0; n < nCoeff; n++)
	{
		result = 1.0f;
		for(k = 0; k < nCoeff; k++)
		{
			if(k != n){
				diff = ((int32_t)n - (int32_t)k);
				result = result * (Delay - k)/diff; 
			}
		}
		pOutput[n] = result;
	}
}

void ratesync_process_mono(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nInSamples, nSavedInSamples;
	uint32_t	nOutSamples;
	uint32_t	Delay;				// Delay of output sample relative to input
	uint32_t 	TimeIn, DeltaIn; 
	uint32_t 	TimeOut, DeltaOut;	// Time delta for output samples
	uint32_t	idxIn, idxOut;
	float		C0, C1, C2, C3;
	float		D0, D1, D2, D3, accum;
	float		S1, S2, S3;
	float		NextInSample;
	RateSyncData_t	*pRS = (RateSyncData_t	*) pHandle;

	DeltaIn = pRS->DeltaIn; 
	DeltaOut = pRS->DeltaOut;	
	Delay = pRS->Delay;

	// Initial settings for time:
	TimeIn = 0 + DeltaIn;				// First IN Sample
	TimeOut =  0 - Delay + DeltaOut;	// First OUT Sample
	S1 = pRS->States[0];
	S2 = pRS->States[1];
	S3 = pRS->States[2];

	idxIn =  idxOut = 0;
	nOutSamples = 0;
	nSavedInSamples = nInSamples = *pInBytes / RATESYNC_ELEM_SIZE_M;

	while(nInSamples > 0)
	{
		NextInSample = (float)((int16_t *)pDataIn)[idxIn];
		while(TimeOut <= TimeIn )
		{
			Delay = TimeIn - TimeOut;
			// Calculate the FIR Filter coefficients
			{	
				D0 =  ((float)Delay) /((float)DeltaIn);		// D0 is the fraction of the IN Samples time
				D1 = (D0-1); D2 = (D0-2); D3 = (D0-3);
				//Coeffs[0] = -(D-1)*(D-2)*(D-3)/6.0f;
				//Coeffs[1] = D*(D-2)*(D-3)/2.0f;
				//Coeffs[2] = -D*(D-1)*(D-3)/2.0f;
				//Coeffs[3] = D*(D-1)*(D-2)/6.0f;
				//calc_coeff(Coeffs, D, 4);
				C0 = -D1*D2*D3/6.0f;
				C1 = D0*D2*D3/2.0f;
				C2 = -D0*D1*D3/2.0f;
				C3 = D0*D1*D2/6.0f;

			}
			// Calculate Output value using data in[idxIn]
			{
				accum = 0.5f;
				accum += NextInSample * C0;
				accum += S1 * C1;
				accum += S2 * C2;
				accum += S3 * C3;
			}
			// Save data out[idxOut]
			((int16_t *)pDataOut)[idxOut + 0] = (int16_t) accum;
			idxOut += RATESYNC_ELEM_STRIDE_M;
			TimeOut += DeltaOut;
			nOutSamples++;
		}
		// Shift and Save State from datain[idxIn++]
		S3 = S2; S2 = S1; S1 = NextInSample;
		idxIn += RATESYNC_ELEM_STRIDE_M;			
		TimeIn += DeltaIn;
		nInSamples--;
	}
	// Save the delay value and states for future calculations
	pRS->Delay = Delay;
	pRS->States[0] = S1;
	pRS->States[1] = S2;
	pRS->States[2] = S3;
	pRS->AddRemoveCnt += (nOutSamples - nSavedInSamples);
	*pOutBytes = nOutSamples * RATESYNC_ELEM_SIZE_M;
	*pInBytes = nInSamples * RATESYNC_ELEM_SIZE_M;
}

void ratesync_process_stereo(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes, uint32_t *pOutBytes)
{
	uint32_t	nInSamples, nSavedInSamples;
	uint32_t	nOutSamples;
	uint32_t	Delay;				// Delay of output sample relative to input
	uint32_t 	TimeIn, DeltaIn; 
	uint32_t 	TimeOut, DeltaOut;	// Time delta for output samples
	uint32_t	idxIn, idxOut;
	float		C0, C1, C2, C3;
	float		D0, D1, D2, D3, accum;
	float		S1, S2, S3;
	float		NextInSample;
	RateSyncData_t	*pRS = (RateSyncData_t	*) pHandle;

	DeltaIn = pRS->DeltaIn; 
	DeltaOut = pRS->DeltaOut;	
	Delay = pRS->Delay;

	// Initial settings for time:
	TimeIn = 0 + DeltaIn;				// First IN Sample
	TimeOut =  0 - Delay + DeltaOut;	// First OUT Sample
	S1 = pRS->States[0];
	S2 = pRS->States[1];
	S3 = pRS->States[2];

	idxIn =  idxOut = 0;
	nOutSamples = 0;
	nSavedInSamples = nInSamples = *pInBytes / RATESYNC_ELEM_SIZE_S;

	while(nInSamples > 0)
	{
		NextInSample = (float)((int16_t *)pDataIn)[idxIn];
		while(TimeOut <= TimeIn )
		{
			Delay = TimeIn - TimeOut;
			// Calculate the FIR Filter coefficients
			{	
				D0 =  ((float)Delay) /((float)DeltaIn);		// D0 is the fraction of the IN Samples time
				D1 = (D0-1); D2 = (D0-2); D3 = (D0-3);
				//Coeffs[0] = -(D-1)*(D-2)*(D-3)/6.0f;
				//Coeffs[1] = D*(D-2)*(D-3)/2.0f;
				//Coeffs[2] = -D*(D-1)*(D-3)/2.0f;
				//Coeffs[3] = D*(D-1)*(D-2)/6.0f;
				//calc_coeff(Coeffs, D, 4);
				C0 = -D1*D2*D3/6.0f;
				C1 = D0*D2*D3/2.0f;
				C2 = -D0*D1*D3/2.0f;
				C3 = D0*D1*D2/6.0f;

			}
			// Calculate Output value using data in[idxIn]
			{
				accum = 0.5f;
				accum += NextInSample * C0;
				accum += S1 * C1;
				accum += S2 * C2;
				accum += S3 * C3;
			}
			// Save data out[idxOut]
			((int16_t *)pDataOut)[idxOut + 0] = (int16_t) accum;
			((int16_t *)pDataOut)[idxOut + 1] = (int16_t) accum;
			idxOut += RATESYNC_ELEM_STRIDE_S;
			TimeOut += DeltaOut;
			nOutSamples++;
		}
		// Shift and Save State from datain[idxIn++]
		S3 = S2; S2 = S1; S1 = NextInSample;
		idxIn += RATESYNC_ELEM_STRIDE_S;			
		TimeIn += DeltaIn;
		nInSamples--;
	}
	// Save the delay value and states for future calculations
	pRS->Delay = Delay;
	pRS->States[0] = S1;
	pRS->States[1] = S2;
	pRS->States[2] = S3;
	pRS->AddRemoveCnt += (nOutSamples - nSavedInSamples);
	*pOutBytes = nOutSamples * RATESYNC_ELEM_SIZE_S;
	*pInBytes = nInSamples * RATESYNC_ELEM_SIZE_S;
}


void ratesync_data_ready_mono(void *pHandle, DataPort_t *pInData)
{
	pInData->Type = RATESYNC_DATA_TYPE_M;
	pInData->Size = RATESYNC_BLOCK_BYTES_M;
}

void ratesync_data_ready_stereo(void *pHandle, DataPort_t *pInData)
{
	pInData->Type = RATESYNC_DATA_TYPE_S;
	pInData->Size = RATESYNC_BLOCK_BYTES_S;
}


void ratesync_info_mono(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = RATESYNC_DATA_TYPE_M;
	pIn->Size = RATESYNC_BLOCK_BYTES_M;
	
	pOut->Type = RATESYNC_DATA_TYPE_M;
	pOut->Size = RATESYNC_BLOCK_BYTES_M;
}

void ratesync_info_stereo(void *pHandle, DataPort_t *pIn, DataPort_t *pOut)
{
	pIn->Type = RATESYNC_DATA_TYPE_S;
	pIn->Size = RATESYNC_BLOCK_BYTES_S;
	
	pOut->Type = RATESYNC_DATA_TYPE_S;
	pOut->Size = RATESYNC_BLOCK_BYTES_S;
}


DataProcessBlock_t  RATESYNC_M = {ratesync_create, ratesync_init, ratesync_info_mono, ratesync_data_ready_mono, ratesync_process_mono, ratesync_close};
DataProcessBlock_t  RATESYNC_S = {ratesync_create, ratesync_init, ratesync_info_stereo, ratesync_data_ready_stereo, ratesync_process_stereo, ratesync_close};

