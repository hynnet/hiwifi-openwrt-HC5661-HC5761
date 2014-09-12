/*
 *  board/oxnas/cmd_otp.c
 *
 *  Overview:
 *   This is the command set used to control the reading and writting of OTP memory under
 *   User interface control. It calls the OTP read and write functions which are part of the 
 *   main line code - accessed through extern function definitions.
 *
 * Credits:
 *	John Larkworthy @ PLX Tech Inc. 
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_OTP)

#include <malloc.h>
#include <watchdog.h>

#include <asm/io.h>
#include <asm/errno.h>

#include <linux/ctype.h>
#include <command.h>

//#define OTP_DEBUG

#ifdef OTP_DEBUG
#define pr_debug(fmt, ...) \
        printf(fmt, ##__VA_ARGS__)
#else
#define pr_debug(...)
#endif

/**
 * read_otp_byte() - read a single byte from OTP.
 * @adr           - address to read. 
 * 
 * To read a byte the following actions are needed:
 * Ensure PS=0, PGENB=1 and LOAD=1.
 * Apply otp_addr[6:0]
 * Assert then deassert STROBE.
 * Read data from otp_rdata[7:0].
 * Repeat writing address and reading data as required.
 * From the IP data sheet:
 * When PS=L, CSB=L, LOAD=L, and PGENB=L, this
 * macro is ready to read data from fuse cells. A byte
 * Q7~Q0 can be read out in any order by raising
 * STROBE high with a proper address selected.
 * During read operation, address signals A9~A7 are
 * “don’t cares”.
 */
#define writelm(A,B) {writel(B,A); __asm__ __volatile__("": : :"memory");}
#define nop() { __asm__ __volatile__ ("nop\n");}

unsigned char read_otp_byte(int adr) {
	
	unsigned otp_control = ~OTP_STROBE; 
	int i;
	unsigned char ret;
	
	/* disable programming */
	otp_control &= ~(OTP_PS);
	writelm(OTP_ADDR_PROG, otp_control);
	udelay(1);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	/* enable core in read mode */
	otp_control &= ~OTP_CSB;
	/* set address */
	otp_control &= ~(OTP_ADDR_MASK);
	otp_control |= adr;
	/* start read from address */
	writelm(OTP_ADDR_PROG, otp_control);
	udelay(1);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	/* strobe data out */
	otp_control |= OTP_STROBE;
	writelm(OTP_ADDR_PROG, otp_control);
	udelay(1);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	otp_control &= ~OTP_STROBE;
	writelm(OTP_ADDR_PROG, otp_control);
	udelay(1);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	/* get data */
	ret = (unsigned char) readl(OTP_READ_DATA);
	
	/* release core select */
	otp_control |= OTP_CSB;
	writelm(OTP_ADDR_PROG, otp_control);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	return ret;
}
/**
 * set_otp_bit() - set a single bit in the OTP.
 * @adr           - bit address to set: byte[6:0], bit [9:7]
 * 
 * Program (PGM) Mode
 * When PS=H, CSB=L, LOAD=L, and PGENB=L, this
 * macro is ready for electrical fuse programming.
 * Any bit in this macro can be programmed in any
 * order by raising STROBE high with a proper
 * address selected.   The selected address needs to
 * satisfy setup and hold time with respect to STROBE
 * to be valid.  Only one bit is programmed at a time.
 * 
 */

void set_otp_bit(int adr) {
	
	unsigned otp_control = ~(OTP_STROBE | OTP_LOAD | OTP_PGENB ); 
	
	/* enable programming mode */
	writelm(OTP_ADDR_PROG, otp_control);
	udelay(1);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	/* core select and address */
	otp_control &= ~OTP_ADDR_MASK;
	otp_control &= ~OTP_CSB;
	otp_control |= adr;
	writelm(OTP_ADDR_PROG, otp_control);
	udelay(1);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	/* strobe into OTP macro */
	otp_control |= OTP_STROBE;
	writelm(OTP_ADDR_PROG, otp_control);
	/* wait for fuse to burn */
	udelay (6);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	otp_control &= ~OTP_STROBE;
	writelm(OTP_ADDR_PROG, otp_control);
	udelay(1);
	pr_debug("otp control 0x%02x\n", otp_control);
	
	otp_control |= OTP_CSB;
	writelm(OTP_ADDR_PROG, otp_control);
	pr_debug("otp control 0x%02x\n", otp_control);
	return;
}

