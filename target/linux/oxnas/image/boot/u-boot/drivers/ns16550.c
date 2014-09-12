/*
 * COM1 NS16550 support
 * originally from linux source (arch/ppc/boot/ns16550.c)
 * modified to use CFG_ISA_MEM and new defines
 */

#include <config.h>

#ifdef CFG_NS16550

#include <ns16550.h>

#define LCRVAL LCR_8N1					/* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)			/* RTS/DTR */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)	/* Clear & enable FIFOs */

#ifdef USE_UART_FRACTIONAL_DIVIDER
static int oxnas_fractional_divider(NS16550_t com_port, int baud_divisor)
{
	// Baud rate is passed around x16
	int real_divisor = baud_divisor >> 4;
	// Top three bits of 8-bit dlf register hold the number of eigths
	// for the fractional part of the divide ratio
	com_port->dlf = (unsigned char)(((baud_divisor - (real_divisor << 4)) << 4) & 0xFF);
	// Return the x1 divider for the normal divider register
	return real_divisor;
}
#endif // USE_UART_FRACTIONAL_DIVIDER

void NS16550_init (NS16550_t com_port, int baud_divisor)
{
#ifdef USE_UART_FRACTIONAL_DIVIDER
	baud_divisor = oxnas_fractional_divider(com_port, baud_divisor);
#endif // USE_UART_FRACTIONAL_DIVIDER

	com_port->ier = 0x00;
#ifdef CONFIG_OMAP1510
	com_port->mdr1 = 0x7;	/* mode select reset TL16C750*/
#endif
	com_port->lcr = LCR_BKSE | LCRVAL;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCRVAL;
	com_port->fcr = FCRVAL;
#if defined(CONFIG_OMAP1510) || defined(CONFIG_OMAP1610) || defined(CONFIG_OMAP730)
	com_port->mdr1 = 0;	/* select uart mode */
#endif
}

void NS16550_reinit (NS16550_t com_port, int baud_divisor)
{
#ifdef USE_UART_FRACTIONAL_DIVIDER
	baud_divisor = oxnas_fractional_divider(com_port, baud_divisor);
#endif // USE_UART_FRACTIONAL_DIVIDER

	com_port->ier = 0x00;
	com_port->lcr = LCR_BKSE;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCRVAL;
	com_port->fcr = FCRVAL;
}

void NS16550_putc (NS16550_t com_port, char c)
{
	while ((com_port->lsr & LSR_THRE) == 0);
	com_port->thr = c;
}

char NS16550_getc (NS16550_t com_port)
{
	while ((com_port->lsr & LSR_DR) == 0) {
#ifdef CONFIG_USB_TTY
		extern void usbtty_poll(void);
		usbtty_poll();
#endif
	}
	return (com_port->rbr);
}

int NS16550_tstc (NS16550_t com_port)
{
	return ((com_port->lsr & LSR_DR) != 0);
}

#endif
