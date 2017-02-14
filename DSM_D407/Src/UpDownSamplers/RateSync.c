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
		float		States[6];		// States memory
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

void ratesync_open(void *pHandle, uint32_t Params)
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
	float		D0, D1, D2, D3, D0D1, D2D3, accum;
	float		S1, S2, S3;
	float		NextInSample;
	float		ONE, ONE_HALF, ONE_SIXTH;
	
	RateSyncData_t	*pRS = (RateSyncData_t	*) pHandle;

	DeltaIn = pRS->DeltaIn; 
	DeltaOut = pRS->DeltaOut;	
	Delay = pRS->Delay;

	ONE = 1.0f;
	ONE_HALF = 0.5f;
	ONE_SIXTH = 1.0f/6.0f;

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
				D1 = (D0-ONE); D2 = (D1-ONE); D3 = (D2-ONE);
				D0D1 = D0*D1; D2D3 = D2*D3;
				//Coeffs[0] = -(D-1)*(D-2)*(D-3)/6.0f;
				//Coeffs[1] = D*(D-2)*(D-3)/2.0f;
				//Coeffs[2] = -D*(D-1)*(D-3)/2.0f;
				//Coeffs[3] = D*(D-1)*(D-2)/6.0f;
				//calc_coeff(Coeffs, D, 4);
				C0 = -D1*D2D3*ONE_SIXTH;
				C1 = D0*D2D3*ONE_HALF;
				C2 = -D0D1*D3*ONE_HALF;
				C3 = D0D1*D2*ONE_SIXTH;
			}
			// Calculate Output value using data in[idxIn]
			{
				accum = ONE_HALF;
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
	float		D0, D1, D2, D3, D0D1, D2D3, accum;
	float		S1L, S2L, S3L;
	float		S1R, S2R, S3R;
	float		NextInLSample, NextInRSample;
	float		ONE, ONE_HALF, ONE_SIXTH;
	
	RateSyncData_t	*pRS = (RateSyncData_t	*) pHandle;

	DeltaIn = pRS->DeltaIn; 
	DeltaOut = pRS->DeltaOut;	
	Delay = pRS->Delay;

	ONE = 1.0f;
	ONE_HALF = 0.5f;
	ONE_SIXTH = 1.0f/6.0f;
	
	
	// Initial settings for time:
	TimeIn = 0 + DeltaIn;				// First IN Sample
	TimeOut =  0 - Delay + DeltaOut;	// First OUT Sample
	S1L = pRS->States[0];
	S2L = pRS->States[1];
	S3L = pRS->States[2];
	S1R = pRS->States[3];
	S2R = pRS->States[4];
	S3R = pRS->States[5];

	idxIn =  idxOut = 0;
	nOutSamples = 0;
	nSavedInSamples = nInSamples = *pInBytes / RATESYNC_ELEM_SIZE_S;

	while(nInSamples > 0)
	{
		NextInLSample = (float)((int16_t *)pDataIn)[idxIn + 0];
		NextInRSample = (float)((int16_t *)pDataIn)[idxIn + 1];
		while(TimeOut <= TimeIn )
		{
			Delay = TimeIn - TimeOut;
			// Calculate the FIR Filter coefficients
			{	
				D0 =  ((float)Delay) /((float)DeltaIn);		// D0 is the fraction of the IN Samples time
				D1 = (D0-ONE); D2 = (D1-ONE); D3 = (D2-ONE);
				D0D1 = D0*D1; D2D3 = D2*D3;
				//Coeffs[0] = -(D-1)*(D-2)*(D-3)/6.0f;
				//Coeffs[1] = D*(D-2)*(D-3)/2.0f;
				//Coeffs[2] = -D*(D-1)*(D-3)/2.0f;
				//Coeffs[3] = D*(D-1)*(D-2)/6.0f;
				//calc_coeff(Coeffs, D, 4);
				C0 = -D1*D2D3*ONE_SIXTH;
				C1 = D0*D2D3*ONE_HALF;
				C2 = -D0D1*D3*ONE_HALF;
				C3 = D0D1*D2*ONE_SIXTH;
			}
			// Calculate Output value using data in[idxIn]
			{
				accum = ONE_HALF;
				accum += NextInLSample * C0;
				accum += S1L * C1;
				accum += S2L * C2;
				accum += S3L * C3;
			}
			// Save data out[idxOut + 0]
			((int16_t *)pDataOut)[idxOut + 0] = (int16_t) accum;

			// Calculate Output value using data in[idxIn + 1]
			{
				accum = ONE_HALF;
				accum += NextInRSample * C0;
				accum += S1R * C1;
				accum += S2R * C2;
				accum += S3R * C3;
			}
			// Save data out[idxOut + 1]
			((int16_t *)pDataOut)[idxOut + 1] = (int16_t) accum;
			idxOut += RATESYNC_ELEM_STRIDE_S;
			TimeOut += DeltaOut;
			nOutSamples++;
		}
		// Shift and Save State from datain[idxIn++]
		S3L = S2L; S2L = S1L; S1L = NextInLSample;
		S3R = S2R; S2R = S1R; S1R = NextInRSample;
		idxIn += RATESYNC_ELEM_STRIDE_S;			
		TimeIn += DeltaIn;
		nInSamples--;
	}
	// Save the delay value and states for future calculations
	pRS->Delay = Delay;
	pRS->States[0] = S1L;
	pRS->States[1] = S2L;
	pRS->States[2] = S3L;
	pRS->States[3] = S1R;
	pRS->States[4] = S2R;
	pRS->States[5] = S3R;
	pRS->AddRemoveCnt += (nOutSamples - nSavedInSamples);
	*pOutBytes = nOutSamples * RATESYNC_ELEM_SIZE_S;
	*pInBytes = nInSamples * RATESYNC_ELEM_SIZE_S;
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


DataProcessBlock_t  RATESYNC_M CCMRAM = {ratesync_create, ratesync_open, ratesync_info_mono, ratesync_process_mono, ratesync_close};
DataProcessBlock_t  RATESYNC_S CCMRAM = {ratesync_create, ratesync_open, ratesync_info_stereo, ratesync_process_stereo, ratesync_close};

