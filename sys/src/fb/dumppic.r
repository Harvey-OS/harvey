#!/bin/rc
if(! ~ $#* 4){
	echo Usage: dumppic file xsize ysize chan >/fd/2
	exit usage
}
nchan=`{echo -n $4|wc -c|sed 's/[ 	]//g'}
echo 'TYPE=dump
WINDOW=0 0' $2 $3'
NCHAN='$nchan'
CHAN='$4'
COMMAND='$0 $"*'
'
cat $1
