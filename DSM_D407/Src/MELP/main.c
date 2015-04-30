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
#include <stdio.h>
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

/* ====== External memory ====== */

#define FRAME				180              /* speech frame size */

typedef unsigned short uint16_t;
typedef signed short int16_t;

int		mode;
int		rate;

/* ========== Static Variables ========== */

float	speech_in[FRAME], speech_out[FRAME];
float	speech[FRAME]; 
char in_name[100], out_name[100];
struct melp_param	melp_ana_par;                 /* melp analysis parameters */
struct melp_param	melp_syn_par;                 /* melp synthesis parameters */


/* ========== Local Private Prototypes ========== */

static void		parseCommandLine(int argc, char *argv[]);
static void		printHelpMessage(char *argv[]);

static char *cmd_line[] = {"melp", "-i", "test_in.raw", "-o", "test_out.raw", 0};

extern int main_cmd(int argc, char *argv[]);

#define exit(a) do{}while(a)

#define SIGMAX 32767
typedef short SPEECH;
SPEECH	int_sp[FRAME]; /*  integer input array	*/
	
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
		length = readbl(speech_in, fp_in, FRAME);
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

static int bInitialized = 0;
static int FrameIdx = 0;

#define DOWNSAMPLE_TAPS  (12)
#define UPSAMPLE_TAPS		 (24)
#define UPDOWNSAMPLE_RATIO (48000/8000)

float DownSampleBuff[FRAME + DOWNSAMPLE_TAPS - 1];
float DownSampleCoeff[DOWNSAMPLE_TAPS] = {
   -0.05217897519f,  -0.0113634998f,  0.03741466254f,   0.1159368753f,   0.1981081218f,
     0.2506353855f,   0.2506353855f,   0.1981081218f,   0.1159368753f,  0.03741466254f,
    -0.0113634998f, -0.05217897519f
};


float UpSampleBuff[(FRAME + UPSAMPLE_TAPS)/UPDOWNSAMPLE_RATIO - 1];
float UpSampleCoeff[UPSAMPLE_TAPS] = {
    0.00258618989f,  0.01110466011f, 0.009563598782f, 0.002841173206f,  -0.0138431713f,
   -0.03166284412f, -0.03691352904f,  -0.0153341908f,  0.03820863366f,   0.1138970926f,
     0.1884739548f,   0.2348130196f,   0.2348130196f,   0.1884739548f,   0.1138970926f,
    0.03820863366f,  -0.0153341908f, -0.03691352904f, -0.03166284412f,  -0.0138431713f,
   0.002841173206f, 0.009563598782f,  0.01110466011f,  0.00258618989f
};

arm_fir_decimate_instance_f32 Dec;
arm_fir_interpolate_instance_f32 Int;

void melp_init()
{
	/* ====== Initialize Decimator and interpolator ====== */
	arm_fir_decimate_init_f32(&Dec, DOWNSAMPLE_TAPS, UPDOWNSAMPLE_RATIO, 
			DownSampleCoeff, DownSampleBuff, FRAME);
	arm_fir_interpolate_init_f32(&Int,  UPDOWNSAMPLE_RATIO, UPSAMPLE_TAPS,
			UpSampleCoeff, UpSampleBuff, FRAME/UPDOWNSAMPLE_RATIO);
	FrameIdx = 0;
	/* ====== Initialize MELP analysis and synthesis ====== */
	melp_ana_init(&melp_ana_par);
	melp_syn_init(&melp_syn_par);
}


void melp_process(float *pDataIn, float *pDataOut)
{	
	if(0 == bInitialized)
	{
		melp_init();
		bInitialized = 1;	
	}

BSP_LED_On(LED3);
	arm_fir_decimate_f32(&Dec, pDataIn, &speech_in[FrameIdx], FRAME);
	arm_fir_interpolate_f32(&Int, &speech_out[FrameIdx], pDataOut, FRAME/UPDOWNSAMPLE_RATIO);
BSP_LED_Off(LED3);
	
	FrameIdx += FRAME/UPDOWNSAMPLE_RATIO;
	if(FrameIdx >= FRAME)
	{
		v_equ(speech_out, speech_in, FRAME);
		v_equ(speech, speech_in, FRAME);
BSP_LED_On(LED4);
		melp_ana(speech, &melp_ana_par);
BSP_LED_Off(LED4);
BSP_LED_On(LED5);
		melp_syn(&melp_syn_par, speech);
		v_equ(speech_out, speech, FRAME);
BSP_LED_Off(LED5);
		FrameIdx = 0;
	}
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
