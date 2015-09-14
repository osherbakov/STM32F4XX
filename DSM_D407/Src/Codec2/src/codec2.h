/*---------------------------------------------------------------------------*\

  FILE........: codec2.h
  AUTHOR......: David Rowe
  DATE CREATED: 21 August 2010

  Codec 2 fully quantised encoder and decoder functions.  If you want use 
  Codec 2, these are the functions you need to call.

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



#ifndef __CODEC2__
#define  __CODEC2__

typedef enum {
CODEC2_MODE_3200 = 3200,
CODEC2_MODE_2400 = 2400,
CODEC2_MODE_1600 = 1600,
CODEC2_MODE_1400 = 1400,
CODEC2_MODE_1300 = 1300,
CODEC2_MODE_1200 = 1200
} CODEC2_MODE_t;

struct CODEC2;

#ifdef __cplusplus
extern "C" {
#endif
void codec2_init(struct CODEC2 *codec2_state, int mode);
void codec2_close(struct CODEC2 *codec2_state);

void codec2_encode(struct CODEC2 *codec2_state, unsigned char * bits, float speech_in[]);
void codec2_decode(struct CODEC2 *codec2_state, float speech_out[], const unsigned char *bits);

int  codec2_state_memory_req(void);
int  codec2_samples_per_frame(struct CODEC2 *codec2_state);
int  codec2_bits_per_frame(struct CODEC2 *codec2_state);

int  codec2_get_spare_bit_index(struct CODEC2 *codec2_state);
int  codec2_rebuild_spare_bit(struct CODEC2 *codec2_state, int unpacked_bits[]);
#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#define CCMRAM
#define RODATA
#else
#define CCMRAM __attribute__((section (".ccmram")))
#define RODATA __attribute__((section (".rodata")))
#endif

#endif



