/*
 *  Copyright (c) Texas Instruments Incorporated 2023-2026
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
 * \file lpm_s2r.c
 *
 * \brief This file contains the code which is loaded to SRAM for S2R support.
 * This code set the DDR in retention mode and ask PMIC to powerdown the SOC.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdint.h>
#include <stdarg.h>
#include <ti/csl/soc.h>
#include <ti/drv/i2c/i2c.h>
#include <ti/csl/cslr_pmmc.h>
#if defined(SOC_J7200)
#include <ti/board/src/j7200_evm/include/board_cfg.h>
#include <ti/board/src/j7200_evm/include/board_pll.h>
#include <ti/board/src/j7200_evm/include/board_ddr.h>
#elif defined(SOC_J784S4)
#include <ti/board/src/j784s4_evm/include/board_cfg.h>
#include <ti/board/src/j784s4_evm/include/board_pll.h>
#include <ti/board/src/j784s4_evm/include/board_ddr.h>
#endif
#include <ti/drv/lpm/include/io_retention/dmsc_cm.h>
#include <ti/drv/pmic/src/pmic_fsm_priv.h>

/* ========================================================================== */
/*                               Debug Options                                */
/* ========================================================================== */

/**
 * Uncomment the define of DEBUG_SRAM_S2R enables the debug messages during
 * suspend sequence.
 * Additional debug messages can be added using the flag DEBUG_FULL_SRAM_S2R.
 * By default debug messages are printed on MCU_UART0, but they can be
 * redirected by enabling DEBUG_UART_MAIN flag.
 * A compilation is needed for each modification.
 */
//#define DEBUG_SRAM_S2R
//#define DEBUG_FULL_SRAM_S2R
//#define DEBUG_UART_MAIN

#ifdef DEBUG_UART_MAIN
#define UART_DEBUG_ADDR CSL_UART0_BASE
#else
#define UART_DEBUG_ADDR CSL_MCU_UART0_BASE
#endif

#if defined(DEBUG_SRAM_S2R) && defined(DEBUG_FULL_SRAM_S2R)
#define Lpm_debugFullPrintf Lpm_debugPrintf
#define Lpm_debugReadPmicA  Lpm_readPmicA
#define Lpm_debugReadPmicB  Lpm_readPmicB
#else
#define Lpm_debugFullPrintf(...) do {} while(0)
#define Lpm_debugReadPmicA(...)  do {} while(0)
#define Lpm_debugReadPmicB(...)  do {} while(0)
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define CSL_REG32_SET(r, m) CSL_REG32_WR(r, CSL_REG32_RD(r) | m)
#define CSL_REG32_CLR(r, m) CSL_REG32_WR(r, CSL_REG32_RD(r) & ~(m))

#define CSL_REG32_SET_OFF(o, r, m) CSL_REG32_WR_OFF(o, r, CSL_REG32_RD_OFF(o, r) | m)
#define CSL_REG32_CLR_OFF(o, r, m) CSL_REG32_WR_OFF(o, r, CSL_REG32_RD_OFF(o, r) & ~(m))

#define CSL_PLL0_CFG_RD(r)     CSL_REG32_RD_OFF(CSL_PLL0_CFG_BASE, r)
#define CSL_PLL0_CFG_WR(r, v)  CSL_REG32_WR_OFF(CSL_PLL0_CFG_BASE, r, v)
#define CSL_PLL0_CFG_SET(r, m) CSL_PLL0_CFG_WR(r, CSL_PLL0_CFG_RD(r) | m)
#define CSL_PLL0_CFG_CLR(r, m) CSL_PLL0_CFG_WR(r, CSL_PLL0_CFG_RD(r) & ~(m))

#define CSL_WKUP_CTRL_MMR0_CFG0_RD(r)     CSL_REG32_RD_OFF(CSL_WKUP_CTRL_MMR0_CFG0_BASE, r)
#define CSL_WKUP_CTRL_MMR0_CFG0_WR(r, v)  CSL_REG32_WR_OFF(CSL_WKUP_CTRL_MMR0_CFG0_BASE, r, v)
#define CSL_WKUP_CTRL_MMR0_CFG0_SET(r, m) CSL_WKUP_CTRL_MMR0_CFG0_WR(r, CSL_WKUP_CTRL_MMR0_CFG0_RD(r) | m)
#define CSL_WKUP_CTRL_MMR0_CFG0_CLR(r, m) CSL_WKUP_CTRL_MMR0_CFG0_WR(r, CSL_WKUP_CTRL_MMR0_CFG0_RD(r) & ~(m))

#define CSL_MCU_CTRL_MMR0_CFG0_RD(r)      CSL_REG32_RD_OFF(CSL_MCU_CTRL_MMR0_CFG0_BASE, r)
#define CSL_MCU_CTRL_MMR0_CFG0_WR(r, v)   CSL_REG32_WR_OFF(CSL_MCU_CTRL_MMR0_CFG0_BASE, r, v)
#define CSL_MCU_CTRL_MMR0_CFG0_SET(r, m)  CSL_MCU_CTRL_MMR0_CFG0_WR(r, CSL_MCU_CTRL_MMR0_CFG0_RD(r) | m)
#define CSL_MCU_CTRL_MMR0_CFG0_CLR(r, m)  CSL_MCU_CTRL_MMR0_CFG0_WR(r, CSL_MCU_CTRL_MMR0_CFG0_RD(r) & ~(m))

#define CSL_WKUP_I2C0_CFG_RD(r)     CSL_REG32_RD_OFF(CSL_WKUP_I2C0_CFG_BASE, r)
#define CSL_WKUP_I2C0_CFG_WR(r, v)  CSL_REG32_WR_OFF(CSL_WKUP_I2C0_CFG_BASE, r, v)
#define CSL_WKUP_I2C0_CFG_SET(r, m) CSL_WKUP_I2C0_CFG_WR(r, CSL_WKUP_I2C0_CFG_RD(r) | m)
#define CSL_WKUP_I2C0_CFG_CLR(r, m) CSL_WKUP_I2C0_CFG_WR(r, CSL_WKUP_I2C0_CFG_RD(r) & ~(m))

