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
		Lpm_debugFullPrintf("Failed freq change request\n");
		ret = -1;
	}

	/* Set the PLL frequency to the requested frequency */
	val = (readl(WKUP_CTRL_MMR_BASE + DDR4_FSP_CLKCHNG_REQ)) & DDR4_FSP_CLKCHNG_REQ_TYPE_MASK;
	if (val == DDR4_FSP_CLKCHNG_REQ_TYPE_FSP0) {
		pll_bypass(&main_pll12, 1);
	} else {
		Lpm_debugFullPrintf("Failed setting PLL frequency to the requested frequency\n");
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
		Lpm_debugFullPrintf("Timeout waiting for request to go away\n");
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
		Lpm_debugFullPrintf("Timeout waiting for CHNG_DDR4_FSP_ACK bit to be 1\n");
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
		Lpm_debugFullPrintf("Failed shifting DDR to boot frequency\n");
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

#define DDRSS_PI_REGISTER_BLOCK__OFFS   0x2000U
#define DDRSS_Data_Slice_0_REGISTER_BLOCK__OFFS 0x4000U
#define DDRSS_Data_Slice_1_REGISTER_BLOCK__OFFS 0x4400U
#define DDRSS_Data_Slice_2_REGISTER_BLOCK__OFFS 0x4800U
#define DDRSS_Data_Slice_3_REGISTER_BLOCK__OFFS 0x4c00U
#define DDRSS_Address_Slice_0_REGISTER_BLOCK__OFFS      0x5000U
#define DDRSS_Address_Slice_1_REGISTER_BLOCK__OFFS      0x5400U
#define DDRSS_Address_Slice_2_REGISTER_BLOCK__OFFS      0x5800U
#define DDRSS_PHY_Core_REGISTER_BLOCK__OFFS     0x5c00U

#define SDRAM_IDX  0x12
#define REGION_IDX 0x12
#define CSL_EMIF_SSCFG_V2A_CTL_REG                                             (0x00000020U)

static struct emif_handle_s Emifhandle = {
	.ss_cfg_base_addr = (u64) (DDRSS0_SS_BASE),
	.ctl_cfg_base_addr = (u64) (DDRSS0_CTRL_BASE)
};

static void configure_sdram_region_idx(struct emif_handle_s *h, u32 sdram_idx, u32 region_idx)
{
	u32 rd_val;

	rd_val = SOC_read32(h->ss_cfg_base_addr + CSL_EMIF_SSCFG_V2A_CTL_REG);
	rd_val = (rd_val & 0xFFFFFC00U);
	rd_val = rd_val | (sdram_idx << 5) | (region_idx);
	SOC_write32((h->ss_cfg_base_addr + CSL_EMIF_SSCFG_V2A_CTL_REG), rd_val); /* Programming the region_idx and sdram_idx fields for address mapping [Set 9:5 and 4:0 to 0x11 for 8GB] */
}

static void start_PI_CTL_init(struct emif_handle_s *h)
{
	u32 wr_init_val;
	u32 i;

	if (h->is_ddr4_mem == 1U) {
		wr_init_val = ((DDR4_DRAM_CLASS_REG_VALUE << 8U) | 0x1U);
	} else { wr_init_val = ((LPDDR4_DRAM_CLASS_REG_VALUE << 8U) | 0x1U); }                                                  /* Decide init value based on memory type */
	SOC_write32(h->ctl_cfg_base_addr + (u32) DDRSS_PI_REGISTER_BLOCK__OFFS + (u32) DENALI_PI_0__SFR_OFFS, wr_init_val);     /* Set START bit in register for PI module */
	for (i = 0; i < 500U; i++) {
		delay_1us();
	}
	SOC_write32(h->ctl_cfg_base_addr + (u32) DENALI_CTL_0__SFR_OFFS, wr_init_val); /* Set START bit in register for controller */
}

static void poll_for_init_completion(struct emif_handle_s *h)
{
    int i = 10000;
#if defined(CTL_INIT_ONLY)
	while (((SOC_read32(h->ctl_cfg_base_addr + DENALI_CTL_350__SFR_OFFS)) & 0x02000000) != 0x02000000) { /* Poll for CTL Init completion */
	}
#elif defined(PI_INIT_ONLY)
	while (((SOC_read32(h->ctl_cfg_base_addr + DDRSS_PI_REGISTER_BLOCK__OFFS + DENALI_PI_87__SFR_OFFS)) & 0x1) != 0x1) { /* Poll for PI Init completion */
	}
#else
	Lpm_debugFullPrintf("wait for PI init\n");
	i = 10000;
	/* Poll for PI Init completion */
	while (((SOC_read32(h->ctl_cfg_base_addr + (u64) DDRSS_PI_REGISTER_BLOCK__OFFS + (u64) DENALI_PI_87__SFR_OFFS)) & 0x1U) != 0x1U) {
		if (i-- < 0) {
			Lpm_debugFullPrintf("TIMEOUT on Poll for PI Init completion\n");

			break;
		}
	}
	Lpm_debugFullPrintf("wait for ctl init\n");
	// TODO: we are stuck in there:
	i = 10000;
	/* Poll for CTL Init completion */
	while (((SOC_read32(h->ctl_cfg_base_addr + (u64) DENALI_CTL_350__SFR_OFFS)) & 0x02000000U) != 0x02000000U) {
		if (i-- < 0) {
			Lpm_debugFullPrintf("TIMEOUT on Poll for CTL Init completion\n");

			break;
		}
	}
#endif
}

void configure_CTL_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_REG_BASE = h->ctl_cfg_base_addr;
	uint32_t wdata_340, wdata_322, wdata_323;
	uint32_t cs_msk, cs_val_lower, cs_val_upper, i;

	for (i = 0; i <= 434U; i++) {
		SOC_write32(DDR_CTL_REG_BASE + (i * 4U), denali_ctl_data[i]);
	}

	/* Disable bank group rotation for DDR4 */
#if defined(DDR4_MEM)
	wdata_340 = DENALI_CTL_340_DATA;
	wdata_340 = wdata_340 & 0xFEFFFFFF;
	SOC_write32(DDR_CTL_REG_BASE + DENALI_CTL_340__SFR_OFFS, wdata_340);
#endif

	/*
	 * DENALI_CTL_322_DATA - BANK_DIFF_1:RW:24:2:=0x01 BANK_DIFF_0:RW:16:2:=0x01 ZQ_CAL_LATCH_MAP_1:RW_D:8:2:=0x00 ZQ_CAL_START_MAP_1:RW_D:0:2:=0x00
	 * DENALI_CTL_323_DATA - COL_DIFF_1:RW:24:4:=0x00 COL_DIFF_0:RW:16:4:=0x00 ROW_DIFF_1:RW:8:3:=0x01 ROW_DIFF_0:RW:0:3:=0x01
	 */
#if defined(DDR4_MEM)
	/* 10 col bits (diff=0), 17 row bits (diff=1), 4 bank bits(diff=0) */
	wdata_322 = DENALI_CTL_322_DATA;
	wdata_322 = wdata_322 & 0xFFFF; /* Reset 31:16 bits to 0 [BANK_DIFF_1:RW:24:2:=0x01 BANK_DIFF_0:RW:16:2:=0x01] */
	wdata_323 = 0x00000101;         /* Row difference set to 1 [17 bits for row] */
#else
	/* 10 col bits (diff=0), 17 row bits (diff=1), 3 bank bits(diff=1) */
	wdata_322 = 0x01010000;                                 /* DENALI_CTL_322_DATA; */
	wdata_322 = ((wdata_322 & 0xFFFFU) | 0x01010000U);      /* Reset 31:16 bits to 0x0101 [BANK_DIFF_1:RW:24:2:=0x01 BANK_DIFF_0:RW:16:2:=0x01] */
	wdata_323 = 0x00000101;                                 /* Row difference set to 1 [17 bits for row] */
#endif
	SOC_write32(DDR_CTL_REG_BASE + (uint32_t) DENALI_CTL_322__SFR_OFFS, wdata_322);
	SOC_write32(DDR_CTL_REG_BASE + (uint32_t) DENALI_CTL_323__SFR_OFFS, wdata_323);

	/*
	 * Chip Select bits
	 * Last bit as chip select for DDR4. For LPDDR4, default would work
	 */
#if defined(DDR4_MEM)
	cs_val_lower = 0x3fff0000;      /* cs = 0 range */
	cs_val_upper = 0x7fff4000;      /* cs = 1 range */
	SOC_write32(DDR_CTL_REG_BASE + DENALI_CTL_324__SFR_OFFS, cs_val_lower);
	SOC_write32(DDR_CTL_REG_BASE + DENALI_CTL_326__SFR_OFFS, cs_val_upper);
#endif
}

