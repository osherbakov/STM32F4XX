/*

2.4 kbps MELP Proposed Federal Standard speech coder

Fixed-point C code, version 1.0

Copyright (c) 1998, Texas Instruments, Inc.

Texas Instruments has intellectual property rights on the MELP
algorithm.	The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).

The fixed-point version of the voice codec Mixed Excitation Linear
Prediction (MELP) is based on specifications on the C-language software
simulation contained in GSM 06.06 which is protected by copyright and
is the property of the European Telecommunications Standards Institute
(ETSI). This standard is available from the ETSI publication office
tel. +33 (0)4 92 94 42 58. ETSI has granted a license to United States
Department of Defense to use the C-language software simulation contained
in GSM 06.06 for the purposes of the development of a fixed-point
version of the voice codec Mixed Excitation Linear Prediction (MELP).
Requests for authorization to make other use of the GSM 06.06 or
otherwise distribute or modify them need to be addressed to the ETSI
Secretariat fax: +33 493 65 47 16.

*/

#ifndef _MATHHALF_H_
#define _MATHHALF_H_

#include "arm_math.h"
#include "arm_const_structs.h"
#include "constant.h"


/* addition */

//int16_t	add(int16_t var1, int16_t var2);                     /* 1 ops */
//static __inline int16_t add(int32_t var1, int32_t var2)
//{
//	int32_t res = var1 + var2;
//	res = res > SW_MAX ? SW_MAX : res;
//	res = res < SW_MIN? SW_MIN : res;
//	return (int16_t) res;
//}
//#define add(a,b)  __QADD16(a,b)
static __inline int16_t add(int16_t a, int16_t b) {return (int16_t) __QADD16(a,b);}

//int16_t	sub(int16_t var1, int16_t var2);                     /* 1 ops */
//#define sub(a,b) ((a)-(b))
//#define sub(a,b)  __QSUB16(a,b)
static __inline int16_t sub(int16_t a, int16_t b) {return (int16_t) __QSUB16(a,b);}


//int32_t	L_add(int32_t L_var1, int32_t L_var2);                 /* 2 ops */
#define L_add(a,b) (((int32_t)a)+((int32_t)b))
//#define L_add(a,b)  __QADD(a,b)

//int32_t	L_sub(int32_t L_var1, int32_t L_var2);                 /* 2 ops */
#define L_sub(a,b) (((int32_t)a)-((int32_t)b))
//#define L_sub(a,b)  __QSUB(a,b)

/* multiplication */

//int16_t	mult(int16_t var1, int16_t var2);                    /* 1 ops */
#define mult(a,b)	(((int16_t)(a)*(int16_t)(b))>>15)

//int32_t	L_mult(int16_t var1, int16_t var2);                  /* 1 ops */
#define L_mult(a,b)	((((int32_t)a)*(b)) + (((int32_t)a)*(b)))

/* arithmetic shifts */

//int16_t	shl(int16_t var1, int16_t var2);                     /* 1 ops */
//static __inline int16_t shl(int32_t a, int32_t b){ return (int16_t)(b >= 0 ? a<<b : a >> -b);}
static int16_t	shr(int16_t var1, int16_t var2);                     /* 1 ops */
static __inline int16_t shl(int16_t var1, int16_t var2)
{
	int16_t	swOut;
	if (var2 < 0) return shr(var1, -var2);
	if (var2 >= 15) 
		return (var1 >= 0) ? SW_MAX : SW_MIN;
	swOut = var1 << var2;
	if(swOut>>var2 != var1)
		return (var1 >= 0) ? SW_MAX : SW_MIN; 
	return (swOut);
}

//int16_t	shr(int16_t var1, int16_t var2);                     /* 1 ops */
//static __inline int16_t shr(int16_t a, int16_t b){ return (int16_t)(b >= 0 ? a>>b : shl(a,-b));}
static __inline int16_t shr(int16_t var1, int16_t var2)
{
	if (var2 < 0) return shl(var1, -var2);
	if (var2 >= 15) return (var1 >= 0) ? 0 : -1;
	return (var1>>var2);
}


