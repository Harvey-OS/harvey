#!/bin/sh -f
# $Id: pv.sh,v 1.4 2004/08/04 00:55:46 giles Exp $
#
# pv - preview a specified page of a dvi file in a Ghostscript window
# usage: pv page file
#
# pv converts the given page to PostScript and displays it
# in a Ghostscript window.
#
if [ $# -lt 2 ] ;then
  echo usage: $0 'page_number file_name[.dvi]'
  exit 1
fi
#
# The following line used to appear here:
#
#RESOLUTION=100
#
# But according to Peter Dyballa
# <pete@lovelace.informatik.uni-frankfurt.de>, "Modern versions of dvips are
# taught to read configuration files which tell them the paths to PK, TFM,
# VF and other files for example PostScript font programmes. These files
# tell #dvips too which default resolution is used and therefore which
# series of PK files (based on 300 DPI or 400 DPI or 600 DPI or even more)
# are held on the system."  So we have deleted this line, and also removed
# the -D switch from the call of dvips below.
#

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

TEMPDIR=.
PAGE=$1
shift
FILE=$1
shift
trap "rm -rf $TEMPDIR/$FILE.$$.pv" 0 1 2 15
#dvips -D$RESOLUTION -p $PAGE -n 1 $FILE $* -o $FILE.$$.pv
dvips -p $PAGE -n 1 $FILE $* -o $FILE.$$.pv
$GS_EXECUTABLE $FILE.$$.pv
exit 0
