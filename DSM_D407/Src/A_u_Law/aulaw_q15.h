#ifndef __AULAW_Q15_H__
#define __AULAW_Q15_H__
#include <stdint.h>

#ifdef __cplusplus
#define  EXTDECL extern "C"
#else
#define	 EXTDECL extern
#endif

EXTDECL uint32_t alaw_mem_req_q15(void);
EXTDECL void alaw_init_q15(void *state);
EXTDECL uint8_t *alaw_encode_q15(void *state, uint8_t *pBits, int16_t *pSamples, int nSamples);
EXTDECL uint8_t *alaw_decode_q15(void *state, int16_t *pSamples, uint8_t *pBits, int nBits);


EXTDECL uint32_t ulaw_mem_req_q15(void);
EXTDECL void ulaw_init_q15(void *state);
EXTDECL uint8_t *ulaw_encode_q15(void *state, uint8_t *pBits, int16_t *pSamples, int nSamples);
EXTDECL uint8_t *ulaw_decode_q15(void *state, int16_t *pSamples, uint8_t *pBits, int nBits);
#endif	// __AULAW_Q15_H__

