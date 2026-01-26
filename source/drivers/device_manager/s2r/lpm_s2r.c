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
#include "dbg_uart.c"

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

static void Lpm_ddrUnlockPll(void)
{
	// defined in source/drivers/hw_include/j722s/cslr_main_pll_mmr.h
#define CSL_MAIN_PLL_MMR_CFG_PLL12_LOCKKEY0 (0x0000C010U)
#define CSL_MAIN_PLL_MMR_CFG_PLL12_LOCKKEY1 (0x0000C014U)
#define CSL_PLL0_CFG_BASE (0x680000UL)
    writel(CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL12_LOCKKEY0, CTRLMMR_LOCK_KICK0_UNLOCK_VAL);
    writel(CSL_PLL0_CFG_BASE + CSL_MAIN_PLL_MMR_CFG_PLL12_LOCKKEY1, CTRLMMR_LOCK_KICK1_UNLOCK_VAL);
}

static void Lpm_ddrEnterRetention(void)
{
	uint32_t val;

    dbg_line(__func__);
	Lpm_ddrUnlockPll(); //PLL_12

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


	// TODO Lpm_setupPmic();
	dbg_line("Lpm_enterRetention: Done! Going to wait now");

	while(1){};
}

