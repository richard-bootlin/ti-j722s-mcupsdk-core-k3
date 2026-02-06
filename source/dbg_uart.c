
#include <stdint.h>
#include <stdarg.h>
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
    dbg_puts(": ");
    dump_byte((val & 0xff000000) >> 24);
    dump_byte((val & 0xff0000) >> 16);
    dump_byte((val & 0xff00) >> 8);
    dump_byte(val & 0xff);
    dbg_puts("\r\n");
}

/**
 * \brief  This function writes data from a specified buffer onto the transmitter
 *         FIFO of UART
 *
 * \param  str  Pointer to a buffer to transmit.
 *         len  Number of bytes to be transmitted.
 *
 */
static void Lpm_debugPutS(const char *str, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len && str[i] != '\0'; i++)
    {
        if (str[i] == '\n')
        {
           dbg_putc('\r');
        }
        dbg_putc(str[i]);
    }
}

/**
 * \brief This function calls to the common procedure in Lpm_debugPrintf.
 *        This functions writes the output in hex format.
 *
 * This function is a partial copy of UART_convertVal from
 * ti/drv/uart/src/UART_stdio.c. Only the hexadecimal support was kept.
 *
 * \param ulValue  The value of the resolved variable passed as argument
 *        ulPos    The buffer position of the printed value
 *        ulCount  Total number of character to be printed
 *        cFill    The char to be placed between number
 *        pcBuf    buffer storing all the numbers
 *
 * \return 0.
 *
 */
static int32_t Lpm_debugConvertVal(uint32_t ulValue, uint32_t ulPos, uint32_t ulCount, char cFill, char *pcBuf)
{
    const char *const g_pcHex = "0123456789abcdef";
    uint32_t ulBase = 16;
    uint32_t ulIdx;
    uint32_t count = ulCount;
    uint32_t pos = ulPos;

    for (ulIdx = 1u;
         (((ulIdx * ulBase) <= ulValue) &&
          (((ulIdx * ulBase) / ulBase) == ulIdx));
         (ulIdx = (ulIdx * ulBase)))
    {
        count--;
    }

    /* Provide additional padding at the beginning of the
     * string conversion if needed. */
    if ((count > 1u) && (count < 16u))
    {
        for (count--; count != 0U; count--)
        {
            pcBuf[pos] = cFill;
            pos++;
        }
    }

    /* Convert the value into a string. */
    for (; ulIdx != 0U; ulIdx /= ulBase)
    {
        pcBuf[pos] = g_pcHex[(ulValue / ulIdx) % ulBase];
        pos++;
    }

    /* Write the string. */
    (void)Lpm_debugPutS(pcBuf, pos);

    return 0;
}

__maybe_unused static void Lpm_debugPrintf(const char *pcString, ...)
{
    uint32_t ulIdx, ulValue, ulPos, ulCount;
    char    pcBuf[16], cFill;
    va_list  vaArgP;
    const char *pStr = pcString;

    /* Start the varargs processing. */
    (void)va_start(vaArgP, pcString);

    /* Loop while there are more characters in the string. */
    while (*pStr != (char)0U)
    {
        /* Find the first non-% character, or the end of the string. */
        for (ulIdx = 0;
             (pStr[ulIdx] != (char) '%') &&
             (pStr[ulIdx] != (char) '\0');
             ulIdx++)
        {}

        /* Write this portion of the string. */
        (void)Lpm_debugPutS(pStr, ulIdx);

        /* Skip the portion of the string that was written. */
        pStr += ulIdx;

        /* See if the next character is a %. */
        if (*pStr == (char) '%')
        {
            /* Skip the %. */
            pStr++;

            /* Set the digit count to zero, and the fill character to space
             * (i.e. to the defaults). */
            ulCount = 0;
            cFill   = (char) ' ';

            /* Determine how to handle the next character. */
            while((*pStr >= (char)'0') && (*pStr <= (char)'9'))
            {
                /* If this is a zero, and it is the first digit, then the
                 * fill character is a zero instead of a space. */
                if ((pStr[-1] == (char) '0') && (ulCount == 0U))
                {
                    cFill = (char) '0';
                }

                /* Update the digit count. */
                ulCount *= 10u;
                ulCount += ((uint32_t)(*pStr)) - (uint32_t) '0';

                /* Get the next character. */
                pStr++;
            }
            switch (*pStr)
            {
                /* Handle the %x and %X commands.  Note that they are treated
                 * identically; i.e. %X will use lower case letters for a-f
                 * instead of the upper case letters is should use.  We also
                 * alias %p to %x. */
                case (char) 'x':
                case (char) 'X':
                case (char) 'p':
                {
                    /* Get the value from the varargs. */
                    ulValue = (uint32_t)va_arg(vaArgP, uint32_t);

                    /* Reset the buffer position. */
                    ulPos = 0;

                    /* Determine the number of digits in the string version of
                     * the value. */
                    (void)Lpm_debugConvertVal(ulValue, ulPos, ulCount, cFill, pcBuf);
 
                    break;
                }

                /* Handle all other commands. */
                default:
                {
                    /* Indicate an error. */
                    (void)Lpm_debugPutS("ERROR", 5u);

                    /* This command has been handled. */
                    break;
                }
            }
            pStr++;
        }
    }

    /* End the varargs processing. */
    va_end(vaArgP);
}

