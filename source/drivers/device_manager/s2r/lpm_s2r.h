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


#define LPM_SRAM_S2R_SIZE_IN_BYTES (773U)

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
    0xe3010270U,     0xe3084278U,     0xe3440101U,     0xe3404f30U, \
    0xeb00004eU,     0xe30102e7U,     0xe3440101U,     0xeb00004bU, \
    0xe5140278U,     0xe2000c0fU,     0xe3500c0bU,     0x1a000002U, \
    0xe30102fdU,     0xe3440101U,     0xeb000070U,     0xe3010300U, \
    0xe3440101U,     0xeb000041U,     0xe3030490U,     0xe30b1c5aU, \
    0xe34608efU,     0xe34d1172U,     0xe3092008U,     0xe3442300U, \
    0xe5820000U,     0xe5821004U,     0xe3092008U,     0xe3442301U, \
    0xe5820000U,     0xe5821004U,     0xe3012008U,     0xe3402450U, \
    0xe5820000U,     0xe5821004U,     0xe3092008U,     0xe3402450U, \
    0xe5820000U,     0xe30a0064U,     0xe3400f30U,     0xe5821004U, \
    0xe30d2468U,     0xe3402f30U,     0xe5901000U,     0xe3811801U, \
    0xe5801000U,     0xe5901048U,     0xe3811c01U,     0xe5801048U, \
    0xe5901078U,     0xe3811c01U,     0xe5801078U,     0xe3a01051U, \
    0xe5920000U,     0xe3800001U,     0xe5820000U,     0xe59200fcU, \
    0xe3800001U,     0xe58200fcU,     0xe59200e8U,     0xe3c00c07U, \
    0xe58200e8U,     0xe5940000U,     0xe7cf0411U,     0xe5840000U, \
    0xe3010294U,     0xe3440101U,     0xeb000010U,     0xe30102bbU, \
    0xe3440101U,     0xeb00000dU,     0xeafffffeU,     0x00000000U, \
    0xe3033490U,     0xe0800701U,     0xe30b1c5aU,     0xe34638efU, \
    0xe34d1172U,     0xe301c008U,     0xe301200cU,     0xe780300cU, \
    0xe7801002U,     0xe12fff1eU,     0x00000000U,     0x00000000U, \
    0xe92d4010U,     0xe300400cU,     0xe3424b30U,     0xeb000027U, \
    0xe3010308U,     0xe3440101U,     0xe5d01000U,     0xe3510000U, \
    0x1a000003U,     0xe3a01003U,     0xe3a02001U,     0xe5841000U, \
    0xe5c02000U,     0xe5941000U,     0xe201207fU,     0xe5842000U, \
    0xe5942008U,     0xe2022060U,     0xe3520060U,     0x1afffffbU, \
    0xe3a0200dU,     0xe504200cU,     0xe5841000U,     0xe5d01000U, \
    0xe3510000U,     0x1a000003U,     0xe3a01003U,     0xe3a02001U, \
    0xe5841000U,     0xe5c02000U,     0xe5940000U,     0xe200107fU, \
    0xe5841000U,     0xe5941008U,     0xe2011060U,     0xe3510060U, \
    0x1afffffbU,     0xe3a0100aU,     0xe504100cU,     0xe5840000U, \
    0xe8bd8010U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0xe92d4830U,     0xe5d01000U,     0xe3510000U,     0x0a000016U, \
    0xe300200cU,     0xe3013308U,     0xe3422b30U,     0xe3443101U, \
    0xe3a0c003U,     0xe3a0e001U,     0xe5d34000U,     0xe3540000U, \
    0x1a000001U,     0xe582c000U,     0xe5c3e000U,     0xe5924000U, \
    0xe204507fU,     0xe5825000U,     0xe5925008U,     0xe2055060U, \
    0xe3550060U,     0x1afffffbU,     0xe502100cU,     0xe5824000U, \
    0xe5f01001U,     0xe3510000U,     0x1affffeeU,     0xe8bd8830U, \
    0x5f6d704cU,     0x65746e65U,     0x74655272U,     0x69746e65U, \
    0x203a6e6fU,     0x65746e45U,     0x65722072U,     0x746e6574U, \
    0x006e6f69U,     0x5f6d704cU,     0x65746e65U,     0x74655272U, \
    0x69746e65U,     0x203a6e6fU,     0x20524444U,     0x65746572U, \
    0x6f69746eU,     0x6f64206eU,     0x4c00656eU,     0x655f6d70U, \
    0x7265746eU,     0x65746552U,     0x6f69746eU,     0x44203a6eU, \
    0x21656e6fU,     0x696f4720U,     0x7420676eU,     0x6177206fU, \
    0x6e207469U,     0x4c00776fU,     0x645f6d70U,     0x6e457264U, \
    0x52726574U,     0x6e657465U,     0x6e6f6974U,     0x00504c00U, \
    0x34524444U,     0x00000000U\
} /* 773 bytes */

#endif
