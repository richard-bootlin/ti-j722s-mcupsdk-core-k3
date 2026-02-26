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
#include <cslr_main_ctrl_mmr.h>
#include <cslr_mcu_ctrl_mmr.h>
#include <cslr_wkup_ctrl_mmr.h>
#include <cslr_main_pll_mmr.h>
#include <cslr_mcu_pll_mmr.h>
#include <cslr_main_padcfg_ctrl_mmr.h>
#include <ddr_functions.h>
#include <DDRSS_addr_map_sfr_offs_ew_32bit.h>
#include <lib/bitops.h>
#include <lpm/cdns_generated_defines/cdns_lpddr4_reg_config_3733_svb.h>
#include <lpm/cdns_generated_defines/cslr_emif.h>
#include <lpm/timeout.h>
#include <wkup_ctrl_mmr.h>
#include "dbg_uart.c"

#define SAVV_CORE_DATA_BARRIER  __asm volatile("   dsb          ;");
#define SAVV_CORE_INSTR_BARRIER __asm volatile("   isb          ;");
#define SAVV_CORE_WFI           __asm volatile(" wfi;");

#define printf Lpm_debugPrintf
#define Lpm_debugFullPrintf Lpm_debugPrintf
#define Lpm_debugReadPmic Lpm_readPmic

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


