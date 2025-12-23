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


#define LPM_SRAM_S2R_SIZE_IN_BYTES (2272U)

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
    0xe3033490U,     0xe30bec5aU,     0xe34638efU,     0xe34de172U, \
    0xe3010008U,     0xe3025030U,     0xe34400f0U,     0xe30cc010U, \
    0xe3405299U,     0xe340c068U,     0xe5803000U,     0xe3a02801U, \
    0xe580e004U,     0xe3090008U,     0xe3440300U,     0xe5803000U, \
    0xe580e004U,     0xe3000210U,     0xe58c3000U,     0xe3400299U, \
    0xe58ce004U,     0xe59543dcU,     0xe59513dcU,     0xe59060f0U, \
    0xe00227a4U,     0xe3a04401U,     0xe0041321U,     0xe1822006U, \
    0xe1821001U,     0xe58010f0U,     0xe59010ecU,     0xe3811401U, \
    0xe58010ecU,     0xe59510d0U,     0xe3c11001U,     0xe58510d0U, \
    0xe5951320U,     0xe3c11c02U,     0xe5851320U,     0xe5955000U, \
    0xe3150002U,     0x0a000003U,     0xe59010e8U,     0xe3811c01U, \
    0xe3811801U,     0xe58010e8U,     0xe3150004U,     0x0a000003U, \
    0xe59010e8U,     0xe3811c01U,     0xe3811401U,     0xe58010e8U, \
    0xe3052008U,     0xe3a01000U,     0xe3402011U,     0xe5823000U, \
    0xe3a03945U,     0xe582e004U,     0xe5831000U,     0xe58d1004U, \
    0xe59d1004U,     0xe3510063U,     0x8a000005U,     0xe59d1004U, \
    0xe2811001U,     0xe58d1004U,     0xe59d1004U,     0xe3510064U, \
    0x3afffff9U,     0xe5931000U,     0xe3042004U,     0xe3402011U, \
    0xe3811c01U,     0xe5831000U,     0xe592107cU,     0xe3110080U, \
    0x0afffffcU,     0xe59c1010U,     0xe3811102U,     0xe58c1010U, \
    0xe59210bcU,     0xe3811001U,     0xe58210bcU,     0xe59210bcU, \
    0xe3110001U,     0x0afffffcU,     0xe5921000U,     0xe3110080U, \
    0x0afffffcU,     0xe3a01000U,     0xe3a03945U,     0xe58210bcU, \
    0xe30f2efcU,     0xe5931000U,     0xe34f2fffU,     0xe0011002U, \
    0xe30f2fffU,     0xe34f20f0U,     0xe5831000U,     0xe5901024U, \
    0xe0011002U,     0xe3a02051U,     0xe5801024U,     0xe5901000U, \
    0xe7de1c12U,     0xe5801000U,     0xe5901284U,     0xe3110b01U, \
    0x0afffffcU,     0xe3a03b01U,     0xe3071f7fU,     0xe3042f4fU, \
    0xe580328cU,     0xe5903024U,     0xe0033001U,     0xe1530002U, \
    0x1afffffbU,     0xe3a00048U,     0xeb0000f8U,     0xe3a00086U, \
    0xe3a01003U,     0xeb00013dU,     0xe3a00048U,     0xeb0000f3U, \
    0xe3a0005aU,     0xeb00015dU,     0xe3500000U,     0x0a00007eU, \
    0xe1a04000U,     0xe3100001U,     0x0a000017U,     0xe3a0005bU, \
    0xeb000156U,     0xe1a05000U,     0xe3100001U,     0x0a000004U, \
    0xe3a0005cU,     0xeb000151U,     0xe1a01000U,     0xe3a0005cU, \
    0xeb00012aU,     0xe3150002U,     0x0a000004U,     0xe3a0005dU, \
    0xeb00014aU,     0xe1a01000U,     0xe3a0005dU,     0xeb000123U, \
    0xe3150004U,     0x0a000004U,     0xe3a0005eU,     0xeb000143U, \
    0xe1a01000U,     0xe3a0005eU,     0xeb00011cU,     0xe3140002U, \
    0x0a000017U,     0xe3a0005fU,     0xeb00013cU,     0xe1a05000U, \
    0xe3100001U,     0x0a000004U,     0xe3a00060U,     0xeb000137U, \
    0xe1a01000U,     0xe3a00060U,     0xeb000110U,     0xe3150002U, \
    0x0a000004U,     0xe3a00061U,     0xeb000130U,     0xe1a01000U, \
    0xe3a00061U,     0xeb000109U,     0xe3150004U,     0x0a000004U, \
    0xe3a00062U,     0xeb000129U,     0xe1a01000U,     0xe3a00062U, \
    0xeb000102U,     0xe3140004U,     0x0a00000cU,     0xe3a00063U, \
    0xeb000122U,     0xe1a05000U,     0xe3100008U,     0x0a000004U, \
    0xe3a00064U,     0xeb00011dU,     0xe1a01000U,     0xe3a00064U, \
    0xeb0000f6U,     0xe3a00063U,     0xe1a01005U,     0xeb0000f3U, \
    0xe3140008U,     0x0a000004U,     0xe3a00065U,     0xeb000113U, \
    0xe1a01000U,     0xe3a00065U,     0xeb0000ecU,     0xe3140010U, \
    0x0a000004U,     0xe3a00066U,     0xeb00010cU,     0xe1a01000U, \
    0xe3a00066U,     0xeb0000e5U,     0xe3140020U,     0x0a000004U, \
    0xe3a00067U,     0xeb000105U,     0xe1a01000U,     0xe3a00067U, \
    0xeb0000deU,     0xe3140040U,     0x0a000004U,     0xe3a00068U, \
    0xeb0000feU,     0xe1a01000U,     0xe3a00068U,     0xeb0000d7U, \
    0xe3140080U,     0x0a00001aU,     0xe3a00069U,     0xeb0000f7U, \
    0xe1a04000U,     0xe3100010U,     0x0a000004U,     0xe3a0006aU, \
    0xeb0000f2U,     0xe1a01000U,     0xe3a0006aU,     0xeb0000cbU, \
    0xe3140020U,     0x0a000004U,     0xe3a0006bU,     0xeb0000ebU, \
    0xe1a01000U,     0xe3a0006bU,     0xeb0000c4U,     0xe3140040U, \
    0x0a000004U,     0xe3a0006cU,     0xeb0000e4U,     0xe1a01000U, \
    0xe3a0006cU,     0xeb0000bdU,     0xe3a00069U,     0xe1a01004U, \
    0xeb0000baU,     0xe3a0005aU,     0xeb0000dcU,     0xe3a0004cU, \
    0xeb00006eU,     0xe3a00065U,     0xe3a01002U,     0xeb0000b3U, \
    0xe3a00048U,     0xeb000069U,     0xe3a00085U,     0xe3a01080U, \
    0xeb0000aeU,     0xe3a0004cU,     0xeb000064U,     0xe3a00085U, \
    0xe3a01080U,     0xeb0000a9U,     0xe3a00048U,     0xeb00005fU, \
    0xe3a00034U,     0xe3a010caU,     0xeb0000a4U,     0xe3a00048U, \
    0xeb00005aU,     0xe3a00064U,     0xe3a01008U,     0xeb00009fU, \
    0xe3a00048U,     0xeb000055U,     0xe3a00050U,     0xe3a010f7U, \
    0xeb00009aU,     0xe3a00048U,     0xeb000050U,     0xe3a00051U, \
    0xe3a0103fU,     0xeb000095U,     0xe3a00048U,     0xeb00004bU, \
    0xe3a0004fU,     0xe3a010ffU,     0xeb000090U,     0xe3a0004cU, \
    0xeb000046U,     0xe3a00032U,     0xe3a01001U,     0xeb00008bU, \
    0xe3a0004cU,     0xeb000041U,     0xe3a00033U,     0xe3a01001U, \
    0xeb000086U,     0xe3a0004cU,     0xeb00003cU,     0xe3a0003dU, \
    0xeb0000a6U,     0xe1a04000U,     0xe3805002U,     0xe3a0004cU, \
    0xeb000036U,     0xe3a0003dU,     0xe1a01005U,     0xeb00007bU, \
    0xe3a0004cU,     0xe3844006U,     0xeb000030U,     0xe3a0003dU, \
    0xe1a01004U,     0xeb000075U,     0xe3a0004cU,     0xeb00002bU, \
    0xe20510fbU,     0xe3a0003dU,     0xeb000070U,     0xe3a0004cU, \
    0xeb000026U,     0xe3a0003dU,     0xe1a01004U,     0xeb00006bU, \
    0xe3a00048U,     0xeb000021U,     0xe3a000cbU,     0xe3a010baU, \
    0xeb000066U,     0xe3a00048U,     0xeb00001cU,     0xe3a00086U, \
    0xe3a01000U,     0xeb000061U,     0xe28dd008U,     0xeafffffeU, \
    0xe1a01211U,     0xe3a02601U,     0xe7903002U,     0xe0033001U, \
    0xe1530001U,     0x1afffffbU,     0xe12fff1eU,     0x00000000U, \
    0xe92d4010U,     0xe3a0c601U,     0xe1a0e000U,     0xe7be400cU, \
    0xe1c42312U,     0xe1822311U,     0xe1a01311U,     0xe58e2000U, \
    0xe790200cU,     0xe0022001U,     0xe1520001U,     0x1afffffbU, \
    0xe8bd8010U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0xe30118e0U,     0xe3441101U,     0xe5d12000U,     0xe1520000U, \
    0x012fff1eU,     0xe30c30f8U,     0xe5c10000U,     0xe3443301U, \
    0xe3002010U,     0xe3442212U,     0xe5931000U,     0xe3811701U, \
    0xe5831000U,     0xe5931004U,     0xe3811701U,     0xe5831004U, \
    0xe5921000U,     0xe3811002U,     0xe5821000U,     0xe5921000U, \
    0xe3811008U,     0xe5821000U,     0xe5921094U,     0xe3811902U, \
    0xe5821094U,     0xe5921080U,     0xe3110001U,     0x0afffffcU, \
    0xe59210a0U,     0xe3a03007U,     0xe7c71013U,     0xe3a03009U, \
    0xe58210a0U,     0xe59210a4U,     0xe7c71013U,     0xe58210a4U, \
    0xe59210a8U,     0xe7c71013U,     0xe30330fcU,     0xe34f3fffU, \
    0xe58210a8U,     0xe5921094U,     0xe0011003U,     0xe3073f7fU, \
    0xe34f3fffU,     0xe5821094U,     0xe5921088U,     0xe7cf101fU, \
    0xe5821088U,     0xe5921084U,     0xe0011003U,     0xe3a030b4U, \
    0xe5821084U,     0xe5921098U,     0xe7c91013U,     0xe5821098U, \
    0xe5921094U,     0xe3811902U,     0xe5821094U,     0xe5921080U, \
    0xe3110001U,     0x0afffffcU,     0xe592109cU,     0xe304c040U, \
    0xe7c9101fU,     0xe1813000U,     0xe3a000a1U,     0xe3a0109bU, \
    0xe582309cU,     0xe582c084U,     0xea000000U,     0x00000000U, \
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
    0xe12fff1eU,     0x00000000U,     0x00000001U,     0x00000000U\
} /* 2272 bytes */

#endif
