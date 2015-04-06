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

/*  Coeff.c: filter coefficient file */
/*                                   */
/*                                                                  */
/* (C) 1993,1994  Texas Instruments                                 */
/*                                                                  */
#include	<stdio.h>
#include "melp.h"

/* Butterworth lowpass filter	*/
/* numerator */
float lpf_num[LPF_ORD+1] = {
      0.00105165f,
      0.00630988f,
      0.01577470f,
      0.02103294f,
      0.01577470f,
      0.00630988f,
      0.00105165f};
/* denominator */
float lpf_den[LPF_ORD+1] = { 
      1.00000000f,
     -2.97852993f,
      4.13608100f,
     -3.25976428f,
      1.51727884f,
     -0.39111723f,
      0.04335699f};

/* Butterworth bandpass filters */
float bpf_num[NUM_BANDS*(BPF_ORD+1)] = {
      0.00002883f,
      0.00017296f,
      0.00043239f,
      0.00057652f,
      0.00043239f,
      0.00017296f,
      0.00002883f,
      0.00530041f,
      0.00000000f,
     -0.01590123f,
      0.00000000f,
      0.01590123f,
      0.00000000f,
     -0.00530041f,
      0.03168934f,
      0.00000000f,
     -0.09506803f,
     -0.00000000f,
      0.09506803f,
     -0.00000000f,
     -0.03168934f,
      0.03168934f,
      0.00000000f,
     -0.09506803f,
      0.00000000f,
      0.09506803f,
      0.00000000f,
     -0.03168934f,
      0.00105165f,
     -0.00630988f,
      0.01577470f,
     -0.02103294f,
      0.01577470f,
     -0.00630988f,
      0.00105165f};

float bpf_den[NUM_BANDS*(BPF_ORD+1)] = {
      1.00000000,
     -4.48456301f, 
      8.52900508f, 
     -8.77910797f, 
      5.14764268f, 
     -1.62771478f, 
      0.21658286f, 
      1.00000000f, 
     -4.42459751f, 
      8.79771496f, 
     -9.95335557f, 
      6.75320305f, 
     -2.60749972f, 
      0.45354593f, 
      1.00000000f, 
     -1.84699031f, 
      2.63060194f, 
     -2.21638838f, 
      1.57491237f, 
     -0.62291281f, 
      0.19782519f, 
      1.00000000f, 
      1.84699031f, 
      2.63060194f, 
      2.21638838f, 
      1.57491237f, 
      0.62291281f, 
      0.19782519f, 
      1.00000000f, 
      2.97852993f, 
      4.13608100f, 
      3.25976428f, 
      1.51727884f, 
      0.39111723f, 
      0.04335699f};

