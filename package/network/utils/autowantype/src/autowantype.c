/*
	Check WAN connection type.
	autowantype: 1		means PPPOE
	autowantype: 2		means DHCP
	autowantype: 3		means Static IP
	autowantype: 99		means Cable problem

	Copyright www.hiwifi.tw <jin.zhong@hiwifi.tw>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include <errno.h>
#include <assert.h>

#include <time.h>

#include <linux/if_ether.h>
#include <linux/sockios.h>
#include <linux/mii.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <net/if.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include "inc.h"

static uint32_t xid;

static uint16_t checksum(void* addr, int count)
{
	/*
	 * Compute Internet Checksum for "count" bytes
	 * beginning at location "addr".
	 */
	register int32_t sum = 0;
	uint16_t *source = (uint16_t *) addr;

	while (count > 1)
	{
		/*  This is the inner loop */
		sum += *source++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0)
	{
		/*
		 * Make sure that the left-over byte is added correctly both
		 * with little and big endian hosts
		 */
		uint16_t tmp = 0;
		*(uint8_t *) (&tmp) = * (uint8_t *) source;
		sum += tmp;
	}
	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

static void dev_get_mac(uint8_t *mac, const char *dev_name)
{
	int	fd;
	int	err;
	struct ifreq ifr;

	fd = socket(PF_PACKET, SOCK_RAW, htons(0x0806));
	assert(fd != -1);

	assert(strlen(dev_name) < IFNAMSIZ);
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	err = ioctl(fd, SIOCGIFHWADDR, &ifr);
	assert(err != -1);
	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

	err = close(fd);
	assert(err != -1);
}

static bool check_offer(const uint8_t* packet, 
						const uint8_t* mac, 
						int type)
{
	struct ether_header* eh = (struct ether_header *)packet;
	struct iphdr* ih = (struct iphdr*) &packet[ETH_HLEN];
	DHCP_PACKET* dhcp = (DHCP_PACKET*) &packet[ETH_HLEN + IP_HLEN + UDP_HLEN];
	bool ret = false;

	switch(type)
	{
	case WAN_TYPE_PPPPOE:
		/* For PADO of PPPoE, the destination MAC is Client'MAC. */
		if (!memcmp(mac, eh->ether_dhost, ETH_ALEN) && 
		   (eh->ether_type == htons(ETH_P_PPP_DISC)))
		{
			printf("Receive PADO.\n");
			ret = true;
		}
		break;

	case WAN_TYPE_DHCP:
		/* For Offer of DHCP, the destination MAC is FF:FF:FF:FF:FF:FF, the IP is 255.255.255.255. */
		if (eh->ether_type == htons(ETH_P_IP) &&
			ih->protocol == IPPROTO_UDP &&
			ntohl(dhcp->xid) == xid &&
			dhcp->op == DHCP_OFFER)	/* It is better to add chaddr compare */
		{
			printf("Receive DHCP Offer.\n");
			ret = true;
		}
		break;

	default:
		break;
	}

	return ret;
}

static bool check_reply(int fd, uint8_t* mac, int type, int timeout)
{
	struct timeval tv;
	time_t prev;
	fd_set fdset;
	int received = -1;
	uint8_t buf[ETH_FRAME_LEN];

	time(&prev);

	while (timeout > 0)
	{
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);

		tv.tv_sec = timeout/1000;
		tv.tv_usec = (timeout % 1000)*1000;

		if (select(fd + 1, &fdset, (fd_set*)NULL, (fd_set*)NULL, &tv) < 0)
		{
			perror("Select error.\n");
		}
		else
		{
			if (FD_ISSET(fd, &fdset))
			{
				memset(&buf, 0, sizeof(buf));
				received = recv(fd, buf, sizeof(buf), 0);
				
				if (check_offer(buf, mac, type) == true)
				{
					return true;
				}
			}
		}

		timeout = timeout - (time(NULL) - prev)*1000;
		time(&prev);
	}

	return false;
}

