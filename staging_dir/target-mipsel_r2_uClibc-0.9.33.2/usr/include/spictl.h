/*
 *  Turbo Wireless SPI read and write
 *
 *  Copyright (C) 2012-2012 duke
 *
 */

#ifndef __SPICTL_H__
#define __SPICTL_H__

#define SPIOP_READ			0x1
#define SPIOP_WRITE			0x2
#define SPIOP_ERASE			0x3

#define SPIOP_IOC_MAGIC		0xc3
#define	SPIOP_IOC_READ		_IOR(SPIOP_IOC_MAGIC, SPIOP_READ, char)
#define SPIOP_IOC_WRITE		_IOW(SPIOP_IOC_MAGIC, SPIOP_WRITE, char)
#define	SPIOP_IOC_ERASE		_IO(SPIOP_IOC_MAGIC, SPIOP_ERASE)

#define	SPIOP_IOC_MAXNR		14
#define SPIOP_DEV_MAJOR		222
#define SPIOP_DEV_MINOR		0

#define SPIOP_SECTOR_SIZE	(64 * 1024)

#define SPI_DEV_STR			"/dev/spi"

typedef struct
{
	unsigned int	addr;		/* flash r/w addr	*/
	unsigned int	len;		/* r/w length		*/
	unsigned char*	buf;		/* user-space buffer*/
	unsigned int	buf_len;	/* buffer length	*/
	unsigned int	reset;		/* reset flag 		*/
} ARG;

int spi_read(unsigned int addr, unsigned char *buf, unsigned int len);
int spi_write(unsigned int addr, const unsigned char *buf, unsigned int len);

#endif //__SPICTL_H__
