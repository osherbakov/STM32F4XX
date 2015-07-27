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
	p_state->V_integrator = 0;
	p_state->V_syllabic = SYLLABIC_MIN;
}



EXTDECL void cvsd_decode_f32(void *state, float *pSamples, uint8_t *pBits, int nBits)
{
	uint32_t		sample_bigger_than_ref;
	float			V_syllabic;
	float			V_integrator;
	float			V_s;
	uint32_t		SR;
	int				bitcount; 
	uint8_t			databyte;
	float			result;

	CVSD_STATE_F32_t *p_state = (CVSD_STATE_F32_t *) state;

	// Get data from the state and put them into work variables
	V_syllabic = p_state->V_syllabic;
	V_integrator = p_state->V_integrator;
	SR = p_state->ShiftRegister;

	bitcount = 8;
	while(nBits--)
	{
		if(++bitcount >= 8)
		{
			databyte = *pBits++;
			bitcount = 0;
		}
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

		//Saturate the result
		result = MIN(V_integrator, SAMPLE_MAX_VALUE);
		result = MAX(result, SAMPLE_MIN_VALUE);
		*pSamples++ = (int16_t)result;
	}

	// Save the working vars in state
	p_state->ShiftRegister = SR;
	p_state->V_syllabic = V_syllabic;
	p_state->V_integrator = V_integrator;
}


void cvsd_encode_f32(void *state, uint8_t *pBits, float *pSamples, int nSamples)
{
	uint32_t		sample_bigger_than_ref;
	float			V_syllabic;
	float			V_integrator;
	float			V_s;
	uint32_t		SR;
	int				bitcount;
	uint8_t			databyte;

	CVSD_STATE_F32_t *p_state = (CVSD_STATE_F32_t *) state;

	V_syllabic = p_state->V_syllabic;
	V_integrator = p_state->V_integrator;
	SR = p_state->ShiftRegister;

	bitcount = 0;
	databyte = 0;

	while(nSamples--)
	{
		// Calculate the comparator output
		sample_bigger_than_ref = (*pSamples++) > V_integrator ? 0x01 : 0x00;

		// Add the comparator bit into the shift register
		SR = ((SR << 1) | sample_bigger_than_ref) & SR_MASK;
		// Add bit into databyte
		databyte = (databyte << 1) | sample_bigger_than_ref;
		if(++bitcount >= 8)
		{
			*pBits++ = databyte;
			bitcount = 0;
		}

		//  PROCESS SYLLABIC BLOCK
		// Apply overflow detector - all ones or all zeros
		V_s = (SR == 0) || (SR == SR_MASK) ? SYLLABIC_MAX : SYLLABIC_MIN;
		_MAC(V_syllabic, V_s - V_syllabic,  SYLLABIC_STEP);

		// PROCESS INTEGRATOR BLOCK
		V_s = sample_bigger_than_ref ? V_syllabic : -V_syllabic;
		_MAC(V_integrator, V_s - V_integrator, INTEGRATOR_STEP);
		// Add leakage...
		// V_integrator -= _MUL(V_integrator, INTEGRATOR_LEAK);

		//Limit the result
		// V_integrator = MIN(V_integrator, SAMPLE_MAX_VALUE);
		// V_integrator = MAX(V_integrator, SAMPLE_MIN_VALUE);
	}
	if(bitcount > 0)
	{
		*pBits = databyte;
	}

	p_state->ShiftRegister = SR;
	p_state->V_syllabic = V_syllabic;
	p_state->V_integrator = V_integrator;
}

