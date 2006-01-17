#!/bin/sh
# $Id: pfbtopfa,v 1.6 2004/08/04 00:55:46 giles Exp $
# Convert .pfb fonts to .pfa format

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

if [ $# -eq 2 ] 
then
    outfile=$2
elif [ $# -eq 1 ]
then
    outfile=`basename "$1" \.pfb`.pfa
else
    echo "Usage: `basename $0` input.pfb [output.pfa]" 1>&2
    exit 1
fi

exec $GS_EXECUTABLE -q -dNODISPLAY -- pfbtopfa.ps "$1" "$outfile"
