/*---------------------------------------------------------------------------*\
                                                                             
  FILE........: sine.c
  AUTHOR......: David Rowe                                           
  DATE CREATED: 19/8/2010
                                                                             
  Sinusoidal analysis and synthesis functions.
                                                                             
\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 1990-2010 David Rowe

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

/*---------------------------------------------------------------------------*\
                                                                             
				INCLUDES                                      
                                                                             
\*---------------------------------------------------------------------------*/

#include <math.h>
#include "defines.h"
#include "sine.h"
#include "codec2_internal.h"

#include "mat.h"

/*---------------------------------------------------------------------------*\
                                                                             
				HEADERS                                     
                                                                             
\*---------------------------------------------------------------------------*/

void hs_pitch_refinement(MODEL *model, COMP Sw[], float pmin, float pmax, 
			 float pstep);

/*---------------------------------------------------------------------------*\
                                                                             
				FUNCTIONS                                     
                                                                             
\*---------------------------------------------------------------------------*/

// Fast atan2f function

float fast_atan2f( float y, float x )
{
	float r, angle;
	float abs_y = fabsf(y);      // kludge to prevent 0/0 condition
	if ( x < 0.0f )
	{
		r = (x + abs_y) / (abs_y - x);
		angle = THRQTR_PI;
	}else
	{
		r = (x - abs_y) / (x + abs_y);
		angle = ONEQTR_PI;
	}
	angle += (0.1963f * r * r - 0.9817f) * r;
	if ( y < 0.0f )
		return( -angle );     // negate if in quad III or IV
	else
		return( angle );
}

// |error| < 0.005
float fastest_atan2f( float y, float x )
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


/*---------------------------------------------------------------------------*\
                                                       
  FUNCTION....: make_analysis_window	     
  AUTHOR......: David Rowe			      
  DATE CREATED: 11/5/94 

  Init function that generates the time domain analysis window and it's DFT.

\*---------------------------------------------------------------------------*/

void make_analysis_window(float w[], COMP W[])
{
  float m;
  COMP  temp;
  int   i,j;

  /* 
     Generate Hamming window centered on M-sample pitch analysis window
  
  0            M/2           M-1
  |-------------|-------------|
        |-------|-------|
            NW samples

     All our analysis/synthsis is centred on the M/2 sample.               
  */

  m = 0.0;

  v_zap(w, P_SIZE);
  for(i=P_SIZE/2-NW/2,j=0; i<P_SIZE/2+NW/2; i++,j++) {
    w[i] = 0.5f - 0.5f*cosf(TWO_PI*j/(NW-1));
    m += w[i]*w[i];
  }

  /* Normalise - makes freq domain amplitude estimation straight forward */
  m = 1.0f/sqrtf(m*FFT_ENC);
  v_scale(w, m, P_SIZE);

  /* 
     Generate DFT of analysis window, used for later processing.  Note
     we modulo FFT_ENC shift the time domain window w[], this makes the
     imaginary part of the DFT W[] equal to zero as the shifted w[] is
     even about the n=0 time axis if NW is odd.  Having the imag part
     of the DFT W[] makes computation easier.

     0                      FFT_ENC-1
     |-------------------------|

      ----\               /----
           \             / 
            \           /          <- shifted version of window w[n]
             \         /
              \       /
               -------

     |---------|     |---------|      
       NW/2              NW/2
  */

  v_zap(W, 2 * FFT_ENC);
  for(i=0; i<NW/2; i++)
    W[i].real = w[i+P_SIZE/2];
  for(i=FFT_ENC-NW/2,j=P_SIZE/2-NW/2; i<FFT_ENC; i++,j++)
	W[i].real = w[j];

  arm_cfft_f32(&arm_cfft_sR_f32_len512, (float32_t *)W, 0, 1);

  /* 
      Re-arrange W[] to be symmetrical about FFT_ENC/2.  Makes later 
      analysis convenient.

   Before:
     0                 FFT_ENC-1
     |----------|---------|
     __                   _       
       \                 /          
        \_______________/      

   After:
     0                 FFT_ENC-1
     |----------|---------|
               ___                        
              /   \                
     ________/     \_______     

  */
             
  for(i=0; i<FFT_ENC/2; i++) {
    temp.real = W[i].real;
    temp.imag = W[i].imag;
    W[i].real = W[i+FFT_ENC/2].real;
    W[i].imag = W[i+FFT_ENC/2].imag;
    W[i+FFT_ENC/2].real = temp.real;
    W[i+FFT_ENC/2].imag = temp.imag;
  }

}


/*---------------------------------------------------------------------------*\
                                                       
  FUNCTION....: dft_speech	     
  AUTHOR......: David Rowe			      
  DATE CREATED: 27/5/94 

  Finds the DFT of the current speech input speech frame.

\*---------------------------------------------------------------------------*/

