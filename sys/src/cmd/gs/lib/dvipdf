#!/bin/sh
# $Id: dvipdf,v 1.5 2004/08/04 00:55:46 giles Exp $
# Convert DVI to PDF.
#
# Please contact Andrew Ford <A.Ford@ford-mason.co.uk> with any questions
# about this file.
#
# Based on ps2pdf

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs


OPTIONS=""
while true
do
	case "$1" in
	-*) OPTIONS="$OPTIONS $1" ;;
	*)  break ;;
	esac
	shift
done

if [ $# -lt 1 -o $# -gt 2 ]; then
	echo "Usage: `basename $0` [options...] input.dvi [output.pdf]" 1>&2
	exit 1
fi

infile=$1;

if [ $# -eq 1 ]
then
	case "${infile}" in
	  *.dvi)	base=`basename "${infile}" .dvi` ;;
	  *)		base=`basename "${infile}"` ;;
	esac
	outfile="${base}".pdf
else
	outfile=$2
fi

# We have to include the options twice because -I only takes effect if it
# appears before other options.
exec dvips -q -f "$infile" | $GS_EXECUTABLE $OPTIONS -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOutputFile="$outfile" $OPTIONS -c .setpdfwrite -