/* Hamming window coefficents */
float win_cof[LPC_FRAME] = {
      0.08000000f, 
      0.08022927f, 
      0.08091685f, 
      0.08206205f, 
      0.08366374f, 
      0.08572031f, 
      0.08822971f, 
      0.09118945f, 
      0.09459658f, 
      0.09844769f, 
      0.10273895f, 
      0.10746609f, 
      0.11262438f, 
      0.11820869f, 
      0.12421345f, 
      0.13063268f, 
      0.13745997f, 
      0.14468852f, 
      0.15231113f, 
      0.16032019f, 
      0.16870773f, 
      0.17746538f, 
      0.18658441f, 
      0.19605574f, 
      0.20586991f, 
      0.21601716f, 
      0.22648736f, 
      0.23727007f, 
      0.24835455f, 
      0.25972975f, 
      0.27138433f, 
      0.28330667f, 
      0.29548489f, 
      0.30790684f, 
      0.32056016f, 
      0.33343221f, 
      0.34651017f, 
      0.35978102f, 
      0.37323150f, 
      0.38684823f, 
      0.40061762f, 
      0.41452595f, 
      0.42855935f, 
      0.44270384f, 
      0.45694532f, 
      0.47126959f, 
      0.48566237f, 
      0.50010932f, 
      0.51459603f, 
      0.52910806f, 
      0.54363095f, 
      0.55815022f, 
      0.57265140f, 
      0.58712003f, 
      0.60154169f, 
      0.61590200f, 
      0.63018666f, 
      0.64438141f, 
      0.65847211f, 
      0.67244472f, 
      0.68628531f, 
      0.69998007f, 
      0.71351536f, 
      0.72687769f, 
      0.74005374f, 
      0.75303036f, 
      0.76579464f, 
      0.77833383f, 
      0.79063545f, 
      0.80268724f, 
      0.81447716f, 
      0.82599349f, 
      0.83722473f, 
      0.84815969f, 
      0.85878747f, 
      0.86909747f, 
      0.87907943f, 
      0.88872338f, 
      0.89801971f, 
      0.90695917f, 
      0.91553283f, 
      0.92373215f, 
      0.93154896f, 
      0.93897547f, 
      0.94600427f, 
      0.95262835f, 
      0.95884112f, 
      0.96463638f, 
      0.97000835f, 
      0.97495168f, 
      0.97946144f, 
      0.98353313f, 
      0.98716270f, 
      0.99034653f, 
      0.99308145f, 
      0.99536472f, 
      0.99719408f, 
      0.99856769f, 
      0.99948420f, 
      0.99994268f, 
      0.99994268f, 
      0.99948420f, 
      0.99856769f, 
      0.99719408f, 
      0.99536472f, 
      0.99308145f, 
      0.99034653f, 
      0.98716270f, 
      0.98353313f, 
      0.97946144f, 
      0.97495168f, 
      0.97000835f, 
      0.96463638f, 
      0.95884112f, 
      0.95262835f, 
      0.94600427f, 
      0.93897547f, 
      0.93154896f, 
      0.92373215f, 
      0.91553283f, 
      0.90695917f, 
      0.89801971f, 
      0.88872338f, 
      0.87907943f, 
      0.86909747f, 
      0.85878747f, 
      0.84815969f, 
      0.83722473f, 
      0.82599349f, 
      0.81447716f, 
      0.80268724f, 
      0.79063545f, 
      0.77833383f, 
      0.76579464f, 
      0.75303036f, 
      0.74005374f, 
      0.72687769f, 
      0.71351536f, 
      0.69998007f, 
      0.68628531f, 
      0.67244472f, 
      0.65847211f, 
      0.64438141f, 
      0.63018666f, 
      0.61590200f, 
      0.60154169f, 
      0.58712003f, 
      0.57265140f, 
      0.55815022f, 
      0.54363095f, 
      0.52910806f, 
      0.51459603f, 
      0.50010932f, 
      0.48566237f, 
      0.47126959f, 
      0.45694532f, 
      0.44270384f, 
      0.42855935f, 
      0.41452595f, 
      0.40061762f, 
      0.38684823f, 
      0.37323150f, 
      0.35978102f, 
      0.34651017f, 
      0.33343221f, 
      0.32056016f, 
      0.30790684f, 
      0.29548489f, 
      0.28330667f, 
      0.27138433f, 
      0.25972975f, 
      0.24835455f, 
      0.23727007f, 
      0.22648736f, 
      0.21601716f, 
      0.20586991f, 
      0.19605574f, 
      0.18658441f, 
      0.17746538f, 
      0.16870773f, 
      0.16032019f, 
      0.15231113f, 
      0.14468852f, 
      0.13745997f, 
      0.13063268f, 
      0.12421345f, 
      0.11820869f, 
      0.11262438f, 
      0.10746609f, 
      0.10273895f, 
      0.09844769f, 
      0.09459658f, 
      0.09118945f, 
      0.08822971f, 
      0.08572031f, 
      0.08366374f, 
      0.08206205f, 
      0.08091685f, 
      0.08022927f, 
      0.08000000f};