static bool try_pppoe(const char* ifname, uint8_t *mac, int *pppfd)
{
	int raw_fd;
	ssize_t sent;
	struct sockaddr sa;		/* for interface name */
	bool ret = false;

	uint8_t padi[] = {
		/* Ethernet header */
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* dest mac */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* our mac */
		0x88, 0x63,	/* ether type = PPPoE Discovery */

		/* PADI packet */
		0x11,		/* ver & type */
		0x09,		/* code (CODE_PADI) */
		0x00, 0x00, /* session ID */
		0x00, 0x0c, /* Payload Length */

		/* PPPoE Tags */
		0x01, 0x03,	/* TAG_TYPE =  Host-Uniq */
		0x00, 0x04,	/* TAG_LENGTH = 0x0004 */
		0x12, 0x34, 0x56, 0x78,	/* TAG_VALUE */
		0x01, 0x01,	/* TAG_TYPE = Service-Name */
		/* When the TAG_LENGTH is zero, this TAG is used to indicate that any service is acceptable. */
		0x00, 0x00, /* TAG_LENGTH = 0x0000 */
		};

	memcpy(padi + ETH_ALEN, mac, ETH_ALEN);

	memset(&sa, 0, sizeof(sa));
	memcpy(sa.sa_data, ifname, strlen(ifname));
	if ((raw_fd = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_PPP_DISC))) < 0)
	{
		printf("Can not open raw socket.\n");
		ret = false;
		goto end;
    }

	printf("Send PADI, ");
	sent = sendto(raw_fd, padi, sizeof(padi), 0, &sa, sizeof(sa));
	printf("%d bytes.\n", sent);

	*pppfd = raw_fd;
	return true;
		
end:
	return ret;
}

