#!/bin/sh
# $Id: catmake,v 1.3 2002/02/21 22:24:51 giles Exp $
# Expand 'includes' in makefiles.  Usage:
#	catmake orig.mak > makefile

awk '
/^include / {
	print "# INCLUDE OF", $2
	while (getline x <$2 > 0)
		if(x !~ /^#/)
			print x
	next
}
{print}
' $1
