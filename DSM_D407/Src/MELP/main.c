/*

2.4 kbps MELP Proposed Federal Standard speech coder

version 1.2

Copyright (c) 1996, Texas Instruments, Inc.  

Texas Instruments has intellectual property rights on the MELP
algorithm.  The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).


*/

/*                                                                  */
/*  melp.c: Mixed Excitation LPC speech coder                       */
/*                                                                  */

/*  compiler include files  */
#include "melp.h"
#include "spbstd.h"
#include "mat.h"
#include "dsp_sub.h"
#include "melp_sub.h"
#include "stm32f4_discovery.h"

#include "cmsis_os.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dataqueues.h"

/* ====== External memory ====== */

typedef unsigned short uint16_t;
typedef signed short int16_t;

int		mode;
int		rate;

#define MELP_FRAME_SIZE  (180)

/* ========== Static Variables ========== */

static float	speech_in[MELP_FRAME_SIZE] CCMRAM, speech_out[MELP_FRAME_SIZE] CCMRAM;
static float	speech[MELP_FRAME_SIZE] CCMRAM; 
static char in_name[100], out_name[100];
struct melp_param	melp_ana_par CCMRAM;                 /* melp analysis parameters */
struct melp_param	melp_syn_par CCMRAM;                 /* melp synthesis parameters */


/* ========== Local Private Prototypes ========== */

static void		parseCommandLine(int argc, char *argv[]);
static void		printHelpMessage(char *argv[]);

static char *cmd_line[] = {"melp", "-i", "test_in.raw", "-o", "test_out.raw", 0};

extern int main_cmd(int argc, char *argv[]);

#define exit(a) do{}while(a)

#define SIGMAX 32767
typedef short SPEECH;
SPEECH	int_sp[MELP_FRAME_SIZE] CCMRAM ; /*  integer input array	*/
	
/*								*/
/*	Subroutine READBL: read block of input data		*/
/*								*/
int readbl(float input[], FILE *fp_in, int size)
{
	int i, length;

	length = fread(int_sp,sizeof(SPEECH),size,fp_in);
	for (i = 0; i < length; i++ )
		input[i] = (float) int_sp[i];
	for (i = length; i < size; i++ )
		input[i] = 0.0f;

	return length;
}


/*								*/
/*	Subroutine WRITEBL: write block of output data		*/
/*								*/
void writebl(float output[], FILE *fp_out, int size)
{
	int i;
	float temp;

	for (i = 0; i < size; i++ ) {
		temp = output[i];
		/* clamp to +- SIGMAX */
		if (temp > SIGMAX)	  temp = SIGMAX;
		if (temp < -SIGMAX)	  temp = -SIGMAX;
		int_sp[i] = (SPEECH)temp;
	}
	fwrite(int_sp,sizeof(SPEECH),size,fp_out);
}


int main_melp()
{
	main_cmd(5, cmd_line);
	return 0;
}

/****************************************************************************
**
** Function:        main
**
** Description:     The main function of the speech coder
**
** Arguments:
**
**  int     argc    ---- number of command line parameters
**  char    *argv[] ---- command line parameters
**
** Return value:    None
**
*****************************************************************************/
int main_cmd(int argc, char *argv[])
{
	int	length;
	int frame_count;

	int		eof_reached = FALSE;
	FILE	*fp_in, *fp_out;

	/* ====== Get input parameters from command line ====== */
	parseCommandLine(argc, argv);

	/* ====== Open input, output, and parameter files ====== */
	if ((fp_in = fopen(in_name,"rb")) == NULL){
		fprintf(stderr, "  ERROR: cannot read file %s.\n", in_name);
		exit(1);
	}
	if ((fp_out = fopen(out_name,"wb")) == NULL){
		fprintf(stderr, "  ERROR: cannot write file %s.\n", out_name);
		fclose(fp_in);
		exit(1);
	}

	/* ====== Initialize MELP analysis and synthesis ====== */
	melp_ana_init(&melp_ana_par);
	melp_syn_init(&melp_syn_par);

	/* ====== Run MELP coder on input signal ====== */
	frame_count = 0;
	eof_reached = FALSE;
	while (!eof_reached)
	{
		fprintf(stderr, "Frame = %d\r", frame_count);

		/* Perform MELP analysis */
		length = readbl(speech_in, fp_in, MELP_FRAME_SIZE);
		if (length < FRAME){
			eof_reached = TRUE;
		}
		melp_ana(speech_in, &melp_ana_par);

		/* Perform MELP synthesis */
		melp_syn(&melp_syn_par, speech_out);
		writebl(speech_out, fp_out, FRAME);
		frame_count++;
	}

	fclose(fp_in);
	fclose(fp_out);
	fprintf(stderr, "\n\n");

	return(0);
}

static int FrameIdx = 0;

#define DOWNSAMPLE_TAPS  	(12)
#define UPSAMPLE_TAPS		(24)
#define UPDOWNSAMPLE_RATIO (48000/8000)

static float DownSampleBuff[MELP_FRAME_SIZE + DOWNSAMPLE_TAPS - 1] CCMRAM;
static float DownSampleCoeff[DOWNSAMPLE_TAPS] RODATA = {
-0.0163654778152704f, -0.0205210950225592f, 0.00911782402545214f, 0.0889585390686989f,
0.195298701524735f, 0.272262066602707f, 0.272262066602707f, 0.195298701524735f,
0.0889585390686989f, 0.00911782402545214f, -0.0205210950225592f, -0.0163654778152704f
};


