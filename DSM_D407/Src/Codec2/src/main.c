/*---------------------------------------------------------------------------*\

  FILE........: c2demo.c
  AUTHOR......: David Rowe
  DATE CREATED: 15/11/2010

  Encodes and decodes a file of raw speech samples using Codec 2.
  Demonstrates use of Codec 2 function API.

  Note to convert a wave file to raw and vice-versa:

    $ sox file.wav -r 8000 -s -2 file.raw
    $ sox -r 8000 -s -2 file.raw file.wav

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2010 David Rowe

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

#ifndef _MSC_VER
#include "stm32f4_discovery.h"
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#include "cmsis_os.h"
static inline void exit(int a){ do{}while(a);}
#endif

#include "arm_math.h"
#include "arm_const_structs.h"


#include "codec2.h"
#include "sine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mat.h"
#include "dataqueues.h"


int main_codec2(int argc, char *argv[])
{
    struct CODEC2 *codec2;
    FILE          *fin;
    FILE          *fout;
    int16_t       *int_buf;
    float         *buf;
    unsigned char *bits;
    int            nsam, nbit, i, r;
	int			   mem_req;

    for(i=0; i<10; i++) {
        r = codec2_rand();
        printf("[%d] r = %d\n", i, r);
    }

    if (argc != 3) {
		printf("usage: %s InputRawSpeechFile OutputRawSpeechFile\n", argv[0]);
		exit(1);
    }

    if ( (fin = fopen(argv[1],"rb")) == NULL ) {
		fprintf(stderr, "Error opening input speech file: %s: %s.\n",
			 argv[1], strerror(errno));
		exit(1);
    }

    if ( (fout = fopen(argv[2],"wb")) == NULL ) {
		fprintf(stderr, "Error opening output speech file: %s: %s.\n",
			 argv[2], strerror(errno));
		exit(1);
    }

    /* Note only one set of Codec 2 states is required for an encoder
       and decoder pair. */
    mem_req = codec2_state_memory_req();
	codec2 = (struct CODEC2 *) malloc(mem_req);
	codec2_init(codec2, CODEC2_MODE_2400);
    nsam = codec2_samples_per_frame(codec2);
    int_buf = (int16_t *) malloc(nsam*sizeof(int16_t));
    buf = (float *) malloc(nsam*sizeof(float));
    nbit = codec2_bits_per_frame(codec2);
    bits = (unsigned char*)malloc(nbit*sizeof(char));

    while(fread(int_buf, sizeof(short), nsam, fin) == (size_t)nsam) {
		arm_q15_to_float(int_buf, buf, nsam);
		codec2_encode(codec2, bits, buf);
		codec2_decode(codec2, buf, bits);
		arm_float_to_q15(buf, int_buf, nsam);
		fwrite(buf, sizeof(short), nsam, fout);
    }

    free(buf);
    free(int_buf);
    free(bits);
    codec2_close(codec2);

    fclose(fin);
    fclose(fout);

    return 0;
}


static struct CODEC2 *p_codec;
static int  frame_size;

#define 	CODEC2_BUFF_SIZE (320)

static float    speech[CODEC2_BUFF_SIZE] CCMRAM;
static unsigned char bits[64] CCMRAM;

#ifndef _MSC_VER

void *codec2_create(uint32_t Params)
{
	int mem_req = codec2_state_memory_req();
	p_codec = osAlloc(mem_req);
	return p_codec;
}

void codec2_deinit(void *pHandle)
{
	codec2_close(p_codec);
	return;
}

void codec2_initialize(void *pHandle)
{
	codec2_init(p_codec, CODEC2_MODE_2400);
	frame_size = codec2_samples_per_frame(p_codec); 
}

uint32_t codec2_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t *pInBytes)
{
	uint32_t	nGenerated = 0;
	while(*pInBytes >= frame_size)
	{
BSP_LED_On(LED4);
		arm_scale_f32(pDataIn, 32767.0f, speech, frame_size);
		codec2_encode(p_codec, bits, speech);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		codec2_decode(p_codec, speech, bits);
		arm_scale_f32(speech, 1.0f/32768.0f, pDataOut, frame_size);
BSP_LED_Off(LED5);
//		v_equ(pDataOut, pDataIn, frame_size);
		pDataIn += frame_size * 4;
		pDataOut += frame_size * 4;
		*pInBytes -= frame_size;
		nGenerated += frame_size;		
	}
	return nGenerated;
}

uint32_t codec2_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return frame_size;
}

DataProcessBlock_t  CODEC = {codec2_create, codec2_initialize, codec2_data_typesize, codec2_process, codec2_deinit};


#endif
