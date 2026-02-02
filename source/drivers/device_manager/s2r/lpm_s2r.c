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

static inline uint32_t readl(uint32_t a)
{
	return *(volatile uint32_t *) (a);
}

static inline void writel(uint32_t v, uint32_t a)
{
	*(volatile uint32_t *) (a) = v;
}

/*
 *
 * PSC stuff, not sure if needed
 *
 *
 */
#define BIT(n)  (((uint32_t) 1U) << (n))
#define PSC_PID                 0x000U
#define PSC_GBLCTL              0x010U
#define PSC_GBLSTAT             0x014U
#define PSC_INTEVAL             0x018U
#define PSC_IPWKCNT             0x01cU
#define PSC_MERRPR0             0x040U
#define PSC_MERRPR1             0x044U
#define PSC_MERRCR0             0x050U
#define PSC_MERRCR1             0x054U
#define PSC_PERRPR              0x060U
#define PSC_PERRCR              0x068U
#define PSC_EPCPR               0x070U
#define PSC_EPCRR               0x078U
#define PSC_RAILSTAT            0x100U
#define PSC_RAILCTL             0x104U
#define PSC_RAILSET             0x108U
#define PSC_PTCMD               0x120U
#define PSC_PTSTAT              0x128U
#define PSC_PDSTAT(domain)      (0x200U + (4U * (domain)))
#define PSC_PDCTL(domain)       (0x300U + (4U * (domain)))
#define PSC_PDCFG(domain)       (0x400U + (4U * (domain)))
#define PSC_MDCFG(id)           (0x600U + (4U * (id)))
#define PSC_MDSTAT(id)          (0x800U + (4U * (id)))
#define PSC_MDCTL(id)           (0xa00U + (4U * (id)))

#define MDSTAT_STATE_MASK               0x3fU
#define MDSTAT_BUSY_MASK                0x30U
#define MDSTAT_STATE_SWRSTDISABLE       0x00U
#define MDSTAT_STATE_SYNCRST            0x01U
#define MDSTAT_STATE_DISABLE            0x02U
#define MDSTAT_STATE_ENABLE             0x03U
#define MDSTAT_STATE_AUTO_SLEEP         0x04U
#define MDSTAT_STATE_AUTO_WAKE          0x05U
#define MDSTAT_STATE_DISABLE_CLK_ON     0x21U
#define MDSTAT_STATE_DISABLE_IN_PROG    0x22U
#define MDSTAT_STATE_RETRY_DISABLE      0x23U
#define MDSTAT_STATE_ENABLE_IN_PROG     0x24U
#define MDSTAT_STATE_SLEEP_IN_PROG      0x25U
#define MDSTAT_STATE_CLK_ON1            0x26U
#define MDSTAT_STATE_CLK_OFF1           0x27U
#define MDSTAT_STATE_CLK_ON2            0x28U
#define MDSTAT_STATE_CLK_OFF2           0x29U
#define MDSTAT_STATE_CLK_ON3            0x2aU
#define MDSTAT_STATE_CLK_OFF3           0x2bU

#define MDSTAT_EMUIHB           BIT(17) /* EMU alters module state */
#define MDSTAT_EMURST           BIT(16) /* EMU alters reset to module */
#define MDSTAT_MCKOUT           BIT(12) /* Module clock output status */
#define MDSTAT_MRSTDONE         BIT(11) /* Module reset done */
#define MDSTAT_MRST             BIT(10) /* Module reset de asserted */
#define MDSTAT_LRSTDONE         BIT(9)  /* Local reset done */
#define MDSTAT_LRST             BIT(8)  /* Local reset de asserted */

#define MDCTL_STATE_MASK        0x3fU
#define MDCTL_FORCE             BIT(31)
#define MDCTL_RESET_ISO         BIT(12)         /* Enable reset isolation */
#define MDCTL_EMUIHBIE          BIT(10)         /* EMU alters module IE */
#define MDCTL_EMURSTIE          BIT(9)          /* EMU alter reset IE */
#define MDCTL_LRST              BIT(8)          /* Assert local reset when 0 */

#define PDSTAT_EMUIHB                   BIT(11) /* EMU alters domain state */
#define PDSTAT_PWRBAD                   BIT(10) /* Power bad error */
#define PDSTAT_PORDONE                  BIT(9)  /* Power on reset done */
#define PDSTAT_POR                      BIT(8)  /* Power on reset de asserted */
#define PDSTAT_STATE_MASK               0x1fU
#define PDSTAT_STATE_OFF                0x00U
#define PDSTAT_STATE_ON                 0x01U
#define PDSTAT_STATE_ON_ARB             0x10U
#define PDSTAT_STATE_SWITCH_ON          0x11U
#define PDSTAT_STATE_ON_RAIL_CNT        0x12U
#define PDSTAT_STATE_WAIT_ALL_SYNC_RST  0x13U
#define PDSTAT_STATE_STRETCH_POR        0x14U
#define PDSTAT_STATE_WAIT_POR_DONE      0x15U
#define PDSTAT_STATE_COUNT16            0x16U
#define PDSTAT_STATE_WAIT_ALL_SWRST_DIS 0x17U
#define PDSTAT_STATE_OFF_ARB            0x18U
#define PDSTAT_STATE_SWITCH_OFF         0x19U
#define PDSTAT_STATE_OFF_RAIL_CNT       0x1aU

