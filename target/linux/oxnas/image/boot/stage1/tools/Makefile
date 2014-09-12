# Make file to create the host programs.

# use the compiler GCC as a linker and search all default libraries to form
# a working executable. 
# - Not trying to be clever and create a stand alone system so the 
# installed compiler system should work.


LD = gcc
LDOPTS = -lc

CC = gcc
CFLAGS = -c -g -I . -I ../include

all : packager

clean :
	rm -f *.o
	rm -f *.d
	rm -f packager

OBJECTS =  crc32.o
SOURCES = ../src/crc32.c packager.c
DEPENDS = $(SOURCES:.c=.d)

#include $(DEPENDS)

%.d : %.c
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o : %.c %.d
	$(CC) $(CFLAGS) $< -o $@

installer : installer.o
	$(LD) $(LDOPTS) installer.o -o $@

packager : $(OBJECTS) packager.o
	$(LD) $(OBJECTS) $(LDOPTS) packager.o -o $@

crc32.o : ../src/crc32.c
	$(CC) $(CFLAGS) $< -o $@

