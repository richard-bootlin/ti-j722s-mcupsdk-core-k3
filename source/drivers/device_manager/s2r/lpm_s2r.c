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


#define DBG_UART_LCR 0x0CU
#define DBG_UART_LSR 0x14U
#define DBG_UART_THR 0x00U

#define DBG_UART_LSR_TX_SR_E_MASK 0x40U
#define DBG_UART_LSR_TX_FIFO_E_MASK 0x20U
#define __maybe_unused __attribute__((__unused__))

/*
 * In order to enable DEBUG, add:
 * #define DEBUG_ENABLED
 * And also apply:
 */
#if 0
--- a/examples/drivers/ipc/ipc_rpmsg_echo_linux/j722s-evm/wkup-r5fss0-0_freertos/main.c
+++ b/examples/drivers/ipc/ipc_rpmsg_echo_linux/j722s-evm/wkup-r5fss0-0_freertos/main.c
@@ -85,7 +85,7 @@ void main_thread(void *args)
     sciServer_init();
 
     /* Close UART as Drivers_open() inside ipc_rpmsg_echo_main() opens the UART again */
-    Drivers_uartClose();
+//    Drivers_uartClose();
 
     ipc_rpmsg_echo_main(NULL);
 
#endif

#define DEBUG_ENABLED
#ifdef DEBUG_ENABLED
static inline void dbg_writel(unsigned int addr, unsigned int val)
{
	*(volatile unsigned int *) (addr) = val;
}

static inline unsigned int dbg_readl(unsigned int addr)
{
	return *(volatile unsigned int *) (addr);
}

static void _dbg_putc(unsigned int base_addr, unsigned char c)
{
	unsigned int lcrRegValue = dbg_readl(base_addr + DBG_UART_LCR);

	dbg_writel(base_addr + DBG_UART_LCR, lcrRegValue & 0x7FU);

	/*
	 * Waits indefinitely until the THR and Transmitter Shift Registers are
	 * empty.
	 */
	while((((unsigned int) DBG_UART_LSR_TX_SR_E_MASK |
		(unsigned int) DBG_UART_LSR_TX_FIFO_E_MASK) !=
	       (dbg_readl(base_addr + DBG_UART_LSR) &
		((unsigned int) DBG_UART_LSR_TX_SR_E_MASK |
		 (unsigned int) DBG_UART_LSR_TX_FIFO_E_MASK))))
	{
		/* Busy wait */
	}

	dbg_writel(base_addr + DBG_UART_THR, c);
	dbg_writel(base_addr + DBG_UART_LCR, lcrRegValue);
}

static void dbg_putc(unsigned char c)
{
	unsigned int base = WKUP_UART0_BASE;

	static int configured;

	if (!configured)
	{
		dbg_writel(base + DBG_UART_LCR, 0x3 /* 8N1 */);
		configured = 1;
	}

	_dbg_putc(base, c);
}

static void dbg_puts(const char *str)
{
	for (; *str != '\0'; str++)
	{
		dbg_putc(*str);
	}
}

__maybe_unused static void dbg_line(const char *str)
{
	dbg_puts(str);
	dbg_putc('\r');
	dbg_putc('\n');
}

static void dump_byte(unsigned char byte)
{
	const char *const hex = "0123456789abcdef";

	if (byte < 0x10) {
		dbg_putc('0');
		dbg_putc(hex[byte]);
	} else {
		dbg_putc(hex[(byte >> 4) & 0xf]);
		dbg_putc(hex[byte & 0xf]);
	}
}

__maybe_unused static void dump_HEX(void *addr, unsigned int size)
{
	unsigned char *c = addr;

	for (unsigned int i = 1; i <= size; i++)
	{
		dump_byte(*c);
		if ((i % 16) == 0)
			dbg_puts("\r\n");
		else
			dbg_putc(' ');
		c++;
	}
}

__maybe_unused static void dump_reg(char *name, unsigned int addr)
{
	unsigned int val = dbg_readl(addr);

	dbg_puts(name);
	dbg_puts(": 0x");
	dump_byte((val & 0xff000000) >> 24);
	dump_byte((val & 0xff0000) >> 16);
	dump_byte((val & 0xff00) >> 8);
	dump_byte(val & 0xff);
	dbg_puts("\r\n");
}

__maybe_unused static void dump_val2(char *str, unsigned int val, unsigned int val2)
{
	dbg_puts(str);
	dbg_puts("0x");
	dump_byte((val & 0xff000000) >> 24);
	dump_byte((val & 0xff0000) >> 16);
	dump_byte((val & 0xff00) >> 8);
	dump_byte(val & 0xff);
	dbg_putc(' ');
	dump_byte((val2 & 0xff000000) >> 24);
	dump_byte((val2 & 0xff0000) >> 16);
	dump_byte((val2 & 0xff00) >> 8);
	dump_byte(val2 & 0xff);
	dbg_puts("\r\n");
}