void configure_PI_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_PI_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_PI_REGISTER_BLOCK__OFFS;
	uint32_t wdata_145, i;

	for (i = 0; i <= 423U; i++) {
		SOC_write32(DDR_CTL_PI_REG_BASE + (i * 4U), denali_pi_data[i]);
	}

	/* Disable bank group rotation for DDR4 */
#if defined(DDR4_MEM)
	wdata_145 = DENALI_PI_145_DATA;
	wdata_145 = wdata_145 & 0xFFFFFEFF;
	SOC_write32(DDR_CTL_PI_REG_BASE + DENALI_PI_145__SFR_OFFS, wdata_145);
#endif
}

void configure_data_slice0_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_DATA_SLICE_0_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_Data_Slice_0_REGISTER_BLOCK__OFFS;
	uint32_t i;

	for (i = 0; i <= 136U; i++) {
		{
			SOC_write32(DDR_CTL_DATA_SLICE_0_REG_BASE + (i * 4U), denali_data_slice0[i]);
		}
	}
}

void configure_data_slice1_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_DATA_SLICE_1_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_Data_Slice_1_REGISTER_BLOCK__OFFS;
	uint32_t i;

	for (i = 0; i <= 136U; i++) {
		{
			SOC_write32(DDR_CTL_DATA_SLICE_1_REG_BASE + (i * 4U), denali_data_slice1[i]);
		}
	}
}

