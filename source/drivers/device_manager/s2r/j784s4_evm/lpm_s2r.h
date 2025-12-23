/*
 *  Copyright (C) 2021 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

/**
*  \file lpm_s2r.h
*
*  \brief File containing the Binary in a C array.
*
*/

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#ifndef LPM_S2R_H_
#define LPM_S2R_H_


#define LPM_SRAM_S2R_SIZE_IN_BYTES (2408U)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

#define LPM_SRAM_S2R { \
    0xe3a00000U,     0xe3a01c02U,     0xe2511001U,     0xf57ff05fU, \
    0xee072f5aU,     0xee072f56U,     0xf57ff04fU,     0x1afffff9U, \
    0xe2800001U,     0xe3500004U,     0x1afffff5U,     0xe24dd008U, \
    0xe3031490U,     0xe30b3c5aU,     0xe34618efU,     0xe34d3172U, \
    0xe3010008U,     0xe30c6010U,     0xe34400f0U,     0xe305a008U, \
    0xe3406068U,     0xe300c000U,     0xe5801000U,     0xe301e8a0U, \
    0xe5803004U,     0xe3090008U,     0xe3440300U,     0xe30f9efcU, \
    0xe340a011U,     0xe340c299U,     0xe344e101U,     0xe34f9fffU, \
    0xe5801000U,     0xe3a05000U,     0xe5803004U,     0xe3a02601U, \
    0xe5861000U,     0xe3a01000U,     0xe1a08003U,     0xe5863004U, \
    0xe1a0600cU,     0xe302040cU,     0xe7b67000U,     0xe5966000U, \
    0xe3a00801U,     0xe59c4300U,     0xe00077a7U,     0xe3a00401U, \
    0xe0006326U,     0xe1874004U,     0xe3a00c21U,     0xe1a0700cU, \
    0xe1844006U,     0xe1a0600cU,     0xe58c4300U,     0xe59c42fcU, \
    0xe3844401U,     0xe58c42fcU,     0xe7b64000U,     0xe3020350U, \
    0xe3c44001U,     0xe5864000U,     0xe7b74000U,     0xe3c44c02U, \
    0xe5874000U,     0xe5176320U,     0xe3160002U,     0x0a000004U, \
    0xe3000100U,     0xe59c42f8U,     0xe3400001U,     0xe1844000U, \
    0xe58c42f8U,     0xe3160004U,     0x0a000004U,     0xe3000100U, \
    0xe59c42f8U,     0xe3400100U,     0xe1844000U,     0xe58c42f8U, \
    0xe0817101U,     0xe3030490U,     0xe34608efU,     0xe08e6187U, \
    0xe58a0000U,     0xe596b004U,     0xe58a8004U,     0xe78b5002U, \
    0xe58d5004U,     0xe59d7004U,     0xe3570063U,     0x8a000005U, \
    0xe59d7004U,     0xe2877001U,     0xe58d7004U,     0xe59d7004U, \
    0xe3570064U,     0x3afffff9U,     0xe1a0400bU,     0xe7b47002U, \
    0xe3873c01U,     0xe596700cU,     0xe5843000U,     0xe7973002U, \
    0xe3130080U,     0x0afffffcU,     0xe5963014U,     0xe5967010U, \
    0xe5934000U,     0xe1a00007U,     0xe3844102U,     0xe5834000U, \
    0xe7b03002U,     0xe3833001U,     0xe5803000U,     0xe7970002U, \
    0xe3100001U,     0x0afffffcU,     0xe5966008U,     0xe7960002U, \
    0xe3100080U,     0x0afffffcU,     0xe2811001U,     0xe7875002U, \
    0xe7bb0002U,     0xe3510004U,     0xe0813101U,     0xe0000009U, \
    0xe79ec183U,     0xe58b0000U,     0x1affffa4U,     0xe30f5fffU, \
    0xe3a01000U,     0xe34f50f0U,     0xe3a02051U,     0xe3a03b01U, \
    0xe3077f7fU,     0xe3046f4fU,     0xe0810101U,     0xe79e4180U, \
    0xe5940234U,     0xe0000005U,     0xe5840234U,     0xe5940210U, \
    0xe7de0c12U,     0xe5840210U,     0xe5940494U,     0xe3100b01U, \
    0x0afffffcU,     0xe584349cU,     0xe5940234U,     0xe0000007U, \
    0xe1500006U,     0x1afffffbU,     0xe2811001U,     0xe3510004U, \
    0x1affffecU,     0xeb0000cdU,     0xe3a00086U,     0xe3a01003U, \
    0xeb000112U,     0xeb0000c9U,     0xe3a0005aU,     0xeb000133U, \
    0xe3500000U,     0x0a00007eU,     0xe1a04000U,     0xe3100001U, \
    0x0a000017U,     0xe3a0005bU,     0xeb00012cU,     0xe1a05000U, \
    0xe3100001U,     0x0a000004U,     0xe3a0005cU,     0xeb000127U, \
    0xe1a01000U,     0xe3a0005cU,     0xeb000100U,     0xe3150002U, \
    0x0a000004U,     0xe3a0005dU,     0xeb000120U,     0xe1a01000U, \
    0xe3a0005dU,     0xeb0000f9U,     0xe3150004U,     0x0a000004U, \
    0xe3a0005eU,     0xeb000119U,     0xe1a01000U,     0xe3a0005eU, \
    0xeb0000f2U,     0xe3140002U,     0x0a000017U,     0xe3a0005fU, \
    0xeb000112U,     0xe1a05000U,     0xe3100001U,     0x0a000004U, \
    0xe3a00060U,     0xeb00010dU,     0xe1a01000U,     0xe3a00060U, \
    0xeb0000e6U,     0xe3150002U,     0x0a000004U,     0xe3a00061U, \
    0xeb000106U,     0xe1a01000U,     0xe3a00061U,     0xeb0000dfU, \
    0xe3150004U,     0x0a000004U,     0xe3a00062U,     0xeb0000ffU, \
    0xe1a01000U,     0xe3a00062U,     0xeb0000d8U,     0xe3140004U, \
    0x0a00000cU,     0xe3a00063U,     0xeb0000f8U,     0xe1a05000U, \
    0xe3100008U,     0x0a000004U,     0xe3a00064U,     0xeb0000f3U, \
    0xe1a01000U,     0xe3a00064U,     0xeb0000ccU,     0xe3a00063U, \
    0xe1a01005U,     0xeb0000c9U,     0xe3140008U,     0x0a000004U, \
    0xe3a00065U,     0xeb0000e9U,     0xe1a01000U,     0xe3a00065U, \
    0xeb0000c2U,     0xe3140010U,     0x0a000004U,     0xe3a00066U, \
    0xeb0000e2U,     0xe1a01000U,     0xe3a00066U,     0xeb0000bbU, \
    0xe3140020U,     0x0a000004U,     0xe3a00067U,     0xeb0000dbU, \
    0xe1a01000U,     0xe3a00067U,     0xeb0000b4U,     0xe3140040U, \
    0x0a000004U,     0xe3a00068U,     0xeb0000d4U,     0xe1a01000U, \
    0xe3a00068U,     0xeb0000adU,     0xe3140080U,     0x0a00001aU, \
    0xe3a00069U,     0xeb0000cdU,     0xe1a04000U,     0xe3100010U, \
    0x0a000004U,     0xe3a0006aU,     0xeb0000c8U,     0xe1a01000U, \
    0xe3a0006aU,     0xeb0000a1U,     0xe3140020U,     0x0a000004U, \
    0xe3a0006bU,     0xeb0000c1U,     0xe1a01000U,     0xe3a0006bU, \
    0xeb00009aU,     0xe3140040U,     0x0a000004U,     0xe3a0006cU, \
    0xeb0000baU,     0xe1a01000U,     0xe3a0006cU,     0xeb000093U, \
    0xe3a00069U,     0xe1a01004U,     0xeb000090U,     0xe3a0005aU, \
    0xeb0000b2U,     0xeb000045U,     0xe3a00085U,     0xe3a01080U, \
    0xeb00008aU,     0xeb000041U,     0xe3a00034U,     0xe3a010caU, \
    0xeb000086U,     0xeb00003dU,     0xe3a00064U,     0xe3a01008U, \
    0xeb000082U,     0xeb000039U,     0xe3a00050U,     0xe3a010f7U, \
    0xeb00007eU,     0xeb000035U,     0xe3a00051U,     0xe3a0103fU, \
    0xeb00007aU,     0xeb000031U,     0xe3a0004fU,     0xe3a010ffU, \
    0xeb000076U,     0xeb00002dU,     0xe3a00036U,     0xe3a01001U, \
    0xeb000072U,     0xeb000029U,     0xe3a0003dU,     0xeb000093U, \
    0xe3804020U,     0xeb000025U,     0xe3a0003dU,     0xe1a01004U, \
    0xeb00006aU,     0xeb000021U,     0xe3a000cbU,     0xe3a010baU, \
    0xeb000066U,     0xeb00001dU,     0xe3a00086U,     0xe3a01000U, \
    0xeb000062U,     0xe28dd008U,     0xeafffffeU,     0x00000000U, \
    0xe1a01211U,     0xe3a02601U,     0xe7903002U,     0xe0033001U, \
    0xe1530001U,     0x1afffffbU,     0xe12fff1eU,     0x00000000U, \
    0xe92d4010U,     0xe3a0c601U,     0xe1a0e000U,     0xe7be400cU, \
    0xe1c42312U,     0xe1822311U,     0xe1a01311U,     0xe58e2000U, \
    0xe790200cU,     0xe0022001U,     0xe1520001U,     0x1afffffbU, \
    0xe8bd8010U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0xe3010968U,     0xe3440101U,     0xe5d01000U,     0xe3510000U, \
    0x112fff1eU,     0xe30c30f8U,     0xe3a01001U,     0xe3443301U, \
    0xe3002010U,     0xe3442212U,     0xe5c01000U,     0xe5930000U, \
    0xe3800701U,     0xe5830000U,     0xe5930004U,     0xe3800701U, \
    0xe5830004U,     0xe5920000U,     0xe3800002U,     0xe5820000U, \
    0xe5920000U,     0xe3800008U,     0xe5820000U,     0xe5920094U, \
    0xe3800902U,     0xe5820094U,     0xe5920080U,     0xe3100001U, \
    0x0afffffcU,     0xe59200a0U,     0xe3a01007U,     0xe7c70011U, \
    0xe3a01009U,     0xe58200a0U,     0xe59200a4U,     0xe7c70011U, \
    0xe58200a4U,     0xe59200a8U,     0xe7c70011U,     0xe30310fcU, \
    0xe34f1fffU,     0xe58200a8U,     0xe5920094U,     0xe0000001U, \
    0xe3071f7fU,     0xe34f1fffU,     0xe5820094U,     0xe5920088U, \
    0xe7cf001fU,     0xe5820088U,     0xe5920084U,     0xe0000001U, \
    0xe3a010b4U,     0xe5820084U,     0xe5920098U,     0xe7c90011U, \
    0xe5820098U,     0xe5920094U,     0xe3800902U,     0xe5820094U, \
    0xe5920080U,     0xe3100001U,     0x0afffffcU,     0xe592309cU, \
    0xe3a00048U,     0xe304c040U,     0xe3a0109bU,     0xe7c93010U, \
    0xe3a000a1U,     0xe582309cU,     0xe582c084U,     0xeaffffffU, \
    0xe92d4010U,     0xe3002024U,     0xe3442212U,     0xe5923000U, \
    0xe3130a01U,     0x1afffffcU,     0xe3a03002U,     0xe3a04000U, \
    0xe3a0c010U,     0xe5823074U,     0xe5923080U,     0xe3833c06U, \
    0xe3c33002U,     0xe5823080U,     0xe5923080U,     0xe383e003U, \
    0xe582e080U,     0xe5923000U,     0xe3130010U,     0x0afffffcU, \
    0xe3540000U,     0x0a000002U,     0xe5821078U,     0xe582c004U, \
    0xea000000U,     0xe5820078U,     0xe2844001U,     0xe3540002U, \
    0x1afffff3U,     0xe5920000U,     0xe3100004U,     0x0afffffcU, \
    0xe5920000U,     0xe5820004U,     0xe8bd8010U,     0x00000000U, \
    0xe3001024U,     0xe3a02000U,     0xe3441212U,     0xe5913000U, \
    0xe3130a01U,     0x0a000002U,     0xe2522001U,     0x1afffffaU, \
    0xea000001U,     0xe3520001U,     0x0a00003fU,     0xe3a02001U, \
    0xe5812074U,     0xe5912080U,     0xe3822c06U,     0xe3c22002U, \
    0xe5812080U,     0xe5912080U,     0xe3823001U,     0xe3a02000U, \
    0xe5813080U,     0xe5913000U,     0xe3130010U,     0x1a000002U, \
    0xe2522001U,     0x1afffffaU,     0xea000001U,     0xe3520001U, \
    0x0a00002dU,     0xe3a03010U,     0xe3a02000U,     0xe5810078U, \
    0xe5813004U,     0xe5910000U,     0xe3100004U,     0x1a000002U, \
    0xe2522001U,     0x1afffffaU,     0xea000001U,     0xe3520001U, \
    0x0a000021U,     0xe5910000U,     0xe3a02002U,     0xe5810004U, \
    0xe5910080U,     0xe7ca0492U,     0xe3a02001U,     0xe5810080U, \
    0xe5812074U,     0xe5910080U,     0xe3802003U,     0xe3a00000U, \
    0xe5812080U,     0xe5912000U,     0xe3120008U,     0x1a000003U, \
    0xe2500001U,     0x1afffffaU,     0xe3a00000U,     0xea000002U, \
    0xe3500001U,     0x0a00000cU,     0xe2600000U,     0xe5912078U, \
    0xe2600000U,     0xe5913000U,     0xe3130004U,     0x1a000002U, \
    0xe2500001U,     0x1afffffaU,     0xea000001U,     0xe3500001U, \
    0x0a000001U,     0xe5910000U,     0xe5810004U,     0xe6ef0072U, \
    0xe12fff1eU,     0x00000001U,     0x00000000U,     0x00000001U, \
    0x02990000U,     0x00014000U,     0x00014004U,     0x00014080U, \
    0x000140c0U,     0x0068c020U,     0x0068c080U,     0x0124f800U, \
    0x3f917fa0U,     0x3f917fa0U,     0x029b0000U,     0x00014010U, \
    0x00014014U,     0x00014090U,     0x000140d0U,     0x0069a020U, \
    0x0069a080U,     0x0124f800U,     0x3f917fa0U,     0x3f917fa0U, \
    0x029d0000U,     0x00014020U,     0x00014024U,     0x000140a0U, \
    0x000140e0U,     0x0069b020U,     0x0069b080U,     0x0124f800U, \
    0x3f917fa0U,     0x3f917fa0U,     0x029f0000U,     0x00014030U, \
    0x00014034U,     0x000140b0U,     0x000140f0U,     0x0069c020U, \
    0x0069c080U,     0x0124f800U,     0x3f917fa0U,     0x3f917fa0U, \
    0x00000000U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0x00000000U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0x00000000U,     0x00000000U\
} /* 2408 bytes */

#endif
