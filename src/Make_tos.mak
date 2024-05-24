# vim: ts=4:noexpandtab:
#
# Makefile for the Atari ST TOS - m68k gcc compiler 4.6.4 with libcmini
#

CC=m68k-atari-mint-gcc
AS=m68k-atari-mint-as -m68000
AR=m68k-atari-mint-ar

CMINI_LIB=/opt/libcmini/lib
CMINI_INC=/opt/libcmini/include
CMINI_MINT_INC=/opt/libcmini/include/mint

SRC = \
	blowfish.c \
	buffer.c \
	charset.c \
	diff.c \
	digraph.c \
	edit.c \
	eval.c \
	ex_cmds.c \
	ex_cmds2.c \
	ex_docmd.c \
	ex_eval.c \
	ex_getln.c \
	fileio.c \
	fold.c \
	getchar.c \
	hardcopy.c \
	hashtab.c \
	main.c \
	mark.c \
	memfile.c \
	memline.c \
	menu.c \
	message.c \
	misc1.c \
	misc2.c \
	move.c \
	mbyte.c \
	normal.c \
	ops.c \
	option.c \
	popupmnu.c \
	quickfix.c \
	regexp.c \
	screen.c \
	search.c \
	sha256.c \
	spell.c \
	syntax.c \
	tag.c \
	term.c \
	ui.c \
	undo.c \
	window.c \
	os_tos.c \
	os_tos_ext.c \
    os_tos_vdo.c

OBJ_T=$(addprefix obj_t/, $(patsubst %.c,%.o,$(SRC)))
OBJ_S=$(addprefix obj_s/, $(patsubst %.c,%.o,$(SRC)))
OBJ_N=$(addprefix obj_n/, $(patsubst %.c,%.o,$(SRC)))

T_FLAGS=-DTOS -DFEAT_TINY -DFEAT_TAG_BINS 
S_FLAGS=-DTOS -DFEAT_SMALL -DFEAT_TAG_BINS
N_FLAGS=-DTOS -DFEAT_NORMAL

RUNTIME_SRC=../runtime_tos
RELROOT=tmp
RELDIR=$(RELROOT)/vim
DOCSDIR=../runtime_tos/doc

.PHONY: lint obj clean release tags tos_utils

# -DTOS_DEBUG - also NATFEATS for hatari conout, else create trace file
# and optionally TRACE_INPUT, TRACE_OUTPUT: trace console I/O

INCL=vim.h globals.h option.h keymap.h macros.h ascii.h term.h os_tos.h structs.h
CFLAGS=--std=gnu99 -Os -s -Iproto \
	-nostdinc -I$(CMINI_INC) -I$(CMINI_MINT_INC) \
	-nostdlib -L$(CMINI_LIB)

all: vimt.ttp vims.ttp vimn.ttp

# version.c is compiled each time, so that it sets the build time.
vimt.ttp: CFLAGS += $(T_FLAGS)
vimt.ttp: obj $(OBJ_T) version.c version.h
	$(CC) $(CFLAGS) -o vimt.ttp $(CMINI_LIB)/crt0.o version.c $(OBJ_T) \
	-lcmini -lgcc 

vims.ttp: CFLAGS += $(S_FLAGS)
vims.ttp: obj $(OBJ_S) version.c version.h
	$(CC) $(CFLAGS) -o vims.ttp $(CMINI_LIB)/crt0.o version.c $(OBJ_S) \
	-lcmini -lgcc 

vimn.ttp: CFLAGS += $(N_FLAGS)
vimn.ttp: obj $(OBJ_N) version.c version.h
	$(CC) $(CFLAGS) -o vimn.ttp $(CMINI_LIB)/crt0.o version.c $(OBJ_N) \
	-lcmini -lgcc 

obj:
	-mkdir -p obj_t
	-mkdir -p obj_s
	-mkdir -p obj_n

obj_t/%.o: %.c obj $(INCL)
	$(CC) -c $(CFLAGS) -o $@ $<

obj_s/%.o: %.c obj $(INCL)
	$(CC) -c $(CFLAGS) -o $@ $<

obj_n/%.o: %.c obj $(INCL)
	$(CC) -c $(CFLAGS) -o $@ $<

###########################################################################

xxd/xxd.ttp: xxd/xxd.c
	$(MAKE) --directory=xxd -f Make_tos.mak

doctags.ttp: $(DOCSDIR)/doctags.c
	$(CC) $(CFLAGS) -o doctags.ttp $(CMINI_LIB)/crt0.o $(DOCSDIR)/doctags.c \
	-lcmini -lgcc 

tos_utils: 
	$(MAKE) --directory=tos_utils

# for creating an ad-hoc test stub etc
test.tos: test.c
	$(CC) -H --std=gnu99 -Iproto -DTOS_DEBUG \
	-nostdinc -I$(CMINI_INC) -I$(CMINI_MINT_INC) \
	-nostdlib -L$(CMINI_LIB) \
	-o test.tos $(CMINI_LIB)/crt0.o test.c \
	-lcmini -lgcc 

###########################################################################

release: clean all xxd/xxd.ttp rt_doctags doctags.ttp tos_utils
	-rm -r $(RELDIR)
	-mkdir -p $(RELDIR)/rt
	cp -r $(RUNTIME_SRC)/* $(RELDIR)/rt
	cp vimt.ttp $(RELDIR)
	cp vims.ttp $(RELDIR)
	cp vimn.ttp $(RELDIR)/vim.ttp
	cp xxd/xxd.ttp $(RELDIR)
	cp doctags.ttp $(RELDIR)
	cp tos_utils/tospal.tos $(RELDIR)
	cp vimrct vimrcs vimrc $(RELDIR)
	cp $(RUNTIME_SRC)/doc/os_atari.txt $(RELDIR)/readme.txt
	-rm $(RELROOT)/vim74.tar.gz
	tar -czvf $(RELROOT)/vim74.tar.gz $(RELDIR)/


rt_doctags:
	cd "$(DOCSDIR)" && \
	gcc -o doctags doctags.c && \
	echo "!_TAG_FILE_SORTED	2	" > tags && \
	./doctags *.txt | LANG=C LC_ALL=C sort -f >> tags && \
	uniq -d -2 tags && \
	rm ./doctags

###########################################################################

lint: 
	cppcheck -q --std=c99 --enable=performance --language=c --template=gcc \
	$(N_FLAGS) $(SRC)

tags:
	ctags `find /opt/libcmini -name "*.h"` *.c *.h

clean: 
	-rm obj_t/*.o
	-rm obj_s/*.o
	-rm obj_n/*.o
	-rmdir obj_t
	-rmdir obj_s
	-rmdir obj_n
	-rm vim*.ttp
	-rm xxd/xxd.ttp
	-rm -r tmp/vim
	-rm doctags.ttp

###########################################################################
