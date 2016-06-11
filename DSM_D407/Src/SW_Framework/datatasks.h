#ifndef		__TASKS_H__
#define		__TASKS_H__

#include "cmsis_os.h"
#include "dataqueues.h"

// Parameters for the input audio that comes from MIC, I2S and USB
#define  SAMPLE_FREQ			(48000)
#define  SAMPLE_FREQ_KHZ		((SAMPLE_FREQ)/1000)
#define  PDM_DOWNSAMPLE_RATIO	(64)
#define  NUM_CHANNELS			(2)

// PDM Microphone on STM32F4-DISCOVERY
#define	 NUM_PDM_BYTES		((SAMPLE_FREQ_KHZ	* PDM_DOWNSAMPLE_RATIO) / 8)
#define	 NUM_PCM_SAMPLES	(SAMPLE_FREQ_KHZ)
#define	 NUM_PCM_BYTES  	(NUM_PCM_SAMPLES * NUM_CHANNELS * AUDIODATA_SIZE)


#define MAX_AUDIO_SAMPLES		(180 * 3)		// Maximum Size of linear buffer in Samples

typedef enum
{
	AUDIO_MODE_IN_MIC = 0,			// Source of the audio is I2S PDM microphone
	AUDIO_MODE_IN_USB = 1,			// Source of the audio is USB
	AUDIO_MODE_IN_I2SX = 2,			// Source of the audio is I2S #2 (External)
} AUDIO_ModeInTypeDef;

#define AUDIO_MONO_Q15_SIZE	sizeof(int16_t)
#define AUDIO_STEREO_Q15_SIZE	(2 * sizeof(int16_t))

// Maximum linear buffer size that can be allocated in BYTES!
#define MAX_AUDIO_SIZE_BYTES	(MAX_AUDIO_SAMPLES * AUDIO_STEREO_Q15_SIZE)
	
void StartDefaultTask(void const * argument);
void StartDataInPDMTask(void const * argument);
void StartDataProcessTask(void const * argument);

typedef enum PROC_STATE {
	STOPPED = 0,
	WAITING_FOR_BUFF = 1,
	RUNNING = 2
} PROC_STATE_t;

typedef struct 
{
		AUDIO_ModeInTypeDef  audioInMode;	// Global Audio IN mode  - from USB, I2S, or PDM/I2S Microphone
		int bStartPlay;						// Sync the start of playing with the next block of PDM data
		PROC_STATE_t ProcessingState;		// State of processing: 0 - stopped, 1 - waiting for 1/2 buffers, 2 - normal

		osMessageQId dataInReadyMsg;		// Message queue to indicate that any input data for processing and routing is ready
	
		uint8_t		*pPDM_In;				// Pointer to DMA Buffer for PDM In
		uint8_t		*pPCM_Out;				// Pointer to DMA Buffer for PCM Out
	
		DQueue_t	*PCM_InQ;				// Data Queue for PCM In (Periph -> CPU) (Converted PDM)  data
		DQueue_t	*PCM_OutQ;				// Data Queue for PCM OUT (CPU -> Periph) CODEC data

		DQueue_t	*DownSampleQ;			// Data Queue for Downsampled  data
		DQueue_t	*UpSampleQ;				// Data Queue for Upsampled data
	
		DQueue_t	*USB_OutQ;				// Data Queue for USB OUT (Host -> Device) SPEAKER data 
		DQueue_t	*USB_InQ;				// Data Queue for USB IN (Device -> Host) MIC data 
	
}osObjects_t;

extern osObjects_t osParams;

#endif // __TASKS_H__
