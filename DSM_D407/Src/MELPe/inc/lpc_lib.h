/*

2.4 kbps MELP Proposed Federal Standard speech coder

Fixed-point C code, version 1.0

Copyright (c) 1998, Texas Instruments, Inc.

Texas Instruments has intellectual property rights on the MELP
algorithm.	The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).

The fixed-point version of the voice codec Mixed Excitation Linear
Prediction (MELP) is based on specifications on the C-language software
simulation contained in GSM 06.06 which is protected by copyright and
is the property of the European Telecommunications Standards Institute
(ETSI). This standard is available from the ETSI publication office
tel. +33 (0)4 92 94 42 58. ETSI has granted a license to United States
Department of Defense to use the C-language software simulation contained
in GSM 06.06 for the purposes of the development of a fixed-point
version of the voice codec Mixed Excitation Linear Prediction (MELP).
Requests for authorization to make other use of the GSM 06.06 or
otherwise distribute or modify them need to be addressed to the ETSI
Secretariat fax: +33 493 65 47 16.

*/

/* ========================= */
/* lpc.h   LPC include file. */
/* ========================= */

#ifndef _LPC_LIB_H_
#define _LPC_LIB_H_

void	lpc_autocorr_q(int16_t input[], const int16_t win_cof[], int16_t r[],
				 int16_t hf_correction, int16_t order, int16_t npts);

int32_t	lpc_aejw_q(int16_t lpc[], int16_t omega, int16_t order);

int16_t	lpc_bw_expand_q(int16_t lpc[], int16_t aw[], int16_t gamma,
					 int16_t order);

int16_t	lpc_clamp_q(int16_t lsp[], int16_t delta, int16_t order);

int16_t	lpc_schur_q(int16_t autocorr[], int16_t lpc[], int16_t order);

int16_t	lpc_pred2lsp_q(int16_t lpc[], int16_t lsf[], int16_t order);

int16_t	lpc_pred2refl_q(int16_t lpc[], int16_t *refc, int16_t order);

int16_t	lpc_lsp2pred_q(int16_t lsf[], int16_t lpc[], int16_t order);

int16_t	lpc_synthesis_q(int16_t x[], int16_t y[], int16_t a[],
					int16_t order, int16_t length);


#endif