void configure_data_slice2_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_DATA_SLICE_2_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_Data_Slice_2_REGISTER_BLOCK__OFFS;
	uint32_t i;

	for (i = 0; i <= 136U; i++) {
		{
			SOC_write32(DDR_CTL_DATA_SLICE_2_REG_BASE + (i * 4U), denali_data_slice2[i]);
		}
	}
}

void configure_data_slice3_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_DATA_SLICE_3_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_Data_Slice_3_REGISTER_BLOCK__OFFS;
	uint32_t i;

	for (i = 0; i <= 136U; i++) {
		{
			SOC_write32(DDR_CTL_DATA_SLICE_3_REG_BASE + (i * 4U), denali_data_slice3[i]);
		}
	}
}

void configure_address_slice0_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_ADDR_SLICE_0_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_Address_Slice_0_REGISTER_BLOCK__OFFS;
	uint32_t i;

	for (i = 0; i <= 48U; i++) {
		SOC_write32(DDR_CTL_ADDR_SLICE_0_REG_BASE + (i * 4U), denali_addr_slice0[i]);
	}
}

void configure_address_slice1_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_ADDR_SLICE_1_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_Address_Slice_1_REGISTER_BLOCK__OFFS;
	uint32_t i;

	for (i = 0; i <= 48U; i++) {
		SOC_write32(DDR_CTL_ADDR_SLICE_1_REG_BASE + (i * 4U), denali_addr_slice1[i]);
	}
}

