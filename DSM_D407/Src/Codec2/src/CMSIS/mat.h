#ifndef  __MAT_H__
#define  __MAT_H__

#ifndef _MSC_VER
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#endif

#define TWO_PI		(2 * PI)		/* mathematical constant                */
#define HALF_PI		(PI/2.0f)
#define ONEQTR_PI	(PI/4.0f)
#define THRQTR_PI   (3.0f*PI/4.0f)

#include "arm_math.h"
#include "arm_const_structs.h"

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

static __INLINE float arm_sqrt(float x)
{
	float32_t  res;
	arm_sqrt_f32(x, &res);
	return res;
}

static __INLINE float arm_sin(float x)
{
	float32_t  res;
	res = arm_sin_f32(x);
	return res;
}

static __INLINE float arm_cos(float x)
{
	float32_t  res;
	res = arm_cos_f32(x);
	return res;
}

static __INLINE void arm_autocorr(float input[], float r[], int order, int npts)
{
    int i;
    for (i = 0; i <= order; i++ )
	{
      r[i] = v_inner(&input[0],&input[i],(npts-i));
	}
}

static __INLINE float powf_fast(float a, float b) {
	union { float d; int x; } u = { a };
	u.x = (int)(b * (u.x - 1064866805) + 1064866805);
	return u.d;
}

static __INLINE float log2f_fast (float val)
{
	union { float d; int x; } u = { val };	
	float  log_2 = (float)(((u.x >> 23) & 255) - 128);
	u.x &= ~(255 << 23);
	u.x += (127 << 23);

	u.d = ((-0.3358287811f) * u.d + 2.0f) * u.d - 0.65871759316667f;   // (1)
	return (u.d + log_2);
} 

static __INLINE float atan2f_fast( float y, float x )
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


#define sinf(x)  	arm_sin(x)
#define cosf(x)	 	arm_cos(x)
#define sqrtf(x) 	arm_sqrt(x)
#define atan2f(y,x)	atan2f_fast((y),(x))
#define powf(a,b)	powf_fast((a),(b))
#define log2f(a)	log2f_fast((a))
#define log10f(a)	(log2f(a)* 0.301029995f)


extern  void arm_sqr_f32(float32_t * pSrc, float32_t * pDst, uint32_t numSamples);

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
