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
	printf "\nFLAGS:\n"
	printf "  -g        \tCompile with debugs flags"
	printf "\n"

}

### MAIN SCRIPT ####
if [ -z "$1" ]
then
	show_help
	exit 1
else
	BUILD_DEBUG=
	while [ -n "$1" ]
	do
		case "$1" in
			"-g")
					BUILD_DEBUG="$CFLAGS_DEBUG"
					;;
			"all")
					build_libs 1
					build_klibs 1
					printf "\n\nALL COMPONENTS COMPILED\n\n"
					;;
			"cleanall")
					build_libs 2
					build_klibs 2
					printf "\n\nALL COMPONENTS CLEANED\n\n"
					;;
			"libs")
					if [ -z "$2" ]
					then
						build_libs 1
					else
						build_a_lib "$2" 0
						shift
					fi
					;;
			"klibs")
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
			*)
				echo "Invalid option <$1>"
				exit 1
				;;
		esac
		shift
	done
fi


