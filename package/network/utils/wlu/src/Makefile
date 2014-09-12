#
# Miscellaneous utilities Makefile
#
# Copyright (C) 2009, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile 361287 2012-10-08 00:28:04Z $
#

WLU_OBJS := wlu.o wlu_cmd.o wlu_iov.o wlu_linux.o bcmutils.o bcmwifi_channels.o wlu_client_shared.o wlu_pipe.o wlu_pipe_linux.o miniopt.o bcm_app_utils.o wlu_rates_matrix.o wlc_ppr.o
ifneq ($(wildcard wlu_common.c),)
WLU_OBJS += wlu_common.o
endif

LUA_OBJS := $(WLU_OBJS) wlu_lua.o


all: wl hcwifi.so

hcwifi.so: $(LUA_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) -shared -llua

wl: $(WLU_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o *~ wl