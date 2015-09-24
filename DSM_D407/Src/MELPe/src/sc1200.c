/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

/* ========================================= */
/* melp.c: Mixed Excitation LPC speech coder */
/* ========================================= */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sc1200.h"
#include "mat_lib.h"
#include "global.h"
#include "macro.h"
#include "mathhalf.h"
#include "dsp_sub.h"
#include "melp_sub.h"
#include "constant.h"
#include "math_lib.h"
#include "math.h"

#if NPP
#include "npp.h"
#endif

/* ========== Static definations ========== */

#define PROGRAM_NAME			"SC1200 1200 bps speech coder"
#define PROGRAM_VERSION			"Version 7 / 42 Bits"
#define PROGRAM_DATE			"10/25/2000"

/* ========== Static Variables ========== */

static int16_t	mode;
static int32_t	frame_count;

char in_name[100], out_name[100];

/* ========== Local Private Prototypes ========== */

static void		parseCommandLine(int argc, char *argv[]);
static void		printHelpMessage(char *argv[]);

/* ========================================================================== */
/* This function reads a block of input data.  It returns the actual number   */
/*    of samples read.  Zeros are filled into input[] if there are not suffi- */
/*    cient data samples.                                                     */
/* ========================================================================== */
int16_t readbl(int16_t input[], FILE *fp_in, int16_t size)
{
	int16_t	length;


	length = (int16_t) fread(input, sizeof(int16_t), size, fp_in);
	v_zap(&(input[length]), (int16_t) (size - length));

	return(length);
}

