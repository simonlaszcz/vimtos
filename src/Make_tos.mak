# vim: ts=4:noexpandtab:
#
# Makefile for the Atari ST TOS - m68k gcc compiler 4.6.4 with libcmini
#

CC=m68k-atari-mint-gcc
AS=m68k-atari-mint-as
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
	version.c \
	ui.c \
	undo.c \
	window.c \
	os_tos.c \
	os_tos_ext.c \
    os_tos_vdo.c 

ASM_SRC =

.PHONY: lint obj clean release tags tos_utils

INCL=vim.h globals.h option.h keymap.h macros.h ascii.h term.h os_tos.h structs.h
LIBS=-lgcc -lcmini -lgcc
# -DTOS_DEBUG - also NATFEATS for hatari conout, else create trace file
# and optionally TRACE_INPUT, TRACE_OUTPUT: trace console I/O
CFLAGS=--std=gnu99 -Os -s -Iproto \
	-nostdinc -I$(CMINI_INC) -I$(CMINI_MINT_INC) -nostdlib
T_FLAGS=-DTOS -DFEAT_TINY -DFEAT_TAG_BINS 
S_FLAGS=-DTOS -DFEAT_SMALL -DFEAT_TAG_BINS
N_FLAGS=-DTOS -DFEAT_NORMAL
M00_FLAGS=-m68000 -L$(CMINI_LIB) -DBUILD=\"68000\"
M20_FLAGS=-m68020-60 -L$(CMINI_LIB)/m68020-60/mfastcall -mfastcall -DBUILD=\"68020-60\ fastcall\"

all: vimt.ttp vims.ttp vim.ttp vimt020.ttp vims020.ttp vim020.ttp

# $1=ttp basename
# $2=feature build FLAGS
# $3=machine flags
define TTP_TEMPLATE
OBJ_$(1)=\
	$(addprefix obj_$(1)/, $(patsubst %.c,%.o,$(SRC))) \
	$(addprefix obj_$(1)/, $(patsubst %.s,%.o,$(ASM_SRC)))
obj_$(1):
	-mkdir -p obj_$(1)

$(1).ttp: CFLAGS += $(2)
$(1).ttp: CFLAGS += $(3)
$(1).ttp: obj_$(1) $$(OBJ_$(1))
	$(CC) $$(CFLAGS) -o $(1).ttp $$(CMINI_LIB)/crt0.o $$(OBJ_$(1)) \
	$$(LIBS)

obj_$(1)/%.o:%.c $$(INCL)
	$$(CC) -c $$(CFLAGS) -o $$@ $$<
obj_$(1)/%.o:%.s
	$$(CC) -c $$(CFLAGS) -o $$@ $$<
endef

$(eval $(call TTP_TEMPLATE,vimt,$(T_FLAGS),$(M00_FLAGS)))
$(eval $(call TTP_TEMPLATE,vims,$(S_FLAGS),$(M00_FLAGS)))
$(eval $(call TTP_TEMPLATE,vim,$(N_FLAGS),$(M00_FLAGS)))
$(eval $(call TTP_TEMPLATE,vimt020,$(T_FLAGS),$(M20_FLAGS)))
$(eval $(call TTP_TEMPLATE,vims020,$(S_FLAGS),$(M20_FLAGS)))
$(eval $(call TTP_TEMPLATE,vim020,$(N_FLAGS),$(M20_FLAGS)))

###########################################################################

RUNTIME_SRC=../runtime_tos
RELROOT=tmp
RELDIR=$(RELROOT)/vim
TARNAME=vim74.tar.gz
DOCSDIR=../runtime_tos/doc

xxd/xxd.ttp: xxd/xxd.c
	$(MAKE) --directory=xxd -f Make_tos.mak

doctags.ttp: $(DOCSDIR)/doctags.c
	$(CC) $(CFLAGS) -o doctags.ttp $(CMINI_LIB)/crt0.o $(DOCSDIR)/doctags.c \
	$(M00_FLAGS) $(LIBS)

tos_utils: 
	$(MAKE) --directory=tos_utils

# for creating an ad-hoc test stub etc
test.tos: test.c
	$(CC) -H --std=gnu99 -Iproto -DTOS_DEBUG \
	-nostdinc -I$(CMINI_INC) -I$(CMINI_MINT_INC) \
	-nostdlib -L$(CMINI_LIB) \
	-o test.tos $(CMINI_LIB)/crt0.o test.c \
	$(LIBS)

rt_doctags:
	cd "$(DOCSDIR)" && \
	gcc -o doctags doctags.c && \
	echo "!_TAG_FILE_SORTED	2	" > tags && \
	./doctags *.txt | LANG=C LC_ALL=C sort -f >> tags && \
	uniq -d -2 tags && \
	rm ./doctags

README_TMP=README.temp
readme:
	awk '/^```$$/{exit}//{print}' ../README.md > $(README_TMP)
	echo '```' >> $(README_TMP)
	cat ../runtime_tos/doc/os_atari.txt >> $(README_TMP)
	echo '```' >> $(README_TMP)
	cp $(README_TMP) ../README.md
	mv $(README_TMP) ../README_tos.txt

release: all xxd/xxd.ttp rt_doctags doctags.ttp tos_utils readme
	-rm -r $(RELDIR)
	-mkdir -p $(RELDIR)/rt
	cp -r $(RUNTIME_SRC)/* $(RELDIR)/rt
	cp vim*.ttp $(RELDIR)
	cp xxd/xxd.ttp $(RELDIR)
	cp doctags.ttp $(RELDIR)
	cp tos_utils/tospal.tos $(RELDIR)
	cp vimrc $(RELDIR)
	cp $(RUNTIME_SRC)/doc/os_atari.txt $(RELDIR)/readme.txt
	cd $(RELROOT) && tar -czvf $(TARNAME) vim/

###########################################################################

lint: 
	cppcheck -q --std=c99 --enable=performance --language=c --template=gcc \
	$(N_FLAGS) $(SRC)

tags:
	ctags `find /opt/libcmini -name "*.h"` *.c *.h

clean: 
	-rm -r obj_vim*
	-rm vim*.ttp
	-rm -r tmp/vim
	-rm doctags.ttp
	$(MAKE) --directory=xxd -f Make_tos.mak clean
	$(MAKE) --directory=tos_utils clean

###########################################################################
