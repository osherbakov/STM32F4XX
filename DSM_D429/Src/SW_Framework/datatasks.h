#ifndef		__TASKS_H__
#define		__TASKS_H__

#include "cmsis_os.h"

#define  SAMPLE_FREQ			(48000)
#define  SAMPLE_FREQ_KHZ	((SAMPLE_FREQ)/1000)
#define  DOWNSAMPLE_RATIO	(64)
#define  NUM_BUFFERS			(2)
#define  NUM_CHANNELS			(2)
#define  AUDIODATA_SIZE         (2)
#define	 NUM_PDM_BYTES		((SAMPLE_FREQ_KHZ	* DOWNSAMPLE_RATIO) / 8)
#define	 NUM_PCM_SAMPLES	(SAMPLE_FREQ_KHZ)

typedef enum
{
	AUDIO_MODE_MIC = 0,		// Source of the audio is microphone
	AUDIO_MODE_USB = 1,		// Source of the audio is USB
	AUDIO_MODE_I2S = 2		// Source of the audio is I2S bus
} AUDIO_ModeTypeDef;

// The input buffer for PDM data
// uint8_t PDM_Buffer[NUM_PDM_BYTES * NUM_BUFFERS];


extern uint8_t *pPDM_Buffer;
extern uint16_t *pPCM_Samples;

// The working bufer that will receive converted PCM samples
// uint16_t PCM_Buffer[NUM_PCM_SAMPLES];
// The output buffer that will be sent out to a stereo codec
// uint16_t PCM_Samples[NUM_PCM_SAMPLES * NUM_BUFFERS * NUM_CHANNELS];


void StartDefaultTask(void const * argument);
void StartDataInPDMTask(void const * argument);
void StartDataInUSBTask(void const * argument);
void StartDataInI2STask(void const * argument);
void StartDataOutI2STask(void const * argument);
void StartDataProcessTask(void const * argument);

extern AUDIO_ModeTypeDef  audioMode;		// Global Audio IN mode  - from USB, I2S, or Microphone
extern int buttonState;									// Global User button State
extern int bStartPDMPlay;						  	// Sync the start of playing with the next block of PDM data

extern osMessageQId dataInPDMMsg;
extern osMessageQId dataInUSBMsg;
extern osMessageQId dataInI2SMsg;
extern osMessageQId dataOutI2SMsg;

#endif // __TASKS_H__