/* ============================================ */
/* This function writes a block of output data. */
/* ============================================ */
void writebl(int16_t output[], FILE *fp_out, int16_t size)
{
	fwrite(output, sizeof(int16_t), size, fp_out);
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
int main(int argc, char *argv[])
{
	int32_t	length;
	int16_t	speech_in[BLOCK], speech_out[BLOCK];
	int16_t	bitBufSize12, bitBufSize24;
                                          /* size of the bitstream buffer */
	BOOLEAN		eof_reached = FALSE;
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
		exit(1);
	}

	/* ====== Initialize MELP analysis and synthesis ====== */
	if (melp_parameters->rate == RATE2400)
		melp_parameters->frameSize = FRAME;
	else
		melp_parameters->frameSize = BLOCK;
	/* Computing bitNum = rate * frameSize / FSAMP.  Note that bitNum        */
	/* computes the number of bytes written to the channel and it has to be   */
	/* exact.  We first carry out the division and then have the multiplica-  */
	/* tion with rounding.                                                    */
    if( melp_parameters->chwordSize == 8 ){
        // packing the bitstream
        bitBufSize12 = 11;
        bitBufSize24 = 7;
    }else if( melp_parameters->chwordSize == 6 ){
        bitBufSize12 = 14;
        bitBufSize24 = 9;
    }else{
        fprintf(stderr,"Channel word size is wrong!\n");
        exit(-1);
    }

    if (melp_parameters->rate == RATE2400){
		melp_parameters->frameSize = FRAME;
		melp_parameters->bitSize = bitBufSize24;
	} else {
		melp_parameters->frameSize = BLOCK;
		melp_parameters->bitSize = bitBufSize12;
	}

	if (mode != SYNTHESIS)
		melp_ana_init(melp_parameters);
	if (mode != ANALYSIS)
		melp_syn_init(melp_parameters);


	/* ====== Run MELP coder on input signal ====== */

	frame_count = 0;
	eof_reached = FALSE;
	while (!eof_reached){
		fprintf(stderr, "Frame = %d\r", frame_count);
		{
			/* Perform MELP analysis */
			if (mode != SYNTHESIS){
				/* read input speech */
				length = readbl(speech_in, fp_in, melp_parameters->frameSize);
				if (length < melp_parameters->frameSize){
					v_zap(&speech_in[length], (int16_t) (melp_parameters->frameSize - length));
					eof_reached = TRUE;
				}

				/* ---- Noise Pre-Processor ---- */
#if NPP
				if (melp_parameters->rate == RATE1200){
					npp(melp_parameters, speech_in, speech_in);
					npp(melp_parameters, &(speech_in[FRAME]), &(speech_in[FRAME]));
					npp(melp_parameters, &(speech_in[2*FRAME]), &(speech_in[2*FRAME]));
				} else
					npp(melp_parameters, speech_in, speech_in);
#endif
				analysis(speech_in, melp_parameters);

				/* ---- Write channel output if needed ---- */
                if (mode == ANALYSIS){
                    if( melp_parameters->chwordSize == 8 ){
					    fwrite(chbuf, sizeof(unsigned char), melp_parameters->bitSize, fp_out);
                    }else{
        				int i;
		        		unsigned int bitData;
				        for(i = 0; i < melp_parameters->bitSize; i++){
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
                    if( melp_parameters->chwordSize == 8 ){
					    length = (int32_t) fread(chbuf, sizeof(unsigned char), melp_parameters->bitSize, fp_in);
                    }else{
    		        	int i, readNum;
        				unsigned int bitData;
		        		for(i = 0; i < melp_parameters->bitSize; i++){
				        	readNum = (int32_t) fread(&bitData,sizeof(unsigned int),1,fp_in);
        					if( readNum != 1 )	break;
		        			chbuf[i] = (unsigned char)bitData;
				        }	
        				length = i;		
                    }
					if (length < melp_parameters->bitSize){
						eof_reached = TRUE;
						break;
					}
				}
				synthesis(melp_parameters, speech_out);
				writebl(speech_out, fp_out, melp_parameters->frameSize);
			}
		}
		frame_count++;
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
	register int16_t	i;
	BOOLEAN		error_flag = FALSE;

	if (argc < 2)
		error_flag = TRUE;

	/* Setting default values. */
	mode = ANA_SYN;
	melp_parameters->rate = RATE2400;
    melp_parameters->chwordSize = 8;         // this is for packed bitstream
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
		else if ((strncmp(argv[i], "-l", 2)) == 0)
			melp_parameters->rate = RATE1200;
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
		} else if ((strncmp(argv[i], "-p", 2)) == 0){
            // for backword compatibility with 2.4 kbps MELP ref. coder
			melp_parameters->chwordSize = 6;
		} else
			error_flag = TRUE;
	}

	if ((in_name[0] == '\0') || (out_name[0] == '\0'))
		error_flag = TRUE;

	if (error_flag){
		printHelpMessage(argv);
		exit(1);
	}

	fprintf(stderr, "\n\n\t%s %s, %s\n\n", PROGRAM_NAME, PROGRAM_VERSION,
			PROGRAM_DATE);
	switch (mode){
	case ANA_SYN:
	case ANALYSIS:
	case SYNTHESIS:
		if (melp_parameters->rate == RATE2400)
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
	fprintf(stderr, "\n\n\t%s %s, %s\n\n", PROGRAM_NAME, PROGRAM_VERSION,
			PROGRAM_DATE);
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "\t%s [-l] [-asudp] -i infile -o outfile\n", argv[0]);
	fprintf(stdout, "\t\tdefault: Analysis/Synthesis at 2.4kbps\n");
	fprintf(stdout, "\t\t-a --analysis\tAnalysis only\n");
	fprintf(stdout, "\t\t-s --synthesis\tSynthesis only\n");
	fprintf(stdout, "\t\t-l --low rate\t1.2kbps coding\n\n");
	fprintf(stdout, "\t\t-u --transcoder from 1.2kbps to 2.4kpbs\n");
	fprintf(stdout, "\t\t-d --transcoder from 2.4kbps to 1.2kbps\n");
    fprintf(stdout, "\t\t-p --using unpacked bitstream for backward compatibility\n\n");
}