static bool try_bootp(const char* ifname, uint8_t *mac, int *boofd)
{
	int udp_fd = -1;
	int raw_fd = -1;

	struct sockaddr_in ludp_sa;
	struct sockaddr_in rbc_sa;	

	struct sockaddr raw_sa;		/* for interface name */
	
	uint8_t pkt_buf[DHCP_PACKET_LEN];

	ssize_t sent;
	bool ret = false;	
	uint32_t flag = 1;

	uint8_t header[] = { /* size == 42 */
		/* ether header (14)*/
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x08, 0x00,

		/* IP header (20)*/
		0x45,		/* version & ihl */
		0x00,		/* tos */
		0x02, 0x40, /* tos_len */
		0x00, 0x00, /* id */
		0x00, 0x00,
		0x40,		/* TTL = 64 */
		0x11,		/* protocol */
		0x00, 0x00, /* IP header check sum (*) */
		0x00, 0x00, 0x00, 0x00, /* src IP */
		0xff, 0xff, 0xff, 0xff, /* dest IP */

		/* UDP header (8)*/
		0x00, 0x44, /* src port */
		0x00, 0x43, /* dest port */
		0x02, 0x2c, /* length */
		0x00, 0x00, /* check sum (*) */
        };

	uint8_t data[] = { /* size == 34 */
		/* dhcp packet */
		0x01,	/* Message type -- BOOTREQUEST */
		0x01,	/* Hardware type -- ethernet */
		0x06,	/* Hardware address length */
		0x00,	/* Hops */
		0x00, 0x00,	0x00, 0x00, /* Transaction ID (*) */
		0x00, 0x00,	/* Time elapsed */
		0x80, 0x00, /* Flags = Broadcast */
		0x00, 0x00, 0x00, 0x00,	/* Client IP address */
		0x00, 0x00, 0x00, 0x00, /* Your IP address */
		0x00, 0x00, 0x00, 0x00, /* Next server IP address */
		0x00, 0x00, 0x00, 0x00, /* Relay agent IP address */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Client hardware address (*) */
		/* 0x00, 0x00, 0x00, 0x00 Server host name */
		};

	uint8_t option[] = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			/* first four bytes of options field is magic cookie (as per RFC 2132) */
			0x63, 0x82, 0x53, 0x63,

			DHCP_OPTION_MESSAGE_TYPE,
			0x01, 	/* Length */
			DHCP_DISCOVER, 	/* Value */

			DHCP_OPTION_CLIENT_IDENTIFIER,
			0x07, 	/* Length */
			0x01, 	/* Hardware type: Ethernet */
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	/* Client MAC address */

			DHCP_OPTION_VENDOR_CLASS_IDENTIFIER,
			0x0b, 	/* Length = 11 */
			0x68, 0x69, 0x77, 0x69, 0x66, 0x69, 0x66, 0x69, 0x77, 0x69, 0x68, /* hiwififiwih */

			DHCP_OPTION_PARAMETER_REQUEST_LIST,
			0x07, 	/* Length */
			DHCP_OPTION_SUBNET_MASK,
			DHCP_OPTION_ROUTER,
			DHCP_OPTION_DOMAIN_NAME_SERVER,
			DHCP_OPTION_HOST_NAME,
			DHCP_OPTION_DOMAIN_NAME,
			DHCP_OPTION_BROADCAST_ADDRESS,
			DHCP_OPTION_NETBIOS_OVER_TCP_IP_NAME_SERVER,
			DHCP_OPTION_END,
			DHCP_OPTION_PADDING
			};
	uint16_t chksum = 0;

	/* set local sa for udp */
	memset(&ludp_sa, 0, sizeof(ludp_sa));
	ludp_sa.sin_family = AF_INET;
	ludp_sa.sin_port = htons(DHCP_CLIENT_PORT);
	ludp_sa.sin_addr.s_addr = INADDR_ANY;
	bzero(&ludp_sa.sin_zero,sizeof(ludp_sa.sin_zero));

	/* send the DHCPDISCOVER packet to broadcast address */
	rbc_sa.sin_family = AF_INET;
	rbc_sa.sin_port = htons(DHCP_SERVER_PORT);
	rbc_sa.sin_addr.s_addr = INADDR_BROADCAST;
	bzero(&rbc_sa.sin_zero, sizeof(rbc_sa.sin_zero));

	memset(pkt_buf, 0, sizeof(pkt_buf));
	memcpy(pkt_buf, data, sizeof(data));
	memcpy(pkt_buf+230, option, sizeof(option));

	memcpy(pkt_buf+28, mac, ETH_ALEN);		/* Client hardware address */
	memcpy(pkt_buf+246, mac, ETH_ALEN);	/* Client hardware address */

	/* Create xid */
	srand(time(NULL));
	xid = random();

	pkt_buf[DHCP_XID_POS]     = xid >> 24;
	pkt_buf[DHCP_XID_POS + 1] = xid >> 16;
	pkt_buf[DHCP_XID_POS + 2] = xid >> 8;
	pkt_buf[DHCP_XID_POS + 3] = xid;

	if ((udp_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("UDP socket creation faild: %s\n", strerror(errno));
		ret = false;
		goto end;
    }

	if ((raw_fd = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_IP))) < 0)
	{
		printf("RAW socket creation failed: %s\n", strerror(errno));
		ret = false;
		goto end;
	}

	/* set the reuse address flag so we don't get errors when restarting */
	if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, (uint8_t *)&flag, sizeof(flag)) < 0)
	{
		printf("Could not set reuse address option: %s\n", strerror(errno));
		ret = false;
		goto end;
	}

	/* set the broadcast option - we need this to listen to DHCP broadcast messages */
	if (setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, (uint8_t *)&flag, sizeof(flag)) < 0)
	{
		printf("Could not set broadcast option: %s\n", strerror(errno));
		ret = false;
		goto end;
    }
	
	/* bind the socket */
	if (bind(udp_fd,(struct sockaddr *)&ludp_sa, sizeof(struct sockaddr)) < 0)
	{
		printf("Could not bind to DHCP socket: %s\n", strerror(errno));
		ret = false;
		goto end;
	}	

	/* Send the UDP Socket Packet */
	sent = sendto(udp_fd, pkt_buf, 548, 0, (struct sockaddr*)&rbc_sa, sizeof(struct sockaddr));
	printf("Send DHCP Discover (udp) -- %d bytes\n", sent);

	/* prepare RAW data */
	chksum = 0;
	memmove(pkt_buf + sizeof(header), pkt_buf, 548);
	memcpy(pkt_buf, header, sizeof(header));
	memcpy(pkt_buf+ETH_ALEN, mac, ETH_ALEN);	/* src eth addr */

	/* Calculate IP checksum */
	chksum = checksum(pkt_buf+ETH_HLEN, IP_HLEN);
	chksum = htons(chksum);
	pkt_buf[IP_CHECKSUM_POS + 1] = chksum;
	pkt_buf[IP_CHECKSUM_POS] = chksum >> 8;
	
	memset(&raw_sa, 0, sizeof(raw_sa));
	memcpy(raw_sa.sa_data, ifname, strlen(ifname));

	/* Send the RAW Socket Packet */
	sent = sendto(raw_fd, pkt_buf, sizeof(pkt_buf), 0, &raw_sa, sizeof(raw_sa));
	printf("Send DHCP Discover (raw) -- %d bytes\n", sent);

	if (udp_fd > 0)
	{
		close(udp_fd);
	}
	*boofd = raw_fd;
	return true;		
	
end:
	if (udp_fd > 0)
	{
		close(udp_fd);
	}
	if (raw_fd > 0)
	{
		close(raw_fd);
	}

	return ret;
}

static int network_ioctl(int request, void* data, const char *errmsg)
{
	int fd = socket(PF_PACKET, SOCK_RAW, htons(0x0806));
	int r = ioctl(fd, request, data);
	
	if (r < 0 && errmsg)
	{
		printf("network_ioctl error.\n");
	}

	if (fd > 0)
	{
		close(fd);
	}
	return r;
}

