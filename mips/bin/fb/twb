#!/bin/rc
fn sigexit sigint sigquit sighup{
	rm -f /tmp/twb.$pid
}
troffflags=-mtwb
dpicflags=()
files=()
for(i) switch($i){
case -[TFds]* -I;	dpicflags=($dpicflags $i)
case -w*;	dpicflags=($dpicflags `{echo $i|sed 's/,/ /g'})
case -[mo]*;	troffflags=($troffflags $i)
case *;		files=($files $i)
}
cat $files>/tmp/twb.$pid
grap='' pic='' eqn='' tbl=''
for(i in `{grep '^\.(G1|PS|EQ|TS)' /tmp/twb.$pid | sed 's/(...).*/\1/' | sort -u})
	switch($i){
	case .G1;	grap='grap |' pic='pic -D |'
	case .PS;	pic='pic -D |'
	case .EQ;	eqn='eqn -p5 -s30 -T202 |'
	case .TS;	tbl='tbl |'
	}
eval $grap $pic $tbl $eqn troff -T202 $troffflags </tmp/twb.$pid | fb/dpic $dpicflags