void configure_address_slice2_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_ADDR_SLICE_2_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_Address_Slice_2_REGISTER_BLOCK__OFFS;
	uint32_t i;

	for (i = 0; i <= 48U; i++) {
		SOC_write32(DDR_CTL_ADDR_SLICE_2_REG_BASE + (i * 4U), denali_addr_slice2[i]);
	}
}

void configure_ddrphy_registers(struct emif_handle_s *h)
{
	uint32_t DDR_CTL_PHY_CORE_REG_BASE = (h->ctl_cfg_base_addr) + (uint32_t) DDRSS_PHY_Core_REGISTER_BLOCK__OFFS;
	uint32_t wdata_1826, i;

	/* Program the PHY */
	for (i = 0; i < 132U; i++) {
		SOC_write32(DDR_CTL_PHY_CORE_REG_BASE + (i * 4U), denali_phy_data[i]);
	}

	/* PHY_SW_GRP0_SHIFT_0:RW+:24:5:=0x00 PHY_FREQ_SEL_INDEX:RW+:16:2:=0x00 PHY_FREQ_SEL_MULTICAST_EN:RW+:8:1:=0x01 PHY_FREQ_SEL_FROM_REGIF:RW_D:0:1:=0x00 */
	SOC_write32(DDR_CTL_PHY_CORE_REG_BASE + (uint32_t) DENALI_PHY_1793__SFR_OFFS, 0x00010000);

	/* Set pll_postdiv to 0 for LPDDR4 memory */
#if (defined(LPDDR4_MEM) || defined(POST_PLLDIV_0))
#if (!(defined(SPEED_250_MTPS)))                /* pll_postdiv should be non-zero for 250MTPS */
	wdata_1826 = 0x00041b42U & 0xFFFFF1FFU; /* Set 11:9 bits to 0 - pll_postdiv in PHY_LP4_BOOT_PLL_CTRL field //#define             DENALI_PHY_1826_DATA 0x00041b42 // */
	SOC_write32(DDR_CTL_PHY_CORE_REG_BASE + (uint32_t) DENALI_PHY_1826__SFR_OFFS, wdata_1826);
#endif
#endif
}

/*
 * -----------------------------------------------------------------------
 * PHY Address Space
 * -----------------------------------------------------------------------
 * Data Slice 0: PHY_BASE_ADDR + 0 DENALI_PHY_0
 * Data Slice 1: PHY_BASE_ADDR + 256 DENALI_PHY_256
 * Data Slice 2: PHY_BASE_ADDR + 512 DENALI_PHY_512
 * Data Slice 3: PHY_BASE_ADDR + 768 DENALI_PHY_768
 * Address Slice 0: PHY_BASE_ADDR + 1024 DENALI_PHY_1024
 * Address Slice 1: PHY_BASE_ADDR + 1280 DENALI_PHY_1280
 * Address Slice 2: PHY_BASE_ADDR + 1536 DENALI_PHY_1536
 * PHY Core: PHY_AC_BASE_ADDR DENALI_PHY_1792
 * -----------------------------------------------------------------------
 */
void configure_PHY_registers(struct emif_handle_s *h)
{
	configure_data_slice0_registers(h);
	configure_data_slice1_registers(h);
	configure_data_slice2_registers(h);
	configure_data_slice3_registers(h);
	configure_address_slice0_registers(h);
	configure_address_slice1_registers(h);
	configure_address_slice2_registers(h);
	configure_ddrphy_registers(h);
}

