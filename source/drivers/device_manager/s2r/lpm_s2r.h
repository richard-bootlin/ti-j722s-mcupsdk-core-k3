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


#define LPM_SRAM_S2R_SIZE_IN_BYTES (531U)

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
    0x1086f241U,     0x1001f2c4U,     0xf882f000U,     0x10fdf241U, \
    0x1001f2c4U,     0xf87cf000U,     0x0010f24cU,     0x4290f243U, \
    0x0068f2c0U,     0x02eff6c6U,     0x60101d01U,     0x405af64bU, \
    0x1072f2cdU,     0xf2496001U,     0xf2c40108U,     0x600a3100U, \
    0xf2496048U,     0xf2c40108U,     0x600a3101U,     0xf2416048U, \
    0xf2c00108U,     0x600a4150U,     0xf2496048U,     0xf2c00108U, \
    0x600a4150U,     0x0264f24aU,     0x7230f6c0U,     0xf24d6048U, \
    0xf6c04168U,     0x68107130U,     0x3080f440U,     0x6c906010U, \
    0x7080f440U,     0x6f906490U,     0x7080f440U,     0xf2486790U, \
    0x68082278U,     0x7230f6c0U,     0x0001f040U,     0xf8d16008U, \
    0xf04000fcU,     0xf8c10001U,     0xf8d100fcU,     0xf42000e8U, \
    0xf8c160e0U,     0x215100e8U,     0xf3616810U,     0x6010200fU, \
    0x10aaf241U,     0x1001f2c4U,     0xf822f000U,     0x10d1f241U, \
    0x1001f2c4U,     0xf81cf000U,     0x0000e7feU,     0x00000000U, \
    0x3081eb00U,     0x4290f243U,     0x415af64bU,     0x02eff6c6U, \
    0x1172f2cdU,     0x0c08f241U,     0x030cf241U,     0x200cf840U, \
    0x477050c1U,     0x00000000U,     0x00000000U,     0x00000000U, \
    0x210cb580U,     0xf6c27802U,     0xb1aa3130U,     0x0c03f04fU, \
    0xc000f8c1U,     0xe000f8d1U,     0x037ff00eU,     0x688b600bU, \
    0x0360f003U,     0xd1fa2b60U,     0x2c0cf841U,     0xe000f8c1U, \
    0x2f01f810U,     0xd1eb2a00U,     0x60082003U,     0xf0006808U, \
    0x600a027fU,     0xf002688aU,     0x2a600260U,     0x220dd1faU, \
    0x2c0cf841U,     0x60082203U,     0x6808600aU,     0x027ff000U, \
    0x688a600aU,     0x0260f002U,     0xd1fa2a60U,     0xf841220aU, \
    0x60082c0cU,     0x704cbd80U,     0x6e655f6dU,     0x52726574U, \
    0x6e657465U,     0x6e6f6974U,     0x6e45203aU,     0x20726574U, \
    0x65746572U,     0x6f69746eU,     0x704c006eU,     0x6e655f6dU, \
    0x52726574U,     0x6e657465U,     0x6e6f6974U,     0x4444203aU, \
    0x65722052U,     0x746e6574U,     0x206e6f69U,     0x656e6f64U, \
    0x6d704c00U,     0x746e655fU,     0x65527265U,     0x746e6574U, \
    0x3a6e6f69U,     0x6e6f4420U,     0x47202165U,     0x676e696fU, \
    0x206f7420U,     0x74696177U,     0x776f6e20U,     0x6d704c00U, \
    0x7264645fU,     0x65746e45U,     0x74655272U,     0x69746e65U, \
    0x00006e6fU\
} /* 531 bytes */

#endif