__maybe_unused static void dump_val(char *str, unsigned int val)
{
	dbg_puts(str);
	dbg_puts("0x");
	dump_byte((val & 0xff000000) >> 24);
	dump_byte((val & 0xff0000) >> 16);
	dump_byte((val & 0xff00) >> 8);
	dump_byte(val & 0xff);
	dbg_puts("\r\n");
}


__maybe_unused static void dump_str(char *name, char *str)
{
	dbg_puts(name);
	dbg_puts(str);
	dbg_puts("\r\n");
}
#else
__maybe_unused static inline void dbg_writel(unsigned int addr, unsigned int val) {}

__maybe_unused static inline unsigned int dbg_readl(unsigned int addr) { return 0;}

__maybe_unused static void _dbg_putc(unsigned int base_addr, unsigned char c) {}

__maybe_unused static void dbg_putc(unsigned char c) {}

__maybe_unused static void dbg_puts(const char *str) {}

__maybe_unused static void dbg_line(const char *str) {}

__maybe_unused static void dump_byte(unsigned char byte) {}

__maybe_unused static void dump_HEX(void *addr, unsigned int size) {}

__maybe_unused static void dump_reg(char *name, unsigned int addr) {}

__maybe_unused static void dump_val2(char *str, unsigned int val, unsigned int val2) {}

__maybe_unused static void dump_val(char *str, unsigned int val) {}

__maybe_unused static void dump_str(char *name, char *str) {}

#endif
#define PMIC_ADDR 0x48

#define PMIC_CONFIG1_REGADDR                   (0x7dU)
#define PMIC_FSM_I2C_TRIGGERS_REGADDR          (0x85U)
#define PMIC_FSM_NSLEEP_TRIGGERS_REGADDR       (0x86U)

#define SCICLIENT_LPM_SCRATCH_PAD_REG_3 (0xCB)
#define SCICLIENT_LPM_MAGIC_SUSPEND     (0xBA)

#define Lpm_debugReadPmic Lpm_readPmic

#define CSL_WKUP_I2C0_CFG_RD(r)     CSL_REG32_RD_OFF(CSL_WKUP_I2C0_CFG_BASE, r)
#define CSL_WKUP_I2C0_CFG_WR(r, v)  CSL_REG32_WR_OFF(CSL_WKUP_I2C0_CFG_BASE, r, v)
#define CSL_WKUP_I2C0_CFG_SET(r, m) CSL_WKUP_I2C0_CFG_WR(r, CSL_WKUP_I2C0_CFG_RD(r) | m)
#define CSL_WKUP_I2C0_CFG_CLR(r, m) CSL_WKUP_I2C0_CFG_WR(r, CSL_WKUP_I2C0_CFG_RD(r) & ~(m))

#define CSL_WKUP_CTRL_MMR0_CFG0_RD(r)     CSL_REG32_RD_OFF(CSL_WKUP_CTRL_MMR0_CFG0_BASE, r)
#define CSL_WKUP_CTRL_MMR0_CFG0_WR(r, v)  CSL_REG32_WR_OFF(CSL_WKUP_CTRL_MMR0_CFG0_BASE, r, v)
#define CSL_WKUP_CTRL_MMR0_CFG0_SET(r, m) CSL_WKUP_CTRL_MMR0_CFG0_WR(r, CSL_WKUP_CTRL_MMR0_CFG0_RD(r) | m)
#define CSL_WKUP_CTRL_MMR0_CFG0_CLR(r, m) CSL_WKUP_CTRL_MMR0_CFG0_WR(r, CSL_WKUP_CTRL_MMR0_CFG0_RD(r) & ~(m))

static void Lpm_cleanAllDCache(void)
{
	unsigned int set, way;

	for (way = 0; way < 4 ; way ++)
		for (set = 0; set < 0x100; set++) {
			unsigned int val = set << 4 | way << 30;
			asm("    dmb");
			__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 2\t\n": "=r"(val));
			__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 2\t\n": "=r"(val));
			asm("    dsb");
		}
}

static inline void writel(uint32_t v, uint32_t a)
{
	*(volatile uint32_t *) (a) = v;
}

static inline uint32_t readl(uint32_t a)
{
	return *(volatile uint32_t *) (a);
}

