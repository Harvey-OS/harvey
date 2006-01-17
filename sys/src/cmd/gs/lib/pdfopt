#!/bin/sh
# $Id: pdfopt,v 1.7 2004/08/04 00:55:46 giles Exp $
# Convert PDF to "optimized" form.

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

OPTIONS="-dSAFER -dDELAYSAFER"
while true
do
	case "$1" in
	-*) OPTIONS="$OPTIONS $1" ;;
	*)  break ;;
	esac
	shift
done

if [ $# -ne 2 ]; then
	echo "Usage: `basename $0` input.pdf output.pdf" 1>&2
	exit 1
fi

exec $GS_EXECUTABLE -q -dNODISPLAY $OPTIONS -- pdfopt.ps "$1" "$2"
