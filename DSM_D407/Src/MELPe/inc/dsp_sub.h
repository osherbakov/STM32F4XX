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

/* ======================= */
/* dsp_sub.h: include file */
/* ======================= */

#ifndef _DSP_SUB_H_
#define _DSP_SUB_H_


#include "arm_math.h"

void	envelope_q(int16_t input[], int16_t prev_in, int16_t output[],
				 int16_t npts);

// void	v_fill(int16_t output[], int16_t fillval, int16_t npts);
#define v_fill(dst,val,n)  arm_fill_q15(val, dst, n)

// void	L_fill(int32_t output[], int32_t fillval, int16_t npts);
#define L_fill(dst,val,n)  arm_fill_q31(val, dst, n)

void	interp_array_q(int16_t prev[], int16_t curr[], int16_t out[], int16_t ifact, int16_t size);

int16_t	median3(int16_t input[]);

void	pack_code_q(int16_t code, unsigned char **ptr_ch_begin, int16_t *ptr_ch_bit, int16_t numbits, int16_t wsize);

int16_t	peakiness_q(int16_t input[], int16_t npts);

void	quant_u_q(int16_t *p_data, int16_t *p_index, int16_t qmin,
				int16_t qmax, int16_t nlev, int16_t nlev_q,
				int16_t double_flag, int16_t scale);

void quant_u_dec_q(int16_t index, int16_t *p_data, int16_t qmin,
				 int16_t qmax, int16_t nlev_q, int16_t scale);

void	rand_num_q(int16_t output[], int16_t amplitude, int16_t npts);

int16_t rand_minstdgen(void);

BOOLEAN	unpack_code_q(unsigned char **ptr_ch_begin, int16_t *ptr_ch_bit,
					int16_t *code, int16_t numbits, int16_t wsize,
					uint16_t erase_mask);

// void	window(int16_t input[], const int16_t win_coeff[],int16_t output[], int16_t npts);
#define window(src,coef,dst,n)  arm_mult_q15((q15_t *)src, (q15_t *)coef, (q15_t *)dst, n)

void	window_Q(int16_t input[], int16_t win_coeff[], int16_t output[], int16_t npts, int16_t Qin);

void	polflt_q(int16_t input[], int16_t coeff[], int16_t output[],
			   int16_t order, int16_t npts);

void	zerflt_q(int16_t input[], const int16_t coeff[], int16_t output[],
			   int16_t order, int16_t npts);

void	zerflt_Q(int16_t input[], const int16_t coeff[],
				 int16_t output[], int16_t order, int16_t npts,
				 int16_t Q_coeff);

void	iir_2nd_d(int16_t input[], const int16_t den[],
				  const int16_t num[], int16_t output[], int16_t delin[],
				  int16_t delout_hi[], int16_t delout_lo[],
				  int16_t npts);

void	iir_2nd_s(int16_t input[], const int16_t den[],
				  const int16_t num[], int16_t output[],
				  int16_t delin[], int16_t delout[], int16_t npts);

int16_t	interp_scalar_q(int16_t prev, int16_t curr, int16_t ifact);


#endif

