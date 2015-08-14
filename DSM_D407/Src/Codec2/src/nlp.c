/*---------------------------------------------------------------------------*\
                                                 
  FILE........: nlp.c                                                   
  AUTHOR......: David Rowe                                      
  DATE CREATED: 23/3/93                                    
                                                         
  Non Linear Pitch (NLP) estimation functions.

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2009 David Rowe

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#include <math.h>

#include "defines.h"
#include "nlp.h"
#include "codec2_internal.h"
#include "codec2.h"
#include "mat.h"
/*---------------------------------------------------------------------------*\
                                                                             
 				DEFINES                                       
                                                                             
\*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*\
                                                                            
 				GLOBALS                                       
                                                                             
\*---------------------------------------------------------------------------*/

/* 48 tap 600Hz low pass FIR filter coefficients */

static const float nlp_fir[] RODATA = {
  -1.0818124e-03,
  -1.1008344e-03,
  -9.2768838e-04,
  -4.2289438e-04,
   5.5034190e-04,
   2.0029849e-03,
   3.7058509e-03,
   5.1449415e-03,
   5.5924666e-03,
   4.3036754e-03,
   8.0284511e-04,
  -4.8204610e-03,
  -1.1705810e-02,
  -1.8199275e-02,
  -2.2065282e-02,
  -2.0920610e-02,
  -1.2808831e-02,
   3.2204775e-03,
   2.6683811e-02,
   5.5520624e-02,
   8.6305944e-02,
   1.1480192e-01,
   1.3674206e-01,
   1.4867556e-01,
   1.4867556e-01,
   1.3674206e-01,
   1.1480192e-01,
   8.6305944e-02,
   5.5520624e-02,
   2.6683811e-02,
   3.2204775e-03,
  -1.2808831e-02,
  -2.0920610e-02,
  -2.2065282e-02,
  -1.8199275e-02,
  -1.1705810e-02,
  -4.8204610e-03,
   8.0284511e-04,
   4.3036754e-03,
   5.5924666e-03,
   5.1449415e-03,
   3.7058509e-03,
   2.0029849e-03,
   5.5034190e-04,
  -4.2289438e-04,
  -9.2768838e-04,
  -1.1008344e-03,
  -1.0818124e-03
};


float post_process_sub_multiples(float Fw[], 
				 int pmin, int pmax, float gmax, int gmax_bin,
				 float prev_Wo);

/*---------------------------------------------------------------------------*\
                                                                             
  nlp_create()                                                                  
                                                                             
  Initialisation function for NLP pitch estimator.

\*---------------------------------------------------------------------------*/

void nlp_init(void *state,
	int    m			/* analysis window size */
)
{
    int  i;
	NLP  *nlp = (NLP *)state;
	
    nlp->m = m;

	nlp->w = tbl.Nlp_w;	
	for(i=0; i<m/DEC; i++) {
		nlp->w[i] = 0.5f - 0.5f*cosf(2*PI*i/(m/DEC-1));
    }
    arm_fir_init_f32 (&nlp->arm_fir, NLP_NTAP, (float32_t *) nlp_fir, nlp->mem_fir, FRAME_SIZE);

	nlp->mem_x = 0.0;
    nlp->mem_y = 0.0;

	v_zap(nlp->sq, PMAX_M);
	v_zap(nlp->mem_fir, NLP_NTAP);
}

/*---------------------------------------------------------------------------*\
                                                                            
  nlp()                                                                  
                                                                             
  Determines the pitch in samples using the Non Linear Pitch (NLP)
  algorithm [1]. Returns the fundamental in Hz.  Note that the actual
  pitch estimate is for the centre of the M sample Sn[] vector, not
  the current N sample input vector.  This is (I think) a delay of 2.5
  frames with N=80 samples.  You should align further analysis using
  this pitch estimate to be centred on the middle of Sn[].

  Two post processors have been tried, the MBE version (as discussed
  in [1]), and a post processor that checks sub-multiples.  Both
  suffer occasional gross pitch errors (i.e. neither are perfect).  In
  the presence of background noise the sub-multiple algorithm tends
  towards low F0 which leads to better sounding background noise than
  the MBE post processor.

  A good way to test and develop the NLP pitch estimator is using the
  tnlp (codec2/unittest) and the codec2/octave/plnlp.m Octave script.

  A pitch tracker searching a few frames forward and backward in time
  would be a useful addition.

  References:

    [1] http://www.itr.unisa.edu.au/~steven/thesis/dgr.pdf Chapter 4
                                                              
\*---------------------------------------------------------------------------*/