static void Lpm_cleanAllDCache(void)
{

	unsigned int set, way;

	for(way = 0; way < 4 ; way ++)
		for(set = 0; set < 0x100; set++) {
			unsigned int val = set << 4 | way << 30;
			asm("    dmb");
			__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 2\t\n": "=r"(val));
			__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 2\t\n": "=r"(val));
			asm("    dsb");
		}
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

#ifndef s32
#define s32 int32_t
#endif
#ifndef u32
#define u32 uint32_t
#endif

#define SOC_read32(a) readl(a)
#define SOC_write32(a,v) writel(v,a)

#define ctrlmmr_raw_readl readl
#define ctrlmmr_raw_writel writel


static void asm_func(void)
{
	asm ("");
}

void delay_1us(void)
{
	/* This while-loop takes 2 instructions. */
	unsigned long x = DM_R5_CORE_FREQUENCY_MHZ / 2;

	while (x != 0U) {
		x--;
		asm_func();
	}
}

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
		dbg_line("Failed freq change request\n");
		ret = -1;
	}

	/* Set the PLL frequency to the requested frequency */
	val = (readl(WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_REQ)) & DDR4_FSP_CLKCHNG_REQ_TYPE_MASK;
	if (val == DDR4_FSP_CLKCHNG_REQ_TYPE_FSP0) {
		pll_bypass(&main_pll12, 1);
	} else {
		dbg_line("Failed setting PLL frequency to the requested frequency\n");
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
		dbg_line("Timeout waiting for request to go away\n");
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
		dbg_line("Timeout waiting for CHNG_DDR4_FSP_ACK bit to be 1\n");
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
void Write_MMR_Field(uint32_t mmr_address, uint32_t field_value,
		     uint32_t width, uint32_t leftshift)
{
    uint32_t * p_mmr;
    uint32_t mask;
    p_mmr = (uint32_t *) mmr_address;   //Grab the MMR value
    mask = ((1 << width) - 1) << leftshift; //Build a mask of 1s for the field.
    mask = ~(mask); //Invert the mask so that the field will be zero'd out with the AND operation.
    *p_mmr &= mask; //Zero out the field in the register.
    *p_mmr |= (field_value << leftshift); //Assign the value to that specific field.
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

static s32 load_magic_words_through_mmr(void)
{
	u32 timeout = TIMEOUT_10_MS;
	s32 ret = 0;

	/* Program the OFF mode MMR in case of IO Only plus DDR mode. */
	writel(WKUP_CANUART_OFF_MAGIC_WORD, WKUP_CTRL_MMR_BASE + CANUART_WAKE_OFF_MODE);

	/* Program the CAN IO MMR. */
	writel(0x0U, WKUP_CTRL_MMR_BASE + CANUART_WAKE_CTRL);
	writel((WKUP_CANUART_MAGIC_WRD | WKUP_CANUART_MAGIC_WRD_LD_EN), WKUP_CTRL_MMR_BASE + CANUART_WAKE_CTRL);

	/* Wait for CAN_ONLY_IO signal to be 1 */
	while ((timeout > 0U) && ((readl(WKUP_CTRL_MMR_BASE + CANUART_WAKE_STAT1)) != WKUP_CANUART_CAN_IO_ISO_SET)) {
		--timeout;
	}
	if (timeout == 0U) {
		ret = -1;
	}

	/* Clear the magic word to prevent any other word loading */
	writel((~WKUP_CANUART_MAGIC_WRD) | WKUP_CANUART_MAGIC_WRD_LD_DIS, WKUP_CTRL_MMR_BASE + CANUART_WAKE_CTRL);

	return ret;
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

	if (load_magic_words_through_mmr() != 0)
		dbg_line("load_magic_words_through_mmr failed");


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
		dbg_line("Failed shifting DDR to boot frequency\n");
	}

	enter_lpm_self_refresh();
	put_ddrss_in_data_retention_thru_wkup_mmr(DDR16SS_RETENTION_EN);

	writel(0, (WKUP_CTRL_MMR_BASE + WKUP0_EN));
	/* Ensure that PMIC EN control from SOC is selected */
	writel((WKUP0_PMCTRL_SYS_LPM_EN_PMIC | WKUP0_LPM_PMIC_OUT_EN), (WKUP_CTRL_MMR_BASE + PMCTRL_SYS));

	/* Enter IO DDR mode */
	writel((WKUP0_PMCTRL_SYS_LPM_EN_PMIC | WKUP0_LPM_PMIC_OUT_DIS), WKUP_CTRL_MMR_BASE + PMCTRL_SYS);
	writel(WWD_STOP, WKUP_CTRL_MMR_BASE + WKUP_WWD0_CTRL);
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
        dump_val("Lpm_i2cReadTimeout: timeout loop exceed ", timeout);
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
        dump_val("Lpm_i2cReadTimeout: timeout for XRDY: loop exceed ", timeout);
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
        dump_val("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed ", timeout);
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
        dump_val("Lpm_i2cReadTimeout: timeout for RRDY: loop exceed ", timeout);
        return(-1);
    }

    *rxd = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_DATA);

    /* wait ARDY --> 1 */
    while(((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) &&
          loop++ < timeout) {}

    if(loop >= timeout)
    {
        dump_val("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed ", timeout);
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
    dump_val2("Lpm_readPmic: reg=", reg, rxd);

    return(rxd);
}

static void Lpm_writePmic(uint8_t reg, uint8_t val)
{
    Lpm_i2cConfigWkup(PMIC_ADDR);
    Lpm_i2cWrite(reg, val);
    dump_val2("Lpm_writePmic: reg=", reg, val);
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
        dump_val("INT_BUCK", val);
        if (val & 1)
        {
            val1 = Lpm_i2cRead(0x5C);
            dump_val("INT_BUCK1_2", val1);
            Lpm_i2cWrite(0x5C, val1);
        }

        if(val & (1 << 1))
        {
            val1 = Lpm_i2cRead(0x5D);
            dump_val("INT_BUCK3_4", val1);
            Lpm_i2cWrite(0x5D, val1);
        }

        if(val & (1 << 2))
        {
            val1 = Lpm_i2cRead(0x5E);
            dump_val("INT_BUCK5", val1);
            Lpm_i2cWrite(0x5E, val1);
        }
    }

    if(int_top & (1 << 1))
    {
        val = Lpm_i2cRead(0x5F);
        dump_val("INT_LDO_VMON", val);
        if(val & 1)
        {
            val1 = Lpm_i2cRead(0x60);
            dump_val("INT_LDO1_2", val1);
            Lpm_i2cWrite(0x60, val1);
        }
        if(val & (1 << 1))
        {
            val1 = Lpm_i2cRead(0x61);
            dump_val("INT_LDO3_4", val1);
            Lpm_i2cWrite(0x61, val1);
        }
        if(val & (1 << 2))
        {
            val1 = Lpm_i2cRead(0x62);
            dump_val("INT_VMON", val1);
            Lpm_i2cWrite(0x62, val1);
        }
    }

    if(int_top & (1 << 2))
    {
        val = Lpm_i2cRead(0x63);
        dump_val("INT_GPIO", val);
        if(val & (1 << 3))
        {
            val1 = Lpm_i2cRead(0x64);
            dump_val("INT_GPIO1_8", val1);
            Lpm_i2cWrite(0x64, val1);
        }
        Lpm_i2cWrite(0x63, val); // clear GPIO9, GPIO10, GPIO11
    }

    if(int_top & (1 << 3))
    {
        val = Lpm_i2cRead(0x65);
        dump_val("INT_STARTUP", val);
        Lpm_i2cWrite(0x65, val);
    }

    if(int_top & (1 << 4))
    {
        val = Lpm_i2cRead(0x66);
        dump_val("INT_MISC", val);
        Lpm_i2cWrite(0x66, val);
    }

    if(int_top & (1 << 5))
    {
        val = Lpm_i2cRead(0x67);
        dump_val("INT_MODERATE_ERR", val);
        Lpm_i2cWrite(0x67, val);
    }

    if(int_top & (1 << 6))
    {
        val = Lpm_i2cRead(0x68);
        dump_val("INT_SEVERE_ERR", val);
        Lpm_i2cWrite(0x68, val);
    }

    if(int_top & (1 << 7))
    {
        val = Lpm_i2cRead(0x69);
        dump_val("INT_FSM_ERR", val);
        if(val & (1 << 4))
        {
            val1 = Lpm_i2cRead(0x6A);
            dump_val("INT_COMM_ERR", val1);
            Lpm_i2cWrite(0x6A, val1);
        }
        if(val & (1 << 5))
        {
            val1 = Lpm_i2cRead(0x6B);
            dump_val("INT_READBACK_ERR", val1);
            Lpm_i2cWrite(0x6B, val1);
        }
        if(val & (1 << 6))
        {
            val1 = Lpm_i2cRead(0x6C);
            dump_val("INT_ESM", val1);
            Lpm_i2cWrite(0x6C, val1);
        }
        Lpm_i2cWrite(0x69, val); // clear INT_FSM_ERR
    }

    int_top = Lpm_i2cRead(0x5A);
    dump_val("INT_TOP", int_top);
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
if (0) {
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
 val |= 0x1; // pmic doesn't wakeup
//	val = 0x1; // pmic cuts power and restart right away
	Lpm_writePmic(PMIC_FSM_I2C_TRIGGERS_REGADDR, val);
	Lpm_debugReadPmic(PMIC_FSM_I2C_TRIGGERS_REGADDR);
} else {
	/* Write magic number to scratch register to indicate the suspend */
	Lpm_writePmic(SCICLIENT_LPM_SCRATCH_PAD_REG_3, SCICLIENT_LPM_MAGIC_SUSPEND);
	Lpm_debugReadPmic(SCICLIENT_LPM_SCRATCH_PAD_REG_3);

#if 0
Abhash Kumar 18/02/2026 06:37 • Could you try this PMIC sequence on j722s.

(Set PMIC to ACTIVE state)
i2c mw 48 86 3 1

(read active interrupts in INT_TOP)
i2c md 48 5a 1

(mask GPIO1_FALL interrupt)
i2c mw 48 4f 3f 1

(clear remaining interrupts)
i2c mw 48 66 01 1
i2c mw 48 65 01 1

(unmask NSLEEP2)
i2c mw 48 7d 43 1

(read FSM_I2C_TRIGGERS)
i2c md 48 85 1

(I2C_TRIGGERS_7 + I2C_TRIGGERS_5) -> io+ddr retained
i2c mw 48 85 a0 1
---------- OR -------------
(Only I2C_TRIGGERS_5) -> only io retained
i2c mw 48 85 20 1

(Make NSLEEP2_BIT = 0) 
i2c mw 48 86 1 1

(create falling edge on PMIC_GPIO1)
mw.w 0x43018080 0x00

Then,
After masking GPIO1_FALL interrupt add the following write

(unmask GPIO1_RISE_MASK)
i2c mw 48 50 3e

Then rest of the sequence to enter to low power.

Once in low power, raise GPIO1


#endif
	/* Set PMIC to ACTIVE state */
	Lpm_writePmic(0x86, 0x03);
	/* read active interrupts in INT_TOP */
	Lpm_debugReadPmic(0x5a);
	/* mask GPIO1_FALL interrupt */
	Lpm_writePmic(0x4f, 0x3f);
	/* unmask GPIO1_RISE_MASK */
	Lpm_writePmic(0x50, 0x3e);
	/* clear remaining interrupts */
	Lpm_writePmic(0x66, 0x01);
	Lpm_writePmic(0x65, 0x01);
	/* unmask NSLEEP2 */
	Lpm_writePmic(0x7d, 0x43);
	/* read FSM_I2C_TRIGGERS */
	Lpm_debugReadPmic(0x85);
	/* I2C_TRIGGERS_7 + I2C_TRIGGERS_5 -> io+ddr retained */
	Lpm_writePmic(0x85, 0xa0);
	/*
	 * Or:
	 * Only I2C_TRIGGERS_5 -> only io retained
	 * Lpm_writePmic(0x85, 0x20);
	 */
	/* Make NSLEEP2_BIT = 0) */
	Lpm_writePmic(0x86, 0x01);

	/* create falling edge on PMIC_GPIO1 */
	writel(0, 0x43018080);


}

}


