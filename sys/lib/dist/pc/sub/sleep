#!/bin/rc

if(! ~ $#* 1) {
	echo 'usage: sleep n' >[1=2]
	exit usage
}

syscall sleep $1^000 >/dev/null >[2]/dev/null
