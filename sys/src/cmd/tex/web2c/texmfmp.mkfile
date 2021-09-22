OTARG=$TARG
</$objtype/mkfile
CPUS=mips 386

BIN=/sys/lib/texmf/bin/$objtype
TARG=_$OTARG
OFILES=\
	`{ls $OTARG[0-9].c | sed 's/\.c/.'$O'/'}\
	$OTARG'ini'.$O\
	texmfmp.$O\
	$EXTRA_OFILES\

LIB=../lib/lib.a$O ../../kpathsea/libkpathsea.a$O

</sys/src/cmd/mkone

CC=pcc
CFLAGS=-I../.. -I.. -I. -D_POSIX_SOURCE -D_BSD_EXTENSION -B -c

texmfmp.$O:	../texmfmp.c
	$CC '-DTEXMFMP_H="'$OTARG'd.h"' $CFLAGS ../texmfmp.c

CPUENDIAN=`{/sys/lib/texmf/bin/$cputype/endian}

#
# To create .c and .h and .pool files from the .web file
#
# bind -a /sys/lib/texmf/bin/$objtype /bin
# bind -a /sys/lib/texmf/bin/rc /bin
# /bin/tangle basename.web basename.ch
# ../web2c/convert basename
# for (i in *.c)
# 	echo ',s/(\(|,) *(nameoffile|addressof)/\1 (char*) \2/g
# 	wq' | ed $i 
#
