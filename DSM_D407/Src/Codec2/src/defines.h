/*---------------------------------------------------------------------------*\

  FILE........: defines.h                                                     
  AUTHOR......: David Rowe 
  DATE CREATED: 23/4/93                                                       
                                                                             
  Defines and structures used throughout the codec.			     
                                                                             
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

#ifndef __DEFINES__
#define __DEFINES__

/*---------------------------------------------------------------------------*\
                                                                             
				DEFINES                                       
                                                                             
\*---------------------------------------------------------------------------*/

/* General defines */

#define FRAME_SIZE 80		/* number of samples per frame (10ms)    */
#define MAX_AMP    80		/* maximum number of harmonics          */

#define SAMPLE_RATE 8000	/* sample rate in Hz                    */
		
#define NW         279          /* analysis window size                 */
#define FFT_ENC    512		/* size of FFT used for encoder         */
#define FFT_DEC    512	    /* size of FFT used in decoder          */
#define TW         40		/* Trapezoidal synthesis window overlap */
#define V_THRESH   6.0f     /* voicing threshold in dB              */
#define LPC_ORD    10		/* LPC order                            */
#define LPC_ORD_LOW 6		/* LPC order for lower rates            */

/* Pitch estimation defines */
#define FFT_PE		512			/* DFT size for pitch estimation        */
#define P_SIZE		320			/* pitch analysis frame size            */
#define P_MIN		20			/* minimum pitch                        */
#define P_MAX		160			/* maximum pitch                        */

#define COEFF       0.95f	/* notch filter parameter               */

#define THRESH      0.1f    /* threshold for local minima candidate */
#define CNLP        0.3f	/* post processor constant              */

/*---------------------------------------------------------------------------*\
                                                                             
				TYPEDEFS                                      
                                                                             
\*---------------------------------------------------------------------------*/

/* Structure to hold model parameters for one frame */

typedef struct {
  float Wo;				/* fundamental frequency estimate in radians  */
  int   L;				/* number of harmonics                        */
  float A[MAX_AMP+1];	/* amplitiude of each harmonic                */
  float phi[MAX_AMP+1];	/* phase of each harmonic                     */
  int   voiced;	        /* non-zero if this frame is voiced           */
} MODEL;

/* describes each codebook  */

struct lsp_codebook {
    int			k;        /* dimension of vector	*/
    int			log2m;    /* number of bits in m	*/
    int			m;        /* elements in codebook	*/
    const float	*cb;	  /* The elements		*/
};

#endif
