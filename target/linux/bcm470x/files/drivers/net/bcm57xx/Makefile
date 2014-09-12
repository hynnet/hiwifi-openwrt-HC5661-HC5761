#
# Broadcom BCM57XX Gigabit Ethernet driver make file.
#
# $Id: Makefile,v 1.3 2008-05-02 22:49:54 $
#

obj-$(CONFIG_BCM57XX) := bcm57xx.o

bcm57xx-objs := b57um.o b57proc.o b57diag.o tigon3.o autoneg.o 5701rls.o tcp_seg.o hndgige.o

EXTRA_CFLAGS += -DDBG=0 -DT3_JUMBO_RCV_RCB_ENTRY_COUNT=256 -DNICE_SUPPORT
EXTRA_CFLAGS += -DPCIX_TARGET_WORKAROUND=1 -DINCLUDE_TBI_SUPPORT -DINCLUDE_5701_AX_FIX=1

EXTRA_CFLAGS += -DLINUX_HYBRID_POSTPROCESSING_REMOVAL

