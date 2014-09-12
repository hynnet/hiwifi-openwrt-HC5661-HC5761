/*******************************************************************
 *
 * File:            ns16550.c
 *
 * Description:     UART driver
 *
 * Date:            06 December 2005
 *
 * Author:          B.H.Clarke
 *
 * Copyright:       Oxford Semiconductor Ltd, 2005
 *
 *******************************************************************/
#include <ns16550.h>
#include <oxnas.h>

#define LCRVAL LCR_8N1                              /* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)                  /* RTS/DTR */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)  /* Clear & enable FIFOs */

static int oxnas_fractional_divider(NS16550_t com_port, int baud_divisor_x16)
{
    // Baud rate is passed around x16
    int real_divisor = baud_divisor_x16 >> 4;
    // Top three bits of 8-bit dlf register hold the number of eigths
    // for the fractional part of the divide ratio
    com_port->dlf = (unsigned char)(((baud_divisor_x16 - (real_divisor << 4)) << 4) & 0xFF);
    // Return the x1 divider for the normal divider register
    return real_divisor;
}

void reinit_NS16550(NS16550_t com_port, int baud_divisor_x16)
{
    int baud_divisor = oxnas_fractional_divider(com_port, baud_divisor_x16);

    com_port->ier = 0x00;
    com_port->lcr = LCR_BKSE | LCRVAL;
    com_port->dll = baud_divisor & 0xff;
    com_port->dlm = (baud_divisor >> 8) & 0xff;
    com_port->lcr = LCRVAL;
    com_port->mcr = MCRVAL;
    com_port->fcr = FCRVAL;
}

void init_NS16550(NS16550_t com_port, int baud_divisor_x16)
{
    
    if (com_port == (NS16550_t)UART_1_BASE) {
        unsigned int pins;
        /* un-reset UART-1 */
        pins = 1 << SYS_CTRL_RSTEN_UARTA_BIT;
       *(volatile unsigned long*)SYS_CTRL_RSTEN_CLR_CTRL = pins;

        /* Setup pin mux'ing for UART2 */
        pins = (1 << UART_A_SIN_MF_PIN) | (1 << UART_A_SOUT_MF_PIN);
        *(volatile unsigned long*)SYS_CTRL_SECONDARY_SEL &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_TERTIARY_SEL  &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_DEBUG_SEL  &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_ALTERNATIVE_SEL  |=  pins; // Route UART1 SOUT and SIN onto external pins
    }

    if (com_port == (NS16550_t)UART_2_BASE) {
        unsigned int pins;
        /* Block reset UART2 */
        pins = 1 << SYS_CTRL_RSTEN_UARTB_BIT;
       *(volatile unsigned long*)SYS_CTRL_RSTEN_CLR_CTRL = pins;

        /* Setup pin mux'ing for UART2 */
        pins = (1 << UART_B_SIN_MF_PIN) | (1 << UART_B_SOUT_MF_PIN);
        *(volatile unsigned long*)SYS_CTRL_SECONDARY_SEL &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_TERTIARY_SEL  &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_ALTERNATIVE_SEL  &=  ~pins; 
        *(volatile unsigned long*)SYS_CTRL_DEBUG_SEL  |=  pins; // Route UART2 SOUT and SIN onto external pins
    }

    reinit_NS16550(com_port, baud_divisor_x16);
}