s32 ddr_exit_low_power_mode(void)
{
	u32 val;
	s32 ret = 0;

	u32 wr_val;
	u32 i;
	u32 rd_val;
	u32 ctl_addr = (Emifhandle.ctl_cfg_base_addr);

	/* Use WKUP_CTRL.WKUP_WWD0_CTRL to ungate clock to RTI */
	writel(WWD_RUN, WKUP_CTRL_MMR_BASE + WKUP_WWD0_CTRL);
    Lpm_debugFullPrintf("configure_sdram_region_idx\n");
	configure_sdram_region_idx(&Emifhandle, SDRAM_IDX, REGION_IDX);
    Lpm_debugFullPrintf("configure_CTL_registers\n");
	configure_CTL_registers(&Emifhandle);           /* Configure Controller registers */
    Lpm_debugFullPrintf("configure_PI_registers\n");
	configure_PI_registers(&Emifhandle);            /* Configure PI registers */
    Lpm_debugFullPrintf("configure_PHY_registers\n");
	configure_PHY_registers(&Emifhandle);           /* Configure PHY registers */
	//restore_registers_optimized(&Emifhandle);       /* Restore register values before LPM */

    Lpm_debugFullPrintf("end configure_PHY_registers\n");
	/* PHY_SET_DFI_INPUT_3:RW_D:24:4:=0x00 PHY_SET_DFI_INPUT_2:RW_D:16:4:=0x00 PHY_SET_DFI_INPUT_1:RW_D:8:4:=0x00 PHY_SET_DFI_INPUT_0:RW_D:0:4:=0x00 */
	rd_val = SOC_read32(ctl_addr + CSL_EMIF_CTLCFG_DENALI_PHY_1820);
	rd_val = (rd_val | 0x40000U);
	SOC_write32((ctl_addr + CSL_EMIF_CTLCFG_DENALI_PHY_1820), rd_val);

	/* PI_TCMD_GAP:RW:16:16:=0x0000 PI_NOTCARE_PHYUPD:RW:8:2:=0x00 PI_INIT_LVL_EN:RW:0:1:=0x00 */
	rd_val = SOC_read32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_4);
	rd_val = (rd_val & 0xFFFFFF00U) | (0x0U);
	SOC_write32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_4, rd_val);

	/* PHY_INDEP_TRAIN_MODE:RW:24:1:=0x01 ODT_VALUE:RW:16:2:=0x01 NO_MRW_INIT:RW:8:1:=0x00 DFI_CMD_RATIO:RD:0:1:=0x00 */
	rd_val = SOC_read32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_20);
	rd_val = (rd_val & 0x00FFFFFFU) | (0x1U << 24);
	SOC_write32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_20, rd_val);

	/* DFIBUS_FREQ_F1:RW:24:5:=0x01 DFIBUS_FREQ_F0:RW:16:5:=0x00 PHY_INDEP_INIT_MODE:RW:8:1:=0x01 TSREF2PHYMSTR:RW:0:6:=0x10 */
	rd_val = SOC_read32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_21);
	rd_val = (rd_val & 0xFFFF00FFU) | (0x1U << 8);
	SOC_write32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_21, rd_val);

	/* PI_DLL_RST_DELAY:RW:16:16:=0x0000 PI_DRAM_INIT_EN:RW:8:1:=0x00 PI_DLL_RST:RW:0:1:=0x00 */
	rd_val = SOC_read32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_150);
	rd_val = (rd_val & 0xFFFF0000U) | (0x101U);
	SOC_write32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_150, rd_val);

	/* SREFRESH_EXIT_NO_REFRESH:RW:24:1:=0x00 PWRUP_SREFRESH_EXIT:RW:16:1:=0x00 TCMDCKE_F2:RW:8:5:=0x03 TCMDCKE_F1:RW:0:5:=0x03 */
	rd_val = SOC_read32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_106);
	rd_val = (rd_val & 0xFF00FFFFU) | (0x0U << 16);
	SOC_write32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_106, rd_val);

	/* PI_SREF_ENTRY_REQ:WR:24:1:=0x00 PI_SREFRESH_EXIT_NO_REFRESH:RW:16:1:=0x00 PI_PWRUP_SREFRESH_EXIT:RW+:8:1:=0x01 PI_MC_PWRUP_SREFRESH_EXIT:RW+:0:1:=0x00 */
	rd_val = SOC_read32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_146);
	rd_val = (rd_val & 0xFFFF00FFU) | (0x1U << 8);
	SOC_write32(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_146, rd_val);

    Lpm_debugFullPrintf("WriteMMR\n");
	/* PI_DRAM_INIT_EN=1 */
	Write_MMR_Field(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_150, 0x1, 1, 8);

	/*
	 * Set following equal to the frequency used for low-power retention entry
	 * DFIBUS_BOOT_FREQ, INIT_FREQ, PI_FREQ_RETENTION_NUM, PI_INIT_WORK_FREQ
	 */
	Write_MMR_Field(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_180, 0x2, 2, 8);      /* DENALI_CTL_180 DFIBUS_BOOT_FREQ bits 9:8 */
	Write_MMR_Field(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_CTL_180, 0x2, 2, 0);      /* DENALI_CTL_180 INIT_FREQ bits 1:0 */
	Write_MMR_Field(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_165, 0x2, 5, 16);      /* DENALI_PI_165 PI_FREQ_RETENTION_NUM bits 20:16 */
	Write_MMR_Field(Emifhandle.ctl_cfg_base_addr + CSL_EMIF_CTLCFG_DENALI_PI_11, 0x2, 5, 0);        /* DENALI_PI_11 PI_INIT_WORK_FREQ bits 4:0 */

    Lpm_debugFullPrintf("put_ddrss_in_data_retention_thru_wkup_mmr\n");
	/* De-asserting data retention pin and wake Control bits */
	put_ddrss_in_data_retention_thru_wkup_mmr(DDR16SS_RETENTION_DIS);

	/* Wait for reg values to set */
	for (i = 0; i < 1000U; i++) {
		delay_1us();
	}

    Lpm_debugFullPrintf("start_PI_CTL_init\n");
	/* Start Initialization [PI_START=1 and START=1] */
	start_PI_CTL_init(&Emifhandle);

    Lpm_debugFullPrintf("poll_for_init_completion\n");
	poll_for_init_completion(&Emifhandle); /* Poll for init completion */
    Lpm_debugFullPrintf("done\n");

	return ret;
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
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout loop exceed 0x%x\n", timeout);
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
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for XRDY: loop exceed 0x%d\n", timeout);
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
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed 0x%d\n", timeout);
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
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for RRDY: loop exceed 0x%d\n", timeout);
        return(-1);
    }

    *rxd = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_DATA);

    /* wait ARDY --> 1 */
    while(((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) &&
          loop++ < timeout) {}

    if(loop >= timeout)
    {
        Lpm_debugFullPrintf("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed 0x%d\n", timeout);
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
//	writel(0, 0x43018080);


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

#if 1
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
#define DO_DM_SUSPEND_RESUME 1
#define DO_RAM_PATTERN_TEST 1
#define FULL_SEQUENCE 1
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

	if (FULL_SEQUENCE)
		Lpm_ddrEnterRetention();
	else {
		dbg_line("Lpm_enterRetention: DDR suspend entry");
		uint32_t *mmrPtr;
		uint32_t lp_status = 0;

		//Enter SR long with Mem clock gating
		//Program self refresh mode
		mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + 0x280);
		*mmrPtr = (0x51<< 8); //LP_MODE_LONG_SELF_REFRESH

		//poll self refresh mode change
		mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + 0x564);
		while(lp_status != 0x10000)
			lp_status = *mmrPtr & 0x10000;
	}
	dbg_line("Lpm_enterRetention: DDR retention done");

	dbg_line("Lpm_enterRetention: Done! Going to wait now");

	for (unsigned int i = 0; i < 3000000U; i++) {
		delay_1us();
	}