#define CSL_DDRSS0_RD(r)     CSL_REG32_RD_OFF(CSL_STD_FW_COMPUTE_CLUSTER0__VBUSP_DDRSS0_DDRSS0_CTLCFG_CTL_CFG_START, r)
#define CSL_DDRSS0_WR(r, v)  CSL_REG32_WR_OFF(CSL_STD_FW_COMPUTE_CLUSTER0__VBUSP_DDRSS0_DDRSS0_CTLCFG_CTL_CFG_START, r, v)
#define CSL_DDRSS0_SET(r, m) CSL_DDRSS0_WR(r, CSL_DDRSS0_RD(r) | m)
#define CSL_DDRSS0_CLR(r, m) CSL_DDRSS0_WR(r, CSL_DDRSS0_RD(r) & ~(m))

#define CSL_CTRL_MMR0_CFG0_RD(r)      CSL_REG32_RD_OFF(CSL_CTRL_MMR0_CFG0_BASE, r)
#define CSL_CTRL_MMR0_CFG0_WR(r, v)   CSL_REG32_WR_OFF(CSL_CTRL_MMR0_CFG0_BASE, r, v)
#define CSL_CTRL_MMR0_CFG0_SET(r, m)  CSL_CTRL_MMR0_CFG0_WR(r, CSL_CTRL_MMR0_CFG0_RD(r) | m)
#define CSL_CTRL_MMR0_CFG0_CLR(r, m)  CSL_CTRL_MMR0_CFG0_WR(r, CSL_CTRL_MMR0_CFG0_RD(r) & ~(m))

#define SCICLIENT_LPM_GPIO2_CONF (0x32)
#define SCICLIENT_LPM_GPIO3_CONF (0x33)
#define SCICLIENT_LPM_GPIO6_CONF (0x36)
#define SCICLIENT_LPM_INT_TOP    (0x5A)

#define SCICLIENT_LPM_DDR_RET_VAL     (1 << 1)
#define SCICLIENT_LPM_DDR_RET_CLK     (1 << 2)
#define SCICLIENT_LPM_EN_DDR_RET_1V1  (1 << 5)
#define SCICLIENT_LPM_GPIO4_BIT       (1 << 3)
#define SCICLIENT_LPM_OD_SHIFT        1
#define SCICLIENT_LPM_DIR_SHIFT       0
#define SCICLIENT_LPM_SCRATCH_PAD_REG_3 (0xCB)
#define SCICLIENT_LPM_MAGIC_SUSPEND     (0xBA)
#define SCICLIENT_LPM_GPIO1_8_FALL 0xFF
#define SCICLIENT_LPM_GPIO1_8_RISE 0xF7

#define SCICLIENT_LPM_FSM_I2C_TRIGGERS (0x80)

#define SCICLIENT_LPM_DEVICE_ID_PMICB     0x86
#define SCICLIENT_LPM_DEVICE_ID_PMICB_EVM 0x7
#define SCICLIENT_LPM_I2C_DETECT_TIMEOUT  500 /* usually the I2C is ready after 221 loops */

struct Ddr_instance {
    unsigned int ddrBase;
    unsigned int chngReq;
    unsigned int chngAck;
    unsigned int clkChngReq;
    unsigned int clkChngAck;
    unsigned int pllCtrl;
    unsigned int pllHsdiv0;
    unsigned int pllFreq[3];
};

#if defined(SOC_J784S4)

const int j784s4 = 1;
const int j7200 = 0;
const int single = 1;

static const struct Ddr_instance ddr[] = {
    {
        .ddrBase = CSL_STD_FW_COMPUTE_CLUSTER0_CFG_WRAP_0__VBUSP_DDRSS0_DDRSS0_CTLCFG_VBUSP_DDRSS0_CTLCFG_START,
        .chngReq = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_REQ0,
        .chngAck = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_ACK0,
        .clkChngReq = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_REQ0,
        .clkChngAck = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_ACK0,
        .pllCtrl = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL12_CTRL,
        .pllHsdiv0 = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL12_HSDIV_CTRL0,
        .pllFreq = { 19200000, 1066500000, 1066500000 }
    },
    {
        .ddrBase = 0x029B0000,
        .chngReq = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_REQ1,
        .chngAck = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_ACK1,
        .clkChngReq = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_REQ1,
        .clkChngAck = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_ACK1,
        .pllCtrl = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL26_CTRL,
        .pllHsdiv0 = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL26_HSDIV_CTRL0,
        .pllFreq = { 19200000, 1066500000, 1066500000 }
    },
    {
        .ddrBase = 0x029D0000,
        .chngReq = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_REQ2,
        .chngAck = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_ACK2,
        .clkChngReq = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_REQ2,
        .clkChngAck = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_ACK2,
        .pllCtrl = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL27_CTRL,
        .pllHsdiv0 = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL27_HSDIV_CTRL0,
        .pllFreq = { 19200000, 1066500000, 1066500000 }
    },
    {
        .ddrBase = 0x029F0000,
        .chngReq = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_REQ3,
        .chngAck = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_ACK3,
        .clkChngReq = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_REQ3,
        .clkChngAck = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_ACK3,
        .pllCtrl = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL28_CTRL,
        .pllHsdiv0 = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL28_HSDIV_CTRL0,
        .pllFreq = { 19200000, 1066500000, 1066500000 }
    },
    { },
};
#else

const int j784s4 = 0;
const int j7200 = 1;
const int single = 0;

static const struct Ddr_instance ddr[] = {
    {
        .ddrBase = CSL_STD_FW_COMPUTE_CLUSTER0__VBUSP_DDRSS0_DDRSS0_CTLCFG_CTL_CFG_START,
        .chngReq = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_REQ,
        .chngAck = CSL_MAIN_CTRL_MMR_CFG0_CHNG_DDR4_FSP_ACK,
        .clkChngReq = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_REQ,
        .clkChngAck = CSL_MAIN_CTRL_MMR_CFG0_DDR4_FSP_CLKCHNG_ACK,
        .pllCtrl = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL12_CTRL,
        .pllHsdiv0 = CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL12_HSDIV_CTRL0,
        .pllFreq = { 19200000, 800000000, 800000000 }
    },
    { },
};
#endif

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

