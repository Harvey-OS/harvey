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

### FOR BUILD TESTS PROGRAMS, Provisional ###

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

##########################

compile_kernel()
{
	export HARVEY="$_BUILD_DIR"
	cd "$KRL_DIR"
	$HARVEY/util/build $KERNEL_CONF.json
	cd "$PATH_ORI" > /dev/null

}

build_kernel()
{
	compile_kernel
	if [ $? -ne 0 ]; then
		echo "SOMETHING WENT WRONG!"
	else
		echo "KERNEL COMPILED OK"
	fi
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

## Be sure amd64/lib is there!! ##

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

build_libs()
{
	export HARVEY="$_BUILD_DIR"
	cd "$SRC_DIR"
	$HARVEY/util/build libs.json
	cd "$PATH_ORI" > /dev/null
}


build_klibs()
{
	export HARVEY="$_BUILD_DIR"
	cd "$SRC_DIR"
	$HARVEY/util/build klibs.json
	cd "$PATH_ORI" > /dev/null

}

#### FOR BUILD CMD, Provisional!! ####

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
	if [ -d "${CMD_DIR}/$1" ]
	then
		cd ${CMD_DIR}/$1
	else
		if [ $1 = ipconfig ]
		then
			cd ${CMD_DIR}/ip/$1
		else
			cd ${CMD_DIR}
		fi
	fi
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

build_cmds_go()
{
	export HARVEY="$_BUILD_DIR"
	cd "$CMD_DIR"
	$HARVEY/util/build cmds.json
	cd "$PATH_ORI" > /dev/null
}
#############################

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
					build_libs
					build_klibs
					build_cmds 1 ### Provisional
					build_kernel
					printf "\n\nALL COMPONENTS COMPILED\n\n"
					;;
			"cleanall")
					printf "\n\nALL COMPONENTS ARE CLEANED AT BUILD TIME\n\n"
					;;
			"libs")
					check_lib_dir
					if [ -z "$2" ]
					then
						build_libs
					fi
					;;
			"klibs")
					check_lib_dir
					if [ -z "$2" ]
					then
						build_klibs
					fi
					;;
			"cleanklibs")
					printf "\n\nALL COMPONENTS ARE CLEANED AT BUILD TIME\n\n"
					;;
			"cleanlibs")
					printf "\n\nALL COMPONENTS ARE CLEANED AT BUILD TIME\n\n"
					;;
			"utils")
					build_go_utils
					;;
			"cmd")
					if [ -z "$2" ]
					then
						build_cmds_go
					else
						printf "\n\nALL COMPONENTS ARE CLEANED AT BUILD TIME\n\n"
					fi
					;;
			"cleancmd")
					printf "\n\nALL COMPONENTS ARE CLEANED AT BUILD TIME\n\n"
					;;
			"kernel")
					build_kernel
					;;
			"cleankernel")
					printf "\n\nALL COMPONENTS ARE CLEANED AT BUILD TIME\n\n"
					;;
			*)
				echo "Invalid option <$1>"
				exit 1
				;;
		esac
		shift
	done
fi


