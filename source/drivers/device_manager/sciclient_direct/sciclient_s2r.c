/*
 *  Copyright (c) Texas Instruments Incorporated 2026
 *  All rights reserved.
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
 */

/**
 * \file sciclient_s2r.c
 *
 * \brief This file contains the code which load DDR retention and PMIC S2R code
 * into SRAM
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <string.h>
#include <stdbool.h>
#include <types/short_types.h>
#include <sciclient.h>
#include <kernel/dpl/ClockP.h>
#include "csl_arm_r5.h"
#include "sciclient_s2r.h"
#include "lpm_s2r.h"
#include "dbg_uart.c"

#define __maybe_unused __attribute__((__unused__))

static void asm_function(void);

#define CSL_COMPUTE_CLUSTER0_MSMC_CFGS0_BASE (0x6e000000UL)
#define CSL_MSMC_CFGS0_CACHE_CTRL (0x00001000U)

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define CSL_MSMC_CFGS0_RD(r) CSL_REG32_RD_OFF(CSL_COMPUTE_CLUSTER0_MSMC_CFGS0_BASE, r)
#define CSL_MSMC_CFGS0_WR(r, v) CSL_REG32_WR_OFF(CSL_COMPUTE_CLUSTER0_MSMC_CFGS0_BASE, r, v)

/**
 * Address in BT SRAM where to load the code that will take care of the
 * DDR retention and PMIC S2R configuration.
 */
#define SCICLIENT_S2R_SRAM_CODE_ADDRESS 0x41011000

/* Address to point the sp register in SRAM */
#define SCICLIENT_S2R_SRAM_STACK_POINTER 0x41014000

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

u32 lpm_sram_s2r[] = LPM_SRAM_S2R;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static void S2R_cleanAllDCache(void)
{
    uint32_t set, way;
    uint32_t numSets = CSL_armR5CacheGetNumSets();
    uint32_t numWays = CSL_armR5CacheGetNumWays();

dbg_line(__func__);
    for (way = 0; way < numWays ; way++)
    {
        for (set = 0; set < numSets; set++)
        {
            CSL_armR5CacheCleanDcacheSetWay(set, way);
        }
    }
}

__maybe_unused static bool S2R_cleanL3Cache(void)
{
    uint64_t t;

    CSL_MSMC_CFGS0_WR(CSL_MSMC_CFGS0_CACHE_CTRL, 0);

    t = ClockP_getTimeUsec();
    while (CSL_MSMC_CFGS0_RD(CSL_MSMC_CFGS0_CACHE_CTRL) != 0)
    {
	    /*
	     * timeout set to 100ms
	     * Maximum value measured for J7200 is 250us
	     */
	    if (ClockP_getTimeUsec() - t > 100000)
		    return false;
    }

    return true;
}

void S2R_goRetention(void)
{
#if !defined(SOC_J784S4) && !defined(SOC_J722S)
    /* Cleaning L3 cache doesn't work yet on j784s4 / J722s */
    if (!S2R_cleanL3Cache())
    {
        dbg_line("Failed to clean L3 cache\n");
        Sciclient_pmDomainReset(DEVGRP_00, SCICLIENT_SERVICE_WAIT_FOREVER);
    }
#endif

    S2R_cleanAllDCache();

    /* load DDR retention code and PMIC S2R into SRAM */
    memcpy((void*)SCICLIENT_S2R_SRAM_CODE_ADDRESS,
           (const void*)&lpm_sram_s2r[0],
           LPM_SRAM_S2R_SIZE_IN_BYTES);

    dbg_line("Suspending");

    int i=0;
    uint8_t *code, *sram;
    code = (uint8_t *)&lpm_sram_s2r[0];
    sram = (uint8_t *)SCICLIENT_S2R_SRAM_CODE_ADDRESS;
    for(i = 0; i < LPM_SRAM_S2R_SIZE_IN_BYTES; i++) {
            if (*code != *sram)
                    dbg_line("error copying to SRAM");
            code +=1;
            sram +=1;
    }

    for (unsigned long j = 0; j < 5 ; j++) { /* 0x80 for full mem */
            uint32_t cksum = 0;
            uint32_t addr = 0x80000000UL + 0x1000000UL * j;
            for (unsigned long i = 0; i < 0x1000000; i+= (sizeof(uint32_t)))
                    cksum ^= *(uint32_t *)(addr + i);
            dump_val2("cksum=from ", addr, cksum);
    }

    for (unsigned long j = 0; j < 1 ; j++) {
            uint32_t addr = 0x80000000UL + 0x1000000UL * j;
	    dump_HEX((uint32_t *)(addr), 0x1000);
    }

    asm_function();

    /* Never reach this point */
}

static void asm_function(void)
{
    asm volatile("mov sp, %0" : : "r" (SCICLIENT_S2R_SRAM_STACK_POINTER));
    asm volatile("blx %0" : : "r" (SCICLIENT_S2R_SRAM_CODE_ADDRESS));
}