/**
  * \brief Set the DRAM in retention mode
  *
  */
static void Lpm_ddrEnterRetention(void);

/**
 * \brief Ask PMIC to powerdown the SOC
 *
 */
static void Lpm_setupPmic(void);

/**
 * \brief Clean R5 Caches
 *
 */
static void Lpm_cleanAllDCache(void);

/**
 * A simple UART based printf function supporting \%p, \%x, and \%X.
 *
 * \param pcString is the format string.
 * \param ... are the optional arguments, which depend on the contents of the
 * format string.
 *
 * This function is a partial copy of UART_printf from
 * ti/drv/uart/src/UART_stdio.c.
 * But only the support of \%p, \%x and \%X was kept.
 *
 * \return None.
 */
static void Lpm_debugPrintf(const char *pcString, ...);

/*
 * \brief Run the suspend sequence (set DDR in retention and powerdown the SOC)
 *
 * This function is the entry point when DM-Firmware jump to SRAM to start the
 * suspend sequence.
 *
 */
void Lpm_enterRetention(void)
{
    Lpm_debugPrintf("Lpm_enterRetention: Enter retention\n");

    /* Make sure that nothing remains in cache before going to retention */
    Lpm_cleanAllDCache();

    Lpm_ddrEnterRetention();
    Lpm_debugFullPrintf("Lpm_enterRetention: DDR retention done\n");


    Lpm_setupPmic();
    Lpm_debugPrintf("Lpm_enterRetention: Done! Going to wait now \n");

    while(1){};
}

static void Lpm_cleanAllDCache(void)
{
    unsigned int set, way;

    for(way = 0; way < 4 ; way ++)
        for(set = 0; set < 0x200; set++) {
            unsigned int val = set << 4 | way << 30;
            asm("    dmb");
            __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 2\t\n": "=r"(val));
            __asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 2\t\n": "=r"(val));
            asm("    dsb");
        }
}

static void set_PLL_for_DDR(const struct Ddr_instance *d, int idx)
{
    unsigned int PLL_freq = d->pllFreq[idx];
    uint32_t val, val1;

    if (PLL_freq <= 27500000) {
        CSL_REG32_SET(d->pllCtrl, 0x80000000);
    } else if (PLL_freq <= 133000000) {
        val1 = 2133000000 / PLL_freq;
        val = CSL_REG32_RD(d->pllHsdiv0) & ~(0x7F << 0);
        CSL_REG32_WR(d->pllHsdiv0, val | (val1 - 1));
        CSL_REG32_CLR(d->pllCtrl, 0x80000000);
    } else if (PLL_freq <= 266500000) {
        val = CSL_REG32_RD(d->pllHsdiv0) & ~(0x7F << 0);
        CSL_REG32_WR(d->pllHsdiv0, val | 0x7);
        CSL_REG32_CLR(d->pllCtrl, 0x80000000);
    } else if (PLL_freq <= 533000000) {
        val = CSL_REG32_RD(d->pllHsdiv0) & ~(0x7F << 0);
        CSL_REG32_WR(d->pllHsdiv0, val | 0x3);
        CSL_REG32_CLR(d->pllCtrl, 0x80000000);
    } else if (PLL_freq <= 800000000) {
        val = CSL_REG32_RD(d->pllHsdiv0) & ~(0x7F << 0);
        CSL_REG32_WR(d->pllHsdiv0, val | 0x3);
        CSL_REG32_CLR(d->pllCtrl, 0x80000000);
    } else if (PLL_freq <= 1066500000) {
        val = CSL_REG32_RD(d->pllHsdiv0) & ~(0x7F << 0);
        CSL_REG32_WR(d->pllHsdiv0, val | 0x1);
        CSL_REG32_CLR(d->pllCtrl, 0x80000000);
    } else {
        Lpm_debugFullPrintf("Unexpected frequency for DDR: 0x%x\n", PLL_freq);
    }
}

void wait_for_set(unsigned int address, unsigned int setval, unsigned int shift)
{
    unsigned int val;

    do {
        val = CSL_CTRL_MMR0_CFG0_RD(address);
    } while((val & (setval << shift)) != (setval << shift));
}

void verified_set(unsigned int address, unsigned int setval,
                  unsigned int mask, unsigned int  shift) {
    unsigned int val;

    val = CSL_CTRL_MMR0_CFG0_RD(address) & ~(mask << shift);
    CSL_CTRL_MMR0_CFG0_WR(address, val | (setval << shift));

    wait_for_set(address, setval, shift);
}

static void Lpm_ddrFreqChange(const struct Ddr_instance *d)
{
    volatile unsigned int t;
    const int fsp = 0;

    CSL_CTRL_MMR0_CFG0_WR(CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK0, KICK0_UNLOCK);
    CSL_CTRL_MMR0_CFG0_WR(CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK1, KICK1_UNLOCK);

    // set REQ_TYPE to fsp
    CSL_CTRL_MMR0_CFG0_WR(d->chngReq, fsp);
    for(t = 0; t < 100; t++) {}

    /*
     * assert req:
     * DDR will respond as if it was its idea to change the frequency
     */
    CSL_CTRL_MMR0_CFG0_SET(d->chngReq, 0x100);

    // DDR asserts a CLKCHNG_REQ
    wait_for_set(d->clkChngReq, 1, 7);

    set_PLL_for_DDR(d, fsp);

    // tells DDR that the new clock is available
    verified_set(d->clkChngAck, 0x1, 0x1, 0);

    // wait for the DDR to complete its stuff
    wait_for_set(d->chngAck, 1, 7);

    CSL_CTRL_MMR0_CFG0_WR(d->clkChngAck, 0);
    CSL_CTRL_MMR0_CFG0_CLR(d->chngReq, 0x103); // de-assert req
}