static int Lpm_i2cReadTimeout(char add, unsigned char *rxd, unsigned int timeout)
{
	unsigned int n, loop = 0;

	/* wait BB --> 0 */
	while (((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 12)) != 0x00 ) && loop++ < timeout) {}

	if (loop >= timeout) {
		dump_val("Lpm_i2cReadTimeout: timeout loop exceed ", timeout);
		return -1;
	}

	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CNT, 1);
	n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON) & ~0x2;
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, (n | (3 << 9)));
	CSL_WKUP_I2C0_CFG_SET(CSL_I2C_CON, (1 << 0));

	/* wait XRDY --> 1 */
	loop = 0;
	while (((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 4)) == 0x00) && loop++ < timeout) {}

	if (loop >= timeout) {
		dump_val("Lpm_i2cReadTimeout: timeout for XRDY: loop exceed ", timeout);
		return -1;
	}

	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_DATA, add); // write enable to register lock
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, 0x1 << 4);

	/* wait ARDY --> 1 */
	loop = 0;
	while (((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) && loop++ < timeout) {}

	if (loop >= timeout) {
		dump_val("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed ", timeout);
		return -1;
	}

	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW));

	n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON) & ~(0x1 << 9);
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, n | (1 << 10));
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CNT, 1);
	CSL_WKUP_I2C0_CFG_SET(CSL_I2C_CON, (3 << 0));

	/* wait RRDY --> 1 */
	loop = 0;
	while (((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 3)) == 0x00) && loop++ < timeout) {}

	if (loop >= timeout) {
		dump_val("Lpm_i2cReadTimeout: timeout for RRDY: loop exceed ", timeout);
		return -1;
	}

	*rxd = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_DATA);

	/* wait ARDY --> 1 */
	while (((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) && loop++ < timeout) {}

	if (loop >= timeout) {
		dump_val("Lpm_i2cReadTimeout: timeout for ARDY: loop exceed ", timeout);
		return -1;
	}

	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW));

	return 0;
}

static unsigned char Lpm_i2cRead(char add)
{
	unsigned char rxData;

	Lpm_i2cReadTimeout(add, &rxData, 0xFFFFFFFF);

	return rxData;
}

static void Lpm_i2cWrite(char add, char data)
{
	unsigned int n;

	/* wait BB --> 0 */
	while ((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 12)) != 0x00) {}

	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CNT, 2);
	n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON) & ~0x2;
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, (n | (3 << 9)));
	n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_CON);
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_CON, (n | (3 << 0)));

	for (n = 0; n < 2; n++) {
		/* wait XRDY --> 1 */
		while ((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 4)) == 0x00) {}
		if (n == 0) {
			/* write enable to register lock */
			CSL_WKUP_I2C0_CFG_WR(CSL_I2C_DATA, add);
		} else {
			/* write enable to register lock */
			CSL_WKUP_I2C0_CFG_WR(CSL_I2C_DATA, data);
			CSL_WKUP_I2C0_CFG_WR(CSL_I2C_IRQSTATUS, (0x1 << 4));
		}
	}

	/* wait ARDY --> 1 */
	while ((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_IRQSTATUS_RAW) & (0x1 << 2)) == 0x00) {}

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
	while ((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SYSS) & 0x1) == 0x00){}

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
	while ((CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SYSS) & 0x1) == 0x00){}

	/* set PMIC ADDRESS */
	n = CSL_WKUP_I2C0_CFG_RD(CSL_I2C_SA) & ~0x3FF;
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_SA, (n | pmic));

	/* Set TX / RX threshold to 1 [5:0] = 1-1; [13:8] = 1-1 */
	CSL_WKUP_I2C0_CFG_WR(CSL_I2C_BUF, ((1 << 6) | (1 << 14)));

	/* write enable */
	Lpm_i2cWrite(0xA1, 0x9B);
}

static uint8_t Lpm_readPmic(uint8_t reg)
{
	unsigned char rxd;

	Lpm_i2cConfigWkup(PMIC_ADDR);
	rxd = Lpm_i2cRead(reg);
	dump_val2("Lpm_readPmic: reg=", reg, rxd);

	return rxd;
}

static void Lpm_writePmic(uint8_t reg, uint8_t val)
{
	Lpm_i2cConfigWkup(PMIC_ADDR);
	Lpm_i2cWrite(reg, val);
	dump_val2("Lpm_writePmic: reg=", reg, val);
}

