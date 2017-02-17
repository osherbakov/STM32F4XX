/*

2.4 kbps MELP Proposed Federal Standard speech coder

Fixed-point C code, version 1.0

Copyright (c) 1998, Texas Instruments, Inc.

Texas Instruments has intellectual property rights on the MELP
algorithm.	The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).

The fixed-point version of the voice codec Mixed Excitation Linear
Prediction (MELP) is based on specifications on the C-language software
simulation contained in GSM 06.06 which is protected by copyright and
is the property of the European Telecommunications Standards Institute
(ETSI). This standard is available from the ETSI publication office
tel. +33 (0)4 92 94 42 58. ETSI has granted a license to United States
Department of Defense to use the C-language software simulation contained
in GSM 06.06 for the purposes of the development of a fixed-point
version of the voice codec Mixed Excitation Linear Prediction (MELP).
Requests for authorization to make other use of the GSM 06.06 or
otherwise distribute or modify them need to be addressed to the ETSI
Secretariat fax: +33 493 65 47 16.

*/

/* =============================== */
/* dsp_sub.c: general subroutines. */
/* =============================== */

/*	compiler include files	*/
#include "sc1200.h"
#include "macro.h"
#include "mathhalf.h"
#include "mat_lib.h"
#include "math_lib.h"
#include "dsp_sub.h"
#include "constant.h"
#include "global.h"

#define MAXSIZE			1024
#define LIMIT_PEAKI		20723               /* Upper limit of (magsq/sum_abs) */
                                        /* to prevent saturation of peak_fact */
#define C2_Q14			-15415                         /* -0.9409 * (1 << 14) */
#define C1_Q14			31565                           /* 1.9266 * (1 << 14) */
#define A				16807u             /* Multiplier for rand_minstdgen() */


/* Prototype */
static int16_t	rand_minstdgen();

//uint32_t	L_mpyu(uint16_t var1, uint16_t var2);
#define L_mpyu(a,b) ((int32_t)(a) * (b))

/* Subroutine envelope: calculate time envelope of signal.                    */
/* Note: the delay history requires one previous sample	of the input signal   */
/* and two previous output samples.  Output is scaled down by 4 bits from     */
/* input signal.  input[], prev_in and output[] are of the same Q value.      */

void envelope_q(int16_t input[], int16_t prev_in, int16_t output[],
			  int16_t npts)
{
	register int16_t	i;
	int16_t	curr_abs, prev_abs;
	int32_t	L_temp;


	prev_abs = abs_s(prev_in);
	for (i = 0; i < npts; i++) {
		curr_abs = abs_s(input[i]);

		/* output[i] = curr_abs - prev_abs + C2*output[i-2] + C1*output[i-1] */
		L_temp = L_shr(L_deposit_h(sub(curr_abs, prev_abs)), 5);
		L_temp = L_mac(L_temp, C1_Q14, output[i - 1]);
		L_temp = L_mac(L_temp, C2_Q14, output[i - 2]);
		L_temp = L_shl(L_temp, 1);
		output[i] = round_l(L_temp);

		prev_abs = curr_abs;
	}
}


/* Subroutine interp_array: interpolate array                  */
/*                                                             */
/*	Q values:                                                  */
/*      ifact - Q15, prev[], curr[], out[] - the same Q value. */

void interp_array_q(int16_t prev[], int16_t curr[], int16_t out[],
				  int16_t ifact, int16_t size)
{
	register int16_t	i;
	int16_t	ifact2;
	int16_t	temp1, temp2;


	if (ifact == 0)
		v_equ(out, prev, size);
	else if (ifact == ONE_Q15)
		v_equ(out, curr, size);
	else {
		ifact2 = sub(ONE_Q15, ifact);
		for (i = 0; i < size; i++){
			temp1 = mult(ifact, curr[i]);
			temp2 = mult(ifact2, prev[i]);
			out[i] = add(temp1, temp2);
		}
	}
}


/* Subroutine median: calculate median value of an array with 3 entries.      */

int16_t median3(int16_t input[])
{
	int16_t	min, max, temp;


	/* In this coder median() is always invoked with npts being NF (== 3).    */
	/* Therefore we can hardwire npts to NF and optimize the procedure and    */
	/* name the result median3().                                             */

	min = (int16_t) Min(input[0], input[1]);
	max = (int16_t) Max(input[0], input[1]);
	temp = input[2];
	if (temp < min)
		return(min);
	else if (temp > max)
		return(max);
	else
		return(temp);
}