static int32_t	L_shr(int32_t L_var1, int16_t var2);                  /* 2 ops */
//static __inline int32_t L_shr(int32_t a, int32_t b){ return b >= 0 ? a>>b : a << -b;}
static __inline int32_t L_shl(int32_t var1, int16_t var2)
{
	int32_t	L_Out;
	if (var2 < 0) return L_shr(var1, -var2);
	if (var2 >= 31)
		return (var1 >= 0) ? LW_MAX : LW_MIN;
	L_Out = var1 << var2;
	if(L_Out>>var2 != var1) 
		return (var1 >= 0) ? LW_MAX : LW_MIN; 
	return (L_Out);
}

int32_t	L_shl(int32_t L_var1, int16_t var2);                  /* 2 ops */
//static __inline int32_t L_shl(int32_t a, int32_t b){ return b >= 0 ? a<<b : a >> -b;}
static __inline int32_t L_shr(int32_t var1, int16_t var2)
{
	if (var2 < 0) return L_shl(var1, -var2);
	if (var2 >= 31) return (var1 >= 0) ? 0 : -1;
	return (var1>>var2);
}

//int16_t	shift_r(int16_t var, int16_t var2);                  /* 2 ops */
static __inline int16_t shift_r(int32_t a, int32_t b){ return (int16_t)(b >= 0 ? a<<b : (a+(1<<(-b-1))) >> -b);}

//int32_t	L_shift_r(int32_t L_var, int16_t var2);               /* 3 ops */
static __inline int32_t L_shift_r(int32_t a, int32_t b){ return b >= 0 ? a<<b : (a+(1<<(-b-1))) >> -b;}

/* absolute value  */

//int16_t	abs_s(int16_t var1);                                   /* 1 ops */
#define abs_s(a)  ((a) >= 0 ? (a) : -(a))

//int32_t	L_abs(int32_t var1);                                    /* 3 ops */
#define L_abs(a)  ((a) >= 0 ? (a) : -(a))

/* multiply accumulate	*/
//int32_t	L_mac(int32_t L_var3, int16_t var1, int16_t var2);	  /* 1 op */
static __inline int32_t L_mac(int32_t acc, int32_t a, int32_t b) { return acc + a*b + a*b; }

//int32_t	L_msu(int32_t L_var3, int16_t var1, int16_t var2);   /* 1 op */
static __inline int32_t L_msu(int32_t acc, int32_t a, int32_t b) { return acc - a*b - a*b; }


/* negation  */

//int16_t	negate(int16_t var1);                                  /* 1 ops */
#define negate(a)	(-(a))

//int32_t	L_negate(int32_t L_var1);                               /* 2 ops */
#define L_negate(a)	(-(a))

/* Accumulator manipulation */

//int32_t	L_deposit_l(int16_t var1);                             /* 1 ops */
#define L_deposit_l(a)		((int32_t)a)

//int32_t	L_deposit_h(int16_t var1);                             /* 1 ops */
#define L_deposit_h(a)		(((int32_t)a)<<16)

//int16_t	extract_l(int32_t L_var1);                              /* 1 ops */
#define extract_l(a) ((int16_t)(a))

//int16_t	extract_h(int32_t L_var1);                              /* 1 ops */
#define extract_h(a) ((int16_t)((a)>>16))

/* Round */

// int16_t	round_l(int32_t L_var1);                                  /* 1 ops */
//static __inline int16_t round_l(int32_t L_var1)
//{
//	int32_t	L_Prod;
//	L_Prod = L_var1 + 0x00008000L;                         /* round MSP */
//	L_Prod = (L_var1 > 0) && (L_Prod < 0) ? LW_MAX : L_Prod;
//	return((int16_t)(L_Prod>>16));
//}
#define round_l(a) (__QADD(a, 0x00008000L)>>16)

/* Normalization */

//int16_t	norm_l(int32_t L_var1);                                /* 30 ops */
//static __inline int16_t norm_l(int32_t L_var1)
//{
//	int16_t cnt = 0;
//	if(L_var1 == 0) return 0;
//	if(L_var1 > 0) while(L_var1 < 0x40000000L){L_var1<<=1; cnt++;}
//	else while(L_var1 > (int32_t)0xC0000000L){L_var1<<=1; cnt++;}
//	return cnt;
//}
#define norm_l(a) ((a) >=0 ? __CLZ(a)-1:__CLZ(-(a))-1)

