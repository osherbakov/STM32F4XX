#ifndef		__TASKS_H__
#define		__TASKS_H__

#include "cmsis_os.h"
#include "dataqueues.h"

#define  SAMPLE_FREQ			(48000)
#define  SAMPLE_FREQ_KHZ	((SAMPLE_FREQ)/1000)
#define  PDM_DOWNSAMPLE_RATIO	(64)
#define  NUM_CHANNELS			(2)
#define	 NUM_PDM_BYTES		((SAMPLE_FREQ_KHZ	* PDM_DOWNSAMPLE_RATIO) / 8)
#define	 NUM_PCM_SAMPLES	(SAMPLE_FREQ_KHZ)
#define	 NUM_PCM_BYTES  	(NUM_PCM_SAMPLES * NUM_CHANNELS * AUDIODATA_SIZE)


#define AUDIO_BLOCK_SAMPLES		(180)		// Size in Samples
#define IV_BLOCK_SAMPLES			(180)		// Size in Samples

typedef enum
{
	AUDIO_MODE_IN_MIC = 0,			// Source of the audio is I2S PDM microphone
	AUDIO_MODE_IN_USB = 1,			// Source of the audio is USB
	AUDIO_MODE_IN_I2S = 2,			// Source of the audio is I2S #2 (External)
} AUDIO_ModeInTypeDef;

typedef enum
{
	AUDIO_MODE_OUT_NONE	= 0x00,	// No destination 
	AUDIO_MODE_OUT_I2S	= 0x01,	// Destination is I2S bus #1(Codec)
	AUDIO_MODE_OUT_USB	= 0x02,	// Destination is USB 
	AUDIO_MODE_OUT_I2SX	= 0x04,	// Destination is I2S bus #2(External)
} AUDIO_ModeOutTypeDef;

typedef enum
{ 
	ACTIVE_FIRST = 1,
	ACTIVE_SECOND = 0,
	DONE_FIRST = ACTIVE_SECOND,
	DONE_SECOND = ACTIVE_FIRST,
} ACTIVE_BUFFER;


#define AUDIO_MONO_Q15_SIZE	sizeof(int16_t)
#define AUDIO_STEREO_Q15_SIZE	(2 * sizeof(int16_t))
#define IV_MONO_Q15_SIZE	(2 * sizeof(int16_t))
#define IV_STEREO_Q15_SIZE	(4 * sizeof(int16_t))

typedef  enum uint16_t
{
	AUDIO_MONO_Q15 = (1<< 4) + AUDIO_MONO_Q15_SIZE  ,
	AUDIO_STEREO_Q15 = (2 << 4) + AUDIO_STEREO_Q15_SIZE,
	IV_MONO_Q15 = (3 << 4) + IV_MONO_Q15_SIZE,
	IV_STEREO_Q15 = (4 << 4) + IV_STEREO_Q15_SIZE,
} DataType_t;

#define AUDIO_SIZE_BYTES	(AUDIO_BLOCK_SAMPLES * AUDIO_STEREO_Q15_SIZE)
#define IV_SIZE_BYTES (IV_BLOCK_SAMPLES * IV_STEREO_Q15_SIZE)
	
void StartDefaultTask(void const * argument);
void StartDataInPDMTask(void const * argument);
void StartDataProcessTask(void const * argument);


typedef struct 
{
		AUDIO_ModeInTypeDef  audioinMode;		// Global Audio IN mode  - from USB, I2S, or PDM/I2S Microphone
		AUDIO_ModeOutTypeDef  audiooutMode;		// Global Audio OUT mode  - To USB, I2S Codec, or External I2S
		int bStartPlay;						  		// Sync the start of playing with the next block of PDM data


		osMessageQId dataInPDMMsg;			// Message queue to indicate that PDM data is ready
		osMessageQId dataReadyMsg;			// Message queue to indicate that any data for routing is ready
	
	  uint8_t		*pPDM_In;							// Pointer to DMA Buffer for PDM In
	  uint8_t		*pPCM_Out;						// Pointer to DMA Buffer for PCM Out
	
		BuffBlk_t	PCM_In_data;					// Data Queue for PCM In (Periph -> CPU) (Converted PDM)  data
		BuffBlk_t	PCM_Out_data;					// Data Queue for PCM OUT (CPU -> Periph) CODEC data
	
		BuffBlk_t	USB_Out_data;					// Data Queue for USB OUT (Host -> Device) SPEAKER data 
		BuffBlk_t	USB_In_data;					// Data Queue for USB IN (Device -> Host) MIC data 
	
}osObjects_t;

extern osObjects_t osParams;

extern void FF_Process(void *dsmHandle, float *pIn[], float *pOut[], uint32_t nSamples);
extern void FB_Process(void *dsmHandle, float *pIn[], uint32_t nSamples);

#endif // __TASKS_H__
