LDFLAGS += 

all: autowantype

%.o: %.c
	$(CC) -c -o $@ $^

autowantype: autowantype.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o all
