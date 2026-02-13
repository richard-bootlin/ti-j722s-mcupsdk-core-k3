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
#include <baseaddress.h>
#include <cslr_i2c.h>
#include <cslr_mcu_padcfg_ctrl_mmr.h>
#include <ddr_functions.h>
#include <DDRSS_addr_map_sfr_offs_ew_32bit.h>
#include <lib/bitops.h>
#include <lpm/timeout.h>
#include <wkup_ctrl_mmr.h>
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

struct pll_raw_data {
	uint32_t base;
	uint32_t freq_ctrl0;
	uint32_t freq_ctrl1;
	uint32_t div_ctrl;
	uint32_t hsdiv[16];
};


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

#define PLL_16FFT_CTRL_OFFSET    ((uint32_t) 0x20UL)
#define PLL_16FFT_CTRL_BYPASS_EN BIT(31)
#define PLLOFFSET(idx) (0x1000U * (idx))
struct pll_raw_data main_pll12 =
{ .base = MAIN_PLL_MMR_BASE + PLLOFFSET(12U), };

void pll_bypass(struct pll_raw_data *pll, int enable)
{
	uint32_t ctrl;

	ctrl = readl(pll->base + PLL_16FFT_CTRL_OFFSET);
	if (enable) {
		ctrl |= PLL_16FFT_CTRL_BYPASS_EN;
	} else {
		ctrl &= ~PLL_16FFT_CTRL_BYPASS_EN;
	}
	writel(ctrl, pll->base + PLL_16FFT_CTRL_OFFSET);
}

static int32_t fsp_shift(void)
{
	int32_t ret = 0;
	uint32_t timeout = 0;
	uint32_t val = 0;

	/* Request freq change */
	val = (readl(WKUP_CTRL_MMR_BASE + CHNG_DDR4_FSP_REQ)) & (~CHNG_DDR4_FSP_REQ_TYPE_MASK);
	val |= CHNG_DDR4_FSP_REQ_TYPE_FSP0;
	writel(val, WKUP_CTRL_MMR_BASE + CHNG_DDR4_FSP_REQ);
	val |= CHNG_DDR4_FSP_REQ_SET;
	writel(val, WKUP_CTRL_MMR_BASE + CHNG_DDR4_FSP_REQ);

	/* Poll for freq change request to be set */
	timeout = TIMEOUT_10_MS;
	while ((timeout > 0U) && ((readl(WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_REQ)
				   & DDR4_FSP_CLKCHNG_REQ_SET) != DDR4_FSP_CLKCHNG_REQ_SET)) {
		--timeout;
	}
	if (timeout == 0U) {
		ret = -1;
	}

	/* Set the PLL frequency to the requested frequency */
	val = (readl(WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_REQ)) & DDR4_FSP_CLKCHNG_REQ_TYPE_MASK;
	if (val == DDR4_FSP_CLKCHNG_REQ_TYPE_FSP0) {
		pll_bypass(&main_pll12, 1);
	} else {
		ret = -1;
	}

	/* Set the FSP ack bit */
	writel(DDR4_FSP_CLKCHNG_REQ_ACK, (WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_ACK));

	/* Wait for request to go away */
	timeout = TIMEOUT_10_MS;
	while ((timeout > 0U) && ((readl(WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_REQ) &
				   DDR4_FSP_CLKCHNG_REQ_SET) != DDR4_FSP_CLKCHNG_REQ_CLR)) {
		--timeout;
	}
	if (timeout == 0U) {
		ret = -1;
	}

	/* Clear the ACK bit */
	val = readl(WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_ACK);
	val &= ~DDR4_FSP_CLKCHNG_REQ_ACK;
	writel(val, (WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_ACK));

	/* Poll for CHNG_DDR4_FSP_ACK bit to be 1 */
	timeout = TIMEOUT_10_MS;
	while ((timeout > 0U) && ((readl(WKUP_CTRL_MMR_BASE + CHNG_DDR4_FSP_ACK) &
				   (CHNG_DDR4_FSP_CHNG_ACK)) != CHNG_DDR4_FSP_CHNG_ACK)) {
		--timeout;
	}
	if (timeout == 0U) {
		ret = -1;
	}

	/* De assert request */
	writel(0, (WKUP_CTRL_MMR_BASE + CHNG_DDR4_FSP_ACK));

	val = readl(WKUP_CTRL_MMR_BASE + CHNG_DDR4_FSP_REQ);
	val &= ~CHNG_DDR4_FSP_REQ_SET;
	writel(val, (WKUP_CTRL_MMR_BASE + CHNG_DDR4_FSP_REQ));

	return ret;
}

