#!/bin/sh
# $Id: pf2afm,v 1.5 2004/08/04 00:55:46 giles Exp $
# Make an AFM file from PFB / PFA and (optionally) PFM files.  Usage:
#	pf2afm fontfilename
# Output goes to fontfilename.afm, which must not already exist.
# See pf2afm.ps for more details.

# This definition is changed on install to match the
# executable name set in the makefile
GS_EXECUTABLE=gs

exec $GS_EXECUTABLE -q -dNODISPLAY -dSAFER -dDELAYSAFER  -- pf2afm.ps "$@"