static float UpSampleBuff[(MELP_FRAME_SIZE + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1] CCMRAM;
static float UpSampleCoeff[UPSAMPLE_TAPS] RODATA = {
0.0420790389180183f, 0.0118295839056373f, -0.0317823477089405f, -0.10541670024395f,
-0.180645510554314f, -0.209222942590714f, -0.140164494514465f, 0.0557445883750916f,
0.365344613790512f, 0.727912843227386f, 1.05075252056122f, 1.24106538295746f, 
1.24106538295746f, 1.05075252056122f, 0.727912843227386f, 0.365344613790512f,
0.0557445883750916f, -0.140164494514465f, -0.209222942590714f, -0.180645510554314f,
-0.10541670024395f, -0.0317823477089405f, 0.0118295839056373f, 0.0420790389180183f
};

static arm_fir_decimate_instance_f32 CCMRAM Dec ;
static arm_fir_interpolate_instance_f32 CCMRAM Int;

void *melp_create(uint32_t Params)
{
	return 0;
}

void melp_close(void *pHandle)
{
	return;
}

void melp_init(void *pHandle)
{
	/* ====== Initialize Decimator and interpolator ====== */
	arm_fir_decimate_init_f32(&Dec, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO, 
			DownSampleCoeff, DownSampleBuff, MELP_FRAME_SIZE);
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSampleCoeff, UpSampleBuff, MELP_FRAME_SIZE/UPDOWNSAMPLE_RATIO);
	FrameIdx = 0;
	/* ====== Initialize MELP analysis and synthesis ====== */
	melp_ana_init(&melp_ana_par);
	melp_syn_init(&melp_syn_par);
}


uint32_t melp_process(void *pHandle, void *pDataIn, void *pDataOut, uint32_t nSamples)
{	
BSP_LED_On(LED3);
	arm_fir_decimate_f32(&Dec, pDataIn, &speech_in[FrameIdx], nSamples);
	arm_fir_interpolate_f32(&Int, &speech_out[FrameIdx], pDataOut, nSamples/UPDOWNSAMPLE_RATIO);
//	v_equ(pDataOut, pDataIn, nSamples);
BSP_LED_Off(LED3);
	
	FrameIdx += nSamples/UPDOWNSAMPLE_RATIO;
	if(FrameIdx >= MELP_FRAME_SIZE)
	{
BSP_LED_On(LED4);
		v_equ(speech, speech_in, MELP_FRAME_SIZE);
//		melp_ana(speech, &melp_ana_par);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
//		melp_syn(&melp_syn_par, speech);
		v_equ(speech_out, speech, MELP_FRAME_SIZE);
BSP_LED_Off(LED5);
		FrameIdx = 0;
	}
	return nSamples;
}

uint32_t melp_data_typesize(void *pHandle, uint32_t *pType)
{
	 *pType = DATA_TYPE_F32 | DATA_NUM_CH_1 | (4);
	 return MELP_FRAME_SIZE;
}


DataProcessBlock_t  MELP = {melp_create, melp_init, melp_data_typesize, melp_process, melp_close};

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
	int16_t	i;
	int		error_flag = FALSE;

	if (argc < 2)
		error_flag = TRUE;

	/* Setting default values. */
	mode = ANA_SYN;
	rate = RATE2400;
	in_name[0] = '\0';
	out_name[0] = '\0';

	for (i = 1; i < argc; i++){
		if ((strncmp(argv[i], "-h", 2 ) == 0) ||
			(strncmp(argv[i], "-help", 5) == 0)){
			printHelpMessage(argv);
			exit(0);
		} else if ((strncmp(argv[i], "-a", 2)) == 0)
			mode = ANALYSIS;
		else if ((strncmp(argv[i], "-s", 2)) == 0)
			mode = SYNTHESIS;
		else if ((strncmp(argv[i], "-i", 2)) == 0){
			i++;
			if (i < argc)
				strcpy(in_name, argv[i]);
			else
				error_flag = TRUE;
		} else if ((strncmp(argv[i], "-o", 2)) == 0){
			i++;
			if (i < argc)
				strcpy(out_name, argv[i]);
			else
				error_flag = TRUE;
		} else
			error_flag = TRUE;
	}

	if ((in_name[0] == '\0') || (out_name[0] == '\0'))
		error_flag = TRUE;

	if (error_flag){
		printHelpMessage(argv);
		exit(1);
	}
	switch (mode){
	case ANA_SYN:
	case ANALYSIS:
	case SYNTHESIS:
		if (rate == RATE2400)
			fprintf(stderr, " ---- 2.4kbps mode.\n");
		else
			fprintf(stderr, " ---- 1.2kbps mode.\n");
		break;
	}
	switch (mode){
	case ANA_SYN:
		fprintf(stderr, " ---- Analysis and Synthesis.\n"); break;
	case ANALYSIS:
		fprintf(stderr, " ---- Analysis only.\n"); break;
	case SYNTHESIS:
		fprintf(stderr, " ---- Synthesis only.\n"); break;
	}

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
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "\t%s [-as] -i infile -o outfile\n", argv[0]);
	fprintf(stdout, "\t\tdefault: Analysis/Synthesis at 2.4kbps\n");
	fprintf(stdout, "\t\t-a --analysis\tAnalysis only\n");
	fprintf(stdout, "\t\t-s --synthesis\tSynthesis only\n\n");
}
