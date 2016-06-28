/*

2.4 kbps MELP Proposed Federal Standard speech coder

version 1.2

Copyright (c) 1996, Texas Instruments, Inc.  

Texas Instruments has intellectual property rights on the MELP
algorithm.  The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).


*/

/*
Name: melp_chn_write, melp_chn_read
Description: Write/read MELP channel bitstream
Inputs:
MELP parameter structure
Outputs: 
updated MELP parameter structure (channel pointers)
Returns: void
*/

#include <math.h>
#include "mat.h"

#include <string.h>
#include "melp.h"
#include "vq.h"
#include "melp_sub.h"
#include "dsp_sub.h"

#define UV_PIND 0    /* Unvoiced pitch index */
#define INVAL_PIND 1 /* Invalid pitch index  */

static int sync_bit = 0; /* sync bit */

void melp_chn_write(struct melp_param *par, unsigned char chbuf[])
{
	int i, bit_cntr;
	unsigned char *bit_ptr; 

	/* FEC: code additional information in redundant indices */
    if (par->uv_flag)
    {
		/* Set pitch index to unvoiced value */
		par->pitch_index = UV_PIND;
	}
//	fec_code(par);

	/*	Fill bit buffer	*/
	bit_ptr = chbuf;
	bit_cntr = 0;

	pack_code(par->gain_index[1],&bit_ptr,&bit_cntr,5,1);

	/* Toggle and write sync bit */
	sync_bit ^= 1;
	pack_code(sync_bit,&bit_ptr,&bit_cntr,1,1);
	pack_code(par->gain_index[0],&bit_ptr,&bit_cntr,3,1);
	pack_code(par->pitch_index,&bit_ptr,&bit_cntr,PIT_BITS,1);
	pack_code(par->jit_index,&bit_ptr,&bit_cntr,1,1);
	pack_code(par->bpvc_index,&bit_ptr,&bit_cntr,NUM_BANDS-1,1);

	for (i = 0; i < par->msvq_par.num_stages; i++) { 
		pack_code(par->msvq_par.indices[i],&bit_ptr,&bit_cntr,par->msvq_par.bits[i],1);
	}
	pack_code(par->fsvq_par.indices[0],&bit_ptr,&bit_cntr, FS_BITS,1);
}

int melp_chn_read(struct melp_param *par, unsigned char chbuf[])
{
	int erase = 0;
	int i, bit_cntr;
	unsigned char *bit_ptr; 

	/*	Read information from  bit buffer	*/
	bit_ptr = chbuf;
	bit_cntr = 0;

	unpack_code(&bit_ptr,&bit_cntr,&par->gain_index[1],5,1);

	/* Read sync bit */
	unpack_code(&bit_ptr,&bit_cntr,&i,1,1);
	unpack_code(&bit_ptr,&bit_cntr,&par->gain_index[0],3,1);
	unpack_code(&bit_ptr,&bit_cntr,&par->pitch_index,PIT_BITS,1);

	unpack_code(&bit_ptr,&bit_cntr,&par->jit_index,1,1);
	unpack_code(&bit_ptr,&bit_cntr,&par->bpvc_index, NUM_BANDS-1,1);

	for (i = 0; i < par->msvq_par.num_stages; i++) {
		unpack_code(&bit_ptr,&bit_cntr,&par->msvq_par.indices[i],par->msvq_par.bits[i],1);
	}
	unpack_code(&bit_ptr,&bit_cntr,&par->fsvq_par.indices[0], FS_BITS,1);

	/* Clear unvoiced flag */
	par->uv_flag = (par->pitch_index == UV_PIND) ? 1 : 0;
	erase = 0; //fec_decode(par,erase);
	/* Return erase flag */
	return(erase);
}