//int16_t	norm_s(int16_t var1);                                 /* 15 ops */
//static __inline int16_t norm_s(int32_t L_var1)
//{
//	int16_t cnt = 0;
//	if(L_var1 == 0) return 0;
//	L_var1 <<= 16;
//	if(L_var1 > 0) while(L_var1 < 0x40000000L){L_var1<<=1; cnt++;}
//	else while(L_var1 > (int32_t)0xC0000000L){L_var1<<=1; cnt++;}
//	return cnt;
//}
#define norm_s(a) norm_l((a)<<16)

/* Division */

//int16_t	divide_s(int16_t var1, int16_t var2);               /* 18 ops */
static __inline int16_t divide_s(int16_t var1, int16_t var2)
{
	if (var1 < 0 || var2 < 0 || var1 > var2)return 0;
	if (var1 == var2) return SW_MAX;
	return (int16_t) ((0x00008000L * (int32_t) var1) / (int32_t) var2);
}


/* -------------------------------------------------------------------------- */
/* 40-Bit Routines....added by Andre 11/23/99 */

/* new 40 bits basic operators */

// Word40 L40_add(Word40 acc, int32_t L_var1);
static __inline Word40 L40_add(Word40 acc, int32_t L_var1)
{
	acc = acc + (Word40)L_var1;
//	acc = acc > MAX_40 ? MAX_40 : acc;
//	acc = acc < MIN_40 ? MIN_40 : acc;
	return(acc);
}
//Word40 L40_sub(Word40 acc, int32_t L_var1);
static __inline Word40 L40_sub(Word40 acc, int32_t L_var1)
{
	acc = acc - (Word40)L_var1;
//	acc = acc > MAX_40 ? MAX_40 : acc;
//	acc = acc < MIN_40 ? MIN_40 : acc;
	return(acc);
}
// Word40 L40_mac(Word40 acc, int16_t var1, int16_t var2);
static __inline Word40 L40_mac(Word40 acc, int16_t var1, int16_t var2)
{
	acc = acc + (int32_t)var1 * var2 + (int32_t)var1 * var2;
//	acc = acc > MAX_40 ? MAX_40 : acc;
//	acc = acc < MIN_40 ? MIN_40 : acc;
	return(acc);
}
// Word40 L40_msu(Word40 acc, int16_t var1, int16_t var2);
static __inline Word40 L40_msu(Word40 acc, int16_t var1, int16_t var2)
{
	acc = acc - (int32_t)var1 * var2 - (int32_t)var1 * var2;
//	acc = acc > MAX_40 ? MAX_40 : acc;
//	acc = acc < MIN_40 ? MIN_40 : acc;
	return(acc);
}

//Word40 L40_shl(Word40 acc, int16_t var1);
static __inline Word40 L40_shl(Word40 a, int16_t b){ return b >= 0 ? a<<b : a >> -b;}

//Word40 L40_shr(Word40 acc, int16_t var1);
static __inline Word40 L40_shr(Word40 a, int16_t b){ return b >= 0 ? a>>b : a << -b;}

//Word40 L40_negate(Word40 acc);
#define  L40_negate(a)  (-(a))

//int16_t norm32(Word40 acc);
//static __inline int16_t norm32(Word40 acc)
//{
//	int16_t	cnt = 0;
//	if(acc == 0) return 0;
//	if(acc > 0) {
//		while(acc > (Word40)MAX_32){acc>>=1; cnt--;}while(acc < (Word40)0x40000000L){acc<<=1; cnt++;}
//	}else {
//		while(acc < (Word40)MIN_32){acc>>=1; cnt--;}while(acc > (Word40)0xC0000000L){acc<<=1; cnt++;}
//	}
//	return(cnt);
//}
#define norm32(a)  norm_l((int32_t)(a))

// int32_t L_sat32(Word40 acc);
#define L_sat32(a)  ((int32_t)a)


#endif