/* ========================================================================== */
/* This function packs bits of "code" into channel.  "numbits" bits of "code" */
/*    is used and they are packed into the array pointed by "ptr_ch_begin".   */
/*    "ptr_ch_bit" points to the position of the next bit being copied onto.  */
/* ========================================================================== */
void pack_code_q(int16_t code, unsigned char **ptr_ch_begin,
			   int16_t *ptr_ch_bit, int16_t numbits, int16_t wsize)
{
	register int16_t	i;
	unsigned char	*ch_word;
	int16_t	ch_bit;
	int16_t	temp;


	ch_bit = *ptr_ch_bit;
	ch_word = *ptr_ch_begin;

	for (i = 0; i < numbits; i++){   /* Mask in bit from code to channel word */
		/*	temp = shr(code & (shl(1, i)), i); */
		temp = (int16_t) (code & 0x0001);
		if (ch_bit == 0)
			*ch_word = (unsigned char) temp;
		else
			*ch_word |= (unsigned char) (shl(temp, ch_bit));

		/* Check for full channel word */
		ch_bit++;
		if (ch_bit >= wsize){
			ch_bit = 0;
			(*ptr_ch_begin)++;
			ch_word++;
		}
		code = shr(code, 1);
	}

	/* Save updated bit counter */
	*ptr_ch_bit = ch_bit;
}


/* Subroutine peakiness: estimate peakiness of input signal using ratio of L2 */
/* to L1 norms.                                                               */
/*                                                                            */
/* Q_values                                                                   */
/* --------                                                                   */
/* peak_fact - Q12, input - Q0                                                */

static int16_t temp_buf[PITCHMAX] CCMRAM;

int16_t peakiness_q(int16_t input[], int16_t npts)
{
	register int16_t	i;
	int16_t	peak_fact, scale = 4;
	int32_t	sum_abs, L_temp;
	int16_t	temp1, temp2;


	v_equ_shr(temp_buf, input, scale, npts);
	L_temp = L_v_magsq(temp_buf, npts, 0, 1);

	if (L_temp){
		temp1 = norm_l(L_temp);
		scale -= temp1/2;
		if (scale < 0)	scale = 0;
	} else
		scale = 0;

	sum_abs = 0;
	for (i = 0; i < npts; i++){
		L_temp = L_deposit_l(abs_s(input[i]));
		sum_abs = L_add(sum_abs, L_temp);
	}

	/* Right shift input signal and put in temp buffer.                       */
	if (scale)
		v_equ_shr(temp_buf, input, scale, npts);

	if (sum_abs > 0){
		/*	peak_fact = sqrt(npts * v_magsq(input, npts))/sum_abs             */
		/*			  = sqrt(npts) * (sqrt(v_magsq(input, npts))/sum_abs)     */
		if (scale)
			L_temp = L_v_magsq(temp_buf, npts, 0, 0);
		else
			L_temp = L_v_magsq(input, npts, 0, 0);
		L_temp = L_deposit_l(L_sqrt_fxp(L_temp, 0));          /* L_temp in Q0 */
		peak_fact = L_divider2(L_temp, sum_abs, 0, 0);

		if (peak_fact > LIMIT_PEAKI){
			peak_fact = SW_MAX;
		} else {                    /* shl 7 is mult , other shift is Q7->Q12 */
			temp1 = scale + 5;
			temp2 = shl(npts, 7);
			temp2 = sqrt_fxp(temp2, 7);
			L_temp = L_mult(peak_fact, temp2);
			L_temp = L_shl(L_temp, temp1);
			peak_fact = extract_h(L_temp);
		}
	} else
		peak_fact = 0;

	return(peak_fact);
}


/* Subroutine quant_u(): quantize positive input value with	symmetrical       */
/* uniform quantizer over given positive input range.                         */

void quant_u_q(int16_t *p_data, int16_t *p_index, int16_t qmin,
			 int16_t qmax, int16_t nlev, int16_t nlev_q,
			 int16_t double_flag, int16_t scale)
{
	register int16_t	i;
	int16_t	step, half_step, qbnd, *p_in;
	int32_t	L_step, L_half_step, L_qbnd, L_qmin, L_p_in;
	int16_t	temp;
	int32_t	L_temp;

	p_in = p_data;

	/*  Define symmetrical quantizer stepsize 	*/
	/* step = (qmax - qmin) / (nlev - 1); */
	temp = sub(qmax, qmin);
	step = divide_s(temp, nlev_q);

	if (double_flag){
		/* double precision specified */
		/*	Search quantizer boundaries 					*/
		/*qbnd = qmin + (0.5 * step); */
		L_step = L_deposit_l(step);
		L_half_step = L_shr(L_step, 1);
		L_qmin = L_shl(L_deposit_l(qmin), scale);
		L_qbnd = L_add(L_qmin, L_half_step);

		L_p_in = L_shl(L_deposit_l(*p_in), scale);
		for (i = 0; i < nlev; i++){
			if (L_p_in < L_qbnd)
				break;
			else
				L_qbnd = L_add(L_qbnd, L_step);
		}
		/* Quantize input to correct level */
		/* *p_in = qmin + (i * step); */
		L_temp = L_sub(L_qbnd, L_half_step);
		*p_in = extract_l(L_shr(L_temp, scale));
		*p_index = i;
	} else {
		/* Search quantizer boundaries */
		/* qbnd = qmin + (0.5 * step); */
		step = shr(step, scale);
		half_step = shr(step, 1);
		qbnd = add(qmin, half_step);

		for (i = 0; i < nlev; i++){
			if (*p_in < qbnd)
				break;
			else
				qbnd += step;
		}
		/*	Quantize input to correct level */
		/* *p_in = qmin + (i * step); */
		*p_in = sub(qbnd, half_step);
		*p_index = i;
	}
}


