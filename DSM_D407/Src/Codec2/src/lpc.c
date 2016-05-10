/*---------------------------------------------------------------------------*\
                                                                           
  FILE........: lpc.c                                                              
  AUTHOR......: David Rowe                                                      
  DATE CREATED: 30 Sep 1990 (!)                                                 
                                                                          
  Linear Prediction functions written in C.                                
                                                                          
\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2009-2012 David Rowe

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

#define LPC_MAX_N 512		/* maximum no. of samples in frame */

#include <math.h>
#include "mat.h"

#include "defines.h"
#include "lpc.h"



/*---------------------------------------------------------------------------*\
                                                                           
  autocorrelate()                                                          
                                                                          
  Finds the first P autocorrelation values of an array of windowed speech 
  samples Sn[].                                                            
                                                                          
\*---------------------------------------------------------------------------*/

void autocorrelate(
  float Sn[],	/* frame of Nsam windowed speech samples */
  float Rn[],	/* array of P+1 autocorrelation coefficients */
  int Nsam,	/* number of windowed samples to use */
  int order	/* order of LPC analysis */
)
{
	arm_autocorr(Sn, Rn, order, Nsam);
}

/*---------------------------------------------------------------------------*\
                                                                            
  levinson_durbin()                                                        
                                                                           
  Given P+1 autocorrelation coefficients, finds P Linear Prediction Coeff. 
  (LPCs) where P is the order of the LPC all-pole model. The Levinson-Durbin
  algorithm is used, and is described in:                                   
                                                                           
    J. Makhoul                                                               
    "Linear prediction, a tutorial review"                                   
    Proceedings of the IEEE                                                
    Vol-63, No. 4, April 1975                                               
                                                                             
\*---------------------------------------------------------------------------*/

void levinson_durbin(
  float R[],		/* order+1 autocorrelation coeff */
  float lpcs[],		/* order+1 LPC's */
  int order		/* order of the LPC analysis */
)
{
  float a[LPC_ORD+1][LPC_ORD+1];
  float sum, e, k;
  int i,j;				/* loop variables */

  e = R[0];				/* Equation 38a, Makhoul */

  for(i=1; i<=order; i++) {
    sum = 0.0;
    for(j=1; j<=i-1; j++)
      sum += a[i-1][j]*R[i-j];
    k = -1.0f*(R[i] + sum)/e;		/* Equation 38b, Makhoul */
    if (fabsf(k) > 1.0f)
      k = 0.0;

    a[i][i] = k;

    for(j=1; j<=i-1; j++)
      a[i][j] = a[i-1][j] + k*a[i-1][i-j];	/* Equation 38c, Makhoul */

    e *= (1-k*k);				/* Equation 38d, Makhoul */
  }

  for(i=1; i <= order; i++)
    lpcs[i] = a[order][i];
  lpcs[0] = 1.0f;  
}

/*---------------------------------------------------------------------------*\

  inverse_filter()

  Inverse Filter, A(z).  Produces an array of residual samples from an array
  of input samples and linear prediction coefficients.

  The filter memory is stored in the first order samples of the input array.

\*---------------------------------------------------------------------------*/

void inverse_filter(
  float Sn[],	/* Nsam input samples */
  float a[],	/* LPCs for this frame of samples */
  int Nsam,	/* number of samples */
  float res[],	/* Nsam residual samples */
  int order	/* order of LPC */
)
{
  int i,j;	/* loop variables */

  for(i=0; i<Nsam; i++) {
    res[i] = 0.0;
    for(j=0; j<=order; j++)
      res[i] += Sn[i-j]*a[j];
  }
}


