#!/bin/rc
fn sigexit sigint sigquit sighup{
	rm -f /tmp/q[012].$pid
}
switch($#*){
case 0;	cat >/tmp/q0.$pid
	inf=/tmp/q0.$pid
case 1;	inf=$1
case *;	echo Usage: $0 '[picture]' >/fd/2
	exit usage
}
fb/mcut $inf >/tmp/q1.$pid
fb/improve /tmp/q1.$pid $inf >/tmp/q2.$pid
fb/3to1 /tmp/q2.$pid $inf