void dft_speech(COMP Sw[], float Sn[], float w[])
{
  int  i;

  v_zap(Sw, 2 * FFT_ENC);
  /* Centre analysis window on time axis, we need to arrange input
     to FFT this way to make FFT phases correct */
  
  /* move 2nd half to start of FFT input vector */
  for(i=0; i<NW/2; i++)
    Sw[i].real = Sn[i+P_SIZE/2]*w[i+P_SIZE/2];

  /* move 1st half to end of FFT input vector */
  for(i=0; i<NW/2; i++)
    Sw[FFT_ENC-NW/2+i].real = Sn[i+P_SIZE/2-NW/2]*w[i+P_SIZE/2-NW/2];

  arm_cfft_f32(&arm_cfft_sR_f32_len512, (float32_t *)Sw, 0, 1);
}

/*---------------------------------------------------------------------------*\
                                                                     
  FUNCTION....: two_stage_pitch_refinement			
  AUTHOR......: David Rowe
  DATE CREATED: 27/5/94				

  Refines the current pitch estimate using the harmonic sum pitch
  estimation technique.

\*---------------------------------------------------------------------------*/

void two_stage_pitch_refinement(MODEL *model, COMP Sw[])
{
  float pmin,pmax,pstep;	/* pitch refinment minimum, maximum and step */ 

  /* Coarse refinement */

  pmax = TWO_PI/model->Wo + 5;
  pmin = TWO_PI/model->Wo - 5;
  pstep = 1.0f;
  hs_pitch_refinement(model,Sw,pmin,pmax,pstep);
  
  /* Fine refinement */
  
  pmax = TWO_PI/model->Wo + 1;
  pmin = TWO_PI/model->Wo - 1;
  pstep = 0.25f;
  hs_pitch_refinement(model,Sw,pmin,pmax,pstep);
  
  /* Limit range */  
  if (model->Wo < TWO_PI/P_MAX) model->Wo = TWO_PI/P_MAX;
  if (model->Wo > TWO_PI/P_MIN) model->Wo = TWO_PI/P_MIN;

  model->L = PI/model->Wo;
}

/*---------------------------------------------------------------------------*\
                                                                
 FUNCTION....: hs_pitch_refinement				
 AUTHOR......: David Rowe			
 DATE CREATED: 27/5/94							     
									  
 Harmonic sum pitch refinement function.			   
									    
 pmin   pitch search range minimum	    
 pmax	pitch search range maximum	    
 step   pitch search step size		    
 model	current pitch estimate in model.Wo  
									    
 model 	refined pitch estimate in model.Wo  
									     
\*---------------------------------------------------------------------------*/

void hs_pitch_refinement(MODEL *model, COMP Sw[], float pmin, float pmax, float pstep)
{
  int m;		/* loop variable */
  int b;		/* bin for current harmonic centre */
  float E;		/* energy for current pitch*/
  float Wo;		/* current "test" fundamental freq. */
  float Wom;		/* Wo that maximises E */
  float Em;		/* mamimum energy */
  float r, one_on_r;	/* number of rads/bin */
  float p;		/* current pitch */
  
  /* Initialisation */
  
  model->L = PI/model->Wo;	/* use initial pitch est. for L */
  Wom = model->Wo;
  Em = 0.0;
  r = TWO_PI/FFT_ENC;
  one_on_r = 1.0f/r;

  /* Determine harmonic sum for a range of Wo values */

  for(p=pmin; p<=pmax; p+=pstep) {
    E = 0.0;
    Wo = TWO_PI/p;

    /* Sum harmonic magnitudes */
    for(m=1; m<=model->L; m++) {
        b = (int)floorf(m*Wo*one_on_r + 0.5f);
        E += Sw[b].real*Sw[b].real + Sw[b].imag*Sw[b].imag;
    }

    /* Compare to see if this is a maximum */    
    if (E > Em) {
      Em = E;
      Wom = Wo;
    }
  }

  model->Wo = Wom;
}

/*---------------------------------------------------------------------------*\
                                                                             
  FUNCTION....: estimate_amplitudes 			      
  AUTHOR......: David Rowe		
  DATE CREATED: 27/5/94			       
									      
  Estimates the complex amplitudes of the harmonics.    
									      
\*---------------------------------------------------------------------------*/

