#!/bin/rc
if(! ~ $#* 5){
	echo Usage: $0 red grn blu xsize ysize >/fd/2
	exit usage
}
echo 'TYPE=pico
WINDOW=0 0 '$4' '$5'
NCHAN=3
CHAN=rgb
COMMAND='$0' $*
'
cat $1 $2 $3
