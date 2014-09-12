EXTRA_CFLAGS+=-I$(srctree)/brcm/include -I$(srctree)/brcm/shared/bcmwifi/include -DBCMDRIVER -Dlinux
EXTRA_AFLAGS+=-I$(srctree)/brcm/include -I$(srctree)/brcm/shared/bcmwifi/include -DBCMDRIVER -Dlinux



AFLAGS_plat-head.o        := -DTEXT_OFFSET=$(TEXT_OFFSET)

ifeq ($(CONFIG_BUZZZ), y)
obj-$(CONFIG_BUZZZ) += buzzz.o
endif
obj-$(CONFIG_PLAT_MPCORE) += plat-head.o
obj-$(CONFIG_PLAT_MPCORE) += mpcore_core.o
obj-$(CONFIG_PLAT_MPCORE) += mpcore_gtimer.o
obj-$(CONFIG_PLAT_MPCORE) += mpcore_cache.o
obj-$(CONFIG_PLAT_MPCORE) += clock.o clkdev.o
obj-$(CONFIG_PLAT_MPCORE) += platsmp.o 
obj-$(CONFIG_PLAT_IPROC) += iproc_cru.o
obj-$(CONFIG_PLAT_BCM5301X) += bcm5301x.o bcm5301x_dmu.o
obj-$(CONFIG_PLAT_BCM5301X) += bcm5301x_pcie.o
obj-$(CONFIG_PLAT_NAND_JFFS2) += bcm5301x_nandc.o
obj-$(CONFIG_PLAT_BCM5301X) += nvram_linux.o
obj-$(CONFIG_PLAT_BCM5301X) += hndarm_ca9.o
obj-$(CONFIG_PLAT_BCM5301X) += hndsoc.o
obj-$(CONFIG_PLAT_BCM5301X) += hndchipc.o
obj-$(CONFIG_PLAT_BCM5301X) += nvram.o
obj-$(CONFIG_PLAT_BCM5301X) += hndpci.o
obj-$(CONFIG_PLAT_BCM5301X) += hndsflash.o
obj-$(CONFIG_PLAT_BCM5301X) += spiflash.o
obj-$(CONFIG_PLAT_BCM5301X) += nandcore.o
obj-$(CONFIG_PLAT_BCM5301X) += hndnand.o
obj-$(CONFIG_PLAT_BCM5301X) += linux_gpio.o
obj-$(CONFIG_CACHE_L310) += cache-l310.o