/**
 * write_otp_byte() set each bit in an OTP location to match data value.
 * @data  data to be set into the OTP memory
 * @add	byte address of the data in OTP memory
 * 
 * Function to compare give and read value from OTP and set each bit high
 * abort writing if data requires any bit to be cleared.
 */

void write_otp_byte(unsigned char data, int adr){
	unsigned char current = read_otp_byte(adr);
	int i;
	unsigned mask;
	/* are any programmed current bits going to be cleared? */
	if (current & ~data)
		return; 
	
	current = ~current & data;
	for (i=0, mask=1; i < 8; i++, mask<<=1) 
		if (current & mask)
			set_otp_bit((i<<7) | (adr & 0x7f));
	current = read_otp_byte(adr);
}

int do_otp (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{

//	int i;
//	printf("do otp argc:%d\n", argc);
//	for (i = 0; i < argc; i++) printf("argv[%d]:%s:\n", i, argv[i]);

	switch (argc) {
	case 0:
	case 1:	/* naked otp command */
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	case 2: /* possible otp get mac address */
		if (strncmp (argv[1], "get_MAC", 7) == 0) {
			printf( "OTP stored mac_adr=%02x:%02x:%02x:%02x:%02x:%02x\n",
		 	read_otp_byte(OTP_MAC), read_otp_byte(OTP_MAC+1),read_otp_byte(OTP_MAC+2),
		 	read_otp_byte(OTP_MAC+3),read_otp_byte(OTP_MAC+4),read_otp_byte(OTP_MAC+5));
			return 1;
		} else if (strncmp (argv[1], "dump", 4) == 0) {
			int i;
			printf("OTP memory dump\n");
			for (i=0; i<1024/8; i++) {
				printf(" 0x%02x", read_otp_byte(i));
				if ((i & 0xf)== 0xf) printf("\n");
			}
			return 1;
		}
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	case 3: /* possible otp read or set mac address */
		if (strncmp (argv[1], "read", 4) == 0) {
			int addr = simple_strtoul (argv[2], NULL, 16);
			printf( "OTP value read 0x%04x:0x%02hx\n", addr,
		 	(unsigned char)read_otp_byte(addr));
			return 1;
			
		} else if (strncmp (argv[1], "set_MAC", 7) == 0) {
			char * data = argv[2];
			char * nxt;
			char val[6];
			int i, n = 0;
				
			for (i=0; i < 6; i++) {
				if (isxdigit(*data)) n++; 
				val[i] = simple_strtoul(data, &nxt, 16);
				if (*nxt == ':') ++nxt;
				data = nxt;
			}
				
			if (n != 6) {
				goto usage;
			}
				
			for (i=0; i < 6; i++) {
		 		write_otp_byte(val[i], OTP_MAC+i);
			}
			
			printf( "OTP MAC address set\n");
			
			return 1;
			
		}
	usage:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	case 4: /* possible otp write single byte */
	default:
		if (strncmp (argv[1], "write", 5) == 0) {
			int addr = simple_strtoul (argv[2], NULL, 16);
			char data = simple_strtoul (argv[3],NULL, 16);
		 	write_otp_byte(data, addr);
			printf( "OTP write - written %02x:0x%02x\n",addr, (unsigned char)read_otp_byte(addr));
			return 1;
		}			
		printf ("Usage:\n%s\n", cmdtp->usage);
		
		return 1;
	}
}

U_BOOT_CMD(
	otp,	5,	0,	do_otp,
	"otp    - OTP sub-system\n",
	"read addr \n"
	"      data byte from memory at address `addr' \n"
	"otp write addr data - write `data' byte to memory address `addr' \n"
	"otp dump - dump contents of OTP memory\n"
	"opt set_MAC MAC_ADDR - write 6 byte MAC address `nn:mm:pp:qq:rr:ss' into OTP\n"
	"opt get_MAC MAC_ADDR - read 6 byte MAC address from OTP\n"
);

#endif	/* CONFIG_COMMANDS & CFG_CMD_OTP */
