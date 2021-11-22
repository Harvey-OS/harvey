#!/bin/sh
mkdir -p ../../../../linux_amd64/bin
9c -O0 -Wno-char-subscripts -g -I .  asm.c list.c obj.c optab.c noop.c pass.c sched.c span.c ../6l/unixcompat.c ../8c/enam.c ../ld/elf.c

9l -o 4l \
asm.o \
enam.o \
elf.o \
list.o \
obj.o \
optab.o \
noop.o \
pass.o \
sched.o \
span.o \
unixcompat.o 

cp 4l ../../../../linux_amd64/bin
