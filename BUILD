#!/bin/bash
set -e

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

#Search for config:
export HARVEY="$(git rev-parse --show-toplevel)"
. "${HARVEY}/BUILD.conf"

build_kernel()
{ (
	cd "$KRL_DIR"
	build "$KERNEL_CONF.json"
) }

build_go_utils()
{ (
	export GOBIN="${UTIL_DIR}"
	export GOPATH="${UTIL_DIR}/third_party:${UTIL_DIR}"
	go get -d harvey/cmd/... # should really vendor these bits
	go install github.com/rminnich/go9p/ufs harvey/cmd/...
) }

build_libs()
{ (
	[ -d "$HARVEY/amd64/lib/" ] && git clean -d -x -f "$HARVEY/amd64/lib/"*
	cd "$SRC_DIR"
	build libs.json
) }

build_klibs()
{ (
	cd "$SRC_DIR"
	build klibs.json
) }

build_cmds()
{ (
	git clean -d -x -f "$HARVEY/amd64/bin/"*
	cd "$CMD_DIR"
	build cmds.json
	build kcmds.json
) }

build_regress()
{ (
	git clean -d -x -f "$HARVEY/amd64/bin/regress"
	cd "$SRC_DIR"/regress
	build regress.json
) }

show_help()
{
cat <<end

BUILD script for Harvey

OPTIONS:
  all                Build all components
  cleanall           Clean all components
  libs               Build the libraries
  libs <libname>     Build the library <libname>
  cleanlibs          Clean the libraries
  klibs              Build the Klibraries
  klibs <libname>    Build the Klibrary <libname>
  cleanklibs         Clean the Klibraries
  utils              Build go utils
  cmd                Build all cmds
  cmd <cmdname>      Build cmd named <cmdname>
  regress            Build all regression tests
  cleancmd           Clean the cmds
  kernel             Build kernel
  cleankernel        Clean kernel

FLAGS:
  -g                 Compile with debugs flags
  -t <test>          Compile <test> app and package it with the kernel

end
}

### MAIN SCRIPT ####
if [ -z "$1" ]
then
	show_help
	exit 1
else
	# We need our binary dir
	mkdir -p "$BIN_DIR"

	#BUILD_DEBUG=
	#Until we have a stable kernel, debug mode is the default.
	export BUILD_DEBUG="$CFLAGS_DEBUG"
	TEST_APP=
	while [ -n "$1" ]
	do
		case "$1" in
			"-g")
					export BUILD_DEBUG="$CFLAGS_DEBUG"
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
					build_regress
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
			"regress")
					build_regress
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
