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
. ${_BUILD_DIR}/BUILD.conf


build_go_utils()
{
	cd "${UTIL_DIR}"
	for i in `ls *.go`
	do
		go build $i
		if [ $? -ne 0 ]
		then
			printf "\nERROR compiling $i \n"
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

#ARGS:
#$1 -> Lib name
build_a_lib()
{
	cd "${SRC_DIR}/$1"
	CFLAGS_EXTRA=
	$1
	for j in $BUILD_IN
	do
		echo "$CC $CFLAGS $CFLAGS_EXTRA -c $j"
		$CC $CFLAGS $CFLAGS_EXTRA -c $j
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
	cd - > /dev/null
}

#ARGS:
#$1 -> Lib name
clean_a_lib()
{
	cd "${SRC_DIR}/$1"
	$1
	printf "Cleaning $1 "
	$CLEAN_COM
	if [ $? -eq 0 ]
	then
		printf "OK\n"
	else
		printf "ERROR\n"
	fi
	cd - > /dev/null
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
			build_a_lib $i
		else
			clean_a_lib $i
		fi
	done
	
	if [ $1 -eq 1 ]
	then
		echo "ALL LIBS COMPILED OK"
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
	printf "  utils     \tBuild go utils\n"
	printf "\n"

}

### MAIN SCRIPT ####
if [ -z "$1" ]
then
	show_help
	exit 1
else
	case "$1" in
		"all")
				build_libs 1
				printf "\n\nALL COMPONENTS COMPILED\n\n"
				;;
		"cleanall")
				build_libs 2
				printf "\n\nALL COMPONENTS CLEANED\n\n"
				;;
		"libs")
				if [ -z "$2" ]
				then
					build_libs 1
				else
					build_a_lib "$2"
				fi
			;;
		"cleanlibs")
				build_libs 2
			;;
		"utils")
				build_go_utils
			;;
		*)
			echo "Invalid option <$1>"
			;;
	esac
fi