/* Subroutine quant_u_dec(): decode uniformly quantized value.                */
void quant_u_dec_q(int16_t index, int16_t *p_data, int16_t qmin,
				 int16_t qmax, int16_t nlev_q, int16_t scale)
{
	int16_t	step, temp;
	int32_t	L_qmin, L_temp;


	/* Define symmetrical quantizer stepsize.  (nlev - 1) is computed in the  */
	/* calling function.                                                      */

	/*	step = (qmax - qmin) / (nlev - 1); */
	temp = sub(qmax, qmin);
	step = divide_s(temp, nlev_q);

	/* Decode quantized level */
	/* double precision specified */

	L_temp = L_shr(L_mult(step, index), 1);
	L_qmin = L_shl(L_deposit_l(qmin), scale);
	L_temp = L_add(L_qmin, L_temp);
	*p_data = extract_l(L_shr(L_temp, scale));
}


/* Subroutine rand_num: generate random numbers to fill array using "minimal  */
/* standard" random number generator.                                         */

void	rand_num_q(int16_t output[], int16_t amplitude, int16_t npts)
{
	register int16_t	i;
	int16_t	temp;


	for (i = 0; i < npts; i++){

		/* rand_minstdgen returns 0 <= x < 1 */
		/* -0.5 <= temp < 0.5 */
		temp = sub(rand_minstdgen(), X05_Q15);
		output[i] = mult(amplitude, shl(temp, 1));
	}
}


/****************************************************************************/
/* RAND() - COMPUTE THE NEXT VALUE IN THE RANDOM NUMBER SEQUENCE.			*/
/*																			*/
/*	   The sequence used is x' = (A*x) mod M,  (A = 16807, M = 2^31 - 1).	*/
/*	   This is the "minimal standard" generator from CACM Oct 1988, p. 1192.*/
/*	   The implementation is based on an algorithm using 2 31-bit registers */
/*	   to represent the product (A*x), from CACM Jan 1990, p. 87.			*/
/*																			*/
/****************************************************************************/

