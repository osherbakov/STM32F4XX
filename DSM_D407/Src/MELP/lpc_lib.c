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

/*

  lpc_lib.c: LPC function library

*/

#include <math.h>
#include "mat.h"

#include "melp.h"
#include "lpc.h"
/* 
    Name: lpc_aejw- Compute square of A(z) evaluated at exp(jw)
    Description:
        Compute the magnitude squared of the z-transform of 
<nf>
        A(z) = 1+a(1)z^-1 + ... +a(p)z^-p
</nf>
        evaluated at z=exp(jw)
     Inputs:
        a- LPC filter (a[0] is undefined, a[1..p])
        w- radian frequency
        p- predictor order
     Returns:
        |A(exp(jw))|^2
     See_Also: cos(3), sin(3)
     Includes:
        spbstd.h
        lpc.h
     Systems and Info. Science Lab
     Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved.

*/

float lpc_aejw(float *a,float w,int p)
{
    int i;
    float c_re,c_im;
    float cs,sn,tmp;

    if (p==0)
        return(1.);

    /* use horners method
    A(exp(jw)) = 1+ e(-jw)[a(1)+e(-jw)[a(2)+e(-jw)[a(3)+..
            ...[a(p-1)+e(-jw)a(p)]]]]
    */

    cs = cosf(w);
    sn = -sinf(w);

    c_re = cs*a[p];
    c_im = sn*a[p];

    for(i=p-1; i > 0; i--)
    {
        /* add a[i] */
        c_re += a[i];

        /* multiply by exp(-jw) */
        c_im = cs*(tmp=c_im) + sn*c_re;
        c_re = cs*c_re - sn*tmp;
    }

    /* add one */
    c_re += 1.0f;

    return(SQR(c_re) + SQR(c_im));
} /* LPC_AEJW */

/*
    Name: lpc_bw_expand- Move the zeros of A(z) toward the origin.
    Aliases: lpc_bw_expand
    Description:
        Expand the zeros of the LPC filter by gamma, which
        moves each zero radially into the origin.
<nf>
        for j = 1 to p
            aw[j] = a[j]*gamma^j
</nf>
        (Can also be used to perform an exponential windowing procedure).
    Inputs:
        a- lpc vector (order p, a[1..p])
        gamma- the bandwidth expansion factor
        p- order of lpc filter
    Outputs:
        aw- the bandwidth expanded LPC filter
    Returns: NULL
    See_Also: lpc_lagw(3l)
    Includes:
        spbstd.h
        lpc.h

    Systems and Info. Science Lab
    Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved.
*/

int lpc_bw_expand(float *a, float *aw, float gamma, int p)
{
    int i;
    float gk;

    for(i=1,gk=gamma; i <= p; i++, gk *= gamma)
        aw[i] = a[i]*gk;
    return(0);
}

/*
    Name: lpc_clamp- Sort and ensure minimum separation in LSPs.
    Aliases: lpc_clamp
    Description:
        Ensure that all LSPs are ordered and separated
        by at least delta.  The algorithm isn't guaranteed
        to work, so it prints an error message when it fails
        to sort the LSPs properly.
    Inputs:
        w- lsp vector (order p, w[1..p])
        delta- the clamping factor
        p- order of lpc filter
    Outputs:
        w- the sorted and clamped lsps
    Returns: NULL
    See_Also:
    Includes:
        spbstd.h
        lpc.h
    Bugs: 
        Currently only supports 10 loops, which is too
        complex and perhaps unneccesary.

    Systems and Info. Science Lab
    Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved.
*
*/

#define MAX_LOOPS 10

int lpc_clamp(float *w, float delta, int p)
{
    int i,j,unsorted;
    float tmp,d,step1,step2;

    /* sort the LSPs- for 10 loops, complexity is approximately 150 p */  
    for (j=0,unsorted=TRUE; unsorted && (j < MAX_LOOPS); j++)
    {
        for(i=1,unsorted=FALSE; i < p; i++)
            if (w[i] > w[i+1])
            {
                tmp = w[i+1];
		w[i+1] = w[i];
		w[i] = tmp;
                unsorted = TRUE;
	    }
    }

    /* ensure minimum separation */
    if (!unsorted) 
    {
        for(j=0; j < MAX_LOOPS; j++)
        {
            for(i=1; i < p; i++)
            {
                if ((d = w[i+1]-w[i]) < delta)
                {
                    step1 = step2 = (delta-d)/2.0f;
                    if (i==1 && (w[i] < delta))
                    {
                        step1 = w[i]/2.0f;
					}else if (i > 1)
                    {
                        if ((tmp = w[i] - w[i-1]) < delta)
                            step1 = 0;
                        else if (tmp < 2*delta)
                            step1 = (tmp-delta)/2.0f;
					}
                    if (i==(p-1) && (w[i+1] > (1.0f-delta)))
                    {
                        step2 = (1-w[i+1])/2.0f;
					}else if (i < (p-1))
                    {
                        if ((tmp = w[i+2] - w[i+1]) < delta)
                            step2 = 0;
                        else if (tmp < 2*delta)
                            step2 = (tmp-delta)/2.0f;
					}
                    w[i] -= step1;
					w[i+1] += step2;
				}
			}
        }
    }
    return(0);
}

