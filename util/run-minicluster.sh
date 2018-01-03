#!/bin/sh -eu
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

if [ $# -lt 1 ]; then
	echo usage: $0 ethername
	exit 1
fi
ethif=$1

trap : 2

$HARVEY/util/ufs -root=$HARVEY &
pids=$!

#
#	install macports
#	port install tftp-hpa
#	port install dhcp
#	port install gcc49
#	port install x86_64-elf-gcc
#	port install x86_64-elf-binutils
#

sudo ifconfig $ethif 10.0.0.10/24
sudo tftpd -vvvv -L --address 10.0.0.10 -s $HARVEY/cfg/pxe/tftpboot &
pids="$pids $!"
sudo dhcpd -d -cf $HARVEY/util/minicluster-dhcpd.conf -lf $HARVEY/util/minicluster-dhcpd.leases $ethif
sudo kill $pids
wait
