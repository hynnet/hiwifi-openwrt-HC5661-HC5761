export TOP:= $(shell pwd)/cfe
export SRCBASE:= $(shell cd .. && pwd)
export CFE_TOOLS_DIR:=$(SRCBASE)/../tools

CROSS_COMPILE?= arm-openwrt-linux-

all::

%::
	make -C build/broadcom/bcm947xx ARCH=arm TOOLPREFIX=$(CROSS_COMPILE) $@
	cp -rf build/broadcom/bcm947xx/compressed/cfez.bin ../..
