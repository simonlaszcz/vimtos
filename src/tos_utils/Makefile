# vim: noexpandtab
#

CC=m68k-atari-mint-gcc
AS=m68k-atari-mint-as -m68000
AR=m68k-atari-mint-ar

CMINI_LIB=/opt/libcmini/lib
CMINI_INC=/opt/libcmini/include
CMINI_MINT_INC=/opt/libcmini/include/mint

CFLAGS=--std=gnu99 -Os -s \
	-nostdinc -I$(CMINI_INC) -I$(CMINI_MINT_INC) \
	-nostdlib -L$(CMINI_LIB)

all: tospal.tos

tospal.tos: tospal.c
	$(CC) $(CFLAGS) -o tospal.tos $(CMINI_LIB)/crt0.o tospal.c \
	-lcmini -lgcc

clean:
	-rm -f tospal.tos *.o
