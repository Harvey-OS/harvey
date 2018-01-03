#!/bin/bash -eu
#
# Copyright (C) 2018 Harvey OS
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#!/bin/sh

if [ $# -lt 1 ]; then
	echo "usage $0 host:port"
	echo "	example: $0 localhost:1234"
	exit 1
fi
remote=$1

(
	cat <<-EOF
		set arch i386:x86-64
		file sys/src/9/amd64/harvey.32bit
		target remote $remote
		set \$off = 0
	EOF
	for i in `seq 1 400`; do
		#echo 'p/a *(void(**)())((long*)$sp+$off)'
		echo 'info line **(void(**)())((long*)$sp+$off++)'
	done
	echo 'quit'
) | gdb
