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

/* =================================================================== */
/* mat.h	 Matrix include file.                                      */
/*			 (Low level matrix and vector functions.)                  */
/*                                                                     */
/* Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved. */
/* =================================================================== */

#ifndef _MAT_LIB_H_
#define _MAT_LIB_H_

#include "arm_math.h"
#include "arm_const_structs.h"

//int16_t	*v_add(int16_t vec1[], const int16_t vec2[], int16_t n);
#define v_add(dst,src,n) arm_add_q15((q15_t *)src, (q15_t *)dst, (q15_t *)dst, n)

//int32_t	*L_v_add(int32_t L_vec1[], int32_t L_vec2[], int16_t n);
#define L_v_add(dst,src,n) arm_add_q31(src, dst, dst, n)

//int16_t	*v_equ(int16_t vec1[], const int16_t v2[], int16_t n);
#define v_equ(dst,src,n) arm_copy_q15((q15_t *)src, (q15_t *)dst, n)

//int16_t	*v_equ_shr(int16_t vec1[], int16_t vec2[], int16_t scale, int16_t n);
#define v_equ_shr(dst,src,scale,n) arm_shift_q15((q15_t *)src, -scale, (q15_t *)dst, n)

//int32_t	*L_v_equ(int32_t L_vec1[], int32_t L_vec2[], int16_t n);
#define L_v_equ(dst,src,n) arm_copy_q31(src, dst, n)

//int16_t	v_inner(int16_t vec1[], int16_t vec2[], int16_t n, int16_t qvec1, int16_t qvec2, int16_t qout);
static __inline int16_t v_inner(int16_t vec1[], int16_t vec2[], int16_t n, int16_t qvec1, int16_t qvec2, int16_t qout)
{
	int64_t	res;
	arm_dot_prod_q15(vec1, vec2, n, &res);
	return (int16_t)(res << (qout - (qvec1 + qvec2)));
}

//int32_t	L_v_inner(int16_t vec1[], int16_t vec2[], int16_t n, int16_t qvec1, int16_t qvec2, int16_t qout);
static __inline int32_t L_v_inner(int16_t vec1[], int16_t vec2[], int16_t n, int16_t qvec1, int16_t qvec2, int16_t qout)
{
	int64_t	res;
	arm_dot_prod_q15(vec1, vec2, n, &res);
	return (int32_t)(res << (qout - (qvec1 + qvec2)));
}

// int16_t	v_magsq(int16_t vec1[], int16_t n, int16_t qvec1, int16_t qout);

//int32_t	L_v_magsq(int16_t vec1[], int16_t n, int16_t qvec1, int16_t qout);
static __inline int32_t	L_v_magsq(int16_t vec1[], int16_t n, int16_t qvec1, int16_t qout)
{
	int64_t  res;
	arm_power_q15(vec1, n, &res);
	return (int32_t) (res << (qout - (qvec1 * 2)));
}

// int16_t	*v_scale(int16_t vec1[], int16_t scale, int16_t n);
#define v_scale(dst,scale,n) arm_scale_q15((q15_t *)dst, scale, 0, (q15_t *)dst, n) 

//int16_t	*v_scale_shl(int16_t vec1[], int16_t scale, int16_t n, int16_t shift);
#define v_scale_shl(dst,scale,n, shift) arm_scale_q15((q15_t *)dst, scale, shift, (q15_t *)dst, n)

//int16_t	*v_sub(int16_t vec1[], const int16_t vec2[], int16_t n);
#define v_sub(dst,src,n) arm_sub_q15((q15_t *)dst, (q15_t *)src, (q15_t *)dst, n)

//int16_t	*v_zap(int16_t vec1[], int16_t n);
#define v_zap(dst,n) arm_fill_q15(0,(q15_t *)dst,n)

// int32_t	*L_v_zap(int32_t L_vec1[], int16_t n);
#define L_v_zap(dst,n) arm_fill_q31(0,dst,n)

#define v_lshift(dst,src,shift,n) arm_shift_q15(src, (int8_t)shift, dst, n)
#define v_rshift(dst,src,shift,n) arm_shift_q15(src, -shift, dst, n)


#endif

