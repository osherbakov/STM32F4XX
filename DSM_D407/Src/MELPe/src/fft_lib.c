/* ================================================================== */
/*                                                                    */
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */
/* ================================================================== */

/* ===================================== */
/* fft_lib.c: Fourier series subroutines */
/* ===================================== */

/* compiler include files */
#include "sc1200.h"
#include "mathhalf.h"
#include "mat_lib.h"
#include "math_lib.h"
#include "constant.h"
#include "global.h"
#include "fft_lib.h"

#define SWAP(a, b)		{int16_t temp1; temp1 = (a); (a) = (b); (b) = temp1;}
#define MAX(a,b)		((a)>=(b) ? (a) : (b))

arm_rfft_instance_q15 S CCMRAM;

/* Radix-2, DIT, 2N-point Real FFT */
void rfft(int16_t datam1[], int16_t n)
{
	arm_rfft_q15(&S, datam1, datam1);
	arm_shift_q15(datam1, 3, datam1, FFTLENGTH/2);
}

/* Subroutine FFT: Fast Fourier Transform */
int16_t cfft(int16_t datam1[], int16_t nn)
{
	arm_cfft_q15(&arm_cfft_sR_q15_len256, datam1, 0, 1);
	arm_shift_q15(datam1, 3, datam1, FFTLENGTH);
	return 6;
}


int16_t fft_npp(int16_t data[], int16_t dir)
{

	arm_cfft_q15(&arm_cfft_sR_q15_len256, data, 0, 1);
	arm_shift_q15(data, 3, data, FFTLENGTH);
	return 6;
}


/* Initialization of wr_array and wi_array */
void fs_init()
{
	arm_rfft_init_q15(&S, 256, 0, 1);
}
