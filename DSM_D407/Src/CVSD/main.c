#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif
#define __TARGET_FPU_VFP 1
#define __FPU_PRESENT 1
#include <stdint.h>
#endif

#include "arm_math.h"
#include "arm_const_structs.h"
#include "cvsd_f32.h"
#include "datatasks.h"
#include "stm32f4_discovery.h"

#define PROGRAM_NAME			"CVSD 16000 bps speech coder"
#define PROGRAM_VERSION			"Version 2.0"
#define PROGRAM_DATE			"14 NOV 2014"
#define BLOCK_SIZE				4000

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef BOOL
typedef int BOOL;
#endif

/* ========== Static Variables ========== */
static char in_name[256], out_name[256];

#define exit(a) do{}while(a)

/* ========== Local Private Prototypes ========== */
static void		parseCommandLine(int argc, char *argv[]);
static void		printHelpMessage(char *argv[]);

/****************************************************************************
**
** Function:        FileTest
**
** Description:     The test function of the speech coder
**
** Arguments:
**
**  int     argc    ---- number of command line parameters
**  char    *argv[] ---- command line parameters
**
** Return value:    None
**
*****************************************************************************/
int FileTest(int argc, char *argv[])
{
	int32_t		length;
	int16_t		speech_in[BLOCK_SIZE], speech_out[BLOCK_SIZE];
	uint8_t		bitstream[BLOCK_SIZE/8];
	/* size of the bitstream buffer */
	int		eof_reached = FALSE;
	FILE		*fp_in, *fp_out;
	void		*enc, *dec;

	/* ====== Get input parameters from command line ====== */
	parseCommandLine(argc, argv);

	/* ====== Open input, output, and parameter files ====== */
	if ((fp_in = fopen(in_name, "rb")) == NULL){
		fprintf(stderr, "  ERROR: cannot read file %s.\n", in_name);
		exit(1);
	}
	if ((fp_out = fopen(out_name, "wb")) == NULL){
		fprintf(stderr, "  ERROR: cannot write file %s.\n", out_name);
		exit(1);
	}

	/* ====== Initialize CVSD analysis and synthesis ====== */
	enc = malloc(cvsd_mem_req_f32());
	dec = malloc(cvsd_mem_req_f32());
	cvsd_init_f32(enc);
	cvsd_init_f32(dec);

	/* ====== Run CVSD coder on input signal ====== */

	eof_reached = FALSE;
	while (!eof_reached)
	{

		length = fread(speech_in, sizeof(int16_t), BLOCK_SIZE, fp_in);

		cvsd_encode_f32(enc, bitstream, speech_in, length);	
		cvsd_decode_f32(dec, speech_out, bitstream, length);

		fwrite(speech_out, sizeof(int16_t), length, fp_out);
		if (length < BLOCK_SIZE)
		{
			eof_reached = TRUE;
		}
	}

	fclose(fp_in);
	fclose(fp_out);
	fprintf(stderr, "\n\n");

	return(0);
}


/****************************************************************************
**
** Function:        parseCommandLine
**
** Description:     Translate command line parameters
**
** Arguments:
**
**  int     argc    ---- number of command line parameters
**  char    *argv[] ---- command line parameters
**
** Return value:    None
**
*****************************************************************************/
static void		parseCommandLine(int argc, char *argv[])
{
	BOOL		error_flag = FALSE;

	if (argc != 3)
		error_flag = TRUE;

	/* Setting default values. */
	in_name[0] = '\0';
	out_name[0] = '\0';

	if( argc == 3 ){
		strcpy(in_name, argv[1]);
		strcpy(out_name, argv[2]);
	}

	if ((in_name[0] == '\0') || (out_name[0] == '\0'))
		error_flag = TRUE;

	if (error_flag){
		printHelpMessage(argv);
		exit(1);
	}

	fprintf(stderr, "\n\n\t%s %s, %s\n\n", PROGRAM_NAME, PROGRAM_VERSION,
		PROGRAM_DATE);

	fprintf(stderr, " ---- Analysis and Synthesis.\n");

	fprintf(stderr, " ---- input from %s.\n", in_name);
	fprintf(stderr, " ---- output to %s.\n", out_name);
}