void ctrlmmr_unlock(uint32_t base, uint8_t partition)
{
	uint32_t addr = base + (partition * CTRL_MMR0_PARTITION_SIZE);

	ctrlmmr_raw_writel(CTRLMMR_LOCK_KICK0_UNLOCK_VAL, addr + CTRLMMR_LOCK_KICK0);
	ctrlmmr_raw_writel(CTRLMMR_LOCK_KICK1_UNLOCK_VAL, addr + CTRLMMR_LOCK_KICK1);
}

/**
 * \brief Write to a specific field in an MMR.
 * \param mmr_address MMR address
 * \param field_value value to be written
 * \param width width of the field
 * \param leftshift the number of bit fields the value has to be left shifted
 */
static void Write_MMR_Field(uint32_t mmr_address, uint32_t field_value,
			    uint32_t width, uint32_t leftshift)
{
	uint32_t *p_mmr;
	uint32_t mask;

	p_mmr = (uint32_t *) mmr_address;                                       /* Grab the MMR value */
	mask = (((uint32_t) 1U << width) - ((uint32_t) 1U << leftshift));       /* Build a mask of 1s for the field. */
	mask = ~(mask);                                                         /* Invert the mask so that the field will be zero'd out with the AND operation. */
	*p_mmr &= mask;                                                         /* Zero out the field in the register. */
	*p_mmr |= (field_value << leftshift);                                   /* Assign the value to that specific field. */
}

static void put_ddrss_in_data_retention_thru_wkup_mmr(uint32_t enable)
{
	uint32_t val = 0U;

	/* Write into data_retention MMR to activate or deactivate DDR data retention */
	writel(enable, WKUP_CTRL_MMR_BASE + DDR16SS_PMCTRL);

	/* Write `1' into data_ret_ld[31] MMR to generate a LD signal to latch the retention signal */
	writel((((DDR16SS_DATA_RET_LD_OPEN << DDR16SS_DATA_RET_LD_BIT) | enable)), WKUP_CTRL_MMR_BASE + DDR16SS_PMCTRL);

	val = readl(WKUP_CTRL_MMR_BASE + DDR16SS_PMCTRL);
	while (val != ((DDR16SS_DATA_RET_LD_OPEN << DDR16SS_DATA_RET_LD_BIT) | enable)) {
		val = readl(WKUP_CTRL_MMR_BASE + DDR16SS_PMCTRL);
	}

	/* Writes `0' into data_ret_ld[31] to close the latch */
	writel((((DDR16SS_DATA_RET_LD_CLOSE << DDR16SS_DATA_RET_LD_BIT) | enable)), WKUP_CTRL_MMR_BASE + DDR16SS_PMCTRL);
}

static void enter_lpm_self_refresh(void)
{
	uint32_t lp_status = 0;

	/* Program Self Refresh mode */
	writel((LP_MODE_LONG_SELF_REFRESH << 8), DDRSS0_CTRL_BASE + (uint32_t) DENALI_CTL_160__SFR_OFFS);

	while (lp_status != STATUS_SR_LONG_ENTERED) {
		lp_status = (readl(DDRSS0_CTRL_BASE + (uint32_t) DENALI_CTL_169__SFR_OFFS) & 0x7F00U);
	}
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

#if 0
	/* Unlock wkup_ctrl_mmr region 2 & 6 */
	ctrlmmr_unlock(WKUP_CTRL_MMR_BASE, 2); // same as Lpm_ddrUnlockWKUP(2)
	ctrlmmr_unlock(WKUP_CTRL_MMR_BASE, 6);

	/* Unlock mcu_ctrl_mmr region 0,2 */
	ctrlmmr_unlock(MCU_CTRL_MMR_BASE, 0); // same as Lpm_ddrUnlockMCU(0)
	ctrlmmr_unlock(MCU_CTRL_MMR_BASE, 2);
#endif

	/* start of enter_io_ddr_mode */
	/* Disable self refresh auto entry and exit */
	Write_MMR_Field(DDR_CTRL_BASE + DENALI_CTL_169__SFR_OFFS, 0, 4, 16);
	Write_MMR_Field(DDR_CTRL_BASE + DENALI_CTL_169__SFR_OFFS, 0, 4, 24);

	/* Set valid data for FSP F0 and F2 mr_fsp_data_valid_fN to initiate DFS request */
	Write_MMR_Field(DDR_CTRL_BASE + DENALI_CTL_279__SFR_OFFS, 1, 1, 24);
	Write_MMR_Field(DDR_CTRL_BASE + DENALI_CTL_280__SFR_OFFS, 1, 1, 8);

	/* Shift to boot frequency */
	fsp_shift();

	/* If shift is not successful, then return fail */
	if (((readl(DDR_CTRL_BASE + DENALI_CTL_179__SFR_OFFS) & 0x3000000U) >> 24U) != 0U) {
		Lpm_debugFullPrintf("Failed shifting DDR to boot frequency\n");
	}

	enter_lpm_self_refresh();
	put_ddrss_in_data_retention_thru_wkup_mmr(DDR16SS_RETENTION_EN);
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
    Lpm_debugFullPrintf("Lpm_readPmic: reg=0x%x 0x%x\n", reg, rxd);

    return(rxd);
}

