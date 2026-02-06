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
 * \file lpm_s2r.c
 *
 * \brief This file contains the code which is loaded to SRAM for S2R support.
 * This code set the DDR in retention mode and ask PMIC to powerdown the SOC.
 *
 */
#include <stdint.h>
#include <cslr_soc_baseaddress.h>
#include <cslr_mcu_padcfg_ctrl_mmr.h>
#include <cslr_i2c.h>
#include "dbg_uart.c"

#define Lpm_debugFullPrintf Lpm_debugPrintf
#define Lpm_debugReadPmic Lpm_readPmic
// TODO: use a common include file for those:
// was in source/drivers/device_manager/rm_pm_hal/rm_pm_hal_src/lpm/include/soc/am62px/baseaddress.h
#define DDR_CTRL_BASE                   (0x0f308000U)
#define MMR0_CFG_BASE                   (0x43000000U)
#define WKUP_CTRL_MMR_BASE              MMR0_CFG_BASE
#define MMR1_CFG_BASE                   (0x04500000U)
#define MCU_CTRL_MMR_BASE               MMR1_CFG_BASE

// was in source/drivers/device_manager/rm_pm_hal/rm_pm_hal_src/lpm/ddr.c
#define CDNS_DENALI_PI_25                                       0x2064U
#define CDNS_DENALI_PI_25_WRLVL_AUTO_REQ                        0x00010000U

#define CDNS_DENALI_PI_43                                       0x20ACU
#define CDNS_DENALI_PI_43_RDLVL_AUTO_REQ                        0x00000100U

#define CDNS_DENALI_PI_55                                       0x20DCU
#define CDNS_DENALI_PI_55_CALVL_AUTO_REQ                        0x00000100U

#define CDNS_DENALI_CTL_158                                     0x278U
#define CDNS_DENALI_CTL_158_LP_CMD_MASK                         0xFF00U
#define CDNS_DENALI_CTL_158_LP_CMD_SUSPEND                      0x5100U
#define CDNS_DENALI_CTL_158_LP_CMD_RESUME                       0x0200U

#define CDNS_DENALI_PHY_1306                                    0x5468U
#define CDNS_DENALI_PHY_1306_PHY_SET_DFI_INPUT_0                0x1U

#define CDNS_DENALI_PHY_1364                                    0x5550U
#define CDNS_DENALI_PHY_1364_PHY_INIT_UPDATE_CONFIG_MASK        0x7U
#define CDNS_DENALI_PHY_1364_PHY_INIT_UPDATE_CONFIG_SHIFT       0x8U

#define CDNS_DENALI_PHY_1369                                    0x5564U
#define CDNS_DENALI_PHY_1369_PHY_UPDATE_MASK                    0x1U


// TODO
static void Lpm_cleanAllDCache(void)
{
#if 0

	unsigned int set, way;

	for(way = 0; way < 4 ; way ++)
		for(set = 0; set < 0x200; set++) {
			unsigned int val = set << 4 | way << 30;
			asm("    dmb");
			__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 2\t\n": "=r"(val));
			__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 2\t\n": "=r"(val));
			asm("    dsb");
		}
#endif
}

#define CTRL_MMR0_PARTITION_SIZE                (0x4000U)

#define CTRLMMR_LOCK_KICK0                      (0x01008U)
#define CTRLMMR_LOCK_KICK0_UNLOCK_VAL           (0x68ef3490U)
#define CTRLMMR_LOCK_KICK0_LOCK_VAL             (0x0U)
#define CTRLMMR_LOCK_KICK0_UNLOCKED_MASK        BIT(0)
#define CTRLMMR_LOCK_KICK0_UNLOCKED_SHIFT       (0U)
#define CTRLMMR_LOCK_KICK1                      (0x0100cU)
#define CTRLMMR_LOCK_KICK1_UNLOCK_VAL           (0xd172bc5aU)
#define CTRLMMR_LOCK_KICK1_LOCK_VAL             (0x0U)

// defined in source/board/pmic/pmic_lld/src/pmic_fsm_priv.h
#define PMIC_FSM_NSLEEP_TRIGGERS_REGADDR          (0x86U)


#define ctrlmmr_raw_readl readl
#define ctrlmmr_raw_writel writel

static inline uint32_t readl(uint32_t a)
{
	return *(volatile uint32_t *) (a);
}

static inline void writel(uint32_t v, uint32_t a)
{
	*(volatile uint32_t *) (a) = v;
}

void ctrlmmr_unlock(uint32_t base, uint8_t partition)
{
	uint32_t addr = base + (partition * CTRL_MMR0_PARTITION_SIZE);

	ctrlmmr_raw_writel(CTRLMMR_LOCK_KICK0_UNLOCK_VAL, addr + CTRLMMR_LOCK_KICK0);
	ctrlmmr_raw_writel(CTRLMMR_LOCK_KICK1_UNLOCK_VAL, addr + CTRLMMR_LOCK_KICK1);
}

