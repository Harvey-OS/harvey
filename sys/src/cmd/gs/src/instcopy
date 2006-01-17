#!/bin/sh
# $Id: instcopy,v 1.3 2002/02/21 22:24:53 giles Exp $
#
# Implement a uniform 'install' syntax independent of which of the two
# "standard" install programs is installed.  Based on ideas in, but not
# copied from, the GNU fileutils install-sh script.  Usage:
#	instcopy -c [-m <mode>] <srcfile> (<dstdir>|<dstfile>)

doit=""
# Uncomment the following line for testing
#doit="echo "

mode=""

    while true; do
	case "$1" in
	    -c) ;;
	    -m) mode=$2; shift ;;
	    *) break ;;
        esac
    shift; done

src=$1
dst=$2

    if [ $# = 2 -a -f $src ]; then true; else
	echo "Usage: instcopy -c [-m <mode>] <srcfile> (<dstdir>|<dstfile>)"
	exit 1
    fi

if [ -d $dst ]; then
    dstdir=`echo $dst | sed -e 's,/$,,'`
    dst="$dstdir"/`basename $src`
else
    dstdir=`echo $dst | sed -e 's,/[^/]*$,,'`
fi
dsttmp=$dstdir/#inst.$$#

$doit cp $src $dsttmp &&
$doit trap "rm -f $dsttmp" 0 &&
if [ x"$mode" != x ]; then $doit chmod $mode $dsttmp; else true; fi &&
$doit rm -f $dst &&
$doit mv $dsttmp $dst &&
exit 0
