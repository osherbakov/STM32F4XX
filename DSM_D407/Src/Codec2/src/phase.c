/*---------------------------------------------------------------------------*\
                                                                             
  FILE........: phase.c                                           
  AUTHOR......: David Rowe                                             
  DATE CREATED: 1/2/09                                                 
                                                                             
  Functions for modelling and synthesising phase.
                                                                             
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
  along with this program; if not,see <http://www.gnu.org/licenses/>. 
*/
#include <math.h>
#include "defines.h"
#include "phase.h"
#include "comp.h"
#include "sine.h"

#include "mat.h"

/*---------------------------------------------------------------------------*\

   phase_synth_zero_order()

   Synthesises phases based on SNR and a rule based approach.  No phase 
   parameters are required apart from the SNR (which can be reduced to a
   1 bit V/UV decision per frame).

   The phase of each harmonic is modelled as the phase of a LPC
   synthesis filter excited by an impulse.  Unlike the first order
   model the position of the impulse is not transmitted, so we create
   an excitation pulse train using a rule based approach.  

   Consider a pulse train with a pulse starting time n=0, with pulses
   repeated at a rate of Wo, the fundamental frequency.  A pulse train
   in the time domain is equivalent to harmonics in the frequency
   domain.  We can make an excitation pulse train using a sum of
   sinsusoids:

     for(m=1; m<=L; m++)
       ex[n] = cos(m*Wo*n)

   Note: the Octave script ../octave/phase.m is an example of this if
   you would like to try making a pulse train.

   The phase of each excitation harmonic is:

     arg(E[m]) = mWo

   where E[m] are the complex excitation (freq domain) samples,
   arg(x), just returns the phase of a complex sample x.

   As we don't transmit the pulse position for this model, we need to
   synthesise it.  Now the excitation pulses occur at a rate of Wo.
   This means the phase of the first harmonic advances by N samples
   over a synthesis frame of N samples.  For example if Wo is pi/20
   (200 Hz), then over a 10ms frame (N=80 samples), the phase of the
   first harmonic would advance (pi/20)*80 = 4*pi or two complete
   cycles.

   We generate the excitation phase of the fundamental (first
   harmonic):

     arg[E[1]] = Wo*N;

   We then relate the phase of the m-th excitation harmonic to the
   phase of the fundamental as:

     arg(E[m]) = m*arg(E[1])

   This E[m] then gets passed through the LPC synthesis filter to
   determine the final harmonic phase.
     
   Comparing to speech synthesised using original phases:

   - Through headphones speech synthesised with this model is not as 
     good. Through a loudspeaker it is very close to original phases.

   - If there are voicing errors, the speech can sound clicky or
     staticy.  If V speech is mistakenly declared UV, this model tends to
     synthesise impulses or clicks, as there is usually very little shift or
     dispersion through the LPC filter.

   - When combined with LPC amplitude modelling there is an additional
     drop in quality.  I am not sure why, theory is interformant energy
     is raised making any phase errors more obvious.

   NOTES:

     1/ This synthesis model is effectively the same as a simple LPC-10
     vocoders, and yet sounds much better.  Why? Conventional wisdom
     (AMBE, MELP) says mixed voicing is required for high quality
     speech.

     2/ I am pretty sure the Lincoln Lab sinusoidal coding guys (like xMBE
     also from MIT) first described this zero phase model, I need to look
     up the paper.

     3/ Note that this approach could cause some discontinuities in
     the phase at the edge of synthesis frames, as no attempt is made
     to make sure that the phase tracks are continuous (the excitation
     phases are continuous, but not the final phases after filtering
     by the LPC spectra).  Technically this is a bad thing.  However
     this may actually be a good thing, disturbing the phase tracks a
     bit.  More research needed, e.g. test a synthesis model that adds
     a small delta-W to make phase tracks line up for voiced
     harmonics.

\*---------------------------------------------------------------------------*/

void phase_synth_zero_order(
    MODEL *model,
    float *ex_phase,            /* excitation phase of fundamental */
    COMP   Aw[]
)
{
    int   m, b;
    float phi, new_phi, r;
    COMP  Ex;		  /* excitation samples */
    COMP  A;		  /* synthesised harmonic sample */
    COMP  H;		  /* LPC freq domain sample */

    r = TWO_PI/(FFT_ENC);

    /* 
       Update excitation fundamental phase track, this sets the position
       of each pitch pulse during voiced speech.  After much experiment
       I found that using just this frame's Wo improved quality for UV
       sounds compared to interpolating two frames Wo like this:
     
       ex_phase[0] += (*prev_Wo+model->Wo)*N/2;
    */
  
    ex_phase[0] += (model->Wo)*FRAME_SIZE;
    ex_phase[0] -= TWO_PI*floorf(ex_phase[0]/TWO_PI + 0.5f);

    for(m=1; m<=model->L; m++) {
        
		/* Sample phase at harmonics */
		b = (int)floorf(m*model->Wo/r + 0.5f);
        phi = -atan2f(Aw[b].imag, Aw[b].real);
//		phi_ = -fastest_atan2f(Aw[b].imag, Aw[b].real);
        H.real = cosf(phi);
        H.imag = sinf(phi);
    
        /* generate excitation */	    
        if (model->voiced) {
            phi = ex_phase[0]*m;
        }else {
            /* When a few samples were tested I found that LPC filter
               phase is not needed in the unvoiced case, but no harm in
               keeping it.
            */
            phi = TWO_PI*(float)codec2_rand()/CODEC2_RAND_MAX;
        }
        Ex.real = cosf(phi);
        Ex.imag = sinf(phi);

        /* filter using LPC filter */

        A.real = H.real*Ex.real - H.imag*Ex.imag;
        A.imag = H.imag*Ex.real + H.real*Ex.imag;

        /* modify sinusoidal phase */
   
        new_phi = atan2f(A.imag, A.real + 1E-12f);
        model->phi[m] = new_phi;
    }
}