static void Lpm_ddrUnlockPll(void)
{
    CSL_PLL0_CFG_WR(CSL_MAIN_PLL_MMR_CFG_PLL12_LOCKKEY0, KICK0_UNLOCK);
    CSL_PLL0_CFG_WR(CSL_MAIN_PLL_MMR_CFG_PLL12_LOCKKEY1, KICK1_UNLOCK);
}

static void Lpm_ddrUnlockWKUP(unsigned partition)
{
    CSL_WKUP_CTRL_MMR0_CFG0_WR(8 + partition * 0x4000 + 0x1000, KICK0_UNLOCK);
    CSL_WKUP_CTRL_MMR0_CFG0_WR(0xC + partition * 0x4000 + 0x1000, KICK1_UNLOCK);
}

static void Lpm_ddrUnlockMCU(unsigned partition)
{
    CSL_MCU_CTRL_MMR0_CFG0_WR(8 + partition * 0x4000 + 0x1000, KICK0_UNLOCK);
    CSL_MCU_CTRL_MMR0_CFG0_WR(0xC + partition * 0x4000 + 0x1000, KICK1_UNLOCK);
}

static void Lpm_ddrEnterRetention(void)
{
    unsigned int n;
    uint32_t fspop, fspwr;

    Lpm_ddrUnlockMCU(0);
    Lpm_ddrUnlockWKUP(2); // was 7 for j7200
    Lpm_ddrUnlockPll(); //PLL_LOCK(12)

   /* try going to a new fsp (boot freq) before entering retention (11/3/21) */
    for (unsigned i = 0; ddr[i].ddrBase > 0; i++)
    {
        Lpm_debugFullPrintf("ddr[i].ddrBase = %x\n", ddr[i].ddrBase);

        /*
         * initialization was completed by the PI
         * initialize the controller to match the current state of the memory
         * MR13 data within the PI is based upon CS but NOT based upon
         * frequency set point
         * MR13 is the current MR13 value
         */
        fspop = (CSL_REG32_RD_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_PI_259)
                 & 1 << 31 ) >> 31;
        fspwr = (CSL_REG32_RD_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_PI_259)
                 & 1U << 30 ) >> 30;
        CSL_REG32_SET_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_192,
                          (fspwr << 24) | (fspop << 16));

        // do not allow CTL to update MR
        CSL_REG32_SET_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_191,
                          1 << 24);
        // do not allow PI to update MR
        CSL_REG32_CLR_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_PI_64, 1 << 0);

        /*
         * WDQLVL periodic is disabled for F0
         * WDQ does not require training for F0 and therefore also does not
         * require periodic training
         * This can be changed this without synchronization because we are
         * in a different FSP, not FSP0
         */
        CSL_REG32_CLR_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_PI_212, 2 << 8);

        // PI_FREQ_MAP defines supported working frequencies
        n = CSL_REG32_RD_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_PI_12);
        if (n & (0x1 << 1)) {
            // define FSP0 and FSP1 as trained
            CSL_REG32_SET_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_190,
                              (0x1 << 16) | (0x1 << 8));
        }
        if (n & (0x1 << 2)) {
            // define FSP0 and FSP2 as trained
            CSL_REG32_SET_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_190,
                              (0x1 << 24) | (0x1 << 8));
        }

        Lpm_ddrFreqChange(&ddr[i]);
    }

    Lpm_debugFullPrintf("Lpm_ddrEnterRetention: Readout PHY regs in boot freq prior to entry into DDR retention\n");
    for (unsigned i = 0; ddr[i].ddrBase; i++)
    {
        /* disable auto entry / exit */
        CSL_REG32_CLR_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_141,
                          ((0xF << 24) | (0xF << 16)));
        n = CSL_REG32_RD_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_132)
            & ~(0x7F << 24);

        /* set low power mode to 0x51 */
        CSL_REG32_WR_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_132,
                         (n | (0x51 << 24)));

        /* wait until low power operation has been completed */
        while((CSL_REG32_RD_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_293)
               & (0x1 << 10)) == 0) {};

        /* clear flag by writing t to DDRSS_CTL_295[10] */
        CSL_REG32_WR_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_295,
                         (0x1 << 10));

        /*
         * bit 6 / 14 -- lp_state valid
         * bits 13:8 / 5:0 0x0F SRPD Long with Mem Clk Gating
         */
        while((CSL_REG32_RD_OFF(ddr[i].ddrBase, CSL_EMIF_CTLCFG_DENALI_CTL_141)
               & 0x7F7F) != 0x4F4F) {};
    }
}

static int Lpm_i2cReadTimeout(char add, unsigned char *rxd, unsigned int timeout)
{
    unsigned int n, loop = 0;

    /* wait BB --> 0 */
    while(((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 12)) != 0x00 ) &&
          loop++ < timeout) {}

    if(loop >= timeout)
    {
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout loop exceed %d\n", timeout);
        return(-1);
    }

    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CNT, 1);
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON) & ~0x2;
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, (n | (3 << 9)));
    CSL_WKUP_I2C0_CFG_SET(CSL_I2C_CON, (1 << 0));

    /* wait XRDY --> 1 */
    loop = 0;
    while(((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 4)) == 0x00) &&
          loop++ < timeout) {}

    if(loop >= timeout)
    {
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for XRDY: loop exceed %d\n", timeout);
        return(-1);
    }

    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_DATA, add); // write enable to register lock
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, 0x1 << 4);

    /* wait ARDY --> 1 */
    loop = 0;
    while(((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) &&
          loop++ < timeout) {}

    if(loop >= timeout)
    {
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed %d\n", timeout);
        return(-1);
    }

    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW));

    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON) & ~(0x1 << 9);
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, n | (1 << 10));
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CNT, 1);
    CSL_WKUP_I2C0_CFG_SET(CSL_I2C_CON, (3 << 0));

    /* wait RRDY --> 1 */
    loop = 0;
    while(((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 3)) == 0x00) &&
          loop++ < timeout) {}

    if(loop >= timeout)
    {
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for RRDY: loop exceed %d\n", timeout);
        return(-1);
    }

    *rxd = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_DATA);

    /* wait ARDY --> 1 */
    while(((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) &&
          loop++ < timeout) {}

    if(loop >= timeout)
    {
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed %d\n", timeout);
        return(-1);
    }

    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW));

    return(0);
}

