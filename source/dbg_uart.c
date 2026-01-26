
#define WKUP_UART0_BASE 0x2b300000U
#define MAIN_UART0_BASE 0x28000000U
#define MCU_UART0_BASE  0x04a00000U
#define DBG_UART_LCR 0x0CU
#define DBG_UART_LSR 0x14U
#define DBG_UART_THR 0x00U

#define DBG_UART_LSR_TX_SR_E_MASK 0x40U
#define DBG_UART_LSR_TX_FIFO_E_MASK 0x20U
#define __maybe_unused __attribute__((__unused__))

static const unsigned int do_wait = 1;

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
    while(do_wait && (((unsigned int) DBG_UART_LSR_TX_SR_E_MASK |
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
    configured = 0;
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

__maybe_unused static void dump_HEX(void *addr, unsigned int size)
{
    const char *const hex = "0123456789abcdef";
    unsigned char *c = addr;

    for (unsigned int i = 1; i <= size; i++)
    {
        if (*c < 0x10) {
            dbg_putc('0');
            dbg_putc(hex[*c]);
        } else {
            dbg_putc(hex[(*c >> 4) & 0xf]);
            dbg_putc(hex[*c & 0xf]);
        }
        if ((i % 16) == 0)
            dbg_puts("\r\n");
        else
            dbg_putc(' ');
        c++;

    }
}
