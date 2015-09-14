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

#define v_equ(v1,v2,n) 			arm_copy_f32((float32_t *)v2, (float32_t *)v1, n)

#define CVSD_BLOCK_SIZE   (120)

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
static char in_name[128], out_name[128];

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
	int16_t		speech_in[CVSD_BLOCK_SIZE], speech_out[CVSD_BLOCK_SIZE];
	float32_t	speech_in_f32[CVSD_BLOCK_SIZE], speech_out_f32[CVSD_BLOCK_SIZE];
	uint8_t		bitstream[(CVSD_BLOCK_SIZE + 7)/8];

	int		    eof_reached = FALSE;
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

		length = fread(speech_in, sizeof(int16_t), CVSD_BLOCK_SIZE, fp_in);

        arm_q15_to_float(speech_in, speech_in_f32, length);
		cvsd_encode_f32(enc, bitstream, speech_in_f32, length);
		cvsd_decode_f32(dec, speech_out_f32, bitstream, length);
        arm_float_to_q15(speech_out_f32, speech_out, length);

		fwrite(speech_out, sizeof(int16_t), length, fp_out);
		if (length < CVSD_BLOCK_SIZE)
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

static int FrameIdx = 0;

#define DOWNSAMPLE_TAPS  (12)
#define UPSAMPLE_TAPS		 (24)
#define UPDOWNSAMPLE_RATIO (48000/16000)

static float DownSampleBuff[CVSD_BLOCK_SIZE + DOWNSAMPLE_TAPS - 1] CCMRAM;
static float DownSampleCoeff[DOWNSAMPLE_TAPS] RODATA = {
-0.0318333953619003f, -0.0245810560882092f, 0.0154596352949739f, 0.0997937619686127f,
0.200223222374916f, 0.268874526023865f, 0.268874526023865f, 0.200223222374916f,
0.0997937619686127f, 0.0154596352949739f, -0.0245810560882092f, -0.0318333953619003};


static float UpSampleBuff[(CVSD_BLOCK_SIZE + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1] CCMRAM;
static float UpSampleCoeff[UPSAMPLE_TAPS] RODATA = {
0.00449248310178518f, -0.0288104526698589f, -0.0499703288078308f, -0.0734485313296318f,
-0.082396112382412f, -0.0617895275354385f, -0.00137842050753534f, 0.0993839502334595f,
0.228658899664879f, 0.363589704036713f, 0.475823938846588f, 0.539592683315277f,
0.539592683315277f, 0.475823938846588f, 0.363589704036713f, 0.228658899664879f,
0.0993839502334595f, -0.00137842050753534f, -0.0617895275354385f, -0.082396112382412f,
-0.0734485313296318f, -0.0499703288078308f, -0.0288104526698589f, 0.00449248310178518f};

static arm_fir_decimate_instance_f32 CCMRAM Dec ;
static arm_fir_interpolate_instance_f32 CCMRAM Int;

void *cvsd_ana;
void *cvsd_syn;

static uint8_t dataBits[CVSD_BLOCK_SIZE] CCMRAM;
static float speech_in[CVSD_BLOCK_SIZE] CCMRAM;
static float speech_out[CVSD_BLOCK_SIZE] CCMRAM;

void *cvsd_create(uint32_t Params)
{
	/* ====== Initialize CVSD analysis and synthesis ====== */
	cvsd_ana = osAlloc(cvsd_mem_req_f32());
	cvsd_syn = osAlloc(cvsd_mem_req_f32());
	return 0;
}

void cvsd_close(void *pHandle)
{
	return;
}

void cvsd_init(void *pHandle)
{
	/* ====== Initialize Decimator and interpolator ====== */
	arm_fir_decimate_init_f32(&Dec, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO,
			DownSampleCoeff, DownSampleBuff, CVSD_BLOCK_SIZE);
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSampleCoeff, UpSampleBuff, CVSD_BLOCK_SIZE/UPDOWNSAMPLE_RATIO);
	FrameIdx = 0;
	cvsd_init_f32(cvsd_ana);
	cvsd_init_f32(cvsd_syn);
}


uint32_t cvsd_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t nSamples)
{

BSP_LED_On(LED3);
	arm_fir_decimate_f32(&Dec, pDataIn, &speech_in[FrameIdx], nSamples);
	arm_fir_interpolate_f32(&Int, &speech_out[FrameIdx], pDataOut, nSamples/UPDOWNSAMPLE_RATIO);
//	v_equ(pDataOut, pDataIn, nSamples);
BSP_LED_Off(LED3);

	FrameIdx += nSamples/UPDOWNSAMPLE_RATIO;
	if(FrameIdx >= CVSD_BLOCK_SIZE)
	{
BSP_LED_On(LED4);
		arm_scale_f32(speech_in, 32767.0f, speech_in, CVSD_BLOCK_SIZE);
		cvsd_encode_f32(cvsd_ana, dataBits, speech_in, CVSD_BLOCK_SIZE);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		cvsd_decode_f32(cvsd_syn, speech_out, dataBits, CVSD_BLOCK_SIZE);
		arm_scale_f32(speech_out, 1.0f/32768.0f, speech_out, CVSD_BLOCK_SIZE);
BSP_LED_Off(LED5);
		FrameIdx = 0;
//		v_equ(speech_out, speech_in, CVSD_BLOCK_SIZE);
	}
	return nSamples;
}

uint32_t cvsd_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return CVSD_BLOCK_SIZE;
}

DataProcessBlock_t  CVSD = {cvsd_create, cvsd_init, cvsd_data_typesize, cvsd_process, cvsd_close};


#define  BYPASS_DATA_TYPE		(DATA_TYPE_F32 | DATA_NUM_CH_1 | (4))
#define  BYPASS_BLOCK_SIZE  	(60)

void *bypass_create(uint32_t Params)
{
	return 0;
}

void bypass_close(void *pHandle)
{
	return;
}

void bypass_init(void *pHandle)
{
}

uint32_t bypass_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t nSamples)
{
BSP_LED_On(LED5);
	v_equ(pDataOut, pDataIn, nSamples );
BSP_LED_Off(LED5);
	return nSamples;
}

uint32_t bypass_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = BYPASS_DATA_TYPE;
	 return BYPASS_BLOCK_SIZE;
}

DataProcessBlock_t  BYPASS = {bypass_create, bypass_init, bypass_data_typesize, bypass_process, bypass_close};
