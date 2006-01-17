#!/bin/sh
# $Id: ps2ps2,v 1.1 2005/07/20 06:00:54 igor Exp $
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

exec $GS_EXECUTABLE -q -sDEVICE=ps2write "-sOutputFile=$2" -dNOPAUSE -dBATCH $OPTIONS "$1"