static unsigned char Lpm_i2cRead(char add)
{
    unsigned char rxData;

    Lpm_i2cReadTimeout(add, &rxData, 0xFFFFFFFF);

    return(rxData);
}

static void Lpm_i2cWrite(char add, char data)
{
    unsigned int n;

    /* wait BB --> 0 */
    while((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 12)) != 0x00) {}

    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CNT, 2);
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON) & ~0x2;
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, (n | (3 << 9)));
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON);
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, (n | (3 << 0)));

    for(n = 0; n < 2; n++)
    {
        /* wait XRDY --> 1 */
        while((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 4)) == 0x00) {}
        if(n==0)
        {
            /* write enable to register lock */
            CSL_WKUP_I2C0_CFG_WR(CSL_I2C_DATA, add);
        }
        else
        {
            /* write enable to register lock */
            CSL_WKUP_I2C0_CFG_WR(CSL_I2C_DATA, data);
            CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, (0x1 << 4));
        }
    }

    /* wait ARDY --> 1 */
    while((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) {}

    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW));
}

static void Lpm_i2cConfigWkup(char pmic)
{
    static char current_pmic;
    unsigned int n;

    /*
     * This is an optimization to prenvent setting again the I2C
     * when it's not needed
     */
    if (pmic == current_pmic)
        return;

    current_pmic = pmic;

    CSL_WKUP_CTRL_MMR0_CFG0_SET(CSL_MAIN_CTRL_MMR_CFG0_PADCONFIG62, (1 << 18));
    CSL_WKUP_CTRL_MMR0_CFG0_SET(CSL_MAIN_CTRL_MMR_CFG0_PADCONFIG63, (1 << 18));

    /*  reset the I2C */
    CSL_WKUP_I2C0_CFG_SET(CSL_I2C_SYSC, (1 << 1));
    CSL_WKUP_I2C0_CFG_SET(CSL_I2C_SYSC, (1 << 3));

    /* enable I2C */
    CSL_WKUP_I2C0_CFG_SET(CSL_I2C_CON, (1 << 15));
    while((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SYSS) & 0x1) == 0x00){}

    /* set divider = 2 */
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_PSC) & ~0xFF;
    /* 96/(7+1) = 12MHz */
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_PSC, (n | 0x7));

    /* 9.6MHz/(n1 + n2) = 0.4 -- > n1 + n2 = 24 */
    /* set SCLL 7 + 9 = 16 */
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SCLL) & ~0xFF;
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_SCLL, (n | 0x9));
    /* set SCLH 5 + 9 = 14 */
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SCLH) & ~0xFF;
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_SCLH, (n | 0x9));

    CSL_WKUP_I2C0_CFG_CLR(CSL_I2C_CON, 0xCF03);
    CSL_WKUP_I2C0_CFG_CLR(CSL_I2C_CNT, 0xFFFF);
    CSL_WKUP_I2C0_CFG_CLR(CSL_I2C_BUF, 0x8080);

    /* set own address = 0xB4 (random) */
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_OA) & ~0x3FF;
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_OA, (n | 0xb4));

    /* enable I2C */
    CSL_WKUP_I2C0_CFG_SET(CSL_I2C_CON, (1 << 15));
    while((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SYSS) & 0x1) == 0x00){}

    /* set PMIC ADDRESS */
    n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SA) & ~0x3FF;
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_SA, (n | pmic));

    /* Set TX / RX threshold to 1 [5:0] = 1-1; [13:8] = 1-1 */
    CSL_WKUP_I2C0_CFG_WR(CSL_I2C_BUF, ((1 << 6) | (1 << 14)));

    /* write enable */
    Lpm_i2cWrite(0xA1, 0x9B);
}

static void Lpm_writePmicA(uint8_t reg, uint8_t val)
{
    Lpm_i2cConfigWkup(BOARD_I2C_LEO_PMIC_A_ADDR);
    Lpm_i2cWrite(reg, val);
    Lpm_debugFullPrintf("Lpm_writePmicA: reg=0x%x 0x%x\n", reg, val);
}

static void Lpm_writePmicB(uint8_t reg, uint8_t val)
{
    /* if there's only one PMIC, just do nothing */
    if (single)
        return;

    Lpm_i2cConfigWkup(BOARD_I2C_LEO_PMIC_B_ADDR);
    Lpm_i2cWrite(reg, val);
    Lpm_debugFullPrintf("Lpm_writePmicB: reg=0x%x 0x%x\n", reg, val);
}

static uint8_t Lpm_readPmicA(uint8_t reg)
{
    unsigned char rxd;

    Lpm_i2cConfigWkup(BOARD_I2C_LEO_PMIC_A_ADDR);
    rxd = Lpm_i2cRead(reg);
    Lpm_debugFullPrintf("Lpm_readPmicA: reg=0x%x 0x%x\n", reg, rxd);

    return(rxd);
}

static uint8_t Lpm_readPmicB(uint8_t reg)
{
    unsigned char rxd;

    /* if there's only one PMIC, return 0 */
    if (single)
        return 0;

    Lpm_i2cConfigWkup(BOARD_I2C_LEO_PMIC_B_ADDR);
    rxd = Lpm_i2cRead(reg);
    Lpm_debugFullPrintf("Lpm_readPmicB: reg=0x%x 0x%x\n", reg, rxd);

    return(rxd);
}

static uint8_t Lpm_readPmicB_timeout(uint8_t reg, unsigned char *rxData, int timeout)
{
    int ret;

    Lpm_i2cConfigWkup(BOARD_I2C_LEO_PMIC_B_ADDR);
    ret = Lpm_i2cReadTimeout(reg, rxData, timeout);
    Lpm_debugFullPrintf("Lpm_readPmicB_timeout: reg=0x%x 0x%x ret=%d timeout=%d\n",
                            reg, *rxData, ret, timeout);

    return(ret);
}

