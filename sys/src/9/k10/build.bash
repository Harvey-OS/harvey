#!/bin/bash

## General script for compiling kernel ##
##-------------------------------------##

set -e

## Params ##
##--------##

CONF="k8cpu"
objtype=amd64
AS=as
LD=gcc
CC=gcc
COLLECT=/usr/lib/gcc/x86_64-linux-gnu/4.9/collect2
COLLECTFLAGS="-plugin /usr/lib/gcc/x86_64-linux-gnu/4.9/liblto_plugin.so -plugin-opt=/usr/lib/gcc/x86_64-linux-gnu/4.9/lto-wrapper --sysroot=/ --build-id -m elf_x86_64 --hash-style=gnu"
AR=ar
LEX=lex
YACC=yacc
AWK=awk
XD=xxd

INC_DIR="/sys/include"
INCX86_64_DIR="/${objtype}/include"
LIB_DIR="/${objtype}/lib"
LDFLAGS=-L${LIB_DIR}

## Compiler options ##
##------------------##

WARNFLAGS="-Wall -Wno-missing-braces -Wno-parentheses -Wno-unknown-pragmas -Wuninitialized -Wmaybe-uninitialized"
GDBFLAG=	# -g
PICFLAG=	# -fPIC
OPTIMIZE=0
EXTENSIONS="-fplan9-extensions"

compiling()
{
	CFLAGS="-O${OPTIMIZE} -static ${EXTENSIONS} -ffreestanding -fno-builtin"

	## General conf file ##
	##-------------------##

	$AWK -v objtype=$objtype -f ../mk/parse -- -mkdevc $CONF > $CONF.c

	## Assembly world ##
	##----------------##

	$AWK -f ../mk/mkenumb amd64.h > amd64l.h # mkenumb is shell independent

	# We don't want one of these (sipi.c depends on sipi.h from l64sipi.s)#
	GLOBIGNORE=*doauthenticate.c:*getpasswd.c:*nopsession.c:*archk8.c:*alloc.c:*rdb.c:*etherbcm.c:*devprobe.c:*tcklock.c:*sipi.c

	## Boot ##
	##------##

	# Do we really need this?? ##
	BOOTDIR=../boot
	BOOTLIB=libboot.a

	echo "Parsing boot${CONF}.c"
	$AWK -f ../mk/parse -- -mkbootconf $CONF > boot$CONF.c

	echo "Making Boot"
	(cd ../boot && 
		$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c *.c && 
		echo "AR ${BOOTLIB}" &&
		$AR rv $BOOTLIB  *.o
	)

	echo "CC boot${CONF}"
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c boot$CONF.c
	echo "CC printstub"
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c ../boot/printstub.c
	echo "LD boot${CONF}.out"

	$COLLECT $COLLECTFLAGS -static -o boot$CONF.out boot$CONF.o printstub.o $LDFLAGS -L$BOOTDIR -lboot -lip -lauth -lc

	## systab.c ##
	##----------##

	$AWK -f ../mk/parse -- -mksystab /sys/src/libc/9syscall/sys.h $CONF > systab.c
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c systab.c

	## init.h ##
	##--------##

	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c init9.c
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c ../port/initcode.c

	# I'm sure this binary is wrong. Check when kernel will build entirely

	$COLLECT $COLLECTFLAGS -static -o init.out init9.o initcode.o $LDFLAGS -lc
	$XD -i init.out > init.h

	## errstr.h ##
	##----------##

	$AWK -f ../mk/parse -- -mkerrstr $CONF > errstr.h

	## Let's go! ##
	##-----------##

	# Do we really need all of *.c sources?? #

	echo "Making all in kernel directories"
	echo
	i="*.c ../386/*.c ../ip/*.c ../port/*.c entry.s l64v.s"
	echo "CC ${i}"
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c $i
}

linking()
{
	# These are not part of the kernel
	rm init9.o initcode.o bootk8cpu.o printstub.o
	$COLLECT $COLLECTFLAGS -static -o 9 *.o $LDFLAGS -lip -lauth -lc
}

args=("$@")
compiling
linking