static bool try_link(const char* ifname)
{
	/* char buffer instead of bona-fide struct avoids aliasing warning */
	uint8_t buf[sizeof(struct ifreq)];
	struct ifreq *const ifreq = (void *)buf;

	struct mii_ioctl_data *mii = (void *)&ifreq->ifr_data;

	memset(ifreq, 0, sizeof(struct ifreq));
	strncpy(ifreq->ifr_name, ifname, IFNAMSIZ);

	if (network_ioctl(SIOCGMIIPHY, ifreq, "SIOCGMIIPHY") < 0)
	{
		printf("SIOCGMIIPHY invalid.\n");
		goto err;
	}

	mii->reg_num = 1;	/* PHY STATUS REG */

	if (network_ioctl(SIOCGMIIREG, ifreq, "SIOCGMIIREG") < 0)
	{
		printf("SIOCGMIIREG invalid.\n");
		goto err;
	}

	return (mii->val_out & 0x0004) ? true : false;

err:
	return false;
}

uint32_t get_wan_type(uint8_t *dev_name, int32_t timeout)
{
	uint32_t wan_type = WAN_TYPE_STATIC;
	int32_t maxfd = -1, pfd, bfd, to_ms;
	uint32_t i, j;
	fd_set rset;
	uint8_t mac[ETH_ALEN];				
	int32_t pppfds[RETRY_TIMES];		
	int32_t boofds[RETRY_TIMES];
	uint8_t buf[ETH_FRAME_LEN];
	struct timeval tv;
	time_t prev;
	
	dev_get_mac(mac, dev_name);		
	for (i = 0; i < RETRY_TIMES; ++i)
	{
		pfd = -1;
		try_pppoe(dev_name, mac, &pfd);		
		maxfd = pfd > maxfd ? pfd:maxfd;
		pppfds[i] = pfd;

		bfd = -1;
		try_bootp(dev_name, mac, &bfd);
		maxfd = bfd > maxfd ? bfd:maxfd;
		boofds[i] = bfd;																		

		time(&prev);
		to_ms = timeout;
		while (to_ms > 0)
		{
			FD_ZERO(&rset);
			for (j = 0; j <= i; ++j)
			{
				if (pppfds[j] >= 0)
				{
					FD_SET(pppfds[j], &rset);
				}
				if (boofds[j] >= 0)
				{
					FD_SET(boofds[j], &rset);
				}
			}	
			
			tv.tv_sec = to_ms/1000;
			tv.tv_usec = (to_ms % 1000)*1000; 

			if (select(maxfd+1, &rset, NULL, NULL, &tv) < 0)
			{
				perror("Select error.\n");				
			}
			else
			{
				//handle pppoe
				for (j = 0; j <= i; ++j)
				{		
					if (FD_ISSET(pppfds[j], &rset))
					{
						memset(&buf, 0, sizeof(buf));

						recv(pppfds[j], buf, sizeof(buf), 0);

						if (check_offer(buf, mac, WAN_TYPE_PPPPOE) == true)
						{
							wan_type = WAN_TYPE_PPPPOE;
							goto ret;				
						}					
					}
				}		
			
				//handle bootp
				for (j = 0; j <= i; ++j)
				{
					if (FD_ISSET(boofds[j], &rset))
					{
						memset(&buf, 0, sizeof(buf));
						recv(boofds[j], buf, sizeof(buf), 0);
						
						if (check_offer(buf, mac, WAN_TYPE_DHCP) == true)
						{
							wan_type = WAN_TYPE_DHCP;
							goto ret;				
						}						
					}	
				}
			}

			to_ms = to_ms - (time(NULL) - prev)*1000;
			time(&prev);
		}
	}	
	
ret:	
	for (j = 0; j <= i; ++j)
	{
		if (pppfds[j] >= 0)
		{
			close(pppfds[j]);
		}			
		if (boofds[j] >= 0)
		{
			close(boofds[j]);
		}
	}			
	
	return wan_type;
}

int main(int argc, char** argv)
{
	uint32_t timeout = 1000;	/* milli-seconds */
	uint8_t* dev_name = "eth1";

	int32_t i = RETRY_TIMES;
	uint32_t wan_type = 0;

	if (argc == 2)
	{
		dev_name = argv[1];
	}
	if (argc == 3)
	{
		dev_name = argv[1];
		timeout = atoi(argv[2]);
	}

	while (i > 0)
	{
		if (false == try_link(dev_name))
		{
			i--;
			if (i == 0)
			{
				wan_type = WAN_UNLINK;
				goto end;	/* if wan cable problem, goto end directly */
			}
			continue;
		}
		else
		{
			break;
		}
	}

	wan_type = get_wan_type(dev_name, timeout);	

end:
	printf("autowantype: %d\n", wan_type);
	return 0;
}

