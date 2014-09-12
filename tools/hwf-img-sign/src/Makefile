CC=gcc
CFLAGS=
WFLAGS=-Wall -Werror

hwf-objs = hwf-img-sign.o

all: hwf-img-sign

%.o: %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

hwf-img-sign: $(hwf-objs)
	$(CC) -o $@ $(hwf-objs) $(LDFLAGS)

clean:
	rm -f *.o hwf-img-sign
