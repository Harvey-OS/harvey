#!/bin/sh
# $Id: gslp,v 1.4 2004/08/04 00:55:46 giles Exp $

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

exec $GS_EXECUTABLE -q -sDEVICE=epson -r180 -dNOPAUSE -sPROGNAME=$0 -- gslp.ps --heading-center "`date`" "$@"
