#!/bin/sh

# System V 3.2 lp interface for parallel, postscript printer
# with ghostscript 2.5.n.
#
# Thanks to Arne Ludwig (arne@rrzbu.hanse.de) for this script.
#

DEVICE=lbp8
GSHOME=/local/gs/2.5b2
FONT=/local
LIBDIR=/usr/spool/lp/admins/lp/interfaces
#EHANDLER=$LIBDIR/ehandler.ps

GS_LIB=$GSHOME:$FONT/fonts:$FONT/fonts/lw:$FONT/fonts/gs
export GS_LIB

copies=$4
shift 5
files="$*"

# serial line settings
# stty 19200 ixon ixoff 0<&1
# stty 1200 tabs cread opost onlcr ixon ixany ff1 cr2 nl0 0<&1

# Brother HL-4: switch to HP laserjet II+ emulation
# echo "\033\015H\c"

i=1
while [ $i -le $copies ]
do
	for file in $files
	do
		$GSHOME/gs \
			-sOUTPUTFILE=/tmp/psp$$.%02d \
			-sDEVICE=$DEVICE \
			$EHANDLER $file \
			< /dev/null >> /usr/tmp/ps_log 2>&1

		cat /tmp/psp$$.* 2>> /usr/tmp/ps_log
		rm -f /tmp/psp$$.*
	done
	i=`expr $i + 1`
done
exit 0