#define DDRSS_CTL_BASE DDR_CTRL_BASE
#ifndef AVV_PASS
#define AVV_PASS		(0)
#endif

#ifndef AVV_FAIL
#define AVV_FAIL		(1)
#endif
#define SOC_AM62PX
#ifdef SOC_AM62PX //AM62P
    #ifndef __SOC_DEFINED
        #define __SOC_DEFINED
    #else
        #define __SOC_MULTIPLE_DEFINES
    #endif
#endif
#ifdef __SOC_MULTIPLE_DEFINES
#error multiple socs defined
#endif

/* define the unlock and lock values */
#define KICK0_UNLOCK_VAL 0x68EF3490
#define KICK1_UNLOCK_VAL 0xD172BC5A
#define KICK_LOCK_VAL    0x00000000

#define _SAVV_MMR_HAS_MAIN_CTRL_MMR
#define _SAVV_MMR_HAS_MCU_CTRL_MMR
#define _SAVV_MMR_HAS_WKUP_CTRL_MMR
#define _SAVV_MMR_HAS_MAIN_PLL_MMR
#define _SAVV_MMR_HAS_MCU_PLL_MMR
#define _SAVV_MMR_HAS_MAIN_PADCONFIG_MMR
#define _SAVV_MMR_HAS_MCU_PADCONFIG_MMR

