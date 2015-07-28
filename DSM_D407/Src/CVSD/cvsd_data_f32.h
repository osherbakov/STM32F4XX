/********************************************************************
	created:	2014/11/12
	created:	12:11:2014   8:59
	filename: 	c:\Projects\CVSD\CVSD_f32\cvsd_data_f32.h
	file path:	c:\Projects\CVSD\CVSD_f32
	file base:	cvsd_data_f32
	file ext:	h
	author:		Oleg Sherbakov
	
	purpose:	
*********************************************************************/

#ifndef __CVSD_DATA_F32_H__
#define __CVSD_DATA_F32_H__
#include <stdint.h>


#ifndef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define _MUL(a,b)				((a) * (b))
#define _DIV(a,b)				((a) / (b))
#define _MAC(a,b,c)				((a) += _MUL((b), (c)))

#define BITRATE					(16000)
#define BITRATE_KB				(BITRATE / 1000.0f)

#define ONE						(1.0f)

#define SR_NUM_BITS				(3)
#define SR_MASK					((1<<SR_NUM_BITS)-1)

#define SAMPLE_NUM_BITS			(16)
#define SAMPLE_MAX_VALUE		(32767.0f)
#define SAMPLE_MIN_VALUE		(-32768.0f)

#define RC_SYLLABIC_STEP_MS		(5)
#define RC_SYLLABIC_LEAK_MS		(5)
#define RC_INTEGRATOR_STEP_MS	(1)
#define RC_INTEGRATOR_LEAK_MS	(1)

#define SYLLABIC_STEP			(ONE / (BITRATE_KB * RC_SYLLABIC_STEP_MS))
#define SYLLABIC_LEAK			(ONE / (BITRATE_KB * RC_SYLLABIC_LEAK_MS))
#define INTEGRATOR_STEP			(ONE / (BITRATE_KB * RC_INTEGRATOR_STEP_MS))
#define INTEGRATOR_LEAK			(ONE / (BITRATE_KB * RC_INTEGRATOR_LEAK_MS))

#define SYLLABIC_MAX_RATIO		(9)
#define SYLLABIC_MIN_RATIO		(16)
#define SYLLABIC_MAX			(SAMPLE_MAX_VALUE * SYLLABIC_MAX_RATIO)
#define SYLLABIC_MIN			(SAMPLE_MAX_VALUE / SYLLABIC_MIN_RATIO)


typedef struct CVSD_STATE_F32{
	uint32_t	ShiftRegister;
	float		V_syllabic;
	float		V_integrator;
	uint32_t	bitcount;
}CVSD_STATE_F32_t;

#endif	// __CVSD_DATA_F32_H__