/*
    Name: lpc_schur- Schur recursion (autocorrelations to refl coef)
    Aliases: lpc_schur
    Description:
        Compute reflection coefficients from autocorrelations
        based on schur recursion.  Will also compute predictor
        parameters by calling lpc_refl2pred(3l) if necessary.
    Inputs:
        r- autocorrelation vector (r[0..p]).
        p- order of lpc filter.
    Outputs:
        a-     predictor parameters    (can be NULL)
        k_tmp- reflection coefficients (can be NULL)
    Returns:
        alphap- the minimum residual energy
    Includes:
        spbstd.h
        lpc.h
    See_Also:
        lpc_refl2pred(3l) in lpc.h or lpc(3l)

*/

static float y[LPC_ORD + 2];
static float y2[LPC_ORD + 2];
static float k[LPC_ORD + 2];
static float b[LPC_ORD + 2];
static float b1[LPC_ORD + 2];
static float a1[LPC_ORD + 2];

static float c0[LPC_ORD/2 + 1];
static float c1[LPC_ORD/2 + 1];
static float *c[2];

static float f0[LPC_ORD/2 + 1];
static float f1[LPC_ORD/2 + 1];
static float *f[2];

float lpc_schur(float *r, float *a, int p)
{
    int i,j;
    float temp,alphap;

    k[1] = -r[1]/r[0];
    alphap = r[0]*(1-SQR(k[1]));

    y2[1] = r[1];
    y2[2] = r[0]+k[1]*r[1];

    for(i=2; i <= p; i++)
    {
        y[1] = temp = r[i];

        for(j=1; j < i; j++)
        {
            y[j+1] = y2[j] + k[j]*temp;
            temp += k[j]*y2[j];
            y2[j] = y[j];
        }

        k[i] = -temp/y2[i];
        y2[i+1] = y2[i]+k[i]*temp;
        y2[i] = y[i];

        alphap *= 1-SQR(k[i]);
    }

    if (a != NULL)
    {
        (void)lpc_refl2pred(k,a,p);
    }

    return(alphap);
}

/* minimum LSP separation-- a global variable */
float lsp_delta = 0.0f;

/* private functions */
static float lsp_g(float x,float *c,int p2);
static int   lsp_roots(float *w,float **c,int p2);

#define DELTA  0.00781250f
#define BISECTIONS 4
    

/* LPC_PRED2LSP
      get LSP coeffs from the predictor coeffs
      Input:
         a- the predictor coefficients
         p- the predictor order
      Output:
         w- the lsp coefficients
   Reference:  Kabal and Ramachandran

*/

int lpc_pred2lsp(float *a,float *w,int p)
{
    int i,p2;

    p2 = p/2;
	c[0] = c0;
	c[1] = c1;
    c0[p2] = c1[p2] = 1.0;

    for(i=1; i <= p2; i++)
    {
        c0[p2-i] = (a[i] + a[p+1-i] - c0[p2+1-i]);
        c1[p2-i] = c1[p2+1-i] + a[i] - a[p+1-i];
    }
    c0[0] /= 2.0f;
    c1[0] /= 2.0f;

    i = lsp_roots(w,c,p2);

    /* ensure minimum separation and sort */
    (void)lpc_clamp(w,lsp_delta,p);

    return(i);
} /* LPC_PRED2LSP */

/* LPC_PRED2REFL
      get refl coeffs from the predictor coeffs
      Input:
         a- the predictor coefficients
         p- the predictor order
      Output:
         k- the reflection coefficients
   Reference:  Markel and Gray, Linear Prediction of Speech
*/

int lpc_pred2refl(float *a,float *k,int p)
{
    float e;
    int   i,j;

    /* equate temporary variables (b = a) */
    for(i=1; i <= p; i++)
        b[i] = a[i];

    /* compute reflection coefficients */
    for(i=p; i > 0; i--)
    {
        k[i] = b[i];
        e = 1 - SQR(k[i]);
        for(j=1; j < i; j++)
            b1[j] = b[j];
        for(j=1; j < i; j++)
            b[j] = (b1[j] - k[i]*b1[i-j])/e;
    }
    return(0);
}
/* LPC_LSP2PRED
      get predictor coefficients from the LSPs
   Synopsis: lpc_lsp2pred(w,a,p)
      Input:
         w- the LSPs
         p- the predictor order
      Output:
         a- the predictor coefficients
   Reference:  Kabal and Ramachandran
*/