#define MAIN_CTRL_MMR_BASE_ADDRESS          CSL_CTRL_MMR0_CFG0_BASE
#define MCU_CTRL_MMR_BASE_ADDRESS           CSL_MCU_CTRL_MMR0_CFG0_BASE
#define WKUP_CTRL_MMR_BASE_ADDRESS          CSL_WKUP_CTRL_MMR0_CFG0_BASE

#define MAIN_PLL_MMR_BASE_ADDRESS	        CSL_PLL0_CFG_BASE
#define MCU_PLL_MMR_BASE_ADDRESS            CSL_WKUP_PLL0_CFG_BASE

#define MAIN_PADCONFIG_MMR_BASE_ADDRESS     CSL_PADCFG_CTRL0_CFG0_BASE
#define MCU_PADCONFIG_MMR_BASE_ADDRESS      CSL_MCU_PADCFG_CTRL0_CFG0_BASE

#define MAIN_SEC_MMR_BASE_ADDRESS           CSL_MAIN_SEC_MMR0_CFG0_BASE
#define MCU_SEC_MMR_BASE_ADDRESS            CSL_MCU_MCU_SEC_MMR0_CFG0_BASE
#define WKUP_SEC_MMR_BASE_ADDRESS           CSL_WKUP_WKUP_SEC_MMR0_CFG0_BASE

static const uint32_t main_ctrl_mmr_kick_offsets[]= {  CSL_MAIN_CTRL_MMR_CFG0_LOCK0_KICK0,
                                                CSL_MAIN_CTRL_MMR_CFG0_LOCK1_KICK0,
                                                CSL_MAIN_CTRL_MMR_CFG0_LOCK2_KICK0,
                                                //CSL_MAIN_CTRL_MMR_CFG0_LOCK3_KICK0,
                                                CSL_MAIN_CTRL_MMR_CFG0_LOCK4_KICK0,
                                                //CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK0,
                                                CSL_MAIN_CTRL_MMR_CFG0_LOCK6_KICK0,
                                                //CSL_MAIN_CTRL_MMR_CFG0_LOCK7_KICK0,
                                             };
