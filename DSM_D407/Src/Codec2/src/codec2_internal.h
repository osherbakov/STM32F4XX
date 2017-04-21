/*---------------------------------------------------------------------------*\
                                                                             
  FILE........: codec2_internal.h
  AUTHOR......: David Rowe                                                          
  DATE CREATED: April 16 2012
                                                                             
  Header file for Codec2 internal states, exposed via this header
  file to assist in testing.
                                                                             
\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2012 David Rowe

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

#ifndef __CODEC2_INTERNAL__
#define __CODEC2_INTERNAL__
#include "mat.h"

// #pragma anon_unions

#define PMAX_M      600		/* maximum NLP analysis window size     */
#define DEC         5		/* decimation factor                    */
#define NLP_NTAP	48	    /* Decimation LPF order */

typedef struct NLP{
    int           m;
    float         *w;				 /* DFT window                   */ 
    float         sq[PMAX_M];	     /* squared speech samples       */
    float         mem_x,mem_y;       /* memory for notch filter      */
    float         mem_fir[NLP_NTAP + FRAME_SIZE - 1]; /* decimation FIR filter memory */
    arm_fir_instance_f32 arm_fir;  /* fir from arm */
} NLP;

typedef struct CODEC2 {
    int           mode;					   /* Current bit-rate						    */

    /* Pre-calculated constant tables/windows   */
    float         *Pn;						/* trapezoidal synthesis window              */
    float         *w;						/* time domain hamming window                */
    COMP          *W;						/* DFT of w[]                                */
	
	/* Analysis parameters and states      */
	float         Sn[P_SIZE];              /* analized input speech                     */
    NLP           nlp;					   /* pitch predictor states                    */
    float         bg_est;                  /* background noise estimate for post filter */
	
	/* Synthesis parameters and states     */
    float         Sn_[2*FRAME_SIZE];	   /* synthesised output speech                 */
    float         ex_phase;                /* excitation model phase track              */

	/* Saved parameters from previous frame	*/
    float         prev_Wo_enc;             /* previous frame's pitch estimate           */
    MODEL         prev_model_dec;          /* previous frame's model parameters         */
    float         prev_lsps_dec[LPC_ORD];  /* previous frame's LSPs                     */
    float         prev_e_dec;              /* previous frame's LPC energy               */
    
    int           gray;                    /* non-zero for gray encoding                */
    int           lpc_pf;                  /* LPC post filter on                        */
    int           bass_boost;              /* LPC post filter bass boost                */
    float         beta;                    /* LPC post filter parameter beta            */
    float         gamma;				   /* LPC post filter parameter beta            */

    float         xq_enc[2];               /* joint pitch and energy VQ states          */
    float         xq_dec[2];

	MODEL		  *models;
} CODEC2_t;

typedef struct {
    /* Pre-calculated Codec2 constant tables/windows   */
    float         Syn_Pn[2*FRAME_SIZE];		/* trapezoidal synthesis window              */
    float         Ana_w[P_SIZE];			/* time domain hamming window                */
    COMP          Ana_W[FFT_ENC];			/* DFT of w[]                                */
    float         Nlp_w[PMAX_M/DEC];		/* DFT window                   */ 
} ConstBuf_t;

typedef struct {
	union {
		COMP  Ww[FFT_ENC];  /* weighting spectrum           */
		float Wn[P_SIZE];
	};
	union {
		COMP  Pw[FFT_ENC];	/* output power spectrum */
		COMP  Sw[FFT_ENC];
	};
	union {
		COMP  Aw[FFT_ENC];
		COMP  Fw[FFT_PE];
		COMP  sw_[FFT_DEC];	/* synthesised signal */
	};
	union {
		MODEL models[4];
	};
} TempBuf_t;

extern TempBuf_t    tmp;
extern ConstBuf_t	tbl;

#endif