static void Lpm_ClearPmicInterrupts(void)
{
    uint8_t int_top, val, val1;

    int_top = Lpm_i2cRead(0x5A);

    if(!int_top)
    {
        return;
    }

    if(int_top & (1 << 0))
    {
        val = Lpm_i2cRead(0x5B);
        Lpm_debugFullPrintf("INT_BUCK = 0x%02X\n", val);
        if (val & 1)
        {
            val1 = Lpm_i2cRead(0x5C);
            Lpm_debugFullPrintf("INT_BUCK1_2 = 0x%02X\n", val1);
            Lpm_i2cWrite(0x5C, val1);
        }

        if(val & (1 << 1))
        {
            val1 = Lpm_i2cRead(0x5D);
            Lpm_debugFullPrintf("INT_BUCK3_4 = 0x%02X\n", val1);
            Lpm_i2cWrite(0x5D, val1);
        }

        if(val & (1 << 2))
        {
            val1 = Lpm_i2cRead(0x5E);
            Lpm_debugFullPrintf("INT_BUCK5 = 0x%02X\n", val1);
            Lpm_i2cWrite(0x5E, val1);
        }
    }

    if(int_top & (1 << 1))
    {
        val = Lpm_i2cRead(0x5F);
        Lpm_debugFullPrintf("INT_LDO_VMON = 0x%02X\n", val);
        if(val & 1)
        {
            val1 = Lpm_i2cRead(0x60);
            Lpm_debugFullPrintf("INT_LDO1_2 = 0x%02X\n", val1);
            Lpm_i2cWrite(0x60, val1);
        }
        if(val & (1 << 1))
        {
            val1 = Lpm_i2cRead(0x61);
            Lpm_debugFullPrintf("INT_LDO3_4 = 0x%02X\n", val1);
            Lpm_i2cWrite(0x61, val1);
        }
        if(val & (1 << 2))
        {
            val1 = Lpm_i2cRead(0x62);
            Lpm_debugFullPrintf("INT_VMON = 0x%02X\n", val1);
            Lpm_i2cWrite(0x62, val1);
        }
    }

    if(int_top & (1 << 2))
    {
        val = Lpm_i2cRead(0x63);
        Lpm_debugFullPrintf("INT_GPIO = 0x%02X\n", val);
        if(val & (1 << 3))
        {
            val1 = Lpm_i2cRead(0x64);
            Lpm_debugFullPrintf("INT_GPIO1_8 = 0x%02X\n", val1);
            Lpm_i2cWrite(0x64, val1);
        }
        Lpm_i2cWrite(0x63, val); // clear GPIO9, GPIO10, GPIO11
    }

    if(int_top & (1 << 3))
    {
        val = Lpm_i2cRead(0x65);
        Lpm_debugFullPrintf("INT_STARTUP = 0x%02X\n", val);
        Lpm_i2cWrite(0x65, val);
    }

    if(int_top & (1 << 4))
    {
        val = Lpm_i2cRead(0x66);
        Lpm_debugFullPrintf("INT_MISC = 0x%02X\n", val);
        Lpm_i2cWrite(0x66, val);
    }

    if(int_top & (1 << 5))
    {
        val = Lpm_i2cRead(0x67);
        Lpm_debugFullPrintf("INT_MODERATE_ERR = 0x%02X\n", val);
        Lpm_i2cWrite(0x67, val);
    }

    if(int_top & (1 << 6))
    {
        val = Lpm_i2cRead(0x68);
        Lpm_debugFullPrintf("INT_SEVERE_ERR = 0x%02X\n", val);
        Lpm_i2cWrite(0x68, val);
    }

    if(int_top & (1 << 7))
    {
        val = Lpm_i2cRead(0x69);
        Lpm_debugFullPrintf("INT_FSM_ERR = 0x%02X\n", val);
        if(val & (1 << 4))
        {
            val1 = Lpm_i2cRead(0x6A);
            Lpm_debugFullPrintf("INT_COMM_ERR = 0x%02X\n", val1);
            Lpm_i2cWrite(0x6A, val1);
        }
        if(val & (1 << 5))
        {
            val1 = Lpm_i2cRead(0x6B);
            Lpm_debugFullPrintf("INT_READBACK_ERR = 0x%02X\n", val1);
            Lpm_i2cWrite(0x6B, val1);
        }
        if(val & (1 << 6))
        {
            val1 = Lpm_i2cRead(0x6C);
            Lpm_debugFullPrintf("INT_ESM = 0x%02X\n", val1);
            Lpm_i2cWrite(0x6C, val1);
        }
        Lpm_i2cWrite(0x69, val); // clear INT_FSM_ERR
    }

    int_top = Lpm_i2cRead(0x5A);
    Lpm_debugFullPrintf("INT_TOP = 0x%02X\n", int_top);
}

/* Taken from Lpm_pmicStateChangeActiveToIORetention, with some changes to
 * SCICLIENT_LPM_FSM_I2C_TRIGGERS for DDRRET support. Explaination on this change is still
 * obscure. */