static const uint32_t main_ctrl_mmr_kick_num = sizeof(main_ctrl_mmr_kick_offsets)/sizeof(uint32_t);

static const uint32_t mcu_ctrl_mmr_kick_offsets[]= {   CSL_MCU_CTRL_MMR_CFG0_LOCK0_KICK0,
                                                CSL_MCU_CTRL_MMR_CFG0_LOCK1_KICK0,
                                                CSL_MCU_CTRL_MMR_CFG0_LOCK2_KICK0,
                                                CSL_MCU_CTRL_MMR_CFG0_LOCK3_KICK0,
                                                CSL_MCU_CTRL_MMR_CFG0_LOCK4_KICK0,
                                                //CSL_MCU_CTRL_MMR_CFG0_LOCK5_KICK0,
                                                CSL_MCU_CTRL_MMR_CFG0_LOCK6_KICK0,
                                                //CSL_MCU_CTRL_MMR_CFG0_LOCK7_KICK0,
                                             };
static const uint32_t mcu_ctrl_mmr_kick_num = sizeof(mcu_ctrl_mmr_kick_offsets)/sizeof(uint32_t);

static const uint32_t wkup_ctrl_mmr_kick_offsets[]= {   CSL_WKUP_CTRL_MMR_CFG0_LOCK0_KICK0,
                                                        CSL_WKUP_CTRL_MMR_CFG0_LOCK1_KICK0,
                                                        CSL_WKUP_CTRL_MMR_CFG0_LOCK2_KICK0,
                                                        CSL_WKUP_CTRL_MMR_CFG0_LOCK3_KICK0,
                                                        CSL_WKUP_CTRL_MMR_CFG0_LOCK4_KICK0,
                                                        CSL_WKUP_CTRL_MMR_CFG0_LOCK5_KICK0,
                                                        CSL_WKUP_CTRL_MMR_CFG0_LOCK6_KICK0,
                                                        CSL_WKUP_CTRL_MMR_CFG0_LOCK7_KICK0,
                                                    };
static const uint32_t wkup_ctrl_mmr_kick_num = sizeof(wkup_ctrl_mmr_kick_offsets)/sizeof(uint32_t);

static const uint32_t main_pll_mmr_kick_offsets[]= {  CSL_MAIN_PLL_MMR_CFG_PLL0_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL1_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL2_LOCKKEY0,
                                               //CSL_MAIN_PLL_MMR_CFG_PLL3_LOCKKEY0,
                                               //CSL_MAIN_PLL_MMR_CFG_PLL4_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL5_LOCKKEY0,
                                               //CSL_MAIN_PLL_MMR_CFG_PLL6_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL7_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL8_LOCKKEY0,
                                               //CSL_MAIN_PLL_MMR_CFG_PLL9_LOCKKEY0,
                                               //CSL_MAIN_PLL_MMR_CFG_PLL10_LOCKKEY0,
                                               //CSL_MAIN_PLL_MMR_CFG_PLL11_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL12_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL15_LOCKKEY0,
                                               CSL_MAIN_PLL_MMR_CFG_PLL17_LOCKKEY0,
                                             };
static const uint32_t main_pll_mmr_kick_num = sizeof(main_pll_mmr_kick_offsets)/sizeof(uint32_t);

static const uint32_t mcu_pll_mmr_kick_offsets[]= {  CSL_MCU_PLL_MMR_CFG_PLL0_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL1_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL2_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL3_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL4_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL5_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL6_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL7_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL8_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL9_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL10_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL11_LOCKKEY0,
                                              //CSL_MCU_PLL_MMR_CFG_PLL12_LOCKKEY0,
                                            };
static const uint32_t mcu_pll_mmr_kick_num = sizeof(mcu_pll_mmr_kick_offsets)/sizeof(uint32_t);

static const uint32_t main_padcfg_mmr_kick_offsets[]= {  CSL_MAIN_PADCFG_CTRL_MMR_CFG0_LOCK0_KICK0,
                                                         CSL_MAIN_PADCFG_CTRL_MMR_CFG0_LOCK1_KICK0,
                                               };
static const uint32_t main_padcfg_mmr_kick_num = sizeof(main_padcfg_mmr_kick_offsets)/sizeof(uint32_t);

