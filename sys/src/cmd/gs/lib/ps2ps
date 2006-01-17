#!/bin/sh
# $Id: ps2ps,v 1.7 2004/08/04 00:55:46 giles Exp $
# "Distill" PostScript.

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

OPTIONS="-dSAFER"
while true
do
	case "$1" in
	-*) OPTIONS="$OPTIONS $1" ;;
	*)  break ;;
	esac
	shift
done

if [ $# -ne 2 ]; then
	echo "Usage: `basename $0` [options] input.ps output.ps" 1>&2
	echo "  e.g. `basename $0` -sPAPERSIZE=a4 input.ps output.ps" 1>&2
	exit 1
fi

exec $GS_EXECUTABLE -q -sDEVICE=pswrite "-sOutputFile=$2" -dNOPAUSE -dBATCH $OPTIONS "$1"
