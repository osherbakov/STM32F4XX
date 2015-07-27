#ifndef __CVSD_F32_H__
#define __CVSD_F32_H__
#include <stdint.h>

#ifdef __cplusplus
#define  EXTDECL extern "C"
#else
#define	 EXTDECL extern
#endif

EXTDECL uint32_t cvsd_mem_req_f32(void);
EXTDECL void cvsd_init_f32(void *state);
EXTDECL void cvsd_encode_f32(void *state, uint8_t *pBits, float *pSamples, int nSamples);
EXTDECL void cvsd_decode_f32(void *state, float *pSamples, uint8_t *pBits, int nBits);

#endif	// __CVSD_F32_H__

