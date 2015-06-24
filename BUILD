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

build_cmds()
{
	export HARVEY="$_BUILD_DIR"
	cd "$CMD_DIR"
	$HARVEY/util/build cmds.json
	$HARVEY/util/build kcmds.json
	cd "$PATH_ORI" > /dev/null
}

show_help()
{
	printf "\n\nBUILD script for Harvey\n\n"
	printf "OPTIONS:\n"
	printf "  all        \tBuild all components\n"
	printf "  cleanall     \tClean all components\n"
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
	# We need our binary dir
	mkdir -p $BIN_DIR

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
					build_go_utils
					build_libs
					build_klibs
					build_cmds
					build_kernel
					printf "\n\nALL COMPONENTS COMPILED\n\n"
					;;
			"libs")
					build_klibs
					build_libs
					;;
			"klibs")
					build_klibs
					;;
			"utils")
					build_go_utils
					;;
			"cmd")
					build_cmds
					;;
			"cleanall"|"cleancmd"|"cleankernel"|"cleanklibs"|"cleanlibs")
					printf "\n\nALL COMPONENTS ARE CLEANED AT BUILD TIME\n\n"
					;;
			"kernel")
					build_kernel
					;;
			*)
				echo "Invalid option <$1>"
				exit 1
				;;
		esac
		shift
	done
fi


