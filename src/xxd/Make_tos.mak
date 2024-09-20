# vim: noexpandtab
#
# Makefile for the Atari ST TOS - m68k gcc compiler 4.6.4 with libcmini
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

APP=xxd.ttp

$(APP): xxd.c
	$(CC) $(CFLAGS) -DTOS -o $(APP) $(CMINI_LIB)/crt0.o xxd.c \
	-lgcc -lcmini -lgcc

clean:
	-rm -f $(APP) xxd.o
