#
# Makefile for Broadcom Home Networking Division (HND) shared driver code
#
# Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# $Id: Makefile,v 1.5 2008-05-02 22:49:54 $
#
EXTRA_CFLAGS+=-I$(srctree)/brcm/include -I$(srctree)/brcm/shared/bcmwifi/include -DBCMDRIVER -Dlinux
EXTRA_AFLAGS+=-I$(srctree)/brcm/include -I$(srctree)/brcm/shared/bcmwifi/include -DBCMDRIVER -Dlinux

ifeq ($(CONFIG_BUZZZ_FUNC),y)
subdir-ccflags-y += -O0 -fno-inline -finstrument-functions
endif # CONFIG_BUZZZ_FUNC

hnd-objs := aiutils.o
HND-objs := $(addprefix $(src)/, aiutils.o)

hnd-objs += bcmotp.o
HND-objs += $(addprefix $(src)/, bcmotp.o)

hnd-objs += bcmsrom.o
HND-objs += $(addprefix $(src)/, bcmsrom.o)

hnd-objs += bcmutils.o
HND-objs += $(addprefix $(src)/, bcmutils.o)

hnd-objs += hnddma.o
HND-objs += $(addprefix $(src)/, hnddma.o)

hnd-objs += hndpmu.o
HND-objs += $(addprefix $(src)/, hndpmu.o)

hnd-objs += linux_osl.o
HND-objs += $(addprefix $(src)/, linux_osl.o)

hnd-objs += nicpci.o
HND-objs += $(addprefix $(src)/, nicpci.o)

hnd-objs += sbutils.o
HND-objs += $(addprefix $(src)/, sbutils.o)

hnd-objs += siutils.o
HND-objs += $(addprefix $(src)/, siutils.o)

WLFLAGS := -DENABLE_ACPHY
WLFLAGS += -DPPR_API
WLFLAGS += -DWL11AC
WLFLAGS += -DWL_BEAMFORMING -DBCMDBG_TRAP
WLFLAGS += -DWLC_LOW -DWLC_HIGH
WLFLAGS += -DBCMNVRAMR
WLFLAGS += -DBCMDMA32
WLFLAGS += -DPHY_HAL

#WLFLAGS += -DCTFPOOL
EXTRA_CFLAGS += $(WLFLAGS) -DBCM5354 -DLINUX_HYBRID_POSTPROCESSING_REMOVAL

# For the ARM platform...
ifneq ($(CONFIG_MACH_BRCM_NS),)
EXTRA_CFLAGS    += -DROBO_SRAB -DBCMDMASGLISTOSL
endif

ifneq ($(CONFIG_BCM947XX),y)
hnd-objs += nvramstubs.o
HND-objs += $(addprefix $(src)/, nvramstubs.o)
endif

ifeq ($(CONFIG_ET_47XX),y)
hnd-objs += bcmrobo.o
HND-objs += $(addprefix $(src)/, bcmrobo.o)
else
ifeq ($(CONFIG_BCM57XX),y)
hnd-objs += bcmrobo.o
HND-objs += $(addprefix $(src)/, bcmrobo.o)
endif
endif

ifdef CONFIG_USB_RNDIS
EXTRA_CFLAGS += -DCONFIG_USBRNDIS_RETAIL
endif                                  

ifeq ($(BCMDBG_MEM),1)
EXTRA_CFLAGS += -DBCMDBG_MEM
endif

obj-$(CONFIG_HND) := hnd.o shared_ksyms.o

$(src)/shared_ksyms.c: $(src)/shared_ksyms.sh
	$(CONFIG_SHELL) -e $< $(HND-objs) > $@

clean-files += shared_ksyms.c

