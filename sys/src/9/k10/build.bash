#!/bin/bash

## General script for compiling kernel ##
##-------------------------------------##

set -e

## Params ##
##--------##

CONF="k8cpu"
objtype=amd64
AS=as
LD=ld
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
GDBFLAG="-g"
PICFLAG=	# -fPIC
OPTIMIZE=0
EXTENSIONS="-fplan9-extensions"
GLOBIGNORE=*devcap.c:*sdaoe.c:ratrace.c:*doauthenticate.c:*getpasswd.c:*nopsession.c:*archk8.c:*rdb.c:*etherbcm.c:*devprobe.c:*tcklock.c:*sipi.c:*sd.c:*ssl.c:*tls.c:*proc-OLD.c:*proc-SMP.c
SOURCE="entry.S *.c ../386/*.c ../ip/*.c ../port/*.c l64v.S l64fpu.S cpuidamd64.S l64idt.S hack.S l64vsyscall.S "

compiling()
{
	CFLAGS="-mcmodel=kernel -O${OPTIMIZE} -static ${EXTENSIONS} -ffreestanding -fno-builtin ${GDBFLAG}"
	# oh, gcc.
	CFLAGS="$CFLAGS -fvar-tracking -fvar-tracking-assignments"
	UCFLAGS="-O${OPTIMIZE} -static ${EXTENSIONS} -ffreestanding -fno-builtin ${GDBFLAG}"

	## General conf file ##
	##-------------------##

	$AWK -v objtype=$objtype -f ../mk/parse -- -mkdevc $CONF > $CONF.c

	## Assembly world ##
	##----------------##

	$AWK -f ../mk/mkenumb amd64.h > amd64l.h # mkenumb is shell independent

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

	$LD -static -o boot$CONF.out boot$CONF.o printstub.o $LDFLAGS -L$BOOTDIR -lboot -lip -lauth -lc -emain -Ttext=0x1020

	## systab.c ##
	##----------##

	$AWK -f ../mk/parse -- -mksystab /sys/src/libc/9syscall/sys.h $CONF > systab.c
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c systab.c

	## init.h ##
	##--------##

	$CC $UCFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -mcmodel=small -c init9.c
	$CC $UCFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -mcmodel=small -c ../port/initcode.c

	# I'm sure this binary is wrong. Check when kernel will build entirely

	$LD -static -o init.out init9.o initcode.o $LDFLAGS -lc -emain -Ttext=0x200020
	strip init.out
	$XD -i init.out > init.h

	## errstr.h ##
	##----------##

	$AWK -f ../mk/parse -- -mkerrstr $CONF > errstr.h

	## mkroot ##
	##--------##

	# We need data2c!! added to plan9-gpl/util

	# Do we must strip binary?
	strip bootk8cpu.out
	data2c bootk8cpu_out bootk8cpu.out >> k8cpu.root.c

	# We haven't these for now. If strip it's not needed, cp step should be gone and /amd64/binary should be passed to data2c.
	#cp /amd64/bin/auth/factotum factotum; strip factotum
    #data2c _amd64_bin_auth_factotum  factotum >> k8cpu.root.c
    #cp /amd64/bin/ip/ipconfig ipconfig; strip ipconfig
	#data2c _amd64_bin_ip_ipconfig  ipconfig >> k8cpu.root.c

	## Let's go! ##
	##-----------##

	# Do we really need all of *.c sources?? #

	echo "Making all in kernel directories"
	echo
	echo $CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c $SOURCE
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c $SOURCE
}

linking()
{
	# These are not part of the kernel
	rm init9.o initcode.o bootk8cpu.o printstub.o

	# building from akaros
	# this just got ugly.
	# entry.o MUST be first.
  /bin/bash link.bash  entry.o `echo *.o | sed s/entry.o//` /amd64/lib/klibc.a /amd64/lib/klibip.a

  # not likely what we want for a kernel. $COLLECT $COLLECTFLAGS -static -o 9 *.o $LDFLAGS -lip -lauth -lc
}

args=("$@")
compiling
linking
