#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "datatasks.h"

#include <string.h>

typedef enum {
	CH_LEFT = 0,
	CH_RIGHT = 1,
	I_LEFT = 0,
	V_LEFT = 1,
	I_RIGHT = 2,
	V_RIGHT = 3,

	CH_MAX = 2,
	IV_MAX = 4
} Channels_t;

//
//  For test purposes the Fake FF Function just copies input data buffer to output buffer
//
void FF_Process(void *dsmHandle, float *pAudioIn[CH_MAX], float *pAudioOut[CH_MAX], uint32_t nSamples)
{
	memcpy(pAudioOut[CH_LEFT], pAudioIn[CH_LEFT], nSamples * sizeof(float));
	memcpy(pAudioOut[CH_RIGHT], pAudioIn[CH_RIGHT], nSamples * sizeof(float));
}

void FB_Process(void *dsmHandle, float *pIVIn[IV_MAX], uint32_t nSamples)
{
}