#define PDCTL_STATE_MASK        BIT(0)
#define PDCTL_EPCGOOD           BIT(8)  /* External power control on */
#define PDCTL_EMUIHBIE          BIT(9)  /* EMU alters domain state IE */
#define PDCTL_FORCE             BIT(31)

#define PSC_TRANSITION_TIMEOUT  100000

#define PD_DDR                           13     /* Main DDR PD and its LPSCs */
#define LPSC_EMIF_LOCAL                  72
#define LPSC_EMIF_CFG_ISO                73
#define LPSC_EMIF_DATA_ISO               74
#define DDR_PD                           PD_DDR       /* DDR power domain */

#define MDCTL_STATE_SWRSTDISABLE       0x00U
#define MDCTL_STATE_SYNCRST            0x01U
#define MDCTL_STATE_DISABLE            0x02U
#define MDCTL_STATE_ENABLE             0x03U
#define MDCTL_STATE_AUTO_SLEEP         0x04U
#define MDCTL_STATE_AUTO_WAKE          0x05U

#define PDCTL_STATE_OFF         0U
#define PDCTL_STATE_ON          1U
#define psc_raw_read readl
#define psc_raw_write writel

#define DM_R5_CORE_FREQUENCY_MHZ         800
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

int32_t psc_raw_pd_wait(uint32_t psc_base, uint8_t pd)
{
	int32_t ret = 0;
	int32_t i = PSC_TRANSITION_TIMEOUT;

	while (((psc_raw_read(psc_base + PSC_PTSTAT) & BIT(pd)) != 0U) && (i != 0)) {
		delay_1us();
		--i;
	}

	if (i == 0) {
		ret = -1;
	}

	return ret;
}

void psc_raw_pd_initiate(uint32_t psc_base, uint8_t pd)
{
	psc_raw_write(BIT(pd), psc_base + PSC_PTCMD);
}

void psc_raw_pd_set_state(uint32_t psc_base, uint8_t pd, uint32_t state, int force)
{
	uint32_t pdctl = psc_raw_read(psc_base + PSC_PDCTL(pd));

	pdctl &= ~PDCTL_STATE_MASK;
	pdctl |= state;

	if (force) {
		pdctl |= PDCTL_FORCE;
	} else {
		pdctl &= ~PDCTL_FORCE;
	}

	psc_raw_write(pdctl, psc_base + PSC_PDCTL(pd));
}

void psc_raw_lpsc_set_state(uint32_t psc_base, uint8_t lpsc, uint32_t state, int force)
{
	uint32_t mdctl = psc_raw_read(psc_base + PSC_MDCTL(lpsc));

	mdctl &= ~MDCTL_STATE_MASK;
	mdctl |= state;

	if (force) {
		mdctl |= MDCTL_FORCE;
	} else {
		mdctl &= ~MDCTL_FORCE;
	}

	psc_raw_write(mdctl, psc_base + PSC_MDCTL(lpsc));
}

uint8_t psc_raw_lpsc_get_state(uint32_t psc_base, uint8_t lpsc)
{
	return (uint8_t) (MDSTAT_STATE_MASK & psc_raw_read(psc_base + PSC_MDSTAT(lpsc)));
}

/*
 *
 * End of PSC stuf
 *
 *
 */



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

    { // end of enter_ddr_low_power_mode() for am62px
        int ret;
#define MAIN_PSC_BASE                   (0x00400000U)
#define MCU_PSC_BASE                    (0x04000000U)
        psc_raw_lpsc_set_state(MAIN_PSC_BASE, LPSC_EMIF_DATA_ISO,
                               MDCTL_STATE_DISABLE, 0);
        psc_raw_pd_initiate(MAIN_PSC_BASE, DDR_PD);

        ret = psc_raw_pd_wait(MAIN_PSC_BASE, DDR_PD);

        if (ret == 0) {
            psc_raw_lpsc_set_state(MAIN_PSC_BASE, LPSC_EMIF_LOCAL,
                                   MDCTL_STATE_DISABLE, 0);
            psc_raw_pd_initiate(MAIN_PSC_BASE, DDR_PD);

            ret = psc_raw_pd_wait(MAIN_PSC_BASE, DDR_PD);
        }

        if (ret == 0) {
            psc_raw_lpsc_set_state(MAIN_PSC_BASE, LPSC_EMIF_CFG_ISO,
                                   MDCTL_STATE_DISABLE, 0);
            psc_raw_pd_initiate(MAIN_PSC_BASE, DDR_PD);

            ret = psc_raw_pd_wait(MAIN_PSC_BASE, DDR_PD);
        }
    }
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