float nlp(
  void	*nlp_state, 
  float  Sn[],			/* input speech vector */
  int    n,				/* frames shift (no. new samples in Sn[]) */
  int    pmin,          /* minimum pitch value */
  int    pmax,			/* maximum pitch value */
  float *pitch,			/* estimated pitch period in samples */
  COMP   Sw[],          /* Freq domain version of Sn[] */
  COMP   W[],           /* Freq domain window */
  float prev_Wo
)
{
    NLP   *nlp;
    float  notch;		    /* current notch filter output    */
    float  gmax;
    int    gmax_bin;
    int    m, i, new_idx, nCount;
	int    idx_start, idx_end;
	float  x, y;
	float  *pData;
    float  best_f0;
    COMP   *Fw = tmp.Fw;	    /* DFT of squared signal (input/output) */

    nlp = (NLP*)nlp_state;
    m = nlp->m;
	new_idx = m - n;
	idx_start = FFT_PE*DEC/pmax;
	idx_end = FFT_PE*DEC/pmin;

    /* Square, notch filter at DC, and LP filter vector */
	arm_mult_f32(&Sn[new_idx],&Sn[new_idx], &nlp->sq[new_idx], n);

	// Implement simplest IIR "mono-quad" filter:
	//   y(n) = x(n) - x(n-1) + C * y(n-1)
	//
	pData = &nlp->sq[new_idx];
	nCount = n;
	x = nlp->mem_x; y = nlp->mem_y;   /* load state variables */
	while(nCount-- > 0) {	/* notch filter at DC */
		notch = *pData;
		y = notch - x + COEFF * y;
		x = notch;
		*pData++ = y;
	}
	nlp->mem_x = x; nlp->mem_y = y; /* store state variables   */
    
	arm_fir_f32 (&nlp->arm_fir, &nlp->sq[new_idx], &nlp->sq[new_idx], n);

	/* Decimate and DFT */
	v_zap(Fw, 2 * FFT_PE);
    for(i=0; i<m/DEC; i++) {
		Fw[i].real = nlp->sq[i*DEC]*nlp->w[i];
    }
	arm_cfft_f32(&arm_cfft_sR_f32_len512, (float32_t *)Fw, 0, 1);


	arm_cmplx_mag_squared_f32((float32_t *)&Fw[0], &((float *)Fw)[0], FFT_PE);

    /* find global peak */
	arm_max_f32(&((float *)Fw)[idx_start], idx_end - idx_start + 1, &gmax, (uint32_t *) &gmax_bin);
	gmax_bin += idx_start;
    
	best_f0 = post_process_sub_multiples((float *)Fw, pmin, pmax, gmax, gmax_bin, prev_Wo);

    /* Shift samples in buffer to make room for new samples */
	v_equ(&nlp->sq[0], &nlp->sq[n], new_idx);

    /* return pitch and F0 estimate */
    *pitch = (float)SAMPLE_RATE/best_f0;

    return(best_f0);  
}

/*---------------------------------------------------------------------------*\
                                                                             
  post_process_sub_multiples() 
                                                                           
  Given the global maximma of Fw[] we search integer submultiples for
  local maxima.  If local maxima exist and they are above an
  experimentally derived threshold (OK a magic number I pulled out of
  the air) we choose the submultiple as the F0 estimate.

  The rational for this is that the lowest frequency peak of Fw[]
  should be F0, as Fw[] can be considered the autocorrelation function
  of Sw[] (the speech spectrum).  However sometimes due to phase
  effects the lowest frequency maxima may not be the global maxima.

  This works OK in practice and favours low F0 values in the presence
  of background noise which means the sinusoidal codec does an OK job
  of synthesising the background noise.  High F0 in background noise
  tends to sound more periodic introducing annoying artifacts.

\*---------------------------------------------------------------------------*/

float post_process_sub_multiples(float Fw[], 
	int pmin, int pmax, float gmax, int gmax_bin,
	float prev_Wo)
{
    int   min_bin, cmax_bin;
    int   mult;
    float thresh, best_f0;
    int   b, bmin, bmax, lmax_bin;
    float lmax;
    int   prev_f0_bin;

    /* post process estimate by searching submultiples */

    mult = 2;
    min_bin = FFT_PE*DEC/pmax; 
    cmax_bin = gmax_bin;
    prev_f0_bin = prev_Wo*(4000.0f/PI)*(FFT_PE*DEC)/SAMPLE_RATE;
    
    while(gmax_bin/mult >= min_bin) {
		b = gmax_bin/mult;			/* determine search interval */
		bmin = 0.8f*b;
		bmax = 1.2f*b;
		if (bmin < min_bin) bmin = min_bin;

		/* lower threshold to favour previous frames pitch estimate,
			this is a form of pitch tracking */

		if ((prev_f0_bin > bmin) && (prev_f0_bin < bmax))
			thresh = CNLP*0.5f*gmax;
		else
			thresh = CNLP*gmax;

		arm_max_f32(&Fw[bmin], bmax - bmin + 1, &lmax, (uint32_t *)&lmax_bin);
		lmax_bin += bmin;

		if ((lmax > thresh) &&
		    (lmax > Fw[lmax_bin-1]) && (lmax > Fw[lmax_bin+1])) {
				cmax_bin = lmax_bin;
	    }
		mult++;
    }

    best_f0 = (((float)cmax_bin ) * SAMPLE_RATE)/(FFT_PE*DEC);

    return best_f0;
}
