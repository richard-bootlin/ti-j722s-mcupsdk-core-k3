
#include <stdint.h>
#include <stdarg.h>
#ifndef WKUP_UART0_BASE
#define WKUP_UART0_BASE 0x2b300000U
#endif
#ifndef MAIN_UART0_BASE
#define MAIN_UART0_BASE 0x28000000U
#endif
#ifndef MCU_UART0_BASE
#define MCU_UART0_BASE  0x04a00000U
#endif
#define DBG_UART_LCR 0x0CU
#define DBG_UART_LSR 0x14U
#define DBG_UART_THR 0x00U

#define DBG_UART_LSR_TX_SR_E_MASK 0x40U
#define DBG_UART_LSR_TX_FIFO_E_MASK 0x20U
#define __maybe_unused __attribute__((__unused__))

#define DEBUG_ENABLED 1
#ifdef DEBUG_ENABLED
 #ifdef DebugP_log
  #undef DebugP_log
 #endif
#define DebugP_log(...) do { ; } while (0)

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

static void dump_decimal(unsigned int val)
{
    char result[10] = { '\0' };
    char *ptr = result + 8;
    int i;

    for (i = 9; i > 0; i--)
    {
	    result[i - 1] = '0' + val % 10;
	    val /= 10;
	    if (result[i - 1] != '0')
		    ptr = result + i - 1;
    }

    dbg_puts(ptr);
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
    unsigned int i;

    for (i = 1; i <= size; i++)
    {
        dump_byte(*c);
        if ((i % 16) == 0)
            dbg_puts("\r\n");
        else
            dbg_putc(' ');
        c++;
    }
}

__maybe_unused static void dump_reg(const char *name, unsigned int addr)
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

__maybe_unused static void dump_val2(const char *str, unsigned int val, unsigned int val2)
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

__maybe_unused static void dump_val(const char *str, unsigned int val)
{
    dbg_puts(str);
    dbg_puts("0x");
    dump_byte((val & 0xff000000) >> 24);
    dump_byte((val & 0xff0000) >> 16);
    dump_byte((val & 0xff00) >> 8);
    dump_byte(val & 0xff);
    dbg_puts("\r\n");
}

__maybe_unused static void dump_vald(const char *str, unsigned int val)
{
    dbg_puts(str);
    dbg_puts(": ");
    dump_decimal(val);
    dbg_puts("\r\n");
}

__maybe_unused static void dump_str(const char *name, const char *str)
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

__maybe_unused static void dump_reg(const char *name, unsigned int addr) {}

__maybe_unused static void dump_val2(const char *str, unsigned int val, unsigned int val2) {}

__maybe_unused static void dump_val(const char *str, unsigned int val) {}

__maybe_unused static void dump_str(const char *name, const char *str) {}

__maybe_unused static void dump_vald(const char *str, unsigned int val) {}
#endif
#define TRACE dump_vald(__func__, __LINE__)
#define DUMP_VAR(var_name) dump_val(# var_name, var_name)