static void Lpm_ddrEnterRetention(void)
{
#define CDNS_DENALI_CTL_0                                       0x0000U
#define CDNS_DENALI_CTL_0_DRAM_CLASS_MASK                       0x00000F00U
#define CDNS_DENALI_CTL_0_DRAM_CLASS_SHIFT                      0x00000008U
#define CDNS_DENALI_CTL_0_DRAM_CLASS_DDR4                       0xAU
#define CDNS_DENALI_CTL_0_DRAM_CLASS_LPDDR4                     0xBU
	uint32_t val, dram_class;

    dbg_line(__func__);
	val = readl(DDR_CTRL_BASE + CDNS_DENALI_CTL_0);
	dram_class = (val & CDNS_DENALI_CTL_0_DRAM_CLASS_MASK) >>
		     CDNS_DENALI_CTL_0_DRAM_CLASS_SHIFT;

    if (dram_class == CDNS_DENALI_CTL_0_DRAM_CLASS_LPDDR4)
        dbg_puts("LP");
    dbg_line("DDR4");

	/* Unlock wkup_ctrl_mmr region 2 & 6 */
	ctrlmmr_unlock(WKUP_CTRL_MMR_BASE, 2); // same as Lpm_ddrUnlockWKUP(2)
	ctrlmmr_unlock(WKUP_CTRL_MMR_BASE, 6);

	/* Unlock mcu_ctrl_mmr region 0,2 */
	ctrlmmr_unlock(MCU_CTRL_MMR_BASE, 0); // same as Lpm_ddrUnlockMCU(0)
	ctrlmmr_unlock(MCU_CTRL_MMR_BASE, 2);

	/*
	 * Enable auto training for WRLVL, RDLVL, CALVL
	 * Assumption: RDLVL_GATE, CALVL auto trainings enabled by bootloader
	 */
	val = readl(DDR_CTRL_BASE + CDNS_DENALI_PI_25);
	val |= CDNS_DENALI_PI_25_WRLVL_AUTO_REQ;
	writel(val, DDR_CTRL_BASE + CDNS_DENALI_PI_25);

	val = readl(DDR_CTRL_BASE + CDNS_DENALI_PI_43);
	val |= CDNS_DENALI_PI_43_RDLVL_AUTO_REQ;
	writel(val, DDR_CTRL_BASE + CDNS_DENALI_PI_43);

	val = readl(DDR_CTRL_BASE + CDNS_DENALI_PI_55);
	val |= CDNS_DENALI_PI_55_CALVL_AUTO_REQ;
	writel(val, DDR_CTRL_BASE + CDNS_DENALI_PI_55);

	/* Maintain reset signal throughout deep sleep */
	val = readl(DDR_CTRL_BASE + CDNS_DENALI_PHY_1306);
	val |= CDNS_DENALI_PHY_1306_PHY_SET_DFI_INPUT_0;
	writel(val, DDR_CTRL_BASE + CDNS_DENALI_PHY_1306);

	/* Set CDNS_DENALI_PHY_1369:PHY UPDATE MASK */
	val = readl(DDR_CTRL_BASE + CDNS_DENALI_PHY_1369);
	val |= CDNS_DENALI_PHY_1369_PHY_UPDATE_MASK;
	writel(val, DDR_CTRL_BASE + CDNS_DENALI_PHY_1369);

	/* Clear 0x7 in CDNS_DENALI_PHY_1364:PHY_INIT_UPDATE_CONFIG */
	val = readl(DDR_CTRL_BASE + CDNS_DENALI_PHY_1364);
	val &= ~(CDNS_DENALI_PHY_1364_PHY_INIT_UPDATE_CONFIG_MASK <<
		 CDNS_DENALI_PHY_1364_PHY_INIT_UPDATE_CONFIG_SHIFT);
	writel(val, DDR_CTRL_BASE + CDNS_DENALI_PHY_1364);

	/* Enter Self refresh with ctrl clk gating in deep sleep */
	val = readl(DDR_CTRL_BASE + CDNS_DENALI_CTL_158);
	val &= ~CDNS_DENALI_CTL_158_LP_CMD_MASK;
	val |= CDNS_DENALI_CTL_158_LP_CMD_SUSPEND;
	writel(val, DDR_CTRL_BASE + CDNS_DENALI_CTL_158);
}

#define CSL_REG32_RD_OFF(p, off)    (CSL_REG32_RD_OFF_RAW( \
                                        (volatile uint32_t *) (p), \
                                        (uint32_t) (off)))
#define CSL_REG32_WR_OFF(p, off, v) (CSL_REG32_WR_OFF_RAW( \
                                        (volatile uint32_t *) (p), \
                                        (uint32_t) (off), \
                                        (uint32_t) (v)))
