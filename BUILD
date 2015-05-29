#!/bin/bash

#BUILD script
#
#Copyright (C) 2015  Rafael
#
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License along
#with this program; if not, write to the Free Software Foundation, Inc.,
#51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#

#Search for config:
_BUILD_DIR=`dirname $0`
export _BUILD_DIR=$(cd "$_BUILD_DIR"; pwd)
. ${_BUILD_DIR}/BUILD.conf

#ARGS:
#  $1 -> pass the $? return
#  $2 -> component name 
check_error()
{
	if [ $1 -ne 0 ]
	then
		echo "ERROR $2"
		exit 1
	fi
}

#ARGS:
#  $1 -> test name
build_a_test()
{
	cd ${TEST_DIR}/$1
	DO_NOTHING=0
	LDFLAGS_EXTRA=
	CFLAGS_EXTRA=
	test_${1} 1
	if [ $DO_NOTHING -eq 0 ]
	then
		echo "$CC $CFLAGS_CMD $CFLAGS_EXTRA $BUILD_DEBUG -c $BUILD_IN"
		$CC $CFLAGS_CMD $CFLAGS_EXTRA $BUILD_DEBUG -c $BUILD_IN 
		if [ $? -ne 0 ]
		then
			echo "ERROR compiling $1"
			cd - > /dev/null
			exit 1
		fi
		LD_LIBS=`process_libs_to_link "$LIBS_TO_LINK"`
		echo $LD $LDFLAGS_EXTRA $LDFLAGS $LD_LIBS -o $BUILD_OUT *.o
		$LD $LDFLAGS_EXTRA $LDFLAGS -o $BUILD_OUT *.o $LD_LIBS
		if [ $? -ne 0 ]
		then
			echo "ERROR linking $1"
			cd - > /dev/null
			exit 1
		fi
	fi
	cd - > /dev/null
}

