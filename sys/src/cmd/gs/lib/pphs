#!/bin/sh
# $Id: pphs,v 1.4 2004/08/04 00:55:46 giles Exp $
# Print the Primary Hint Stream from a linearized PDF file.  Usage:
#	pphs filename.pdf
# Output goes to stdout.

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

exec $GS_EXECUTABLE -q -dNODISPLAY -- pphs.ps "$@"