static void Lpm_setupPmic(void)
{
    /* Write 0x02 to FSM_NSLEEP_TRIGGERS register
       This should happen before clearing the interrupts */

    /* If you clear the interrupts before you write the NSLEEP bits,
     * it will transition to S2R state.
     * This is because as soon as you write NSLEEP2 to 0x0,
     * the trigger is present to move to S2R state.
     * By setting the NSLEEP bits before you clear the interrupts,
     * you can configure both NSLEEP bits before the PMIC reacts to the change.
     */

    /* Change FSM_NSLEEP_TRIGGERS: NSLEEP1=high, NSLEEP2=high */
    Lpm_writePmicA(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR, 0x03);
    Lpm_debugFullPrintf("Lpm_setupPmic: Write FSM_NSLEEP_TRIGGERS\n");
    Lpm_debugReadPmicA(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR);

    /* Clear interrupts */
    Lpm_i2cConfigWkup(BOARD_I2C_LEO_PMIC_A_ADDR);
    Lpm_ClearPmicInterrupts();
    Lpm_writePmicB(0x65, 0x02);

    /* Change SCICLIENT_LPM_FSM_I2C_TRIGGERS */
    Lpm_writePmicA(PMIC_FSM_I2C_TRIGGERS_REGADDR, SCICLIENT_LPM_FSM_I2C_TRIGGERS);
    Lpm_writePmicB(PMIC_FSM_I2C_TRIGGERS_REGADDR, SCICLIENT_LPM_FSM_I2C_TRIGGERS);
    Lpm_debugFullPrintf("Lpm_setupPmic: Write FSM_TRIGGERS\n");
    Lpm_debugReadPmicA(PMIC_FSM_I2C_TRIGGERS_REGADDR);

    /* Configure GPIO4_CONF: input, pull-down, signal LP_WKUP1 */
    Lpm_writePmicA(0x34, 0xca);
    Lpm_debugFullPrintf("Lpm_setupPmic: Write GPIO4_CONF\n");
    Lpm_debugReadPmicA(0x34);

    /* Configure INT_GPIO1_8 (enable GPIO4 interrupt): clear GPIO4_INT */
    Lpm_writePmicA(0x64, SCICLIENT_LPM_GPIO4_BIT);
    Lpm_debugFullPrintf("Lpm_setupPmic: Write INT_GPIO1_8\n");
    Lpm_debugReadPmicA(0x64);

    /* Configure MASK_GPIO*_RISE */
    Lpm_writePmicA(0x50, SCICLIENT_LPM_GPIO1_8_RISE);
    Lpm_writePmicA(0x51, 0x3F);

    /* Configure MASK_SCICLIENT_LPM_GPIO1_8_FALL (configure GPIO4 falling edge interrupt): enable INT on GPIO4 */
    Lpm_writePmicA(0x4F, SCICLIENT_LPM_GPIO1_8_FALL);
    Lpm_debugFullPrintf("Lpm_setupPmic: Write MASK_SCICLIENT_LPM_GPIO1_8_FALL\n");
    Lpm_debugReadPmicA(0x4F);

    /*
     * Configure DDR_RET pin
     * The sequence is different for each board.
     */
    if (j7200) {
	    uint8_t buf;
	    /* Put GPIO2 0x32 and GPIO3 0x33 as output push-pull no pull-up or pull down */
	    Lpm_writePmicB(SCICLIENT_LPM_GPIO2_CONF,
			   1 << SCICLIENT_LPM_DIR_SHIFT | 0 << SCICLIENT_LPM_OD_SHIFT);
	    Lpm_writePmicB(SCICLIENT_LPM_GPIO3_CONF,
			   1 << SCICLIENT_LPM_DIR_SHIFT | 0 << SCICLIENT_LPM_OD_SHIFT);
	    /* GPIO_OUT_1 */
	    buf = Lpm_readPmicB(0x3D) | SCICLIENT_LPM_DDR_RET_VAL; // 1<<1, GPIO2_OUT on
	    Lpm_writePmicB(0x3D, buf);
	    Lpm_writePmicB(0x3D, buf | SCICLIENT_LPM_DDR_RET_CLK);  // 1<<2, GPIO3_OUT on
	    Lpm_writePmicB(0x3D, buf & ~SCICLIENT_LPM_DDR_RET_CLK); // 1<<2, GPIO3_OUT off
	    Lpm_writePmicB(0x3D, buf | SCICLIENT_LPM_DDR_RET_CLK);  // 1<<2, GPIO3_OUT on
    }
    if (j784s4) {
	    uint8_t buf;

	    /* Put GPIO6 as output push-pull no pull-up or pull down */
	    Lpm_writePmicA(SCICLIENT_LPM_GPIO6_CONF,
			   1 << SCICLIENT_LPM_DIR_SHIFT | 0 << SCICLIENT_LPM_OD_SHIFT);
	    /* GPIO_OUT_1 */
	    buf = Lpm_readPmicA(0x3D) | SCICLIENT_LPM_EN_DDR_RET_1V1; // 1<<5, GPIO6_OUT on
	    Lpm_writePmicA(0x3D, buf);
    }

    /* Write magic number to scratch register to indicate the suspend */
    Lpm_writePmicA(SCICLIENT_LPM_SCRATCH_PAD_REG_3, SCICLIENT_LPM_MAGIC_SUSPEND);
    Lpm_debugReadPmicA(SCICLIENT_LPM_SCRATCH_PAD_REG_3);

    Lpm_debugReadPmicA(SCICLIENT_LPM_INT_TOP);

    /* Change FSM_NSLEEP_TRIGGERS: NSLEEP1=low, NSLEEP2=low */
    Lpm_writePmicA(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR, 0x00);
}

#ifdef DEBUG_SRAM_S2R
/**
 * \brief   This function writes a byte on the serial console
 *
 * This function is based on UART_putc from ti/drv/uart/src/UART_stdio.c.
 *
 * \param   baseAdd  Address of the UART port to use.
 *          byteTx   The byte to be transmitted.
 *
 */
static void _Lpm_debugPutC(uint32_t baseAdd, uint8_t byteTx)
{
    uint32_t lcrRegValue = CSL_REG32_RD_OFF(baseAdd, UART_LCR);

    CSL_REG32_WR_OFF(baseAdd, UART_LCR, lcrRegValue & 0x7FU);
    /*
    ** Waits indefinitely until the THR and Transmitter Shift Registers are
    ** empty.
    */
    while(((unsigned int) UART_LSR_TX_SR_E_MASK |
           (unsigned int) UART_LSR_TX_FIFO_E_MASK) !=
          (CSL_REG32_RD_OFF(baseAdd, UART_LSR) &
           ((unsigned int) UART_LSR_TX_SR_E_MASK |
            (unsigned int) UART_LSR_TX_FIFO_E_MASK)))
    {
        /* Busy wait */
    }

    CSL_REG32_WR_OFF(baseAdd, UART_THR, byteTx);
    CSL_REG32_WR_OFF(baseAdd, UART_LCR, lcrRegValue);
}

