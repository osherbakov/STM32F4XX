/* ----------------------------------------------------------------------    
* Copyright (C) 2010-2014 ARM Limited. All rights reserved.    
*    
* $Date:        19. March 2015
* $Revision: 	V.1.4.5
*    
* Project: 	    CMSIS DSP Library    
* Title:	    arm_mult_f32.c    
*    
* Description:	Floating-point vector multiplication.    
*    
* Target Processor: Cortex-M4/Cortex-M3/Cortex-M0
*  
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the 
*     distribution.
*   - Neither the name of ARM LIMITED nor the names of its contributors
*     may be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.  
* -------------------------------------------------------------------- */
#include "arm_math.h"
#include "arm_const_structs.h"

/**        
 * @ingroup groupMath        
 */

/**        
 * @defgroup BasicMult Vector Multiplication        
 *        
 * Element-by-element squaring of a vector.        
 *        
 * <pre>        
 *     pDst[n] = pSrcA[n] * pSrcA[n],   0 <= n < blockSize.        
 * </pre>        
 *        
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.        
 */

/**        
 * @addtogroup BasicMult        
 * @{        
 */

/**        
 * @brief Floating-point vector squaring.        
 * @param[in]       *pSrcA points to the input vector            
 * @param[out]      *pDst points to the output vector        
 * @param[in]       blockSize number of samples in each vector        
 * @return none.        
 */

void arm_sqr_f32(
  float32_t * pSrcA,
  float32_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;                               /* loop counters */
#ifndef ARM_MATH_CM0_FAMILY

  /* Run the below code for Cortex-M4 and Cortex-M3 */
  float32_t inA1, inA2, inA3, inA4;              /* temporary input variables */
  float32_t out1, out2, out3, out4;              /* temporary output variables */

  /* loop Unrolling */
  blkCnt = blockSize >> 2u;

  /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.        
   ** a second loop below computes the remaining 1 to 3 samples. */
  while(blkCnt > 0u)
  {
    /* C = A * A */
    /* Multiply the inputs and store the results in output buffer */
    /* read sample from sourceA */
    
	inA1 = *pSrcA;
    /* read sample from sourceA */
    inA2 = *(pSrcA + 1);
    /* out = sourceA * sourceA */
    out1 = inA1 * inA1;
    
	/* read sample from sourceA */
    inA3 = *(pSrcA + 2);    
	out2 = inA2 * inA2;

    /* out = sourceA * sourceA */
    out3 = inA3 * inA3;

    /* read sample from sourceA */
    inA4 = *(pSrcA + 3);

    /* store result to destination buffer */
    *pDst = out1;

    /* out = sourceA * sourceB */
    out4 = inA4 * inA4;

    /* store result to destination buffer */
    *(pDst + 1) = out2;

    /* store result to destination buffer */
    *(pDst + 2) = out3;
    /* store result to destination buffer */
    *(pDst + 3) = out4;


    /* update pointers to process next samples */
    pSrcA += 4u;
    pDst += 4u;

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
    /* C = A * A */
    /* Multiply the inputs and store the results in output buffer */
    *pDst++ = (*pSrcA) * (*pSrcA);
	pSrcA++;
    /* Decrement the blockSize loop counter */
    blkCnt--;
  }
}

//
//  Fast math functions
//

float fdivf(float a, float b) {
    union { float f; int i; } v = {b};
    float w;
    v.i = (int)(0x7EF127EA - v.i);
    w = b * v.f;
    // Efficient Iterative Approximation Improvement in horner polynomial form.
    v.f = v.f * (2.0f - w);     // Single iteration, Err = -3.36e-3 * 2^(-flr(log2(x)))
    // v.f = v.f * ( 4 + w * (-6 + w * (4 - w)));  // Second iteration, Err = -1.13e-5 * 2^(-flr(log2(x)))
    // v.f = v.f * (8 + w * (-28 + w * (56 + w * (-70 + w *(56 + w * (-28 + w * (8 - w)))))));  // Third Iteration, Err = +-6.8e-8 *  2^(-flr(log2(x)))
    return a * v.f;
}

float finvf(float x) {
    union { float f; int i; } v = {x};
    float w;
    v.i = (int)(0x7EF127EA - v.i);
    w = x * v.f;
    // Efficient Iterative Approximation Improvement in horner polynomial form.
    v.f = v.f * (2.0f - w);     // Single iteration, Err = -3.36e-3 * 2^(-flr(log2(x)))
    // v.f = v.f * ( 4 + w * (-6 + w * (4 - w)));  // Second iteration, Err = -1.13e-5 * 2^(-flr(log2(x)))
    // v.f = v.f * (8 + w * (-28 + w * (56 + w * (-70 + w *(56 + w * (-28 + w * (8 - w)))))));  // Third Iteration, Err = +-6.8e-8 *  2^(-flr(log2(x)))
    return v.f ;
}

float fpowf(float a, float b)
{
    union { float d; int x; } u = { a };
    u.x = (int)(b * (u.x - 1064866805) + 1064866805);
    return u.d;
}
float flog2f(float val)
{
   union { float d; int x; } u = { val };
   float  log_2 = (float)(((u.x >> 23) & 255) - 128);
   u.x &= ~(255 << 23);
   u.x += (127 << 23);
   u.d = ((-0.3358287811f) * u.d + 2.0f) * u.d - 0.65871759316667f;   // (1)
   return (u.d + log_2);
}


#define SQRT_MAGIC_F 0x5f3759df
float  fsqrtf(const float x)
{
  const float xhalf = 0.5f*x;
  union // get bits for floating value
  {
    float x;
    int i;
  } u;
  u.x = x;
  u.i = SQRT_MAGIC_F - (u.i >> 1);  // gives initial guess y0
  return x*u.x*(1.5f - xhalf*u.x*u.x);// Newton step, repeating increases accuracy
}

#define B (4.0f/(float)M_PI)
#define C (-4.0f/((float)M_PI*(float)M_PI))
#define P (0.225f)
#define Q (0.775f)

#ifndef M_PI
#define M_PI (3.14159265358979323846F)
#endif

#ifndef M_PI_2
#define M_PI_2 ((float)M_PI/2)
#endif

float fsinf(float x)
{
	float y;
    y = B * x + C * x * fabsf(x);
    y = P * (y * fabsf(y) - y) + y;   // Q * y + P * y * abs(y)
    return y;
}

float fcosf(float x) {
	return fsinf(x + (float)M_PI_2);
}

/**        
 * @} end of BasicMult group        
 */
