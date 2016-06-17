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
/* global.h: global variables for the sc1200 coder */
/* =============================================== */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#include "sc1200.h"

/* ====== General parameters ====== */
extern struct melp_param	melp_parameters[];           /* melp analysis parameters */

/* ====== Quantization ====== */
extern const int16_t	msvq_bits[];
extern const int16_t	msvq_levels[];
extern struct quant_param	quant_par;

/* ====== Buffers ====== */
extern int16_t	hpspeech[];             /* input speech buffer dc removed */
extern int16_t	lpres_delin[];
extern int16_t	lpres_delout[];


/* ====== Classifier ====== */
extern int16_t	voicedEn, silenceEn;
extern int32_t	voicedCnt;

/* ====== Fourier Harmonics Weights ====== */
extern int16_t	w_fs[];
extern int16_t	w_fs_inv[];
extern BOOLEAN	w_fs_init;

#endif