void estimate_amplitudes(MODEL *model, COMP Sw[], COMP W[])
{
  int   i,m;		/* loop variables */
  int   am,bm;		/* bounds of current harmonic */
  int   b;		/* DFT bin of centre of current harmonic */
  float den;		/* denominator of amplitude expression */
  float r, one_on_r;	/* number of rads/bin */
  int   offset;
  COMP  Am;

  r = TWO_PI/FFT_ENC;
  one_on_r = 1.0f/r;

  for(m=1; m<=model->L; m++) {
    am = (int)floorf((m - 0.5f)*model->Wo*one_on_r + 0.5f);
    bm = (int)floorf((m + 0.5f)*model->Wo*one_on_r + 0.5f);
    b = (int)floorf(m*model->Wo/r + 0.5f);

    /* Estimate ampltude of harmonic */

    den = 0.0;
    Am.real = Am.imag = 0.0;
    offset = FFT_ENC/2 - (int)floorf(m*model->Wo*one_on_r + 0.5f);
    for(i=am; i<bm; i++) {
      den += Sw[i].real*Sw[i].real + Sw[i].imag*Sw[i].imag;
      Am.real += Sw[i].real*W[i + offset].real;
      Am.imag += Sw[i].imag*W[i + offset].real;
    }

    model->A[m] = sqrtf(den);
  }
}

/*---------------------------------------------------------------------------*\
                                                                             
  est_voicing_mbe()          
                                                                             
  Returns the error of the MBE cost function for a fiven F0.

  Note: I think a lot of the operations below can be simplified as
  W[].imag = 0 and has been normalised such that den always equals 1.
                                                                             
\*---------------------------------------------------------------------------*/

float est_voicing_mbe(
    MODEL *model,
    COMP   Sw[],
	COMP   W[])
{
    int   l,al,bl,m;    /* loop variables */
    COMP  Am;             /* amplitude sample for this band */
    int   offset;         /* centers Hw[] about current harmonic */
    float den;            /* denominator of Am expression */
    float error;          /* accumulated error between original and synthesised */
	float W_real;
    float Wo;
	COMP  Ew;
	COMP  Sw_;
    float sig, snr;
    float elow, ehigh, eratio;
    float sixty;

    /* Calculate signal energy in the first 1000 Hz (L/4) */
	arm_power_f32(&model->A[1], model->L/4, &sig);

    Wo = model->Wo; 
    error = 1E-4f;

    /* Just test across the harmonics in the first 1000 Hz (L/4) */

    for(l=1; l<=model->L/4; l++) {
		Am.real = 0.0;
		Am.imag = 0.0;
		al = ceilf((l - 0.5f)*Wo*FFT_ENC/TWO_PI);
		bl = ceilf((l + 0.5f)*Wo*FFT_ENC/TWO_PI);
		offset = floorf(FFT_ENC/2 - l*Wo*FFT_ENC/TWO_PI + 0.5f);

		/* Estimate amplitude of harmonic assuming harmonic is totally voiced */

		den = 0.0;
		for(m=al; m<bl; m++) {
			W_real = W[offset+m].real;
			Am.real += Sw[m].real*W_real;
			Am.imag += Sw[m].imag*W_real;
			den += W_real*W_real;
		}

		Am.real = Am.real/den;
		Am.imag = Am.imag/den;

		/* Determine error between estimated harmonic and original */

		for(m=al; m<bl; m++) {
			W_real = W[offset+m].real;
			Sw_.real = Am.real*W_real;
			Sw_.imag = Am.imag*W_real;
			Ew.real = Sw[m].real - Sw_.real;
			Ew.imag = Sw[m].imag - Sw_.imag;
			error += Ew.real*Ew.real + Ew.imag*Ew.imag;
		}
    }
    
    snr = 10.0f*log10f(sig/error);
    if (snr > V_THRESH)
		model->voiced = 1;
    else
		model->voiced = 0;
 
    /* post processing, helps clean up some voicing errors ------------------*/

    /* 
       Determine the ratio of low freqency to high frequency energy,
       voiced speech tends to be dominated by low frequency energy,
       unvoiced by high frequency. This measure can be used to
       determine if we have made any gross errors.
    */

//    elow = ehigh = 1E-4f;
//    for(l=1; l<=model->L/2; l++) {
//		elow += model->A[l]*model->A[l];
//    }
//    for(l=model->L/2; l<=model->L; l++) {
//		ehigh += model->A[l]*model->A[l];
//    }
	arm_power_f32(&model->A[1], model->L/2, &elow);
	arm_power_f32(&model->A[model->L/2], model->L/2, &ehigh);

    eratio = 10.0f*log10f(elow/ehigh);

    /* Look for Type 1 errors, strongly V speech that has been
       accidentally declared UV */

    if (model->voiced == 0)
		if (eratio > 10.0f)
			model->voiced = 1;

    /* Look for Type 2 errors, strongly UV speech that has been
       accidentally declared V */

    if (model->voiced == 1) {
		if (eratio < -10.0f)
			model->voiced = 0;

		/* A common source of Type 2 errors is the pitch estimator
		   gives a low (50Hz) estimate for UV speech, which gives a
		   good match with noise due to the close harmoonic spacing.
		   These errors are much more common than people with 50Hz3
		   pitch, so we have just a small eratio threshold. */
		sixty = 60.0f*TWO_PI/SAMPLE_RATE;
		if ((eratio < -4.0f) && (model->Wo <= sixty))
			model->voiced = 0;
    }
    return snr;
}

