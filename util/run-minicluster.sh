#!/bin/bash

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