int lpc_lsp2pred(float *w,float *a,int p)
{
    int i,j,k,p2;
    float c[2];


    /* ensure minimum separation and sort */
    (void)lpc_clamp(w,lsp_delta,p);

    p2 = p/2;
	f[0] = f0;
	f[1] = f1;
    f[0][0] = f[1][0] = 1.0;
    f[0][1] = (float)-2.0f*cosf(w[1]*PI);
    f[1][1] = (float)-2.0f*cosf(w[2]*PI);

    k = 3;

    for(i=2; i <= p2; i++)
    {
        c[0] = (float)-2.0f*cosf(w[k++]*PI);
        c[1] = (float)-2.0f*cosf(w[k++]*PI);
        f[0][i] = f[0][i-2];
        f[1][i] = f[1][i-2];

        for(j = i; j >= 2; j--)
        {
            f[0][j] += c[0]*f[0][j-1]+f[0][j-2];
            f[1][j] += c[1]*f[1][j-1]+f[1][j-2];
        }
        f[0][1] += c[0]*f[0][0];
        f[1][1] += c[1]*f[1][0];
    }

    for(i = p2; i > 0; i--)
    {
        f[0][i] += f[0][i-1];
        f[1][i] -= f[1][i-1];

        a[i] = 0.5f * (f[0][i]+f[1][i]);
        a[p+1-i] = 0.5f * (f[0][i]-f[1][i]);
    }

    return(0);
}

/* LPC_REFL2PRED
      get predictor coefficients from the reflection coeffs

      Input:
         k- the reflection coeffs
         p- the predictor order
      Output:
         a- the predictor coefficients
   Reference:  Markel and Gray, Linear Prediction of Speech
*/

int lpc_refl2pred(float *k,float *a,int p)
{
    int   i,j;

    for(i=1; i <= p; i++)
    {
        /* refl to a recursion */
        a[i] = k[i];
        for(j=1; j < i; j++)
            a1[j] = a[j];
        for(j=1; j < i; j++)
            a[j] = a1[j] + k[i]*a1[i-j];
    }
    return(0);
} /* LPC_REFL2PRED */

/* G - compute the value of the Chebychev series
                sum c_k T_k(x) = x b_1(x) - b_2(x) + c_0
                b_k(x) = 2x b_{k+1}(x) - b_{k+2}(x) + c_k 
*/
static float lsp_g(float x,float *c,int p2)
{
    int i;
    float b[3];

    b[1] = b[2] = 0.0;

    for(i=p2; i > 0; i--)
    {
        b[0] = 2.0f * x * b[1] - b[2] + c[i];
        b[2] = b[1];
        b[1] = b[0];
    }
    b[0] = x*b[1]-b[2]+c[0];
    return(b[0]);
} /* G */

/* LSP_ROOTS
        - find the roots of the two polynomials G_1(x) and G_2(x)
          the first root corresponds to G_1(x)
          compute the inverse cos (and these are the LSFs) 
*/
static int lsp_roots(float *w,float **c,int p2)
{
    int i,k;
    float x,x0,x1,y,*ptr,g0,g1;

    w[0] = 0.0f;

    ptr = c[0];
    x = 1.0f;
    g0 = lsp_g(x,ptr,p2);

    for(k=1,x = 1.0f-DELTA; x > -DELTA-1.0f; x -= DELTA)
    {
        /* Search for a zero crossing */
        if (g0*(g1 = lsp_g(x,ptr,p2)) <= 0.0f)
        {
            /* Search Incrementally using bisection */
            x0 = x+DELTA;
            x1 = x;

            for(i=0; i < BISECTIONS; i++)
            {
                x = (x0+x1)/2.0f;
                y = lsp_g(x,ptr,p2);

                if(y*g0 < 0.0f)
                {
                    x1 = x;
                    g1 = y;
                }
                else
                {
                    x0 = x;
                    g0 = y;
                }
            }
            /* Linear interpolate */
            x = (g1*x0-g0*x1)/(g1-g0);

            /* Evaluate the LSF */
            w[k] = (float)acosf(x)/PI;

            ptr = c[k % 2];
            k++;
            if (k > 2*p2)
                return(0);
            g1 = lsp_g(x,ptr,p2);
        }
        g0 = g1;
    }
    return(1);
} /* LSP_ROOTS */