/*---------------------------------------------------------------------------*\
                                                       
  FUNCTION....: make_synthesis_window	     
  AUTHOR......: David Rowe			      
  DATE CREATED: 11/5/94 

  Init function that generates the trapezoidal (Parzen) sythesis window.

\*---------------------------------------------------------------------------*/

void make_synthesis_window(float Pn[])
{
  int   i;
  float win;

  /* Generate Parzen window in time domain */

  win = 0.0;
  for(i=0; i<FRAME_SIZE/2-TW; i++)
    Pn[i] = 0.0;
  win = 0.0;
  for(i=FRAME_SIZE/2-TW; i<FRAME_SIZE/2+TW; win+=1.0f/(2*TW), i++ )
    Pn[i] = win;
  for(i=FRAME_SIZE/2+TW; i<3*FRAME_SIZE/2-TW; i++)
    Pn[i] = 1.0f;
  win = 1.0;
  for(i=3*FRAME_SIZE/2-TW; i<3*FRAME_SIZE/2+TW; win-=1.0f/(2*TW), i++)
    Pn[i] = win;
  for(i=3*FRAME_SIZE/2+TW; i<2*FRAME_SIZE; i++)
    Pn[i] = 0.0;
}

/*---------------------------------------------------------------------------*\
                                                                             
  FUNCTION....: synthesise 			      
  AUTHOR......: David Rowe		
  DATE CREATED: 20/2/95		       
									      
  Synthesise a speech signal in the frequency domain from the
  sinusodal model parameters.  Uses overlap-add with a trapezoidal
  window to smoothly interpolate betwen frames.
									      
\*---------------------------------------------------------------------------*/

void synthesise(
  float  Sn_[],		/* time domain synthesised signal              */
  MODEL *model,		/* ptr to model parameters for this frame      */
  float  Pn[]		/* time domain Parzen window                   */
)
{
    int   i,j,b;			/* loop variables */
    COMP  *sw_ = tmp.sw_;	/* synthesised signal */

	/* Update memories */
	v_equ(&Sn_[0], &Sn_[FRAME_SIZE], FRAME_SIZE);

    /*
      Nov 2010 - found that synthesis using time domain cos() functions
      gives better results for synthesis frames greater than 10ms.  Inverse
      FFT synthesis using a 512 pt FFT works well for 10ms window.  I think
      (but am not sure) that the problem is related to the quantisation of
      the harmonic frequencies to the FFT bin size, e.g. there is a 
      8000/512 Hz step between FFT bins.  For some reason this makes
      the speech from longer frame > 10ms sound poor.  The effect can also
      be seen when synthesising test signals like single sine waves, some
      sort of amplitude modulation at the frame rate.

      Another possibility is using a larger FFT size (1024 or 2048).
    */
	v_zap(sw_, 2 * FFT_DEC);

	/* Now set up frequency domain synthesised speech */
    for(i=1; i<=model->L; i++) {
		b = (int)floorf(i*model->Wo*FFT_DEC/TWO_PI + 0.5f);
		if (b > ((FFT_DEC/2)-1))  b = (FFT_DEC/2)-1;
		sw_[b].real = model->A[i]*cosf(model->phi[i]);
		sw_[b].imag = model->A[i]*sinf(model->phi[i]);
		sw_[FFT_DEC-b].real = sw_[b].real;
		sw_[FFT_DEC-b].imag = -sw_[b].imag;
    }
    /* Perform inverse DFT */
	arm_cfft_f32(&arm_cfft_sR_f32_len512, (float32_t *) sw_, 1, 1);
	arm_scale_f32((float32_t *)sw_, FFT_DEC, (float32_t *)sw_, 2 * FFT_DEC);

    /* Overlap add to previous samples */
    for(i=0; i<FRAME_SIZE; i++)
		Sn_[i] += sw_[FFT_DEC-FRAME_SIZE+i].real*Pn[i];

	for(i=FRAME_SIZE,j=0; i<2*FRAME_SIZE; i++,j++)
		Sn_[i] = sw_[j].real*Pn[i];
}


static unsigned long next = 1;

int codec2_rand(void) {
    next = next * 1103515245 + 12345;
    return((unsigned)(next/65536) % 32768);
}

