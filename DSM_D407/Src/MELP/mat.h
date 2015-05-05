/*

2.4 kbps MELP Proposed Federal Standard speech coder

version 1.2

Copyright (c) 1996, Texas Instruments, Inc.

Texas Instruments has intellectual property rights on the MELP
algorithm.  The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).


*/

/*
   mat.h     Matrix include file.
             (Low level matrix and vector functions.)

   Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved.
*/
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif

#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include "arm_math.h"

float v_inner(float *v1,float *v2,int n);
float v_magsq(float *v,int n);

#define v_zap(v,n)					arm_fill_f32(0.0f, (float32_t *)v, n)
#define v_equ(v1,v2,n) 				arm_copy_f32((float32_t *)v2, (float32_t *)v1, n)
#define v_sub(v1,v2,n) 				arm_sub_f32((float32_t *)v1, (float32_t *)v2, (float32_t *)v1, n)
#define v_add(v1,v2,n) 				arm_add_f32((float32_t *)v1, (float32_t *)v2, (float32_t *)v1, n)
#define v_scale(v,scale,n)		    arm_scale_f32((float32_t *)v, scale, v, n)

#define v_zap_int(v,n)				arm_fill_q31(0, (q31_t *)v, n)
#define v_equ_int(v1,v2,n) 		    arm_copy_q31((q31_t *)v2, (q31_t *)v1, n)


