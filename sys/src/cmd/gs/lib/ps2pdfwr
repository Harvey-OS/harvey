#!/bin/sh
# $Id: ps2pdfwr,v 1.10 2004/08/04 00:55:46 giles Exp $
# Convert PostScript to PDF without specifying CompatibilityLevel.

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

OPTIONS="-dSAFER"
while true
do
	case "$1" in
	-?*) OPTIONS="$OPTIONS $1" ;;
	*)  break ;;
	esac
	shift
done

if [ $# -lt 1 -o $# -gt 2 ]; then
	echo "Usage: `basename $0` [options...] (input.[e]ps|-) [output.pdf|-]" 1>&2
	exit 1
fi

infile="$1";

if [ $# -eq 1 ]
then
	case "${infile}" in
	  -)		outfile=- ;;
	  *.eps)	base=`basename "${infile}" .eps`; outfile="${base}.pdf" ;;
	  *.ps)		base=`basename "${infile}" .ps`; outfile="${base}.pdf" ;;
	  *)		base=`basename "${infile}"`; outfile="${base}.pdf" ;;
	esac
else
	outfile="$2"
fi

# We have to include the options twice because -I only takes effect if it
# appears before other options.
exec $GS_EXECUTABLE $OPTIONS -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite "-sOutputFile=$outfile" $OPTIONS -c .setpdfwrite -f "$infile"