static void Lpm_writePmic(uint8_t reg, uint8_t val)
{
    Lpm_i2cConfigWkup(PMIC_ADDR);
    Lpm_i2cWrite(reg, val);
    Lpm_debugFullPrintf("Lpm_writePmic: reg=0x%x 0x%x\n", reg, val);
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

#define PMIC_CONFIG1_REGADDR                   (0x7dU)
#define PMIC_FSM_I2C_TRIGGERS_REGADDR          (0x85U)
#define PMIC_FSM_NSLEEP_TRIGGERS_REGADDR       (0x86U)

#define PMIC_NSLEEP1_MASK BIT(6)
#define PMIC_NSLEEP2_MASK BIT(7)

#define SCICLIENT_LPM_FSM_I2C_TRIGGERS (0x80)
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

static void Lpm_dumpPmic(void)
{
	for (uint8_t i = 1; i < 0xF2; i++)
	    Lpm_debugReadPmic(i);
}

#ifndef __maybe_unused
#define __maybe_unused __attribute__((__unused__)
#endif

__maybe_unused static void Lpm_setupPmic(void)
{
	uint8_t val;

	/* Write 0x02 to FSM_NSLEEP_TRIGGERS register
	   This should happen before clearing the interrupts */

	/* If you clear the interrupts before you write the NSLEEP bits,
	 * it will transition to S2R state.
	 * This is because as soon as you write NSLEEP2 to 0x0,
	 * the trigger is present to move to S2R state.
	 * By setting the NSLEEP bits before you clear the interrupts,
	 * you can configure both NSLEEP bits before the PMIC reacts to the change.
	 */
Lpm_dumpPmic();
	/* Change FSM_NSLEEP_TRIGGERS: NSLEEP1=high, NSLEEP2=high */
	Lpm_writePmic(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR, 0x03);
	Lpm_debugReadPmic(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR);

	/* Clear interrupts */
	Lpm_i2cConfigWkup(PMIC_ADDR);
	Lpm_ClearPmicInterrupts();

	/* unmask NSLEEP2 */
	val = Lpm_readPmic(PMIC_CONFIG1_REGADDR);
	val &= ~(PMIC_NSLEEP2_MASK | PMIC_NSLEEP1_MASK);
	Lpm_writePmic(PMIC_CONFIG1_REGADDR, val);
	Lpm_debugReadPmic(PMIC_CONFIG1_REGADDR);

	/* Write magic number to scratch register to indicate the suspend */
	Lpm_writePmic(SCICLIENT_LPM_SCRATCH_PAD_REG_3, SCICLIENT_LPM_MAGIC_SUSPEND);
	Lpm_debugReadPmic(SCICLIENT_LPM_SCRATCH_PAD_REG_3);

	Lpm_debugReadPmic(SCICLIENT_LPM_INT_TOP);

	/* Change FSM_NSLEEP_TRIGGERS: NSLEEP1=high, NSLEEP2=low */
	Lpm_writePmic(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR, 0x01);
	/*
	 * TODO: this triggers the suspend sequence right away,
	 * and also wake up right away
	 */
	val = Lpm_readPmic(PMIC_FSM_I2C_TRIGGERS_REGADDR);
	// val |= 0x1; // pmic doesn't wakeup
	val = 0x1; // pmic cuts power and restart right away
	Lpm_writePmic(PMIC_FSM_I2C_TRIGGERS_REGADDR, val);
	Lpm_debugReadPmic(PMIC_FSM_I2C_TRIGGERS_REGADDR);

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

	/* Make sure that nothing remains in cache before going to retention */
	Lpm_cleanAllDCache();

	Lpm_ddrEnterRetention();
	dbg_line("Lpm_enterRetention: DDR retention done");

	dbg_line("Lpm_enterRetention: Done! Going to wait now");

	Lpm_setupPmic();
	while(1){};
}