/****************************************************************************
**
** Function:        printHelpMessage
**
** Description:     Print Command Line Usage
**
** Arguments:
**
** Return value:    None
**
*****************************************************************************/
static void		printHelpMessage(char *argv[])
{
	fprintf(stderr, "\n\n\t%s %s, %s\n\n", PROGRAM_NAME, PROGRAM_VERSION,
		PROGRAM_DATE);
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "\t%s infile outfile\n\n", argv[0]);
}

//
// *****************************
//

static int bInitialized = 0;
static int FrameIdx = 0;

#define DOWNSAMPLE_TAPS  (12)
#define UPSAMPLE_TAPS		 (24)
#define UPDOWNSAMPLE_RATIO (48000/16000)

static float DownSampleBuff[AUDIO_BLOCK_SAMPLES + DOWNSAMPLE_TAPS - 1];
static float DownSampleCoeff[DOWNSAMPLE_TAPS] = {
   -0.05217897519f,  -0.0113634998f,  0.03741466254f,   0.1159368753f,   0.1981081218f,
     0.2506353855f,   0.2506353855f,   0.1981081218f,   0.1159368753f,  0.03741466254f,
    -0.0113634998f, -0.05217897519f
};


static float UpSampleBuff[(AUDIO_BLOCK_SAMPLES + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1];
static float UpSampleCoeff[UPSAMPLE_TAPS] = {
    0.00258618989f,  0.01110466011f, 0.009563598782f, 0.002841173206f,  -0.0138431713f,
   -0.03166284412f, -0.03691352904f,  -0.0153341908f,  0.03820863366f,   0.1138970926f,
     0.1884739548f,   0.2348130196f,   0.2348130196f,   0.1884739548f,   0.1138970926f,
    0.03820863366f,  -0.0153341908f, -0.03691352904f, -0.03166284412f,  -0.0138431713f,
   0.002841173206f, 0.009563598782f,  0.01110466011f,  0.00258618989f
};

static arm_fir_decimate_instance_f32 Dec;
static arm_fir_interpolate_instance_f32 Int;

void *cvsd_ana;
void *cvsd_syn;

static uint8_t dataBits[AUDIO_BLOCK_SAMPLES];
static float speech_in[AUDIO_BLOCK_SAMPLES];
static float speech_out[AUDIO_BLOCK_SAMPLES];

void cvsd_init()
{
	/* ====== Initialize Decimator and interpolator ====== */
	arm_fir_decimate_init_f32(&Dec, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO, 
			DownSampleCoeff, DownSampleBuff, AUDIO_BLOCK_SAMPLES);
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSampleCoeff, UpSampleBuff, AUDIO_BLOCK_SAMPLES/UPDOWNSAMPLE_RATIO);
	FrameIdx = 0;
	/* ====== Initialize CVSD analysis and synthesis ====== */
	cvsd_ana = osAlloc(cvsd_mem_req_f32());
	cvsd_syn = osAlloc(cvsd_mem_req_f32());
	
	cvsd_init_f32(cvsd_ana);
	cvsd_init_f32(cvsd_syn);
}


void cvsd_process(float *pDataIn, float *pDataOut, int nSamples)
{	
	if(0 == bInitialized)
	{
		cvsd_init();
		bInitialized = 1;	
	}

BSP_LED_On(LED3);
	arm_fir_decimate_f32(&Dec, pDataIn, &speech_in[FrameIdx], nSamples);
	arm_fir_interpolate_f32(&Int, &speech_out[FrameIdx], pDataOut, nSamples/UPDOWNSAMPLE_RATIO);
BSP_LED_Off(LED3);
	
	FrameIdx += nSamples/UPDOWNSAMPLE_RATIO;
	if(FrameIdx >= AUDIO_BLOCK_SAMPLES)
	{
BSP_LED_On(LED4);
		cvsd_encode_f32(cvsd_ana, dataBits, speech_in, AUDIO_BLOCK_SAMPLES);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		cvsd_decode_f32(cvsd_syn, speech_out, dataBits, AUDIO_BLOCK_SAMPLES);
BSP_LED_Off(LED5);
		FrameIdx = 0;
	}
}