#if DO_DM_SUSPEND_RESUME
	if (FULL_SEQUENCE)
		ddr_exit_low_power_mode();
	else {
		dbg_line("Lpm_enterRetention: DDR suspend exit");
		uint32_t *mmrPtr;
		uint32_t lp_status = 0;

		//Enter SR long with Mem clock gating
		//Program self refresh mode
		mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + 0x280);
		*mmrPtr = (0x2<< 8); //LP_MODE_NONE = 0x2

		//poll self refresh mode change
		mmrPtr = (uint32_t *) (DDRSS_CTL_BASE + 0x564);
		while(lp_status != 0x10000)
			lp_status = *mmrPtr & 0x10000;
		dbg_line("Lpm_enterRetention: DDR exited suspend");
	}

	if (DO_RAM_PATTERN_TEST) {
		dump_HEX((void*)RAM_START, 2048);
	}
	int error=0;
	for (unsigned int i = 0; i < SZ; i++) {
		uint32_t val = readl(RAM_START + 4 * i);
		if (val != i) {
			error++;
			if (error == 50) {
				Lpm_debugFullPrintf("too many errors\n");
			}
			if (error < 50) {
				Lpm_debugFullPrintf("0x%x != 0x%x\n", i, val);
			}
		}
	}
	Lpm_debugFullPrintf("end check 0x%x error(s)\n", error);