/**
 * \brief  Wrapper function to call _Lpm_debugPutC
 *
 * \param  byteTx The byte to be transmitted.
 *
 */
static void Lpm_debugPutC(uint8_t byteTx)
{
#ifndef DEBUG_UART_MAIN
    static bool Lpm_uartConfigured = false;
    if (Lpm_uartConfigured == false)
    {
        CSL_REG32_WR_OFF(CSL_MCU_UART0_BASE, UART_LCR,
                         UART_FRAME_WORD_LENGTH_8 | UART_FRAME_NUM_STB_1 | UART_PARITY_NONE);
	Lpm_uartConfigured = true;
    }
#endif
    _Lpm_debugPutC(UART_DEBUG_ADDR, byteTx);
}

/**
 * \brief  This function writes data from a specified buffer onto the transmitter
 *         FIFO of UART
 *
 * \param  str  Pointer to a buffer to transmit.
 *         len  Number of bytes to be transmitted.
 *
 */
static void Lpm_debugPutS(const char *str, uint32_t len)
{
    unsigned int i;
    for (i = 0; i < len && str[i] != '\0'; i++)
    {
        if (str[i] == '\n')
        {
           Lpm_debugPutC('\r');
        }
        Lpm_debugPutC(str[i]);
    }
}

/**
 * \brief This function calls to the common procedure in Lpm_debugPrintf.
 *        This functions writes the output in hex format.
 *
 * This function is a partial copy of UART_convertVal from
 * ti/drv/uart/src/UART_stdio.c. Only the hexadecimal support was kept.
 *
 * \param ulValue  The value of the resolved variable passed as argument
 *        ulPos    The buffer position of the printed value
 *        ulCount  Total number of character to be printed
 *        cFill    The char to be placed between number
 *        pcBuf    buffer storing all the numbers
 *
 * \return 0.
 *
 */
static int32_t Lpm_debugConvertVal(uint32_t ulValue, uint32_t ulPos, uint32_t ulCount, char cFill, char *pcBuf)
{
    const char *const g_pcHex = "0123456789abcdef";
    uint32_t ulBase = 16;
    uint32_t ulIdx;
    uint32_t count = ulCount;
    uint32_t pos = ulPos;

    for (ulIdx = 1u;
         (((ulIdx * ulBase) <= ulValue) &&
          (((ulIdx * ulBase) / ulBase) == ulIdx));
         (ulIdx = (ulIdx * ulBase)))
    {
        count--;
    }

    /* Provide additional padding at the beginning of the
     * string conversion if needed. */
    if ((count > 1u) && (count < 16u))
    {
        for (count--; count != 0U; count--)
        {
            pcBuf[pos] = cFill;
            pos++;
        }
    }

    /* Convert the value into a string. */
    for (; ulIdx != 0U; ulIdx /= ulBase)
    {
        pcBuf[pos] = g_pcHex[(ulValue / ulIdx) % ulBase];
        pos++;
    }

    /* Write the string. */
    (void)Lpm_debugPutS(pcBuf, pos);

    return 0;
}

static void Lpm_debugPrintf(const char *pcString, ...)
{
    uint32_t ulIdx, ulValue, ulPos, ulCount;
    char    pcBuf[16], cFill;
    va_list  vaArgP;
    const char *pStr = pcString;

    /* Start the varargs processing. */
    (void)va_start(vaArgP, pcString);

    /* Loop while there are more characters in the string. */
    while (*pStr != (char)0U)
    {
        /* Find the first non-% character, or the end of the string. */
        for (ulIdx = 0;
             (pStr[ulIdx] != (char) '%') &&
             (pStr[ulIdx] != (char) '\0');
             ulIdx++)
        {}

        /* Write this portion of the string. */
        (void)Lpm_debugPutS(pStr, ulIdx);

        /* Skip the portion of the string that was written. */
        pStr += ulIdx;

        /* See if the next character is a %. */
        if (*pStr == (char) '%')
        {
            /* Skip the %. */
            pStr++;

            /* Set the digit count to zero, and the fill character to space
             * (i.e. to the defaults). */
            ulCount = 0;
            cFill   = (char) ' ';

            /* Determine how to handle the next character. */
            while((*pStr >= (char)'0') && (*pStr <= (char)'9'))
            {
                /* If this is a zero, and it is the first digit, then the
                 * fill character is a zero instead of a space. */
                if ((pStr[-1] == (char) '0') && (ulCount == 0U))
                {
                    cFill = (char) '0';
                }

                /* Update the digit count. */
                ulCount *= 10u;
                ulCount += ((uint32_t)(*pStr)) - (uint32_t) '0';

                /* Get the next character. */
                pStr++;
            }
            switch (*pStr)
            {
                /* Handle the %x and %X commands.  Note that they are treated
                 * identically; i.e. %X will use lower case letters for a-f
                 * instead of the upper case letters is should use.  We also
                 * alias %p to %x. */
                case (char) 'x':
                case (char) 'X':
                case (char) 'p':
                {
                    /* Get the value from the varargs. */
                    ulValue = (uint32_t)va_arg(vaArgP, uint32_t);

                    /* Reset the buffer position. */
                    ulPos = 0;

                    /* Determine the number of digits in the string version of
                     * the value. */
                    (void)Lpm_debugConvertVal(ulValue, ulPos, ulCount, cFill, pcBuf);
 
                    break;
                }

                /* Handle all other commands. */
                default:
                {
                    /* Indicate an error. */
                    (void)Lpm_debugPutS("ERROR", 5u);

                    /* This command has been handled. */
                    break;
                }
            }
            pStr++;
        }
    }

    /* End the varargs processing. */
    va_end(vaArgP);
}

#else
void Lpm_debugPrintf(const char *pcString, ...) {}
#endif /* DEBUG_SRAM_S2R */
