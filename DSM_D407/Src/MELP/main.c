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

/* note: CHSIZE is shortest integer number of words in channel packet */
#define CHSIZE 9
#define NUM_CH_BITS 54



#include <stdio.h>
#include <string.h>
#include <stdlib.h>


//#include "sc1200.h"
//#include "mat_lib.h"
//#include "global.h"
//#include "macro.h"
//#include "mathhalf.h"
//#include "dsp_sub.h"
//#include "melp_sub.h"
//#include "constant.h"
//#include "math_lib.h"
//#include "math.h"
//#include "transcode.h"

/* ====== External memory ====== */

#define FRAME				180                          /* speech frame size */
#define NF					3                /* number of frames in one block */
#define BLOCK				(NF*FRAME)

typedef unsigned short uint16_t;
typedef signed short int16_t;

int	  mode;
int   chwordsize;
int		frameSize;
int		rate;

/* ========== Static Variables ========== */

char in_name[256], out_name[256];
// char chbuf[1024];
// struct melp_param melp_par;
struct melp_param	melp_par[NF];                 /* melp analysis parameters */
unsigned int	chbuf[CHSIZE * NF];                     /* channel bit data buffer */


/* ========== Local Private Prototypes ========== */

static void		parseCommandLine(int argc, char *argv[]);
static void		printHelpMessage(char *argv[]);

static char *cmd_line[] = {"melpe", "-i", "test_in.raw", "-o", "test_out.raw", 0};

extern int main_cmd(int argc, char *argv[]);
int melp_main()
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
	int bitNum12;
	int bitNum24;
	int frame_count;
	int i;

	float	speech_in[BLOCK], speech_out[BLOCK];
	int16_t	bitBufSize, bitBufSize12, bitBufSize24;
                                          /* size of the bitstream buffer */
	int		eof_reached = FALSE;
	FILE	*fp_in, *fp_out;

	/* ====== Get input parameters from command line ====== */
	parseCommandLine(argc, argv);

	/* ====== Open input, output, and parameter files ====== */
	if ((fp_in = fopen(in_name,"rb")) == NULL){
		fprintf(stderr, "  ERROR: cannot read file %s.\n", in_name);
		while(1){}
//		exit(1);
	}
	if ((fp_out = fopen(out_name,"wb")) == NULL){
		fprintf(stderr, "  ERROR: cannot write file %s.\n", out_name);
		while(1){}		
//		exit(1);
	}

	/* ====== Initialize MELP analysis and synthesis ====== */
	frameSize = (int16_t) FRAME;
	/* Computing bit=Num = rate * frameSize / FSAMP.  Note that bitNum        */
	/* computes the number of bytes written to the channel and it has to be   */
	/* exact.  We first carry out the division and then have the multiplica-  */
	/* tion with rounding.                                                    */
    bitNum12 = 81;
    bitNum24 = 54;
    if( chwordsize == 8 ){
        // packing the bitstream
        bitBufSize12 = 11;
        bitBufSize24 = 7;
    }else if( chwordsize == 6 ){
        bitBufSize12 = 14;
        bitBufSize24 = 9;
    }else{
        fprintf(stderr,"Channel word size is wrong!\n");
				while(1){}			
//        exit(-1);
    }

    if (rate == RATE2400){
		frameSize = FRAME;
		bitBufSize = bitBufSize24;
	} else {
		frameSize = BLOCK;
		bitBufSize = bitBufSize12;
	}

	for(i = 0; i < NF; i++){
		melp_par[i].chptr = &chbuf[i*CHSIZE];
	}

	if (mode != SYNTHESIS)
	{
		melp_ana_init();
	}
	if (mode != ANALYSIS)
	{
		melp_syn_init();
	}

	/* ====== Run MELP coder on input signal ====== */

	frame_count = 0;
	eof_reached = FALSE;
	while (!eof_reached)
	{
//		fprintf(stderr, "Frame = %ld\r", frame_count);


		/* Perform MELP analysis */
		if (mode != SYNTHESIS){
			/* read input speech */
			length = readbl(speech_in, fp_in, frameSize);
			if (length < frameSize){
				v_zap(&speech_in[length], (int16_t) (FRAME - length));
				eof_reached = TRUE;
			}

			melp_ana(speech_in, melp_par);

			/* ---- Write channel output if needed ---- */
            if (mode == ANALYSIS){
                if( chwordsize == 8 ){
				    fwrite(chbuf, sizeof(unsigned char), bitBufSize, fp_out);
                }else{
    				int i;
	        		unsigned int bitData;
			        for(i = 0; i < bitBufSize; i++){
				        bitData = (unsigned int)(chbuf[i]);
				        fwrite(&bitData, sizeof(unsigned int), 1, fp_out);
			        }
		        }
            }
		}
		/* ====== Perform MELP synthesis (skip first frame) ====== */
		if (mode != ANALYSIS){

			/* Read channel input if needed */
			if (mode == SYNTHESIS){
                if( chwordsize == 8 ){
				    length = fread(chbuf, sizeof(unsigned char), bitBufSize,
					    		   fp_in);
                }else{
		        	int i, readNum;
    				unsigned int bitData;
	        		for(i = 0; i < bitBufSize; i++){
			        	readNum = fread(&bitData,sizeof(unsigned int),1,fp_in);
    					if( readNum != 1 )	break;
	        			chbuf[i] = (unsigned char)bitData;
			        }
    				length = i;
                }
				if (length < bitBufSize){
					eof_reached = TRUE;
					break;
				}
			}
			melp_syn(melp_par, speech_out);
			writebl(speech_out, fp_out, frameSize);
		}
		frame_count ++;
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
	int16_t	i;
	int		error_flag = FALSE;

	if (argc < 2)
		error_flag = TRUE;

	/* Setting default values. */
	mode = ANA_SYN;
	rate = RATE2400;
    chwordsize = 8;         // this is for packed bitstream
	in_name[0] = '\0';
	out_name[0] = '\0';

	for (i = 1; i < argc; i++){
		if ((strncmp(argv[i], "-h", 2 ) == 0) ||
			(strncmp(argv[i], "-help", 5) == 0)){
			printHelpMessage(argv);
		  while(1){}
//			exit(0);
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
			while(1){}
//		exit(1);
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