static void Lpm_setupPmic(void)
{
	/* Write magic number to scratch register to indicate the suspend */
	Lpm_writePmic(SCICLIENT_LPM_SCRATCH_PAD_REG_3, SCICLIENT_LPM_MAGIC_SUSPEND);

	/* Set PMIC to ACTIVE state */
	Lpm_writePmic(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR, 0x03);

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
	Lpm_writePmic(PMIC_CONFIG1_REGADDR, 0x43);

	/* read FSM_I2C_TRIGGERS */
	Lpm_debugReadPmic(PMIC_FSM_I2C_TRIGGERS_REGADDR);

	/* I2C_TRIGGERS_7 + I2C_TRIGGERS_5 -> io+ddr retained */
	Lpm_writePmic(PMIC_FSM_I2C_TRIGGERS_REGADDR, 0xa0);

	/* Make NSLEEP2_BIT = 0) */
	Lpm_writePmic(PMIC_FSM_NSLEEP_TRIGGERS_REGADDR, 0x01);

	/* create falling edge on PMIC_GPIO1 */
	writel(0, 0x43018080);
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

static int32_t load_magic_words_through_mmr(void)
{
	uint32_t timeout = TIMEOUT_10_MS;
	int32_t ret = 0;

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

#define PLL_16FFT_CTRL_OFFSET		((uint32_t) 0x20UL)
#define PLL_16FFT_CTRL_BYPASS_EN	BIT(31)
#define PLLOFFSET(idx)			(0x1000U * (idx))

static void pll_bypass(uint32_t base)
{
	uint32_t ctrl;

	ctrl = readl(base + PLL_16FFT_CTRL_OFFSET);
	ctrl |= PLL_16FFT_CTRL_BYPASS_EN;
	writel(ctrl, base + PLL_16FFT_CTRL_OFFSET);
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
		pll_bypass(MAIN_PLL_MMR_BASE + PLLOFFSET(12U));
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

static void enter_lpm_self_refresh(void)
{
	uint32_t lp_status = 0;

	/* Program Self Refresh mode */
	writel((LP_MODE_LONG_SELF_REFRESH << 8), DDRSS0_CTRL_BASE + (uint32_t) DENALI_CTL_160__SFR_OFFS);

	while (lp_status != STATUS_SR_LONG_ENTERED) {
		lp_status = (readl(DDRSS0_CTRL_BASE + (uint32_t) DENALI_CTL_169__SFR_OFFS) & 0x7F00U);
	}
}

#define CTRL_MMR0_PARTITION_SIZE                (0x4000U)

#define CTRLMMR_LOCK_KICK0                      (0x01008U)
#define CTRLMMR_LOCK_KICK0_UNLOCK_VAL           (0x68ef3490U)
#define CTRLMMR_LOCK_KICK1                      (0x0100cU)
#define CTRLMMR_LOCK_KICK1_UNLOCK_VAL           (0xd172bc5aU)
#define CTRLMMR_LOCK_KICK1_LOCK_VAL             (0x0U)
void ctrlmmr_unlock(uint32_t base, uint8_t partition)
{
	uint32_t addr = base + (partition * CTRL_MMR0_PARTITION_SIZE);

	writel(CTRLMMR_LOCK_KICK0_UNLOCK_VAL, addr + CTRLMMR_LOCK_KICK0);
	writel(CTRLMMR_LOCK_KICK1_UNLOCK_VAL, addr + CTRLMMR_LOCK_KICK1);
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

	// Grab the MMR value
	p_mmr = (uint32_t *) mmr_address;
	// Build a mask of 1s for the field.
	mask = ((1U << width) - 1) << leftshift;
	// Invert the mask so that the field will be zero'd out with the AND operation.
	mask = ~(mask);
	// Zero out the field in the register.
	*p_mmr &= mask;
	// Assign the value to that specific field.
	*p_mmr |= (field_value << leftshift);
}

static void Lpm_ddrEnterRetention(void)
{
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
	if (((readl(DDR_CTRL_BASE + DENALI_CTL_179__SFR_OFFS) & 0x3000000U) >> 24U) != 0U)
		dbg_line("Failed shifting DDR to boot frequency\n");

	enter_lpm_self_refresh();
	put_ddrss_in_data_retention_thru_wkup_mmr(DDR16SS_RETENTION_EN);

	writel(0, (WKUP_CTRL_MMR_BASE + WKUP0_EN));
	/* Ensure that PMIC EN control from SOC is selected */
	writel((WKUP0_PMCTRL_SYS_LPM_EN_PMIC | WKUP0_LPM_PMIC_OUT_EN), (WKUP_CTRL_MMR_BASE + PMCTRL_SYS));

	/* Enter IO DDR mode */
	writel((WKUP0_PMCTRL_SYS_LPM_EN_PMIC | WKUP0_LPM_PMIC_OUT_DIS), WKUP_CTRL_MMR_BASE + PMCTRL_SYS);
	writel(WWD_STOP, WKUP_CTRL_MMR_BASE + WKUP_WWD0_CTRL);
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

	dbg_line("Lpm_enterRetention: DDR suspend entry");
	Lpm_ddrEnterRetention();

	dbg_line("Lpm_enterRetention: DDR retention done");

	Lpm_setupPmic();

	while(1){};
}
