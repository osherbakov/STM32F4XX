#include "cvsd_f32.h"
#include "cvsd_data_f32.h"


EXTDECL uint32_t cvsd_mem_req_f32()
{
	return sizeof(CVSD_STATE_F32_t);
}

EXTDECL void cvsd_init_f32(void *state)
{
	CVSD_STATE_F32_t *p_state = (CVSD_STATE_F32_t *) state;
	p_state->ShiftRegister = 0;
	p_state->bitcount = 0;
	p_state->V_integrator = 0;
	p_state->V_syllabic = SYLLABIC_MIN;
}

EXTDECL uint8_t *cvsd_encode_f32(void *state, uint8_t *pBits, float *pSamples, int nSamples)
{
	uint32_t		sample_bigger_than_ref;
	float			sample;
	float			V_syllabic;
	float			V_integrator;
	float			V;
	uint32_t		SR;
	int				bitcount;
	uint8_t			databyte;

	CVSD_STATE_F32_t *p_state = (CVSD_STATE_F32_t *) state;

	V_syllabic = p_state->V_syllabic;
	V_integrator = p_state->V_integrator;
	SR = p_state->ShiftRegister;
	bitcount = p_state->bitcount;
	databyte = *pBits;
	
	if(bitcount) databyte >>= (8 - bitcount);
	
	while(nSamples--)
	{
		sample = *pSamples++;
		// Calculate the comparator output
		sample_bigger_than_ref = (sample > V_integrator) ? 0x01 : 0x00;

		// Add the comparator bit into the shift register
		SR = ((SR << 1) | sample_bigger_than_ref) & SR_MASK;
		// Add bit into databyte
		databyte = (databyte << 1) | sample_bigger_than_ref;

		bitcount += 1;
		if(bitcount >= 8)
		{
			*pBits++ = databyte;
			bitcount = 0;
			databyte = 0;
		}

		//  PROCESS SYLLABIC BLOCK
		// Apply overflow detector - all ones or all zeros
		V = ((SR == 0) || (SR == SR_MASK)) ? SYLLABIC_MAX : SYLLABIC_MIN;
		_MAC(V_syllabic, V - V_syllabic,  SYLLABIC_STEP);

		// PROCESS INTEGRATOR BLOCK
		V = sample_bigger_than_ref ? V_syllabic : -V_syllabic;
		_MAC(V_integrator, V - V_integrator, INTEGRATOR_STEP);
		// Add leakage...
		// V_integrator -= _MUL(V_integrator, INTEGRATOR_LEAK);
		_MAC(V_integrator, -V_integrator, INTEGRATOR_LEAK);
	}
	if(bitcount > 0)
	{
		databyte <<= (8 - bitcount);
		*pBits = databyte;
	}

	p_state->ShiftRegister = SR;
	p_state->bitcount = bitcount;
	p_state->V_syllabic = V_syllabic;
	p_state->V_integrator = V_integrator;
	return pBits;
}

EXTDECL uint8_t *cvsd_decode_f32(void *state, float *pSamples, uint8_t *pBits, int nBits)
{
	uint32_t		sample_bigger_than_ref;
	float			V_syllabic;
	float			V_integrator;
	float			V_s;
	uint32_t		SR;
	int				bitcount; 
	uint8_t			databyte;
	float			sample;

	CVSD_STATE_F32_t *p_state = (CVSD_STATE_F32_t *) state;

	// Get data from the state and put them into work variables
	V_syllabic = p_state->V_syllabic;
	V_integrator = p_state->V_integrator;
	SR = p_state->ShiftRegister;
	bitcount = p_state->bitcount;
	databyte = *pBits++;
	databyte <<= bitcount;
	
	while(nBits--)
	{
		// Extract the comparator output bit
		sample_bigger_than_ref = (databyte & 0x80) ? 0x01 : 0x00;
		databyte <<= 1;

		// Add the bit into the shift register
		SR = ((SR << 1) | sample_bigger_than_ref) & SR_MASK ;

		//  PROCESS SYLLABIC BLOCK
		// Apply overflow detector - all ones or all zeros
		V_s = (SR == 0) || (SR == SR_MASK) ? SYLLABIC_MAX : SYLLABIC_MIN;
		_MAC(V_syllabic, V_s - V_syllabic,  SYLLABIC_STEP);

		// PROCESS INTEGRATOR BLOCK
		V_s = sample_bigger_than_ref ? V_syllabic : -V_syllabic;
		_MAC(V_integrator, V_s - V_integrator, INTEGRATOR_STEP);
		// Add leakage.. 
		// V_integrator -= _MUL(V_integrator, INTEGRATOR_LEAK);
		_MAC(V_integrator, -V_integrator, INTEGRATOR_LEAK);

		sample = MAX(V_integrator, SAMPLE_MIN_VALUE);
		sample = MIN(sample, SAMPLE_MAX_VALUE);
		*pSamples++ = sample;

		bitcount += 1;
		if(bitcount >= 8)
		{
			databyte = *pBits++;
			bitcount = 0;
		}
	}

	// Save the working vars in state
	p_state->ShiftRegister = SR;
	p_state->bitcount = bitcount;
	p_state->V_syllabic = V_syllabic;
	p_state->V_integrator = V_integrator;
	if(bitcount) pBits -= 1;	

	return	pBits;
}

