
#define WKUP_UART0_BASE 0x2b300000U
#define MCU_UART0_BASE  0x04a00000U
#define DBG_UART_LCR 0x0CU
#define DBG_UART_LSR 0x14U
#define DBG_UART_THR 0x00U

#define UART_LSR_TX_SR_E_MASK 0x40U
#define UART_LSR_TX_FIFO_E_MASK 0x20U

static inline void dbg_writel(unsigned int addr, unsigned int val)
{
	*(volatile unsigned int *) (addr) = val;
}

static inline unsigned int dbg_readl(unsigned int addr)
{
	return *(volatile unsigned int *) (addr);
}

static void _dgb_putc(unsigned int base_addr, unsigned char c)
{
    unsigned int lcrRegValue = dbg_readl(base_addr + DBG_UART_LCR);

    dbg_writel(base_addr + DBG_UART_LCR, lcrRegValue & 0x7FU);

    /*
     * Waits indefinitely until the THR and Transmitter Shift Registers are
     * empty.
     */
    while(((unsigned int) UART_LSR_TX_SR_E_MASK |
           (unsigned int) UART_LSR_TX_FIFO_E_MASK) !=
          (dbg_readl(base_addr + DBG_UART_LSR) &
           ((unsigned int) UART_LSR_TX_SR_E_MASK |
            (unsigned int) UART_LSR_TX_FIFO_E_MASK)))
    {
        /* Busy wait */
    }

    dbg_writel(base_addr + DBG_UART_THR, c);
    dbg_writel(base_addr + DBG_UART_LCR, lcrRegValue);
}

static void dgb_putc(unsigned char c)
{
    unsigned int base = WKUP_UART0_BASE;

    static int configured;
    if (!configured)
    {
        dbg_writel(base + DBG_UART_LCR, 0x3 /* 8N1 */);
	configured = 1;
    }

    _dgb_putc(base, c);
}
