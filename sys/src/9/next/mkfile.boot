CONF=rob
CONFLIST=rob lfs

objtype=68020
</$objtype/mkfile

NPROC=4
BUILTINS=
DEV=`rc ../port/mkdevlist $CONF`
STREAM=`rc ../port/mkstreamlist $CONF`
MISC=`rc ../port/mkmisclist $CONF`

OBJ=\
	vec.2\
	l.2\
	$CONF.2\
	alarm.2\
	queue.2\
	chan.2\
	clock.2\
	dev.2\
	fault.2\
	fault68040.2\
	lock.2\
	main.2\
	malloc.2\
	mmu.2\
	page.2\
	pgrp.2\
	print.2\
	proc.2\
	qlock.2\
	screen.2\
	sysfile.2\
	sysproc.2\
	trap.2\
	$DEV\
	stream.2\
	$MISC $STREAM\
	fcall.2\
	tcpinput.2 \
	tcpif.2 \
	tcptimer.2 \
	tcpoutput.2

a:V:	install

<../port/mkfile
<../lfs/mkfile

9.$CONF:	$OBJ /68020/lib/libgnot.a
	2l -o $target -l -H4 -T0x04000000 -R0x2000 $prereq -lgnot -lg -lc

install:V: 9.$CONF
	mv 9.$CONF 9boot

clean:V:
	rm -f 9.* *.2 devtab.h boot.h

%.2:	%.s
	2a $stem.s

%.2:	%.c
	2c -I. -w $stem.c

%.2:	u.h lib.h mem.h dat.h fns.h io.h errno.h ../port/portdat.h ../port/portfns.h

main.2:		init.h
devroot.2:	boot.h errstr.h
dev%.2:		devtab.h /68020/include/fcall.h
stream.2:	devtab.h
$CONF.2:	devtab.h
cache.2:	cfs.h

$CONF.c:	$CONF ../port/mkdevc
	../port/mkdevc $CONF > $CONF.c

devtab.h:	../port/mkdevh $CONFLIST
	rc ../port/mkdevh $CONFLIST > devtab.h

errstr.h:	../port/mkerrstr errno.h
	rc ../port/mkerrstr > errstr.h

screen.2:	screen.c keys.h /68020/include/gnot.h
	2c -I/sys/src/libgnot/68020 screen.c

devbit.2:	devbit.c /68020/include/gnot.h
	2c -I/sys/src/libgnot/68020 devbit.c

init.h:	initcode /sys/src/libc/680209sys/sys.h
	2a initcode
	2l -l -o init.out -H2 -T0x2020 -R0 initcode.2
	{echo 'ulong initcode[]={'
	 xd -r init.out |
		sed '/\*/,$d' |
		sed 's/.*  //; s/ /\n/g' |
		sed 's/.*/	0x0&,/'
	 echo '};'} > init.h
boot.h:	bboot.c /sys/src/libc/680209sys/sys.h /68020/lib/libc.a
	2c -o boot.2 -w bboot.c 
	2l -o boot.out -s boot.2 -lc
	{echo 'ulong bootcode[]={'
	 xd boot.out |
		sed 's/.*  //; s/ /\n/g' |
		sed 's/.*/	0x0&,/'
	 echo '};'} > boot.h
	2l -o boot.out boot.2 -lc

cfs.h:	/68020/bin/cfs
	cp /68020/bin/cfs cfs.out
	strip cfs.out
	{echo 'ulong cfscode[]={'
	 xd cfs.out |
		sed 's/.*  //; s/ /\n/g' |
		sed 's/.*/	0x0&,/'
	 echo '};'
	 echo 'long cfslen = sizeof cfscode;'} > cfs.h

backup:
	ar vu arch

nm:	$OBJ
	2l -a -o 9 -l -T0x80002020 $prereq -lgnot -lc > nm
