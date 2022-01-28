#!/bin/sh
9c -O0 -Wno-char-subscripts -g -I . asm.c list.c obj.c optab.c pass.c span.c ../6l/unixcompat.c ../8c/enam.c ../ld/elf.c

9l -o 8l \
asm.o \
enam.o \
elf.o \
list.o \
obj.o \
optab.o \
pass.o \
span.o \
unixcompat.o 

cp 8l $HARVEY_LINUX_BIN
