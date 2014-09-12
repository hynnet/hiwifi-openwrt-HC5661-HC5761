#
# Router Wireless Interface Configuration Utility Makefile
#
# Copyright (C) 2012, Broadcom Corporation
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: Makefile 341899 2012-06-29 04:06:38Z $
#

SHARED_OBJS := nvparse.o shutils.o wl.o wl_linux.o linux_timer.o defaults.o wlif_utils.o nvram_linux.o

WLCONF_OBJS := bcmwifi_channels.o wlconf.o

NVRAM_OBJS := main.o 

EAPD_OBJS = eapd.o eapd_linux.o nas_eap.o wps_eap.o

NAS_OBJS = mppe.o wpa.o nas.o nas_linux.o nas_wl.o nas_wksp.o bcmwpa.o bcmutils.o

all: libshared.so wlconf nvram eapd nas

libshared.so: $(SHARED_OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

wlconf: $(WLCONF_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -L. -lshared

nvram: $(NVRAM_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -L. -lshared

eapd: $(EAPD_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -L. -lshared

nas: $(NAS_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -L. -lshared -lbcmcrypto

clean:
	rm -f *.o libshared.so wlconf nvram eapd nas
