#ifndef _MSC_VER
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#endif

#include "aulaw_q15.h"
#include "aulaw_data_q15.h"
#include <arm_math.h> 

#define ALAW_MAX 0xFFF
#define MULAW_MAX 0x1FFF
#define MULAW_BIAS  33

EXTDECL uint32_t aulaw_mem_req_q15()
{
	return sizeof(AU_LAW_STATE_Q15_t);
}

EXTDECL void aulaw_init_q15(void *state)
{
}
   

 
int8_t ALaw_Encode(int16_t number)
{
   uint8_t sign = 0;
   uint8_t lsb;
   uint8_t msb;
   if (number < 0){
      number = -number;
      sign = 0x80;
   }
   if (number > ALAW_MAX){
      number = ALAW_MAX;
   }

   msb = ( number <= (1<<4) ) ? 0 : (27 - __CLZ(number));
   lsb = (number >> ((msb == 0) ? (1) : msb)) & 0x0f;
   return (sign | ( msb << 4) | lsb) ^ 0x55;
}

/*
 * Description:
 *  Decodes an 8-bit unsigned integer using the A-Law.
 * Parameters:
 *  number - the number who will be decoded
 * Returns:
 *  The decoded number
 */
int16_t ALaw_Decode(int8_t number)
{
   uint8_t sign = 0;
   uint8_t position = 0;
   int16_t decoded = 0;
   number^=0x55;
   if(number & 0x80)
   {
      number &= ~(1<<7);
      sign = 0x80;
   }
   position = (number & 0xF0) >> 4;
   if(position != 0){
      decoded = ( 0x21 << (position - 1)) | ( ( number & 0x0F ) << position);
   }else{
      decoded = (number<<1) | 1;
   }
   return (sign==0)?(decoded):(-decoded);
}

int8_t MuLaw_Encode(int16_t number)
{
   uint8_t sign = 0;
   uint8_t lsb;
   uint8_t msb;

   if (number < 0) {
      number = -number;
      sign = 0x80;
   }
   number += MULAW_BIAS;
   if (number > MULAW_MAX){
      number = MULAW_MAX;
   }
   msb = (number <= (1<<4)) ? 0 : (27 - __CLZ(number)); 
   lsb = (number >> msb) & 0x0f;
   return (~(sign | ((msb - 1) << 4) | lsb));
}


int16_t MuLaw_Decode(int8_t number)
{
   uint8_t sign = 0;
   uint8_t position;
   int16_t decoded;
   
   number = ~number;
   if (number & 0x80)
   {
      number &= ~(1 << 7);
      sign = 0x80;
   }
   position = ((number & 0xF0) >> 4);
   decoded = (( 0x21 << position ) | ((number & 0x0F) << (position + 1))) - MULAW_BIAS;			 
   return (sign == 0) ? (decoded) : (-(decoded));
}


EXTDECL uint8_t *alaw_encode_q15(void *state, uint8_t *pBits, int16_t *pSamples, int nSamples)
{	
	while(nSamples--)
	{
		*pBits++ = ALaw_Encode((*pSamples++) >> 3);
	}
	return pBits;
}

EXTDECL uint8_t *alaw_decode_q15(void *state, int16_t *pSamples, uint8_t *pBits, int nBits)
{	
	while(nBits--)
	{
		*pSamples++ = ALaw_Decode(*pBits++) << 3;
	}
	return	pBits;
}


EXTDECL uint8_t *ulaw_encode_q15(void *state, uint8_t *pBits, int16_t *pSamples, int nSamples)
{	
	while(nSamples--)
	{
		*pBits++ = MuLaw_Encode((*pSamples++) >> 2);
	}
	return pBits;
}

EXTDECL uint8_t *ulaw_decode_q15(void *state, int16_t *pSamples, uint8_t *pBits, int nBits)
{	
	while(nBits--)
	{
		*pSamples++ = MuLaw_Decode(*pBits++) << 2;
	}
	return	pBits;
}
