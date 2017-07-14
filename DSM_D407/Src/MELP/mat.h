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
#ifndef  __MAT_H__
#define  __MAT_H__

#include "arm_math.h"
#include "arm_const_structs.h"

/*
** Constant definitions.
*/
#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif

#ifndef PI
#define PI			3.14159265358979f
#endif
#define TWO_PI		(2 * PI)		/* mathematical constant                */
#define HALF_PI		(PI/2.0f)
#define ONEQTR_PI	(PI/4.0f)
#define THRQTR_PI   (3.0f*PI/4.0f)

static __INLINE float v_inner(float *v1,float *v2,int n)
{
	float innerprod;
	arm_dot_prod_f32(v1, v2, n, &innerprod);
	return innerprod;
}

static __INLINE float v_magsq(float *v,int n)
{
	float magsq;
	arm_power_f32(v, n, &magsq);
	return magsq;
}

static __INLINE int findmax(float input[], int npts)
{
	uint32_t	maxloc;
	float   maxval;
	arm_max_f32(input, npts, &maxval, &maxloc);
	return (maxloc);
}

static __INLINE float fsqrtf(float x)
{
	float32_t  res;
	arm_sqrt_f32(x, &res);
	return res;
}

static __INLINE float fsinf(float x)
{
	float32_t  res;
	res = arm_sin_f32(x);
	return res;
}

static __INLINE float fcosf(float x)
{
	float32_t  res;
	res = arm_cos_f32(x);
	return res;
}

static __INLINE float fpowf(float a, float b) {
	union { float d; int x; } u = { a };
	u.x = (int)(b * (u.x - 1064866805) + 1064866805);
	return u.d;
}

static __INLINE float flog2f(float val)
{
	union { float d; int x; } u = { val };	
	float  log_2 = (float)(((u.x >> 23) & 255) - 128);
	u.x &= ~(255 << 23);
	u.x += (127 << 23);

	u.d = ((-0.3358287811f) * u.d + 2.0f) * u.d - 0.65871759316667f;   // (1)
	return (u.d + log_2);
} 

static __INLINE float fatan2f( float y, float x )
{
	float atan, z;	
	if ( x == 0.0f )
	{
		if ( y > 0.0f ) return HALF_PI;
		if ( y == 0.0f ) return 0.0f;
		return -HALF_PI;
	}

	z = y/x;
	if ( fabsf( z ) < 1.0f )
	{
		atan = z/(1.0f + 0.28f*z*z);
		if ( x < 0.0f )
		{
			if ( y < 0.0f ) return atan - PI;
			return atan + PI;
		}
	}else
	{
		atan = HALF_PI - z/(z*z + 0.28f);
		if ( y < 0.0f ) return atan - PI;
	}
	return atan;
}

static __INLINE float facosf(float x) {
   return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f;
}


//#define sqrtf		arm_sqrt
//#define sinf		arm_sin
//#define cosf		arm_cos
//#define powf		powf_fast
//#define log2f(a)	log2f_fast((a))
//#define log10f(a)	(log2f(a)* 0.301029995f)
//#define acosf		acosf_fast
//#define atan2f(y,x)	atan2f_fast((y),(x))

#ifndef SQR
#define SQR(x)          ((x)*(x))
#endif


#define window(inp,cof,outp,n)		arm_mult_f32((float32_t *)inp, (float32_t *)cof, (float32_t *)outp, n)
#define v_zap(v,n)					arm_fill_f32(0.0f, (float32_t *)v, n)
#define v_fill(v,val,n)				arm_fill_f32(val, (float32_t *)v, n)
#define v_equ(v1,v2,n) 				arm_copy_f32((float32_t *)v2, (float32_t *)v1, n)
#define v_sub(v1,v2,n) 				arm_sub_f32((float32_t *)v1, (float32_t *)v2, (float32_t *)v1, n)
#define v_add(v1,v2,n) 				arm_add_f32((float32_t *)v1, (float32_t *)v2, (float32_t *)v1, n)
#define v_scale(v,scale,n)		    arm_scale_f32((float32_t *)v, scale, v, n)

#define v_zap_int(v,n)				arm_fill_q31(0, (q31_t *)v, n)
#define v_equ_int(v1,v2,n) 		    arm_copy_q31((q31_t *)v2, (q31_t *)v1, n)

#endif	//  __MAT_H__
