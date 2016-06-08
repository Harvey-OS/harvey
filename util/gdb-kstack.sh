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
