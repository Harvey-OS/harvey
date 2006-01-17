#!/bin/sh
# $Id: eps2eps,v 1.6 2004/08/04 00:55:46 giles Exp $
# "Distill" Encapsulated PostScript.

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

OPTIONS="-dDEVICEWIDTH=250000 -dDEVICEHEIGHT=250000"
while true
do
	case "$1" in
	-*) OPTIONS="$OPTIONS $1" ;;
	*)  break ;;
	esac
	shift
done

if [ $# -ne 2 ]; then
	echo "Usage: `basename $0` ...switches... input.eps output.eps" 1>&2
	exit 1
fi

exec $GS_EXECUTABLE -q -sDEVICE=epswrite "-sOutputFile=$2" -dNOPAUSE -dBATCH -dSAFER $OPTIONS "$1"
