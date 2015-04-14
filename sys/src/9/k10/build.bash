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

DATE=$(date +%s)
EXTKERNDATE="-DKERNDATE=${DATE}"

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
	CFLAGS="-mcmodel=kernel -O${OPTIMIZE} -static ${EXTENSIONS} -ffreestanding -fno-builtin ${GDBFLAG} ${EXTKERNDATE}"
	# oh, gcc.
	CFLAGS="$CFLAGS -fvar-tracking -fvar-tracking-assignments"
	UCFLAGS="-O${OPTIMIZE} -static ${EXTENSIONS} -ffreestanding -fno-builtin ${GDBFLAG}"

	## General conf file ##

	$AWK -v objtype=$objtype -f ../mk/parse -- -mkdevc $CONF > $CONF.c

	## Assembly world ##

	$AWK -f ../mk/mkenumb amd64.h > amd64l.h # mkenumb is shell independent

	## Boot ##

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
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -mcmodel=small -c boot$CONF.c
	echo "CC printstub"
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -mcmodel=small -c ../boot/printstub.c
	echo "LD boot${CONF}.out"

	$LD -static -o boot$CONF.elf.out boot$CONF.o printstub.o $LDFLAGS -L$BOOTDIR -lboot -lip -lauth -lc -emain -Ttext=0x200020

	## systab.c ##

	$AWK -f ../mk/parse -- -mksystab /sys/src/libc/9syscall/sys.h $CONF > systab.c
	$CC $CFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -c systab.c

	## init.h ##

	$CC $UCFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -mcmodel=small -c init9.c
	$CC $UCFLAGS $WARNFLAGS -I$INC_DIR -I$INCX86_64_DIR -I. -mcmodel=small -c ../port/initcode.c

	$LD -static -o init init9.o initcode.o $LDFLAGS -lc -emain -Ttext=0x200020
	elf2c init > init.h

	## errstr.h ##

	$AWK -f ../mk/parse -- -mkerrstr $CONF > errstr.h

	## mkroot ##

	# We need data2c, look at plan9-gpl/util

	objcopy -j .text  -O binary boot$CONF.elf.out boot$CONF.code.out
	objcopy -j .data  -O binary boot$CONF.elf.out boot$CONF.data.out
	file boot$CONF*.out
	cat boot$CONF.code.out boot$CONF.data.out >> boot$CONF.all.out
	
	data2c bootk8cpu_out boot$CONF.all.out >> k8cpu.root.c

	# We haven't these for now. Must be concatenated text+data as with boot$CONF.elf.out.
	# You need to run BUILDKFACTOTUM into /sys/src/cmd/auth/factotum
	# in order to have working this.

	objcopy -j .text  -O binary /sys/src/cmd/auth/factotum/factotum.elf.out factotum.code.out
	objcopy -j .data  -O binary /sys/src/cmd/auth/factotum/factotum.elf.out factotum.data.out
	file factotum*.out
	cat factotum.code.out factotum.data.out >> factotum.all.out
	data2c _amd64_bin_auth_factotum  factotum.all.out >> k8cpu.root.c

	# You need to run BUILDKIPCONFIG into /sys/src/cmd/ip/ipconfig
	# in order to have working this.
	
	objcopy -j .text  -O binary /sys/src/cmd/ip/ipconfig/ipconfig.elf.out ipconfig.code.out
	objcopy -j .data  -O binary /sys/src/cmd/ip/ipconfig/ipconfig.elf.out ipconfig.data.out
	file ipconfig*.out
	cat ipconfig.code.out ipconfig.data.out >> ipconfig.all.out
	data2c _amd64_bin_ip_ipconfig  ipconfig.all.out >> k8cpu.root.c

	# You need to run BUILDKRC into /sys/src/cmd/rc
	# in order to have working this.
	
	objcopy -j .text  -O binary /sys/src/cmd/rc/rc.elf.out rc.code.out
	objcopy -j .data  -O binary /sys/src/cmd/rc/rc.elf.out rc.data.out
	file rc*.out
	cat rc.code.out rc.data.out >> rc.all.out
	data2c _amd64_bin_rc rc.all.out >> k8cpu.root.c

	## Making all ##

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
}

args=("$@")
compiling
linking
