#!/bin/sh
9c -O0 -Wno-char-subscripts -g -I . asm.c list.c obj.c optab.c pass.c span.c unixcompat.c ../6c/enam.c ../ld/elf.c

9l -o 6l \
asm.o \
enam.o \
elf.o \
list.o \
obj.o \
optab.o \
pass.o \
span.o \
unixcompat.o 

cp 6l $HARVEY_LINUX_BIN