static const uint32_t mcu_padcfg_mmr_kick_offsets[]= {   CSL_MCU_PADCFG_CTRL_MMR_CFG0_LOCK0_KICK0,
                                                         CSL_MCU_PADCFG_CTRL_MMR_CFG0_LOCK1_KICK0,
                                              };
static const uint32_t mcu_padcfg_mmr_kick_num = sizeof(mcu_padcfg_mmr_kick_offsets)/sizeof(uint32_t);

typedef enum {
    MMR_UNLOCK=1,
    MMR_LOCK=0
} mmr_lock_actions_t;
uint32_t MMR_change_lock(mmr_lock_actions_t target_state, uint32_t * kick0) {
	uint32_t * kick1 = kick0 + 1;
	uint32_t lock_state = (*kick0 & 0x1); //status is 1 if unlocked, 0 if locked

	//If lock state is not what we want, change it
	if (lock_state != (uint32_t) target_state ) {
		switch(target_state) {
		case MMR_LOCK:
			// lock the partition by writing the lock values to the kick lock registers
			*kick0 = KICK_LOCK_VAL;
			*kick1 = KICK_LOCK_VAL;
			break;
		case MMR_UNLOCK:
			// unlock the partition by writing the unlock values to the kick lock registers
			*kick0 = KICK0_UNLOCK_VAL;
			*kick1 = KICK1_UNLOCK_VAL;
			break;
		}

		lock_state = (*kick0 & 0x1);
		SAVV_CORE_DATA_BARRIER;
		//Error out if the change did not take effect
		if(lock_state!= (uint32_t) target_state ){
			//Could insert debug statement here
			//printf("SAVV_DEBUG: Error in changing MMR lock state at address %llx", kick0 );
			return AVV_FAIL;
		}
	}
	//Return pass if lock is already what we want or if changing lock succeeds
	return AVV_PASS;
}
uint32_t generic_mmr_change_all_locks(mmr_lock_actions_t target_state, uint32_t base_addr, const uint32_t * offset_array, uint32_t array_size) {
	uint32_t errors=0;
	uint32_t i=0;
	uint32_t * kick0_ptr;
	for(i=0;i<array_size;i++) {
		kick0_ptr = (uint32_t *) (base_addr + offset_array[i]);
		if(MMR_change_lock(target_state, kick0_ptr) == AVV_FAIL){
			errors++;
		}
	}
	return errors;
}

uint32_t WKUP_CTRL_MMR_change_all_locks(mmr_lock_actions_t target_state) {
	uint32_t errors=generic_mmr_change_all_locks(target_state, (uint32_t) WKUP_CTRL_MMR_BASE_ADDRESS, wkup_ctrl_mmr_kick_offsets, wkup_ctrl_mmr_kick_num);
	if(errors==0) { return AVV_PASS; }
	else          { return AVV_FAIL; }
}

uint32_t WKUP_CTRL_MMR_unlock_all() {
	return WKUP_CTRL_MMR_change_all_locks(MMR_UNLOCK);
}
uint32_t WKUP_CTRL_MMR_lock_all() {
	return WKUP_CTRL_MMR_change_all_locks(MMR_LOCK);
}
uint32_t MCU_PADCONFIG_MMR_change_all_locks(mmr_lock_actions_t target_state) {
	uint32_t errors=generic_mmr_change_all_locks(target_state, (uint32_t) MCU_PADCONFIG_MMR_BASE_ADDRESS, mcu_padcfg_mmr_kick_offsets, mcu_padcfg_mmr_kick_num);
	if(errors==0) { return AVV_PASS; }
	else          { return AVV_FAIL; }
}

