#ifndef __INC_H__
#define __INC_H__

#include <stdint.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

enum
{
	WAN_TYPE_PPPPOE	= 1,
	WAN_TYPE_DHCP	= 2,
	WAN_TYPE_STATIC	= 3,

	WAN_UNLINK		= 99
};

#define RETRY_TIMES		3

#define DHCP_DISCOVER	0x01
#define DHCP_OFFER		0x02

#define DHCP_SERVER_PORT   67
#define DHCP_CLIENT_PORT   68

#define DHCP_OPTION_MESSAGE_TYPE						0x35
#define DHCP_OPTION_CLIENT_IDENTIFIER					0x3d
#define DHCP_OPTION_VENDOR_CLASS_IDENTIFIER				0x3c
#define DHCP_OPTION_PARAMETER_REQUEST_LIST				0x37
#define DHCP_OPTION_SUBNET_MASK							0x01
#define DHCP_OPTION_ROUTER								0x03
#define DHCP_OPTION_DOMAIN_NAME_SERVER					0x06
#define DHCP_OPTION_HOST_NAME							0x0c
#define DHCP_OPTION_DOMAIN_NAME							0x0f
#define DHCP_OPTION_BROADCAST_ADDRESS					0x1c
#define DHCP_OPTION_NETBIOS_OVER_TCP_IP_NAME_SERVER		0x2c
#define DHCP_OPTION_END									0xff
#define DHCP_OPTION_PADDING								0x00

#define	DHCP_XID_POS	4
#define IP_CHECKSUM_POS	24

#define IP_HLEN		sizeof(struct iphdr)	/* 20 (no options)*/
#define UDP_HLEN	sizeof(struct udphdr)	/* 8 */

#define MAX_DHCP_CHADDR_LENGTH			16
#define MAX_DHCP_SNAME_LENGTH			64
#define MAX_DHCP_FILE_LENGTH			128
#define MIN_DHCP_OPTIONS_LENGTH			312

#define DHCP_PACKET_LEN					590 //548

typedef struct dhcpPacket
{
        uint8_t		op;			/* packet type */
        uint8_t		htype;		/* type of hardware address for this machine (Ethernet, etc) */
        uint8_t		hlen;		/* length of hardware address (of this machine) */
        uint8_t		hops;		/* hops */
        uint32_t	xid;		/* random transaction id number - chosen by this machine */
        uint16_t	secs;		/* seconds used in timing */
        uint16_t	flags;		/* flags */
        uint32_t	ciaddr;		/* IP address of this machine (if we already have one) */
        uint32_t	yiaddr;		/* IP address of this machine (offered by the DHCP server) */
        uint32_t	siaddr;		/* IP address of DHCP server */
        uint32_t	giaddr;		/* IP address of DHCP relay */
        uint8_t		chaddr[MAX_DHCP_CHADDR_LENGTH];		/* hardware address of this machine */
        uint8_t		sname[MAX_DHCP_SNAME_LENGTH];		/* name of DHCP server */
        uint8_t		file[MAX_DHCP_FILE_LENGTH];			/* boot file name (used for disk less booting) */
        uint8_t		options[MIN_DHCP_OPTIONS_LENGTH]; 
} DHCP_PACKET;

#endif

