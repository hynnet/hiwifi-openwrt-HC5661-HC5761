CROSS_COMPILE :=~/openwrt-pandorabox/staging_dir/toolchain-arm_v6_gcc-4.6-linaro_uClibc-0.9.33.2_eabi/bin/arm-openwrt-linux-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
LD = $(CC)

PROG = nandbd_upgrade
SCRIPTS = $(shell ls *.sh)

OBJS = main.o nandwrite_mlc.o nandwrite_fec.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG)
	$(STRIP) $(PROG)

$(OBJS): %.o: %.c
	$(CC) -Wall $(CFLAGS) -c $*.c -o $*.o

clean:
	@rm -f $(OBJS) $(PROG)

