#!/bin/sh
9c -Wno-char-subscripts \
   -Wno-format \
   -Wno-unused-variable \
   -Wno-format-extra-args \
   -Wno-maybe-uninitialized \
   -Wno-int-conversion \
   -nostdlib \
   -g -I .   \
	cpp.c\
	lex.c\
	nlist.c\
	tokens.c\
	macro.c\
	eval.c\
	include.c\
	hideset.c\

9l -o cpp \
	cpp.o\
	lex.o\
	nlist.o\
	tokens.o\
	macro.o\
	eval.o\
	include.o\
	hideset.o\

cp cpp $HARVEY_LINUX_BIN