#define CSL_WKUP_I2C0_CFG_RD(r)     CSL_REG32_RD_OFF(CSL_WKUP_I2C0_CFG_BASE, r)
#define CSL_WKUP_I2C0_CFG_WR(r, v)  CSL_REG32_WR_OFF(CSL_WKUP_I2C0_CFG_BASE, r, v)
#define CSL_WKUP_I2C0_CFG_SET(r, m) CSL_WKUP_I2C0_CFG_WR(r, CSL_WKUP_I2C0_CFG_RD(r) | m)
#define CSL_WKUP_I2C0_CFG_CLR(r, m) CSL_WKUP_I2C0_CFG_WR(r, CSL_WKUP_I2C0_CFG_RD(r) & ~(m))

#define CSL_WKUP_CTRL_MMR0_CFG0_RD(r)     CSL_REG32_RD_OFF(CSL_WKUP_CTRL_MMR0_CFG0_BASE, r)
#define CSL_WKUP_CTRL_MMR0_CFG0_WR(r, v)  CSL_REG32_WR_OFF(CSL_WKUP_CTRL_MMR0_CFG0_BASE, r, v)
#define CSL_WKUP_CTRL_MMR0_CFG0_SET(r, m) CSL_WKUP_CTRL_MMR0_CFG0_WR(r, CSL_WKUP_CTRL_MMR0_CFG0_RD(r) | m)
#define CSL_WKUP_CTRL_MMR0_CFG0_CLR(r, m) CSL_WKUP_CTRL_MMR0_CFG0_WR(r, CSL_WKUP_CTRL_MMR0_CFG0_RD(r) & ~(m))

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
    uint32_t val;

    /*
     * This is an optimization to prenvent setting again the I2C
     * when it's not needed
     */
    if (pmic == current_pmic)
        return;

    current_pmic = pmic;

    /*
     * Pinmux I2C:
     * B9 MCU_PADCONFIG19 WKUP_I2C0_SCL 0x0408404C mux0
     * D11 MCU_PADCONFIG20 WKUP_I2C0_SDA 0x04084050 mux0
     */
    val = CSL_REG32_RD_OFF(CSL_MCU_PADCFG_CTRL0_CFG0_BASE,
			   CSL_MCU_PADCFG_CTRL_MMR_CFG0_PADCONFIG19);
    val |= 1 << 18;
    CSL_REG32_WR_OFF(CSL_MCU_PADCFG_CTRL0_CFG0_BASE,
		     CSL_MCU_PADCFG_CTRL_MMR_CFG0_PADCONFIG19, val);
    val = CSL_REG32_RD_OFF(CSL_MCU_PADCFG_CTRL0_CFG0_BASE,
			   CSL_MCU_PADCFG_CTRL_MMR_CFG0_PADCONFIG20);
    val |= 1 << 18;
    CSL_REG32_WR_OFF(CSL_MCU_PADCFG_CTRL0_CFG0_BASE,
		     CSL_MCU_PADCFG_CTRL_MMR_CFG0_PADCONFIG20, val);

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

#define PMIC_ADDR 0x48
static uint8_t Lpm_readPmic(uint8_t reg)
{
    unsigned char rxd;
    Lpm_i2cConfigWkup(PMIC_ADDR);
    rxd = Lpm_i2cRead(reg);
    Lpm_debugFullPrintf("Lpm_readPmicA: reg=0x%x 0x%x\n", reg, rxd);

    return(rxd);
}

static void Lpm_writePmic(uint8_t reg, uint8_t val)
{
    Lpm_i2cConfigWkup(PMIC_ADDR);
    Lpm_i2cWrite(reg, val);
    Lpm_debugFullPrintf("Lpm_writePmic: reg=0x%x 0x%x\n", reg, val);
}

/*
 * \brief Run the suspend sequence (set DDR in retention and powerdown the SOC)
 *
 * This function is the entry point when DM-Firmware jump to SRAM to start the
 * suspend sequence.
 *
 */
void Lpm_enterRetention(void)
{
	dbg_line("Lpm_enterRetention: Enter retention");

	uint8_t test = Lpm_readPmic(0x86);
	dump_byte(test);
	dbg_line("^^");
	/* Make sure that nothing remains in cache before going to retention */
	Lpm_cleanAllDCache();

	Lpm_ddrEnterRetention();
	dbg_line("Lpm_enterRetention: DDR retention done");


	// TODO Lpm_setupPmic();
	dbg_line("Lpm_enterRetention: Done! Going to wait now");

//i2cset -f -y -m 0xFF -r -a 0 0x48 0x86 0x2
	Lpm_writePmic(0x86, 0x02);
	test = Lpm_readPmic(0x86);
	dump_byte(test);
	dbg_line("^^");
	while(1){};
}

