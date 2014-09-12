ARFLAGS=rc

all: libexfat.a mkfs.exfat fsck.exfat

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

libexfat.a: libexfat/cluster.o libexfat/io.o libexfat/log.o libexfat/lookup.o libexfat/mount.o libexfat/node.o libexfat/time.o libexfat/utf.o libexfat/utils.o
	$(AR) $(ARFLAGS) libexfat/$@ $^
	$(RANLIB) libexfat/$@

mkfs.exfat: mkfs/cbm.o mkfs/fat.o mkfs/main.o mkfs/mkexfat.o mkfs/rootdir.o mkfs/uct.o mkfs/uctc.o mkfs/vbr.o
	$(CC) $(LDFLAGS) -o $@ $^ -Llibexfat -lexfat

fsck.exfat: fsck/main.o
	$(CC) $(LDFLAGS) -o $@ $^ -Llibexfat -lexfat