clean_kernel()
{
	cd "$KRL_DIR"
	
	printf "Cleaning kernel "
	rm -f *.o *.root.c *.out errstr.h init.h amd64^l.h libboot.a ../boot/*.o boot$CONF.c
	if [ $? -eq 0 ]
	then
		printf "OK\n"
	else
		printf "ERROR\n"
	fi 
	cd - > /dev/null
}

compile_kernel()
{
	CONF=$KERNEL_CONF
	SOURCE=$KERNEL_SOURCE
	CFLAGS="$KERNEL_CFLAGS"
	if [ -n "$BUILD_DEBUG" ]
	then
		CFLAGS="$KERNEL_CFLAGS $KERNEL_CFLAGS_DEBUG"
	fi
	
	WARNFLAGS="$KERNEL_WARNFLAGS"
	UCFLAGS="$KERNEL_UCFLAGS $BUILD_DEBUG"
	GLOBIGNORE=$KERNEL_GLOBIGNORE

	PATH_ORI=`pwd`
	cd "$KRL_DIR"
	
	echo "$AWK -v objtype=$ARCH -f ../mk/parse -- -mkdevc $CONF > $CONF.c"
	$AWK -v objtype=$ARCH -f ../mk/parse -- -mkdevc $CONF > $CONF.c
	check_error $? "executing $AWK"
	echo "$AWK -f ../mk/mkenumb amd64.h > amd64l.h"
	$AWK -f ../mk/mkenumb amd64.h > amd64l.h # mkenumb is shell independent
	check_error $? "executing $AWK"
	
	## BOOT ##
	
	BOOTDIR=../boot
    BOOTLIB=libboot.a
	echo "Parsing boot${CONF}.c"
	echo "$AWK -f ../mk/parse -- -mkbootconf $CONF > boot$CONF.c"
    $AWK -f ../mk/parse -- -mkbootconf $CONF > boot$CONF.c
    check_error $? "executing $AWK"
	echo "Making Boot"
    cd ../boot
    echo $CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -c *.c
    $CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -c *.c
    check_error $? "compiling $BOOTLIB"
    echo "$AR ${BOOTLIB} *.o"
    $AR rv $BOOTLIB  *.o
    check_error $? "building $BOOTLIB"
    cd - > /dev/null
	echo "$CC boot${CONF}"
	echo "$CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c boot$CONF.c"
    $CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c boot$CONF.c
    check_error $? "compiling boot${CONF}"
    echo "$CC printstub"
    echo "$CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c ../boot/printstub.c"
    $CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c ../boot/printstub.c
    check_error $? "compiling printstub.c"
    echo "$LD boot${CONF}.out"
    $LD -static -o boot$CONF.elf.out boot$CONF.o printstub.o $LDFLAGS -L$BOOTDIR -lboot -lip -lauth -lc -e_main
	check_error $? "linking boot$CONF.elf.out"
	
	## systab.c ##

	echo "$AWK -f ../mk/parse -- -mksystab ${SRC_DIR}/libc/9syscall/sys.h $CONF > systab.c"
    $AWK -f ../mk/parse -- -mksystab ${SRC_DIR}/libc/9syscall/sys.h $CONF > systab.c
    check_error $? "parsing systab.c"
    echo "$CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -c systab.c"
    $CC $CFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -c systab.c
	check_error $? "compiling systab.c"

    ## init.h ##

	echo "$CC $UCFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c init9.c"
    $CC $UCFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c init9.c
    check_error $? "compiling init9.c"
    echo "$CC $UCFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c ../port/initcode.c"
    $CC $UCFLAGS $WARNFLAGS -I${INC_ARCH} -I${INC_DIR} -I. -mcmodel=small -c ../port/initcode.c
    check_error $? "compiling initcode.c"

	echo "$LD -static -o init init9.o initcode.o $LDFLAGS -lc -e_main -Ttext=0x200020"
    $LD -static -o init init9.o initcode.o $LDFLAGS -lc -e_main -Ttext=0x200020
    check_error $? "linking init"
    echo "${UTIL_DIR}/elf2c init > init.h"
    ${UTIL_DIR}/elf2c init > init.h
    check_error $? "executing elf2c"

    ## errstr.h ##

	echo "$AWK -f ../mk/parse -- -mkerrstr $CONF > errstr.h"
    $AWK -f ../mk/parse -- -mkerrstr $CONF > errstr.h
	check_error $? "parsing errstr"
	
	## mkroot ##
	
	#strip boot$CONF.elf.out
    #check_error $? "to strip boot$CONF.elf.out"
    #echo "${UTIL_DIR}/data2c "boot$CONF"_out boot$CONF.elf.out >> k8cpu.root.c"
    #${UTIL_DIR}/data2c "boot$CONF"_out boot$CONF.elf.out >> k8cpu.root.c
	#check_error $? "executing data2c"
	

	# we need a WAY better way to do this. We're having to do it for each file.
    echo "${UTIL_DIR}/data2c boot_fs boot.fs >> k8cpu.root.c"
    ${UTIL_DIR}/data2c boot_fs boot.fs >> k8cpu.root.c
	check_error $? "executing data2c"

	echo "cp ${BASEDIR}/rc/lib/rcmain rcmain"
	cp ${BASEDIR}/rc/lib/rcmain rcmain
    echo "${UTIL_DIR}/data2c _rc_lib_rcmain rcmain >> k8cpu.root.c"
    ${UTIL_DIR}/data2c _rc_lib_rcmain rcmain >> k8cpu.root.c
	check_error $? "executing data2c"

	##### Factotum / ipconfig ######
	
	##cp /sys/src/cmd/auth/factotum/factotum.elf.out factotum.elf.out
    ##strip factotum.elf.out
    ##data2c _amd64_bin_auth_factotum factotum.elf.out >> k8cpu.root.c

    ##cp /sys/src/cmd/ip/ipconfig/ipconfig.elf.out ipconfig.elf.out
    ##strip ipconfig.elf.out
    ##data2c _amd64_bin_ip_ipconfig ipconfig.elf.out >> k8cpu.root.c
	
	## rc or test ##
	
	if [ -n "$TEST_APP" ]
	then
		build_a_test "$TEST_APP"
		#test_${1} 1
		cp ${TEST_DIR}/${TEST_APP}/${BUILD_OUT} rc.elf.out
		check_error $? "copying test $TEST_APP"
	else
		echo "cp ${CMD_DIR}/rc/rc.elf.out rc.elf.out"
		cp ${CMD_DIR}/rc/rc.elf.out rc.elf.out
		check_error $? "copying rc"
	fi
	echo "strip rc.elf.out"
    strip rc.elf.out
    check_error $? "to strip rc"
    echo "${UTIL_DIR}/data2c _amd64_bin_rc rc.elf.out >> k8cpu.root.c"
    ${UTIL_DIR}/data2c _amd64_bin_rc rc.elf.out >> k8cpu.root.c
	check_error $? "executing data2c"

	## Rest of programs into ramfs ##

	RAMFS_LIST="bind mount echo cat cp ls ip/ipconfig/ipconfig ip/ping ip/telnet ip/dhcpclient ps mkdir pwd chmod rio/rio date dd aux/vga/vga"

	for elem in $RAMFS_LIST
	do
		elf_out=`basename $elem.elf.out`
		bname=`basename $elem`
		echo "cp ${CMD_DIR}/$elem.elf.out $elf_out"
		cp ${CMD_DIR}/$elem.elf.out $elf_out
		check_error $? "copying $elem"
		echo "strip $elf_out"
		strip $elf_out
		check_error $? "to strip $elem"
		echo "${UTIL_DIR}/data2c _amd64_bin_$bname $elf_out>> k8cpu.root.c"
		${UTIL_DIR}/data2c _amd64_bin_$bname $elf_out>> k8cpu.root.c
		check_error $? "executing data2c"
	done
	
	## Making all ##

	echo
	echo "Making all in kernel directories"
    echo
    echo $CC $CFLAGS $WARNFLAGS -I${INC_DIR} -I${INC_ARCH} -I. -c $SOURCE
    $CC $CFLAGS $WARNFLAGS -I${INC_DIR} -I${INC_ARCH} -I. -c $SOURCE
	check_error $? "compiling kernel"
	GLOBIGNORE=
	cd "$PATH_ORI" > /dev/null
}

gen_symtab_obj()
{
        $NM -n $KERNEL_OBJECT > $KSYM_MAP
        #awk 'BEGIN{ print "//#include <kdebug.h>";
                    #print "struct symtab_entry gbl_symtab[]={" }
             #{ if(NF==3){print "{\"" $3 "\", 0x" $1 "},"}}
             #END{print "{0,0} };"}' $KSYM_MAP > $KSYM_C
        echo "FIX THIS SOMEDAY"
        #$CC $NOSTDINC_FLAGS $AKAROSINCLUDE $CFLAGS_KERNEL -o $KSYM_O -c $KSYM_C
}

link_kernel()
{
	PATH_ORI=`pwd`
	cd "$KRL_DIR"
	rm init9.o initcode.o bootk8cpu.o printstub.o

	LDFLAGS="$KERNEL_LDFLAGS"
	LINKER_SCRIPT=kernel.ld
	KSYM_MAP=./ksyms.map
	KSYM_C=./ksyms-refl.c
	KSYM_O=./ksyms-refl.o
	
	#entry.o MUST be first.
	GLOBIGNORE=entry.o
	LIST_OBJ=`ls *.o`
	REMAINING_ARGS="entry.o $LIST_OBJ ${LIB_DIR}/klibc.a ${LIB_DIR}/klibip.a"
	

	# Generates the first version of $KERNEL_OBJECT
	echo $LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS
	$LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS
	check_error $? "linking $KERNEL_OBJECT"

	# Generates a C and obj file with a table of the correct size, with relocs
	echo FIX ME gen_symtab_obj

	# Links the syms with the kernel and inserts the glb_symtab in the kernel.
	echo $LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS # $KSYM_O
	$LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS # $KSYM_O
	check_error $? "linking $KERNEL_OBJECT"

	# Need to recheck/compute the symbols (table size won't change)
	# gen_symtab_obj

	# Final link
	# $LD -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS # $KSYM_O

	# And objdump for debugging
	echo "objdump -S $KERNEL_OBJECT > $KERNEL_OBJECT.asm"
	objdump -S $KERNEL_OBJECT > $KERNEL_OBJECT.asm
	check_error $? "generating $KERNEL_OBJECT.asm"

	objcopy -I elf64-x86-64 -O elf32-i386 9k 9k.32bit
	check_error $? "creating 9k.32bit"

	cd "$PATH_ORI"
}

build_kernel()
{
	clean_kernel
	compile_kernel
	link_kernel
	echo "KERNEL COMPILED OK"
}


build_go_utils()
{
	cd "${UTIL_DIR}"
	for i in `ls *.go`
	do
		go build $i
		if [ $? -ne 0 ]
		then
			printf "\nERROR compiling $i \n"
			exit 1
		fi
	done
	cd - > /dev/null
}

check_utils()
{
	if [ -f "${UTIL_DIR}"/mksys ]
	then
		echo 0
	else
		echo 1
	fi
}

check_lib_dir()
{
	if [ ! -d "$LIB_DIR" ]
	then
		mkdir "$LIB_DIR"
		if [ $? -ne 0 ]
		then
			echo "ERROR creating <$LIB_DIR> directory"
		fi
	fi
}

#ARGS:
#$1 -> libname
build_l()
{
	if [ $DO_NOTHING -eq 0 ]
	then
		for j in $BUILD_IN
		do
			echo "$CC $CFLAGS_LIB $CFLAGS_EXTRA $BUILD_DEBUG -c $j"
			$CC $CFLAGS_LIB $CFLAGS_EXTRA $BUILD_DEBUG -c $j
			if [ $? -ne 0 ]
			then
				printf "\n\n ERROR COMPILING $1\n"
				exit 1
			fi
		done
		echo "$AR rv \"$BUILD_OUT\" *.o"
		$AR rv "$BUILD_OUT" *.o
		if [ $? -ne 0 ]
		then
			printf "\n\n ERROR CREATING LIBRARY $1\n"
		fi
	fi
}

#ARGS:
#$1 -> libname
build_kl()
{
	if [ $DO_NOTHING -eq 0 ]
	then
		echo "$CC $CFLAGS_KLIB $CFLAGS_EXTRA -c $BUILD_IN"
		$CC $CFLAGS_KLIB $CFLAGS_EXTRA $BUILD_DEBUG -c $BUILD_IN
		if [ $? -ne 0 ]
		then
			printf "\n\n ERROR COMPILING $1\n"
			exit 1
		fi
		echo "$AR rv \"$BUILD_OUT\" *.o"
		$AR rv "$BUILD_OUT" *.o
		if [ $? -ne 0 ]
		then
			printf "\n\n ERROR CREATING LIBRARY $1\n"
		fi
	fi
}


#ARGS:
#$1 -> Lib name
#$2 -> 0 a lib | 1 a klib
build_a_lib()
{
	PATH_ORI=`pwd`
	cd "${SRC_DIR}/$1"
	CFLAGS_EXTRA=
	SUBDIRS=
	DO_NOTHING=0
	if [ $2 -eq 1 ]
	then
		k${1} 1
	else
		$1 1
	fi
	if [ -n "$SUBDIRS" ]
	then
		for k in $SUBDIRS
		do
			CFLAGS_EXTRA=
			DO_NOTHING=0
			cd $k
			if [ $2 -eq 1 ]
			then
				k${1}_$k 1
				build_kl k${1}
			else
				$1_$k 1
				build_l $1
			fi
			cd ..
		done
	else
		if [ $2 -eq 1 ]
		then
			build_kl $1
		else
			build_l $1
		fi
	fi
	
	cd "$PATH_ORI" > /dev/null
}

#ARGS:
#$1 -> Lib name
#$2 -> 0 a lib | 1 a klib
clean_a_lib()
{
	PATH_ORI=`pwd`
	cd "${SRC_DIR}/$1"
	SUBDIRS=
	DO_NOTHING=0
	if [ $2 -eq 1 ]
	then
		k${1} 2
	else
		$1 2
	fi
	if [ $DO_NOTHING -eq 0 ]
	then
		if [ -n "$SUBDIRS" ]
		then
			for k in $SUBDIRS
			do
				DO_NOTHING=0
				cd $k
				if [ $2 -eq 1 ]
				then
					k${1}_$k 2
					if [ $DO_NOTHING -eq 0 ]
					then
						printf "Cleaning k${1}/$k "
						$CLEAN_COM
						if [ $? -eq 0 ]
						then
							printf "OK\n"
						else
							printf "ERROR\n"
						fi 
					fi
				else
					$1_$k 2
					if [ $DO_NOTHING -eq 0 ]
					then
						printf "Cleaning ${1}/$k "
						$CLEAN_COM
						if [ $? -eq 0 ]
						then
							printf "OK\n"
						else
							printf "ERROR\n"
						fi 
					fi
				fi
				cd ..
			done
		else
			if [ $2 -eq 1 ]
			then
				K="k"
			else
				K=""
			fi
			printf "Cleaning ${K}$1 "
			$CLEAN_COM
			if [ $? -eq 0 ]
			then
				printf "OK\n"
			else
				printf "ERROR\n"
			fi 
		fi
	fi
	cd "$PATH_ORI" > /dev/null
}

#ARGS:
#$1 -> ACTION:
#      1) Build
#      2) Clean
build_libs()
{
	if [ $1 -eq 1 ]
	then
		SALIDA=`check_utils`
		if [ $SALIDA -eq 1 ]
		then
			build_go_utils
		fi
	fi
	
	for i in $BUILD_LIBS
	do
		if [ $1 -eq 1 ]
		then
			build_a_lib $i 0
		else
			clean_a_lib $i 0
		fi
	done
	
	if [ $1 -eq 1 ]
	then
		echo "ALL LIBS COMPILED OK"
	fi
}

#ARGS:
#$1 -> ACTION:
#      1) Build
#      2) Clean
build_klibs()
{
	if [ $1 -eq 1 ]
	then
		SALIDA=`check_utils`
		if [ $SALIDA -eq 1 ]
		then
			build_go_utils
		fi
	fi
	
	for i in $BUILD_KLIBS
	do
		if [ $1 -eq 1 ]
		then
			build_a_lib $i 1
		else
			clean_a_lib $i 1
		fi
	done
	
	if [ $1 -eq 1 ]
	then
		echo "ALL KLIBS COMPILED OK"
	fi
}

#ARGS:
#   $1 -> LIB name
do_link_lib()
{
	AUX=`echo $1 | grep ^lib`
	_LIB_NAME=
	if [ -n "$AUX" ]
	then
		#Name format is libName
		_LIB_NAME=`echo $AUX | cut -c 4-`
		_LIB_NAME="-l$_LIB_NAME"
	else
		#Name format is Name
		_LIB_NAME="-l$1"
	fi
	echo $_LIB_NAME
}

#ARGS:
#	$1 -> libs list
process_libs_to_link()
{
	_RETURN=
	for i in $1
	do
		_RETURN=${_RETURN}" "`do_link_lib $i`
	done
	echo $_RETURN
}

#ARGS:
#  $1 -> cmd name
build_a_cmd()
{
	# test if binary has its own subdir
	if [ -d "${CMD_DIR}/$1" ]
	then
		cd ${CMD_DIR}/$1
	else
		cd ${CMD_DIR}
	fi
	DO_NOTHING=0
	LDFLAGS_EXTRA=
	CFLAGS_EXTRA=
	BUILD_DIR=
	cmd_${1} 1
	if [ -n "$BUILD_DIR" ]
	then
		cd -
		cd "$BUILD_DIR"
	fi
	if [ $DO_NOTHING -eq 0 ]
	then
		echo "$CC $CFLAGS_CMD $CFLAGS_EXTRA $BUILD_DEBUG -c $BUILD_IN"
		$CC $CFLAGS_CMD $CFLAGS_EXTRA $BUILD_DEBUG -c $BUILD_IN 
		if [ $? -ne 0 ]
		then
			echo "ERROR compiling $1"
			cd - > /dev/null
			exit 1
		fi
	# If it's a stand alone source don't get all object files
		if [ -d "${CMD_DIR}/$1" ]
		then
			LD_LIBS=`process_libs_to_link "$LIBS_TO_LINK"`
			echo $LD $LDFLAGS $LDFLAGS_EXTRA $LD_LIBS -o $BUILD_OUT *.o
			$LD $LDFLAGS_EXTRA $LDFLAGS -o $BUILD_OUT *.o $LD_LIBS
		elif [ -n "$BUILD_DIR" ] && [ -d `dirname $BUILD_DIR`"/$1" ]
		then
			LD_LIBS=`process_libs_to_link "$LIBS_TO_LINK"`
			echo $LD $LDFLAGS $LDFLAGS_EXTRA $LD_LIBS -o $BUILD_OUT *.o
			$LD $LDFLAGS_EXTRA $LDFLAGS -o $BUILD_OUT *.o $LD_LIBS
		else
			LD_LIBS=`process_libs_to_link "$LIBS_TO_LINK"`
			echo $LD $LDFLAGS $LDFLAGS_EXTRA $LD_LIBS -o $BUILD_OUT $1.o
			$LD $LDFLAGS_EXTRA $LDFLAGS -o $BUILD_OUT $1.o $LD_LIBS
		fi
		if [ $? -ne 0 ]
		then
			echo "ERROR linking $1"
			cd - > /dev/null
			exit 1
		fi
	fi
	cd - > /dev/null
}

#ARGS:
#   $1 -> cmd name
clean_a_cmd()
{
	cd ${CMD_DIR}/$1
	DO_NOTHING=0
	cmd_${1} 2
	if [ $DO_NOTHING -eq 0 ]
	then
		printf "Cleaning $1 "
		$CLEAN_COM
		if [ $? -eq 0 ]
		then
			printf "OK\n"
		else
			printf "ERROR\n"
		fi 
	fi
}

#ARGS:
#$1 -> ACTION:
#      1) Build
#      2) Clean
build_cmds()
{
	for i in $BUILD_CMD
	do
		if [ $1 -eq 1 ]
		then
			build_a_cmd $i
		else
			clean_a_cmd $i
		fi
	done
	
	if [ $1 -eq 1 ]
	then
		echo "ALL CMDS COMPILED OK"
	fi
}

show_help()
{
	printf "\n\nBUILD script for Harvey\n\n"
	printf "OPTIONS:\n"
	printf "  all        \tBuild all components\n"
	printf "  cleanll     \tClean all components\n"
	printf "  libs       \tBuild the libraries\n"
	printf "  libs <libname>\tBuild the library <libname>\n"
	printf "  cleanlibs\tClean the libraries\n"
	printf "  klibs       \tBuild the Klibraries\n"
	printf "  klibs <libname>\tBuild the Klibrary <libname>\n"
	printf "  cleanklibs\tClean the Klibraries\n"
	printf "  utils     \tBuild go utils\n"
	printf "  cmd       \tBuild all cmds \n"
	printf "  cmd <cmdname>\tBuild cmd named <cmdname>\n"
	printf "  cleancmd   \tClean the cmds\n"
	printf "  kernel     \tBuild kernel\n"
	printf "  cleankernel\tClean kernel\n"
	printf "\nFLAGS:\n"
	printf "  -g        \tCompile with debugs flags\n"
	printf "  -t <test> \tCompile <test> app and package it with the kernel"
	printf "\n"

}

### MAIN SCRIPT ####
if [ -z "$1" ]
then
	show_help
	exit 1
else
	#BUILD_DEBUG=
	#Until we have a stable kernel, debug mode is the default.
	BUILD_DEBUG="$CFLAGS_DEBUG"
	TEST_APP=
	while [ -n "$1" ]
	do
		case "$1" in
			"-g")
					BUILD_DEBUG="$CFLAGS_DEBUG"
					;;
			"-t"*)
					#is -tSomething?
					TEST_APP=${1:2}
					if [ -z "$TEST_APP" ]
					then
						if [ -n "$2" ]
						then
							#form -t something
							TEST_APP="$2"
							shift
						else
							#-t nothing
							echo "Error. Use -t testapp"
							exit 1
						fi
					fi
					;;
			"all")
					check_lib_dir
					build_go_utils
					build_libs 1
					build_klibs 1
					build_cmds 1
					build_kernel
					printf "\n\nALL COMPONENTS COMPILED\n\n"
					;;
			"cleanall")
					build_libs 2
					build_klibs 2
					build_cmds 2
					clean_kernel
					printf "\n\nALL COMPONENTS CLEANED\n\n"
					;;
			"libs")
					check_lib_dir
					if [ -z "$2" ]
					then
						build_libs 1
					else
						build_a_lib "$2" 0
						shift
					fi
					;;
			"klibs")
					check_lib_dir
					if [ -z "$2" ]
					then
						build_klibs 1
					else
						build_a_lib "$2" 1
						shift
					fi
					;;
			"cleanklibs")
					build_klibs 2
					;;
			"cleanlibs")
					build_libs 2
					;;
			"utils")
					build_go_utils
					;;
			"cmd")
					if [ -z "$2" ]
					then
						build_cmds 1
					else
						build_a_cmd "$2"
						shift
					fi
					;;
			"cleancmd")
					build_cmds 2
					;;
			"kernel")
					build_kernel
					;;
			"cleankernel")
					clean_kernel
					;;
			*)
				echo "Invalid option <$1>"
				exit 1
				;;
		esac
		shift
	done
fi