/* Bandpass filter coeffients */
float bp_cof[NUM_BANDS][MIX_ORD+1] = {
{
     -0.00000000f, 
     -0.00302890f, 
     -0.00701117f, 
     -0.01130619f, 
     -0.01494082f, 
     -0.01672586f, 
     -0.01544189f, 
     -0.01006619f, 
      0.00000000f, 
      0.01474923f, 
      0.03347158f, 
      0.05477206f, 
      0.07670890f, 
      0.09703726f, 
      0.11352143f, 
      0.12426379f, 
      0.12799355f, 
      0.12426379f, 
      0.11352143f, 
      0.09703726f, 
      0.07670890f, 
      0.05477206f, 
      0.03347158f, 
      0.01474923f, 
      0.00000000f, 
     -0.01006619f, 
     -0.01544189f, 
     -0.01672586f, 
     -0.01494082f, 
     -0.01130619f, 
     -0.00701117f, 
     -0.00302890f, 
     -0.00000000
}, 
{
     -0.00000000f, 
     -0.00249420f, 
     -0.00282091f, 
      0.00257679f, 
      0.01451271f, 
      0.02868120f, 
      0.03621179f, 
      0.02784469f, 
     -0.00000000f, 
     -0.04079870f, 
     -0.07849207f, 
     -0.09392213f, 
     -0.07451087f, 
     -0.02211575f, 
      0.04567473f, 
      0.10232715f, 
      0.12432599f, 
      0.10232715f, 
      0.04567473f, 
     -0.02211575f, 
     -0.07451087f, 
     -0.09392213f, 
     -0.07849207f, 
     -0.04079870f, 
     -0.00000000f, 
      0.02784469f, 
      0.03621179f, 
      0.02868120f, 
      0.01451271f, 
      0.00257679f, 
     -0.00282091f, 
     -0.00249420f, 
     -0.00000000
}, 
{
     -0.00000000f, 
     -0.00231491f, 
      0.00990113f, 
      0.02086129f, 
     -0.00000000f, 
     -0.03086123f, 
     -0.02180695f, 
      0.00769333f, 
     -0.00000000f, 
     -0.01127245f, 
      0.04726837f, 
      0.10106105f, 
     -0.00000000f, 
     -0.17904543f, 
     -0.16031428f, 
      0.09497157f, 
      0.25562154f, 
      0.09497157f, 
     -0.16031428f, 
     -0.17904543f, 
     -0.00000000f, 
      0.10106105f, 
      0.04726837f, 
     -0.01127245f, 
     -0.00000000f, 
      0.00769333f, 
     -0.02180695f, 
     -0.03086123f, 
     -0.00000000f, 
      0.02086129f, 
      0.00990113f, 
     -0.00231491f, 
     -0.00000000
}, 
{
     -0.00000000f, 
      0.00231491f, 
      0.00990113f, 
     -0.02086129f, 
      0.00000000f, 
      0.03086123f, 
     -0.02180695f, 
     -0.00769333f, 
     -0.00000000f, 
      0.01127245f, 
      0.04726837f, 
     -0.10106105f, 
      0.00000000f, 
      0.17904543f, 
     -0.16031428f, 
     -0.09497157f, 
      0.25562154f, 
     -0.09497157f, 
     -0.16031428f, 
      0.17904543f, 
      0.00000000f, 
     -0.10106105f, 
      0.04726837f, 
      0.01127245f, 
     -0.00000000f, 
     -0.00769333f, 
     -0.02180695f, 
      0.03086123f, 
      0.00000000f, 
     -0.02086129f, 
      0.00990113f, 
      0.00231491f, 
     -0.00000000
}, 
{
      0.00000000f, 
      0.00554149f, 
     -0.00981750f, 
      0.00856805f, 
     -0.00000000f, 
     -0.01267517f, 
      0.02162277f, 
     -0.01841647f, 
      0.00000000f, 
      0.02698425f, 
     -0.04686914f, 
      0.04150730f, 
     -0.00000000f, 
     -0.07353666f, 
      0.15896026f, 
     -0.22734513f, 
      0.25346255f, 
     -0.22734513f, 
      0.15896026f, 
     -0.07353666f, 
     -0.00000000f, 
      0.04150730f, 
     -0.04686914f, 
      0.02698425f, 
      0.00000000f, 
     -0.01841647f, 
      0.02162277f, 
     -0.01267517f, 
     -0.00000000f, 
      0.00856805f, 
     -0.00981750f, 
      0.00554149f, 
      0.00000000}};
/* Triangle pulse dispersion filter */
float disp_cof[DISP_ORD+1] = {
     -0.17304259f, 
     -0.01405709f, 
      0.01224406f, 
      0.11364226f, 
      0.00198199f, 
      0.00000658f, 
      0.04529633f, 
     -0.00092027f, 
     -0.00103078f, 
      0.02552787f, 
     -0.06339257f, 
     -0.00122031f, 
      0.01412525f, 
      0.24325127f, 
     -0.01767043f, 
     -0.00018612f, 
      0.05869485f, 
     -0.00327456f, 
      0.00607395f, 
      0.02753924f, 
     -0.03351673f, 
      0.00602189f, 
      0.01436539f, 
      0.82854582f, 
      0.00033165f, 
     -0.00360180f, 
      0.07343483f, 
     -0.00518645f, 
      0.01298488f, 
      0.02928440f, 
     -0.01989405f, 
      0.01216758f, 
      0.01180979f, 
     -0.38924775f, 
      0.00720325f, 
     -0.01154561f, 
      0.08426287f, 
     -0.00355720f, 
      0.02151233f, 
      0.02968464f, 
     -0.01247640f, 
      0.01854666f, 
      0.00076184f, 
     -0.07749640f, 
      0.01244697f, 
     -0.02721777f, 
      0.07266098f, 
      0.00472008f, 
      0.03526439f, 
      0.02674603f, 
     -0.00744038f, 
      0.02582623f, 
      0.00019707f, 
     -0.02825247f, 
      0.01720989f, 
     -0.06004292f, 
     -0.07076744f, 
      0.00914347f, 
      0.06082730f, 
      0.01805528f, 
     -0.00318634f, 
      0.03444110f, 
      0.00026302f, 
     -0.01053809f, 
      0.02165922f};





