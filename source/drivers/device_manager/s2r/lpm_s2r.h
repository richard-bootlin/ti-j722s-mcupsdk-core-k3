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


#define LPM_SRAM_S2R_SIZE_IN_BYTES (607U)

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
    0xf241b580U,     0xf2c4200cU,     0xf0001001U,     0xf000f811U, \
    0xf000f847U,     0xe7fff84dU,     0xf241e7ffU,     0xf2c42031U, \
    0xf0001001U,     0xe7fff805U,     0x0000e7feU,     0x00000000U, \
    0x9000b081U,     0x4770b001U,     0x00000000U,     0x00000000U, \
    0xb084b580U,     0xf88d9003U,     0x9803100bU,     0x100bf89dU, \
    0x3081eb00U,     0x98019001U,     0x0108f241U,     0xf2434401U, \
    0xf6c64090U,     0xf00000efU,     0x9801f813U,     0x010cf241U, \
    0xf64b4401U,     0xf2cd405aU,     0xf0001072U,     0xb004f809U, \
    0x0000bd80U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0x9001b082U,     0x98019100U,     0x60089900U,     0x4770b002U, \
    0x00004770U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0xb08cb580U,     0xf88cf000U,     0x4086f04fU,     0x21029001U, \
    0xf7ff9103U,     0x9801ffbdU,     0xf7ff2106U,     0xf04fffb9U, \
    0x9002608aU,     0xf7ff2100U,     0x9802ffb3U,     0xf7ff9903U, \
    0xf24affafU,     0xf6c00064U,     0x90047030U,     0xf888f000U, \
    0x900b9904U,     0xf440980bU,     0x900b3080U,     0xf7ff980bU, \
    0xf24affc7U,     0xf6c000acU,     0x90057030U,     0xf878f000U, \
    0x900b9905U,     0xf440980bU,     0x900b7080U,     0xf7ff980bU, \
    0xf24affb7U,     0xf6c000dcU,     0x90067030U,     0xf868f000U, \
    0x900b9906U,     0xf440980bU,     0x900b7080U,     0xf7ff980bU, \
    0xf24dffa7U,     0xf6c04068U,     0x90077030U,     0xf858f000U, \
    0x900b9907U,     0xf040980bU,     0x900b0001U,     0xf7ff980bU, \
    0xf24dff97U,     0xf6c05064U,     0x90087030U,     0xf848f000U, \
    0x900b9908U,     0xf040980bU,     0x900b0001U,     0xf7ff980bU, \
    0xf24dff87U,     0xf6c05050U,     0x90097030U,     0xf838f000U, \
    0x900b9909U,     0xf420980bU,     0x900b60e0U,     0xf7ff980bU, \
    0xf248ff77U,     0xf6c02078U,     0x900a7030U,     0xf828f000U, \
    0x900b990aU,     0xf420980bU,     0x900b407fU,     0xf440980bU, \
    0x900b40a2U,     0xf7ff980bU,     0xb00cff63U,     0x0000bd80U, \
    0xf24cb580U,     0xf2c00010U,     0xf2430068U,     0xf6c64190U, \
    0xf7ff01efU,     0xf24cff55U,     0xf2c00014U,     0xf64b0068U, \
    0xf2cd415aU,     0xf7ff1172U,     0xbd80ff4bU,     0x00000000U, \
    0x9000b081U,     0x68009800U,     0x4770b001U,     0x5f6d704cU, \
    0x65746e65U,     0x74655272U,     0x69746e65U,     0x203a6e6fU, \
    0x65746e45U,     0x65722072U,     0x746e6574U,     0x0a6e6f69U, \
    0x6d704c00U,     0x746e655fU,     0x65527265U,     0x746e6574U, \
    0x3a6e6f69U,     0x6e6f4420U,     0x47202165U,     0x676e696fU, \
    0x206f7420U,     0x74696177U,     0x776f6e20U,     0x00000a20U\
} /* 607 bytes */

#endif
