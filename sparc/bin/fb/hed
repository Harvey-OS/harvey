#!/bin/rc
# hed -- run sed on the header of a picture file
if(~ $#* 3 && ~ $1 -n){
	nflag=-n
	shift
}
if(! ~ $#* 2){
	echo 'Usage: hed [-n] script file' >[2] /dev/null
	exit 1
}
sed '/^$/q' $2|sed $nflag -e $1
{dd 'bs='`{sed '/^$/q' $2|wc -c} 'skip=1' >[2] /dev/null;cat} <$2
