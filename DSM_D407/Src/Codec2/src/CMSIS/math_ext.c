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
#ifndef _MSC_VER
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#endif
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
 * @brief Floating-point vector multiplication.        
 * @param[in]       *pSrcA points to the first input vector        
 * @param[in]       *pSrcB points to the second input vector        
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

/**        
 * @} end of BasicMult group        
 */
