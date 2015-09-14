/*---------------------------------------------------------------------------*\
                                                                             
  FILE........: sine.h
  AUTHOR......: David Rowe                                                          
  DATE CREATED: 1/11/94
                                                                             
  Header file for sinusoidal analysis and synthesis functions.
                                                                             
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

#ifndef __SINE__
#define __SINE__

#include "defines.h"
#include "comp.h"

void make_analysis_window(float w[], COMP W[]);
void dft_speech(COMP Sw[], float Sn[], float w[]);
void two_stage_pitch_refinement(MODEL *model, COMP Sw[]);
void estimate_amplitudes(MODEL *model, COMP Sw[], COMP W[]);
float est_voicing_mbe(MODEL *model, COMP Sw[], COMP W[]);
void make_synthesis_window(float Pn[]);
void synthesise(float Sn_[], MODEL *model, float Pn[]);
float fast_atan2f(float y, float x);
float fastest_atan2f(float y, float x);

#define CODEC2_RAND_MAX 32767
int codec2_rand(void);

#endif