uint32_t MCU_PADCONFIG_MMR_unlock_all() {
	return MCU_PADCONFIG_MMR_change_all_locks(MMR_UNLOCK);
}
uint32_t MCU_PADCONFIG_MMR_lock_all() {
	return MCU_PADCONFIG_MMR_change_all_locks(MMR_LOCK);
}
#define DDR4 1
#define LPDDR4 2
//Enables DDR SR and data retention
uint32_t ddrLPM_Entry (uint32_t memType)
{
    volatile uint32_t *mmrPtr;
    uint32_t lp_status = 0;

    //Configure PHY for LPI Wakeup
    //DDR4 - write lpi_sr_long_mcclk_gate_wakeup registers
    if(memType == DDR4){

        mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_161);
        *mmrPtr = *mmrPtr & 0xFFFF0000;
        *mmrPtr = *mmrPtr | (0x0101 << 0);

        mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_163);
        *mmrPtr = *mmrPtr & 0xFF0000FF;
        *mmrPtr = *mmrPtr | (0x0101 << 8);

        mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_165);
        *mmrPtr = *mmrPtr & 0x0000FFFF;
        *mmrPtr = *mmrPtr | (0x0101 << 16);
    }
    //LPDD4 - write lpi_srpd_long_mcclk_gate_wakeup registers
    else{

        mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_162);
        *mmrPtr = *mmrPtr & 0xFFFF0000;
        *mmrPtr = *mmrPtr | (0x0101 << 0);

        mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_164);
        *mmrPtr = *mmrPtr & 0xFF0000FF;
        *mmrPtr = *mmrPtr | (0x0101 << 8);

        mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_166);
        *mmrPtr = *mmrPtr & 0x0000FFFF;
        *mmrPtr = *mmrPtr | (0x0101 << 16);
    }

    //configure phy_lp_wakeup
    mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_PHY_1835);
    *mmrPtr = *mmrPtr & 0xFFFFFF00;
    *mmrPtr = *mmrPtr | 0x7;

    //configure lpi_wakeup enable
    mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_167);
    *mmrPtr = *mmrPtr & 0xFFFF00FF;
    *mmrPtr = *mmrPtr | (0xF << 8);


    //Enter SR long with Mem clock gating
    //Program self refresh mode
    mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_160);
    *mmrPtr = (0x51<< 8); //LP_MODE_LONG_SELF_REFRESH

    //poll self refresh mode change
    mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + CSL_EMIF_CTLCFG_DENALI_CTL_345);
    while(lp_status != 0x10000)
        lp_status = *mmrPtr & 0x10000;
#define CSL_WKUP_CTRL_MMR_CFG0_DDR16SS_PMCTRL                            (0x000080D0U)

    //enable ddr date retention
    Write_MMR_Field (WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_DDR16SS_PMCTRL, 0x6, 4, 0);

    return 0;
}

uint32_t disableLPM_PMIC_EN(void)
{
//    WKUP_CTRL_MMR_unlock_all();

    //Set pmic_en=0 and lpm_en=0 (note write enable bit fields are set)
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS +CSL_WKUP_CTRL_MMR_CFG0_PMCTRL_SYS, 0x0, 32, 0);
    return 0;
}


uint32_t ioIsolationEnable()
{
//    WKUP_CTRL_MMR_unlock_all();

    //Set io_iso_ctrl_0
    uint32_t * pmctrl_IO0 = (uint32_t *)(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_PMCTRL_IO_0);
    *pmctrl_IO0 = *pmctrl_IO0 | (1<<24);

    //Set io_iso_ctrl_1
    uint32_t * pmctrl_IO1 = (uint32_t *)(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_PMCTRL_IO_1);
    *pmctrl_IO1 = *pmctrl_IO1 | (1<<24);

    return 0;
}

#define WKUP_SOURCES 0x40000   //CANUART IO Daisy Chain

#define CANIO_MW       0xAAAAAAAA  //CANIO Magic Word (Note: 0xAAAAAAAA will be left shifted 1 when written)
#define CANIO_NOT_MW   0x12345678