#else
	Lpm_setupPmic();
#endif
	while(1){};
}
#else
void Lpm_enterRetention(void)
{
	dbg_line("Lpm_enterRetention: Enter retention");
#define DO_DM_SUSPEND_RESUME 0
#define DO_RAM_PATTERN_TEST 1
#define RAM_START 0x80000000U
#define SZ 2500U

//DDR Defines
#define NUM_TIMES           (1*1024)
#define SPACING             (1024*1024/4)


    uint32_t i =0;
    uint32_t j =0;
    uint64_t *mem_ptr = (uint64_t *) 0x80000000;
    uint32_t errors = 0;

    WKUP_CTRL_MMR_unlock_all();
    MCU_PADCONFIG_MMR_unlock_all();

	Lpm_cleanAllDCache();


    //******* IO Only + DDR Entry Sequence ****

    //Set Wakeup Source to CANUART IO Daisy Chain; Configure pinmux to use MCU CAN as wakeup source
dbg_line("Lpm_enterRetention: configureCANIO_Wakeup\n");
    //configureCANIO_Wakeup();

dbg_line("Enables DDR SR and Data retention\n");
    //Enables DDR SR and Data retention
    ddrLPM_Entry(LPDDR4);

//dbg_line("Enable IO Isolation\n");
    //Enable IO Isolation
//    ioIsolationEnable();
    //printf("IO ISO_0 Status: %d\n", getIOIsoStatus());

//dbg_line("Enable CANIO Mode\n");
    //Enable CANIO Mode
//    enterCANIORetention();
    //printf("CANIO Mode: %d\n", getCANIOStatus());

dbg_line("Generate the LD signal to latch retention signal\n");
    //Generate the LD signal to latch retention signal
    generateLD_DataRet();

dbg_line("disableLPM_PMIC_EN\n");
    //AM62A signal the PMIC to power down the supplies except VDDSHV_CANUART, VDD_CANUART, DDR and IO Supplies. Set PMIC_LPM_EN transitions 1->0
    disableLPM_PMIC_EN();


#if DO_DM_SUSPEND_RESUME
ddr_exit_low_power_mode();

	Lpm_cleanAllDCache();
	if (DO_RAM_PATTERN_TEST) {
		dump_HEX((void*)RAM_START, 2048);
	}
	Lpm_cleanAllDCache();
	int error=0;
	for (unsigned int i = 0; i < SZ; i++) {
		uint32_t val = readl(RAM_START + 4 * i);
		if (val != i) {
			error++;
			if (error == 50) {
				Lpm_debugFullPrintf("too many errors\n");
			}
			if (error < 50) {
				Lpm_debugFullPrintf("0x%x != 0x%x\n", i, val);
			}
		}
	}
	Lpm_cleanAllDCache();
	Lpm_debugFullPrintf("end check 0x%x error(s)\n", error);
#else
	for (unsigned int i = 0; i < 1000000U; i++) {
		delay_1us();
	}

dbg_line("Lpm_setupPmic\n");
	Lpm_setupPmic();
#endif
dbg_line("end\n");
	while(1){};
}
#endif
