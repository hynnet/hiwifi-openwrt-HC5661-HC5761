/*******************************************************************
*
* File:            ns16550.h
*
* Description:     Definitions for control of the ns16550 UART
*
* Author:          Brian Clarke
*
* Copyright:       Oxford Semiconductor Ltd, 2009
*
*
*/
#if !defined(__NS16550_H__)
#define __NS16550_H__

struct NS16550 {
    unsigned char rbr;
    unsigned char ier;
    unsigned char fcr;
    unsigned char lcr;
    unsigned char mcr;
    unsigned char lsr;
    unsigned char msr;
    unsigned char scr;
    unsigned char ext;
    unsigned char dlf;
} __attribute__ ((packed));

#define thr rbr
#define iir fcr
#define dll rbr
#define dlm ier

typedef volatile struct NS16550 *NS16550_t;

#define FCR_FIFO_EN 0x01    /* Fifo enable */
#define FCR_RXSR    0x02    /* Receiver soft reset */
#define FCR_TXSR    0x04    /* Transmitter soft reset */

#define MCR_DTR     0x01
#define MCR_RTS     0x02
#define MCR_DMA_EN  0x04
#define MCR_TX_DFR  0x08

#define LCR_WLS_MSK	0x03   /* character length slect mask */
#define LCR_WLS_5   0x00   /* 5 bit character length */
#define LCR_WLS_6   0x01   /* 6 bit character length */
#define LCR_WLS_7   0x02   /* 7 bit character length */
#define LCR_WLS_8   0x03   /* 8 bit character length */
#define LCR_STB     0x04   /* Number of stop Bits, off = 1, on = 1.5 or 2) */
#define LCR_PEN     0x08   /* Parity eneble */
#define LCR_EPS     0x10   /* Even Parity Select */
#define LCR_STKP    0x20   /* Stick Parity */
#define LCR_SBRK    0x40   /* Set Break */
#define LCR_BKSE    0x80   /* Bank select enable */

#define LSR_DR      0x01   /* Data ready */
#define LSR_OE      0x02   /* Overrun */
#define LSR_PE      0x04   /* Parity error */
#define LSR_FE      0x08   /* Framing error */
#define LSR_BI      0x10   /* Break */
#define LSR_THRE    0x20   /* Xmit holding register empty */
#define LSR_TEMT    0x40   /* Xmitter empty */
#define LSR_ERR     0x80   /* Error */

/* useful defaults for LCR */
#define LCR_8N1		0x03

extern void init_NS16550(NS16550_t com_port, int baud_divisor_x16);
extern void reinit_NS16550(NS16550_t com_port, int baud_divisor_x16);

static inline void putc_NS16550(NS16550_t com_port, char c)
{
    while ((com_port->lsr & LSR_THRE) == 0)
        ;
    com_port->thr = c;
}

static inline char getc_NS16550(NS16550_t com_port)
{
    while ((com_port->lsr & LSR_DR) == 0)
        ;
    return (com_port->rbr);
}

#endif        //  #if !defined(__NS16550_H__)

