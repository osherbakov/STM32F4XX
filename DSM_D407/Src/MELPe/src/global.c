/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

/* =============================================== */
/* global.c: global variables for the sc1200 coder */
/* =============================================== */

#include "sc1200.h"

/* ====== General parameters ====== */
struct melp_param	melp_parameters[NF] CCMRAM;          /* melp analysis parameters */
unsigned char		chbuf[NUM_CH_BITS] CCMRAM;           /* channel bit data buffer */

/* ====== Quantization ====== */
const int16_t		msvq_bits[MSVQ_STAGES] RODATA = {7, 6, 6, 6};
const int16_t		msvq_levels[MSVQ_STAGES] RODATA = {128, 64, 64, 64};
struct quant_param	quant_par;

/* ====== Buffers ====== */
int16_t	hpspeech[IN_BEG + BLOCK] CCMRAM;       /* input speech buffer dc removed */
int16_t	dcdel[DC_ORD] CCMRAM;
int16_t	dcdelin[DC_ORD] CCMRAM;
int16_t	dcdelout_hi[DC_ORD] CCMRAM;
int16_t	dcdelout_lo[DC_ORD] CCMRAM;

/* ====== Classifier ====== */
int16_t	voicedEn, silenceEn;                                       /* Q11 */
int32_t	voicedCnt;

/* ====== Fourier Harmonics Weights ====== */
int16_t	w_fs[NUM_HARM] CCMRAM;                                            /* Q14 */
int16_t	w_fs_inv[NUM_HARM] CCMRAM;
BOOLEAN	w_fs_init = FALSE ;

