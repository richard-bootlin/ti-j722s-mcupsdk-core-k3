/******************************************************************************
 *  @file  csl_arm_r5.S
 *
 *  @brief
 *   Implementation file for the ARM R5 module CSL-FL.
 *
 *   Contains the different control command and status query functions definitions
 *
 *   \par
 *   ============================================================================
 *   @n   (C) Copyright 2017-2026, Texas Instruments, Inc.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the
 *     distribution.
 *
 *     Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
 *   DATA, OR PROFITS;  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*==============================================================================
*   uint32_t CSL_armR5CacheGetNumSets(void )
*==============================================================================*/
    .global CSL_armR5CacheGetNumSets
CSL_armR5CacheGetNumSets:
    DMB
    MRC       p15, #1, r0, c0, c0, #0   /* Read Cache Size ID Register */
    LSL       r0, r0, #(31-27)
    LSR       r0, r0, #(31-27+13)       /* Extract bits from [27:13] */
    ADD       r0, r0, #1
    BX        lr

/*==============================================================================
*   uint32_t CSL_armR5CacheGetNumWays(void )
*==============================================================================*/
    .global CSL_armR5CacheGetNumWays
CSL_armR5CacheGetNumWays:
    DMB
    MRC       p15, #1, r0, c0, c0, #0   /* Read Cache Size ID Register */
    LSL       r0, r0, #(31-12)
    LSR       r0, r0, #(31-12+3)        /* Extract bits from [12:3] */
    ADD       r0, r0, #1
    BX        lr

/*==============================================================================
*   void CSL_armR5CacheCleanDcacheSetWay( uint32_t set, uint32_t way )
*==============================================================================*/
    .global CSL_armR5CacheCleanDcacheSetWay
CSL_armR5CacheCleanDcacheSetWay:
    DMB     /* Ensure all previous memory accesses are completed */
    AND     r1, r1, #0x3
    LSL     r0, #(32-9)
    LSR     r0, #(32-9)                     /* set &= 0x1FF */
    LSL     r1, r1, #30
    ORR     r0, r1, r0, LSL #5
    MCR     p15, #0, r0, c7, c10, #2
    DSB
    BX      lr