int16_t	rand_minstdgen()
{
	static uint32_t	next = 1;                /* seed; must not be zero!!! */
	uint16_t	x0 = extract_l(next);                      /* 16 LSBs OF SEED */
	uint16_t	x1 = extract_h(next);                      /* 16 MSBs OF SEED */
	uint32_t	p, q;                                  /* MSW, LSW OF PRODUCT */
	uint32_t	L_temp1, L_temp2, L_temp3;


	/*----------------------------------------------------------------------*/
	/* COMPUTE THE PRODUCT (A * next) USING CROSS MULTIPLICATION OF         */
	/* 16-BIT HALVES OF THE INPUT VALUES.	THE RESULT IS REPRESENTED AS 2  */
	/* 31-BIT VALUES.	SINCE 'A' FITS IN 15 BITS, ITS UPPER HALF CAN BE    */
	/* DISREGARDED.  USING THE NOTATION val[m::n] TO MEAN "BITS n THROUGH   */
	/* m OF val", THE PRODUCT IS COMPUTED AS:                               */
	/*   q = (A * x)[0::30]  = ((A * x1)[0::14] << 16) + (A * x0)[0::30]    */
	/*   p = (A * x)[31::60] =  (A * x1)[15::30]		+ (A * x0)[31]	+ C */
	/* WHERE C = q[31] (CARRY BIT FROM q).  NOTE THAT BECAUSE A < 2^15,     */
	/* (A * x0)[31] IS ALWAYS 0.                                            */
	/*----------------------------------------------------------------------*/
	/* save and reset saturation count                                      */

	/* q = ((A * x1) << 17 >> 1) + (A * x0); */
	/* p = ((A * x1) >> 15) + (q >> 31); */
	/* q = q << 1 >> 1; */                                     /* CLEAR CARRY */
	L_temp1 = L_mpyu(A, x1);
	/* store bit 15 to bit 31 in p */
	p = L_shr(L_temp1, 15);
	/* mask bit 15 to bit 31 */
	L_temp1 = L_shl((L_temp1 & (int32_t) 0x00007fff), 16);
	L_temp2 = L_mpyu(A, x0);
	L_temp3 = L_sub(LW_MAX, L_temp1);
	if (L_temp2 > L_temp3){
		/* subtract 0x80000000 from sum */
		L_temp1 = L_sub(L_temp1, (int32_t) 0x7fffffff);
		L_temp1 = L_sub(L_temp1, 1);
		q = L_add(L_temp1, L_temp2);
		p = L_add(p, 1);
	} else
		q = L_add(L_temp1, L_temp2);

	/*---------------------------------------------------------------------*/
	/* IF (p + q) < 2^31, RESULT IS (p + q).  OTHERWISE, RESULT IS         */
	/* (p + q) - 2^31 + 1.  (SEE REFERENCES).                              */
	/*---------------------------------------------------------------------*/
	/* p += q; next = ((p + (p >> 31)) << 1) >> 1; */
	/* ADD CARRY, THEN CLEAR IT */

	L_temp3 = L_sub(LW_MAX, p);
	if (q > L_temp3){
		/* subtract 0x7fffffff from sum */
		L_temp1 = L_sub(p, (int32_t) 0x7fffffff);
		L_temp1 = L_add(L_temp1, q);
	} else
		L_temp1 = L_add(p, q);
	next = L_temp1;

	x1 = extract_h(next);
	return (x1);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: L_mpyu
 *
 *	 PURPOSE:
 *
 *	   Perform an unsigned fractional multipy of the two unsigned 16 bit
 *	   input numbers with saturation.  Output a 32 bit unsigned number.
 *
 *	 INPUTS:
 *
 *	   var1
 *					   16 bit short unsigned integer (int16_t) whose value
 *					   falls in the range 0xffff 0000 <= var1 <= 0x0000 ffff.
 *	   var2
 *					   16 bit short unsigned integer (int16_t) whose value
 *					   falls in the range 0xffff 0000 <= var2 <= 0x0000 ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_Out
 *					   32 bit long unsigned integer (int32_t) whose value
 *					   falls in the range
 *					   0x0000 0000 <= L_var1 <= 0xffff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Multiply the two unsigned 16 bit input numbers.
 *
 *	 KEYWORDS: multiply, mult, mpy
 *
 *************************************************************************/

//static uint32_t	L_mpyu(uint16_t var1, uint16_t var2)
//{
//	uint32_t	L_product;


//	L_product = (uint32_t) var1 *var2;                   /* integer multiply */
//	return (L_product);
//}


/* ========================================================================== */
/* This function unpacks bits of "code" from channel.  It returns 1 if an     */
/*    erasure is encountered, or 0 otherwise.  "numbits" bits of "code" */
/*    is used and they are packed into the array pointed by "ptr_ch_begin".   */
/*    "ptr_ch_bit" points to the position of the next bit being copied onto.  */
/* ========================================================================== */
void	unpack_code_q(unsigned char **ptr_ch_begin, int16_t *ptr_ch_bit,
					int16_t *code, int16_t numbits, int16_t wsize)
{
	register int16_t	i;
	unsigned char	*ch_word;
	int16_t	ch_bit;


	ch_bit = *ptr_ch_bit;
	ch_word = *ptr_ch_begin;
	*code = 0;

	for (i = 0; i < numbits; i++){
		/* Mask in bit from channel word to code */
		*code |= shl(shr((int16_t) ((int16_t) *ch_word & shl(1, ch_bit)), ch_bit), i);

		/* Check for end of channel word */
		ch_bit++;
		if (ch_bit >= wsize){
			ch_bit = 0;
			(*ptr_ch_begin) ++;
			ch_word++ ;
		}
	}

	/* Save updated bit counter */
	*ptr_ch_bit = ch_bit;
}


/* ============================================ */
/* Subroutine window: multiply signal by window */
/*                                              */
/* Q values:                                    */
/*    input - Q0, win_coeff - Q15, output - Q0  */
/* ============================================ */

//void window(int16_t input[], const int16_t win_coeff[], int16_t output[],
//			int16_t npts)
//{
//	register int16_t	i;

//	for (i = 0; i < npts; i++){
//		output[i] = mult(win_coeff[i], input[i]);
//	}
//}


/* ============================================== */
/* Subroutine window_Q: multiply signal by window */
/*                                                */
/* Q values:                                      */
/*    win_coeff - Qin, output = input             */
/* ============================================== */

void window_q15Q(int16_t input[], int16_t win_coeff[], int16_t output[],
			  int16_t Qin, int16_t npts)
{
//	register int16_t	i;
	int16_t	shift;

	/* After computing "shift", win_coeff[]*2^(-shift) is considered Q15.     */
	shift = 15 -  Qin;

	arm_mult_shift_q15(input, win_coeff, output, shift, npts);

//	for (i = 0; i < npts; i++){
//		output[i] = extract_h(L_shl(L_mult(win_coeff[i], input[i]), shift));
//	}
}


/* ========================================================================== */
/* Subroutine iir_2nd_d: Second order IIR filter (Double precision)           */
/*    Note: the output array can overlay the input.                           */
/*                                                                            */
/* Input scaled down by a factor of 2 to prevent overflow                     */
/* ========================================================================== */
void iir_2nd_d(int16_t input[], const int16_t den[], const int16_t num[],
			   int16_t output[], int16_t delin[], int16_t delout_hi[],
			   int16_t delout_lo[], int16_t npts)
{
	register int16_t	i;
	int16_t	temp;
	int32_t	accum;

	for (i = 0; i < npts; i++){
		accum = L_mult(delout_lo[0], den[1]);
		accum = L_mac(accum, delout_lo[1], den[2]);
		accum = L_shr(accum, 14);
		accum = L_mac(accum, delout_hi[0], den[1]);
		accum = L_mac(accum, delout_hi[1], den[2]);

		accum = L_mac(accum, shr(input[i], 1), num[0]);
		accum = L_mac(accum, delin[0], num[1]);
		accum = L_mac(accum, delin[1], num[2]);

		/* shift result to correct Q value */
		accum = L_shl(accum, 2);                /* assume coefficients in Q13 */

		/* update input delay buffer */
		delin[1] = delin[0];
		delin[0] = shr(input[i], 1);

		/* update output delay buffer */
		delout_hi[1] = delout_hi[0];
		delout_lo[1] = delout_lo[0];
		delout_hi[0] = extract_h(accum);
		temp = shr(extract_l(accum), 2);
		delout_lo[0] = (int16_t) (temp & (int16_t)0x3FFF);

		/* round off result */
		accum = L_shl(accum, 1);
		output[i] = round_l(accum);
	}
}


/* ========================================================================== */
/* Subroutine iir_2nd_s: Second order IIR filter (Single precision)           */
/*    Note: the output array can overlay the input.                           */
/*                                                                            */
/* Input scaled down by a factor of 2 to prevent overflow                     */
/* ========================================================================== */
void iir_2nd_s(int16_t input[], const int16_t den[], const int16_t num[],
			   int16_t output[], int16_t delin[], int16_t delout[],
			   int16_t npts)
{
	register int16_t	i;
	int32_t	accum;

	for (i = 0; i < npts; i++){
		accum = L_mult(input[i], num[0]);
		accum = L_mac(accum, delin[0], num[1]);
		accum = L_mac(accum, delin[1], num[2]);

		accum = L_mac(accum, delout[0], den[1]);
		accum = L_mac(accum, delout[1], den[2]);

		/* shift result to correct Q value */
		accum = L_shl(accum, 2);                /* assume coefficients in Q13 */

		/* update input delay buffer */
		delin[1] = delin[0];
		delin[0] = input[i];

		/* round off result */
		output[i] = round_l(accum);

		/* update output delay buffer */
		delout[1] = delout[0];
		delout[0] = output[i];
	}
}


/*								*/
/*	Subroutine zerflt: all zero (FIR) filter.		*/
/*      Note: the output array can overlay the input.           */
/*								*/
void zerflt_q15(int16_t *pSrc, const int16_t *pCoeffs, int16_t *pDst, int16_t order, int16_t npts)
{
   const int16_t *px, *pb;                    /* Temporary pointers for state and coefficient buffers */
   int32_t acc0, acc1, acc2, acc3;			  /* Accumulators */
   int16_t x0, x1, x2, x3, c0;				  /* Temporary variables to hold state and coefficient values */
   uint32_t numTaps, tapCnt, blkCnt;          /* Loop counters */
   int32_t p0,p1,p2,p3;						  /* Temporary product values */

   pSrc += (npts - 1);
   pDst += (npts - 1);
   numTaps = order + 1;


   /* Apply loop unrolling and compute 4 output values simultaneously.
    * The variables acc0 ... acc3 hold output values that are being computed
   */
   blkCnt = npts;

   /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
   while(blkCnt >= 4)
   {
      /* Set all accumulators to zero */
      acc0 = 0;
      acc1 = 0;
      acc2 = 0;
      acc3 = 0;

	  /* Initialize state pointer */
      px = pSrc;

      /* Initialize coeff pointer */
      pb = pCoeffs;

      /* Read the first three samples from the state buffer */
      x0 = *px--;
      x1 = *px--;
      x2 = *px--;

      /* Loop unrolling.  Process 4 taps at a time. */
      tapCnt = numTaps;

      /* Loop over the number of taps.  Unroll by a factor of 4.
       ** Repeat until we've computed numTaps-4 coefficients. */
      while(tapCnt >= 4)
      {
         /* Read the b[0] coefficient */
         c0 = *(pb++);

         x3 = *(px--);

         p0 = (q31_t) x0 * c0;
         p1 = (q31_t) x1 * c0;
         p2 = (q31_t) x2 * c0;
         p3 = (q31_t) x3 * c0;

         /* Read the b[1] coefficient */
         c0 = *(pb++);
		 x0 = *(px--);

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

         /* Perform the multiply-accumulate */
         p0 = (q31_t) x1 * c0;
         p1 = (q31_t) x2 * c0;
         p2 = (q31_t) x3 * c0;
         p3 = (q31_t) x0 * c0;

         /* Read the b[2] coefficient */
         c0 = *(pb++);
         x1 = *(px--);

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

         /* Perform the multiply-accumulates */
         p0 = (q31_t) x2 * c0;
         p1 = (q31_t) x3 * c0;
         p2 = (q31_t) x0 * c0;
         p3 = (q31_t) x1 * c0;

		 /* Read the b[3] coefficient */
         c0 = *(pb++);
         x2 = *(px--);

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

         /* Perform the multiply-accumulates */
         p0 = (q31_t) x3 * c0;
         p1 = (q31_t) x0 * c0;
         p2 = (q31_t) x1 * c0;
         p3 = (q31_t) x2 * c0;

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

		 tapCnt -= 4;
     }

      /* If the filter length is not a multiple of 4, compute the remaining filter taps */
      while(tapCnt > 0)
      {
         /* Read coefficients */
         c0 = *(pb++);

         /* Fetch 1 state variable */
         x3 = *(px--);

         /* Perform the multiply-accumulates */
         p0 = (q31_t) x0 * c0;
         p1 = (q31_t) x1 * c0;
         p2 = (q31_t) x2 * c0;
         p3 = (q31_t) x3 * c0;

         /* Reuse the present sample states for next sample */
         x0 = x1;
         x1 = x2;
         x2 = x3;

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;
         /* Decrement the loop counter */
         tapCnt--;
      }


      /* The results in the 4 accumulators, store in the destination buffer. */
      *pDst-- = (q15_t) (__SSAT((acc0 >> 12), 16));
      *pDst-- = (q15_t) (__SSAT((acc1 >> 12), 16));
      *pDst-- = (q15_t) (__SSAT((acc2 >> 12), 16));
      *pDst-- = (q15_t) (__SSAT((acc3 >> 12), 16));
      /* Advance the state pointer by 4 to process the next group of 4 samples */
      pSrc -= 4;
      blkCnt -= 4;
   }

   /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */

   while(blkCnt > 0)
   {
      /* Set the accumulator to zero */
      acc0 = 0.0f;
      /* Initialize state pointer */
      px = pSrc;
	  /* Initialize Coefficient pointer */
      pb = pCoeffs;
      tapCnt = numTaps;

	  /* Perform the multiply-accumulates */
      do
      {
         acc0 += (q31_t) *px-- * *pb++;
         tapCnt--;
      } while(tapCnt > 0);

      /* The result is store in the destination buffer. */
      *pDst-- = (q15_t) (__SSAT((acc0 >> 12), 16));
      /* Advance state pointer by 1 for the next sample */
      pSrc--;
      blkCnt--;
   }
}

/*								*/
/*	Subroutine zerflt: all zero (FIR) filter.		*/
/*      Note: the output array can overlay the input.           */
/*								*/
void zerflt_q15Q(int16_t *pSrc, const int16_t *pCoeffs, int16_t *pDst, int16_t order, int16_t Q_coeff, int16_t npts)
{
   const int16_t *px, *pb;                    /* Temporary pointers for state and coefficient buffers */
   int32_t acc0, acc1, acc2, acc3;			  /* Accumulators */
   int16_t x0, x1, x2, x3, c0;				  /* Temporary variables to hold state and coefficient values */
   uint32_t numTaps, tapCnt, blkCnt;          /* Loop counters */
   int32_t p0,p1,p2,p3;						  /* Temporary product values */

   pSrc += (npts - 1);
   pDst += (npts - 1);
   numTaps = order + 1;


   /* Apply loop unrolling and compute 4 output values simultaneously.
    * The variables acc0 ... acc3 hold output values that are being computed
   */
   blkCnt = npts;

   /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
   while(blkCnt >= 4)
   {
      /* Set all accumulators to zero */
      acc0 = 0;
      acc1 = 0;
      acc2 = 0;
      acc3 = 0;

	  /* Initialize state pointer */
      px = pSrc;

      /* Initialize coeff pointer */
      pb = pCoeffs;

      /* Read the first three samples from the state buffer */
      x0 = *px--;
      x1 = *px--;
      x2 = *px--;

      /* Loop unrolling.  Process 4 taps at a time. */
      tapCnt = numTaps;

      /* Loop over the number of taps.  Unroll by a factor of 4.
       ** Repeat until we've computed numTaps-4 coefficients. */
      while(tapCnt >= 4)
      {
         /* Read the b[0] coefficient */
         c0 = *(pb++);

         x3 = *(px--);

         p0 = (q31_t) x0 * c0;
         p1 = (q31_t) x1 * c0;
         p2 = (q31_t) x2 * c0;
         p3 = (q31_t) x3 * c0;

         /* Read the b[1] coefficient */
         c0 = *(pb++);
		 x0 = *(px--);

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

         /* Perform the multiply-accumulate */
         p0 = (q31_t) x1 * c0;
         p1 = (q31_t) x2 * c0;
         p2 = (q31_t) x3 * c0;
         p3 = (q31_t) x0 * c0;

         /* Read the b[2] coefficient */
         c0 = *(pb++);
         x1 = *(px--);

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

         /* Perform the multiply-accumulates */
         p0 = (q31_t) x2 * c0;
         p1 = (q31_t) x3 * c0;
         p2 = (q31_t) x0 * c0;
         p3 = (q31_t) x1 * c0;

		 /* Read the b[3] coefficient */
         c0 = *(pb++);
         x2 = *(px--);

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

         /* Perform the multiply-accumulates */
         p0 = (q31_t) x3 * c0;
         p1 = (q31_t) x0 * c0;
         p2 = (q31_t) x1 * c0;
         p3 = (q31_t) x2 * c0;

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;

		 tapCnt -= 4;
     }

      /* If the filter length is not a multiple of 4, compute the remaining filter taps */
      while(tapCnt > 0)
      {
         /* Read coefficients */
         c0 = *(pb++);

         /* Fetch 1 state variable */
         x3 = *(px--);

         /* Perform the multiply-accumulates */
         p0 = (q31_t) x0 * c0;
         p1 = (q31_t) x1 * c0;
         p2 = (q31_t) x2 * c0;
         p3 = (q31_t) x3 * c0;

         /* Reuse the present sample states for next sample */
         x0 = x1;
         x1 = x2;
         x2 = x3;

         acc0 += p0;
         acc1 += p1;
         acc2 += p2;
         acc3 += p3;
         /* Decrement the loop counter */
         tapCnt--;
      }


      /* The results in the 4 accumulators, store in the destination buffer. */
      *pDst-- = (q15_t) (__SSAT((acc0 >> Q_coeff), 16));
      *pDst-- = (q15_t) (__SSAT((acc1 >> Q_coeff), 16));
      *pDst-- = (q15_t) (__SSAT((acc2 >> Q_coeff), 16));
      *pDst-- = (q15_t) (__SSAT((acc3 >> Q_coeff), 16));
      /* Advance the state pointer by 4 to process the next group of 4 samples */
      pSrc -= 4;
      blkCnt -= 4;
   }

   /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */

   while(blkCnt > 0)
   {
      /* Set the accumulator to zero */
      acc0 = 0.0f;
      /* Initialize state pointer */
      px = pSrc;
	  /* Initialize Coefficient pointer */
      pb = pCoeffs;
      tapCnt = numTaps;

	  /* Perform the multiply-accumulates */
      do
      {
         acc0 += (q31_t) *px-- * *pb++;
         tapCnt--;
      } while(tapCnt > 0);

      /* The result is store in the destination buffer. */
      *pDst-- = (q15_t) (__SSAT((acc0 >> Q_coeff), 16));
      /* Advance state pointer by 1 for the next sample */
      pSrc--;
      blkCnt--;
   }
}

/*								*/
/*	Subroutine polflt: all pole (IIR) filter.		*/
/*	Note: The filter coefficients represent the		*/
/*	denominator only, and the leading coefficient		*/
/*	is assumed to be 1.					*/
/*	The (order) samples BEFORE the output[0] contain previous	states		*/
/*      The output array can overlay the input.                 */
/*								*/
void polflt_q15Q(int16_t input[], const int16_t coeff[], int16_t output[], int order,int16_t Q_coeff, int npts)
{
	int numTaps, numBlk;
	int32_t accum0, accum1, accum2, accum3;
	int16_t	a1, a2, a3, a4, c0;
	int16_t y1, y2, y3, y4;
	const int16_t *pc;
	int16_t *py;
	int32_t	shift;

	shift = Q_coeff + 1;

	a1 = coeff[1]; a2 = coeff[2]; a3 = coeff[3]; a4 = coeff[4];
	y1 = output[-1]; y2 = output[-2]; y3 = output[-3]; y4 = output[-4];
	numBlk = npts;
	while (numBlk >= 4) {
		accum0 = (((q31_t) *input++) << shift) - ((q31_t) y1 * a1) - ((q31_t) y2 * a2) - ((q31_t) y3 * a3) - ((q31_t) y4 * a4);
		accum1 = (((q31_t) *input++) << shift) - ((q31_t) y1 * a2) - ((q31_t) y2 * a3) - ((q31_t) y3 * a4);
		accum2 = (((q31_t) *input++) << shift) - ((q31_t) y1 * a3) - ((q31_t) y2 * a4);
		accum3 = (((q31_t) *input++) << shift) - ((q31_t) y1 * a4);

		pc = &coeff[5];
		py = &output[-5];
		numTaps = order - 4;
		while(numTaps > 0)
		{
			c0 = *pc++;
			y1 = y2;
			y2 = y3;
			y3 = y4;
			y4 = *py--;
			accum0 -= ( (q31_t) y4 * c0);
			accum1 -= ( (q31_t) y3 * c0);
			accum2 -= ( (q31_t) y2 * c0);
			accum3 -= ( (q31_t) y1 * c0);
			numTaps--;
		}
		accum1 -= (accum0 * a1);
		accum2 -= (accum1 * a1) + (accum0 * a2);
		accum3 -= (accum2 * a1) + (accum1 * a2) + (accum0 * a3);

		y4 = *output++ = (q15_t) (__SSAT((accum0 >> Q_coeff), 16));
		y3 = *output++ = (q15_t) (__SSAT((accum1 >> Q_coeff), 16));
		y2 = *output++ = (q15_t) (__SSAT((accum2 >> Q_coeff), 16));
		y1 = *output++ = (q15_t) (__SSAT((accum3 >> Q_coeff), 16));
		numBlk -= 4;
	}
	// For the rest (non-multiples of 4) of samples do it normally
	while(numBlk > 0)
	{
		accum0 = (((q31_t) *input++) << shift);
		pc = &coeff[1];
		py = &output[-1];
		numTaps = order;
		while(numTaps > 0)
		{
			accum0 -= ( (q31_t)(*py--) * (*pc++) );
			numTaps--;
		}
		*output++ = (q15_t) (__SSAT((accum0 >> Q_coeff), 16));
		numBlk--;
	}
}

/**
 * @brief           Q15 vector multiplication  with scaling (Left Shift)
 * @param[in]       *pSrcA points to the first input vector
 * @param[in]       *pSrcB points to the second input vector
 * @param[out]      *pDst points to the output vector
 * @param[in]       blockSize number of samples in each vector
 * @return none.
 *
 * <b>Scaling and Overflow Behavior:</b>
 * \par
 * The function uses saturating arithmetic.
 * Results outside of the allowable Q15 range [0x8000 0x7FFF] will be saturated.
 */

void arm_mult_shift_q15(
  q15_t * pSrcA,
  q15_t * pSrcB,
  q15_t * pDst,
  int32_t LeftShift,
  uint32_t blockSize)
{
  uint32_t blkCnt;                               /* loop counters */
  uint32_t RightShift;                           /* The shift for final result */

  RightShift = 15 - LeftShift;

#ifndef ARM_MATH_CM0_FAMILY
/* Run the below code for Cortex-M4 and Cortex-M3 */
  q31_t inA1, inA2, inB1, inB2;                  /* temporary input variables */
  q15_t out1, out2, out3, out4;                  /* temporary output variables */
  q31_t mul1, mul2, mul3, mul4;                  /* temporary variables */

  /* loop Unrolling */
  blkCnt = blockSize >> 2u;

  /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while(blkCnt > 0u)
  {
    /* read two samples at a time from sourceA */
    inA1 = *__SIMD32(pSrcA)++;
    /* read two samples at a time from sourceB */
    inB1 = *__SIMD32(pSrcB)++;
    /* read two samples at a time from sourceA */
    inA2 = *__SIMD32(pSrcA)++;
    /* read two samples at a time from sourceB */
    inB2 = *__SIMD32(pSrcB)++;

    /* multiply mul = sourceA * sourceB */
    mul1 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1 >> 16));
    mul2 = (q31_t) ((q15_t) inA1 * (q15_t) inB1);
    mul3 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) (inB2 >> 16));
    mul4 = (q31_t) ((q15_t) inA2 * (q15_t) inB2);

    /* saturate result to 16 bit */
    out1 = (q15_t) __SSAT(mul1 >> RightShift, 16);
    out2 = (q15_t) __SSAT(mul2 >> RightShift, 16);
    out3 = (q15_t) __SSAT(mul3 >> RightShift, 16);
    out4 = (q15_t) __SSAT(mul4 >> RightShift, 16);

    /* store the result */
#ifndef ARM_MATH_BIG_ENDIAN

    *__SIMD32(pDst)++ = __PKHBT(out2, out1, 16);
    *__SIMD32(pDst)++ = __PKHBT(out4, out3, 16);

#else

    *__SIMD32(pDst)++ = __PKHBT(out2, out1, 16);
    *__SIMD32(pDst)++ = __PKHBT(out4, out3, 16);

#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

    /* Decrement the blockSize loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize % 0x4u;

#else

  /* Run the below code for Cortex-M0 */

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #ifndef ARM_MATH_CM0_FAMILY */


  while(blkCnt > 0u)
  {
    /* C = A * B */
    /* Multiply the inputs and store the result in the destination buffer */
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> RightShift), 16);

    /* Decrement the blockSize loop counter */
    blkCnt--;
  }
}
