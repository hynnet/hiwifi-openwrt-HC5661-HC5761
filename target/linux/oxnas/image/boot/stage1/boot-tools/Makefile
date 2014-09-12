# Make file to create the host programs.

# use the compiler GCC as a linker and search all default libraries to form
# a working executable. 
# - Not trying to be clever and create a stand alone system so the 
# installed compiler system should work.


LD = gcc
LDOPTS = -lc -v

CC = gcc
CFLAGS = -c -v -g -I . -I ../include


all : update_header

clean :
	rm -f  *.o
	rm -f *.d
	rm -f update_header

OBJECTS =  crc32.o
SOURCES = crc32.c update_header.c
DEPENDS = $(SOURCES:.c=.d)

include $(DEPENDS)

%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$



%.o : %.c %.d
	$(CC) $(CFLAGS) $< -o $@


packager : $(OBJECTS) packager.o
	$(LD) $(OBJECTS) $(LDOPTS) packager.o -o $@

installer : installer.o
	$(LD) $(LDOPTS) installer.o -o $@

update_header : $(OBJECTS) update_header.o
	$(LD) $(OBJECTS) $(LDOPTS) update_header.o -o $@

crc32.o : crc32.c
	$(CC) $(CFLAGS) $< -o $@