#define CANIO_WAKE_RESUME_KEY0_VAL 0x0000FFFF
#define CANIO_WAKE_RESUME_KEY1_VAL 0x1111FFFF
#define CANIO_WAKE_RESUME_KEY2_VAL 0x2222FFFF
#define CANIO_WAKE_RESUME_KEY3_VAL 0x3333FFFF
#define CANIO_WAKE_OFF_MODE_MW_VAL 0x12555555
uint32_t configureCANIO_Wakeup()
{
    int i=0;

//    WKUP_CTRL_MMR_unlock_all();
//    MCU_PADCONFIG_MMR_unlock_all();

    //Set global_wuen0
    uint32_t * pmctrl_IO0 = (uint32_t *)(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_PMCTRL_IO_0);
    *pmctrl_IO0 = *pmctrl_IO0 | (1<<16);

    //Enables Wakeup from CANUART IO Daisy Chain
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_WKUP0_EN, WKUP_SOURCES, 32, 0);

    //Set (CANUART PADs) PADCFG5-16 registers  - wkup enabled, input mode enabled, GPIO mode
    for (i=5; i<=16; i++)
    {
        //MCU_PADCGF10(WKUP_UART0_TX) not configured as wakeup src.EVM has pullup on WKUP_UART0_TX that will create a wakeup event and causes PMIC_LPM_EN stay high
        if (i!=13) //if ((i==10) || (i == 9) || (i==5) || (i==6))
            continue;

        Write_MMR_Field(MCU_PADCONFIG_MMR_BASE_ADDRESS + (0x4000+(4*i)), 1, 1, 29);     //wkup_en=1
        Write_MMR_Field(MCU_PADCONFIG_MMR_BASE_ADDRESS + (0x4000+(4*i)), 1, 1, 18);     //RX enabled = 1
        Write_MMR_Field(MCU_PADCONFIG_MMR_BASE_ADDRESS + (0x4000+(4*i)), 0, 1, 17);     //pulldown selected
        Write_MMR_Field(MCU_PADCONFIG_MMR_BASE_ADDRESS + (0x4000+(4*i)), 0, 1, 16);     //pull up/down enabled
        Write_MMR_Field(MCU_PADCONFIG_MMR_BASE_ADDRESS + (0x4000+(4*i)), 7, 3, 0);      //MUX mode=7 GPIO mode
        Write_MMR_Field(MCU_PADCONFIG_MMR_BASE_ADDRESS + (0x4000+(4*i)), 1, 1, 7);      //wk_lvl_en = 1
        Write_MMR_Field(MCU_PADCONFIG_MMR_BASE_ADDRESS + (0x4000+(4*i)), 1, 1, 8);      //wk_pol = 1
    }

    return 0;
}

uint32_t enterCANIORetention()
{
//    WKUP_CTRL_MMR_unlock_all();

    //Set CANIO Resume Keys
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_RESUME_KEY(0), CANIO_WAKE_RESUME_KEY0_VAL, 32, 0);
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_RESUME_KEY(1), CANIO_WAKE_RESUME_KEY1_VAL, 32, 0);
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_RESUME_KEY(2), CANIO_WAKE_RESUME_KEY2_VAL, 32, 0);
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_RESUME_KEY(3), CANIO_WAKE_RESUME_KEY3_VAL, 32, 0);
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_OFF_MODE, CANIO_WAKE_OFF_MODE_MW_VAL, 32, 0);

    //add delay
    for (int i=0; i<=100; i++);

    //Set CAN IO magic word
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_CTRL, CANIO_MW, 31, 1);

    //Loads and locks the MW field for CAN IO isolation.
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_CTRL, 0x1, 1, 0);

    //add delay
    for (int i=0; i<=100; i++);

    //write non-magic word and set lock bit=1
    Write_MMR_Field(WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_CANUART_WAKE_CTRL, (CANIO_NOT_MW | 0x1) , 32, 0);

    return 0;
}
//
//generate LD to latch data retention
uint32_t generateLD_DataRet(){

    //Set data_ret_ld = 1 to generate a LD signal to latch the retention signal
    Write_MMR_Field (WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_DDR16SS_PMCTRL, 1, 1, 31);

    //Set data_ret_ld = 0 to close the latch
    Write_MMR_Field (WKUP_CTRL_MMR_BASE_ADDRESS + CSL_WKUP_CTRL_MMR_CFG0_DDR16SS_PMCTRL, 0, 1, 31);

    return 0;
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
#define DO_RAM_PATTERN_TEST 0
#define RAM_START 0x80000000U
#define SZ 25000U

	if (DO_RAM_PATTERN_TEST) {
		for (unsigned int i = 0; i < SZ; i++) {
			writel(i, RAM_START + 4 * i);
		}
	}
	/* Make sure that nothing remains in cache before going to retention */
	Lpm_cleanAllDCache();

	if (DO_RAM_PATTERN_TEST) {
		dump_HEX((void*)RAM_START, 2500);
	}

	Lpm_ddrEnterRetention();
	dbg_line("Lpm_enterRetention: DDR retention done");

	dbg_line("Lpm_enterRetention: Done! Going to wait now!");

	Lpm_setupPmic();

	while(1){};
}
