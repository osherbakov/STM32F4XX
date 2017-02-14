#ifndef __CVSD_F32_H__
#define __CVSD_F32_H__
#include <stdint.h>

#ifdef __cplusplus
#define  EXTDECL extern "C"
#else
#define	 EXTDECL extern
#endif

#ifdef _MSC_VER
#define CCMRAM
#define RODATA
#else
#define CCMRAM __attribute__((section (".ccmram")))
#define RODATA __attribute__((section (".rodata")))
#endif

EXTDECL uint32_t cvsd_mem_req_f32(void);
EXTDECL void cvsd_init_f32(void *state);
EXTDECL uint8_t *cvsd_encode_f32(void *state, uint8_t *pBits, float *pSamples, int nSamples);
EXTDECL uint8_t *cvsd_decode_f32(void *state, float *pSamples, uint8_t *pBits, int nBits);

#endif	// __CVSD_F32_H__

