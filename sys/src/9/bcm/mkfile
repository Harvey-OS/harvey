CONF=pi
CONFLIST=pi picpu pifat
EXTRACOPIES=

loadaddr=0x80008000

objtype=arm
</$objtype/mkfile
p=9

DEVS=`{rc ../port/mkdevlist $CONF}

PORT=\
	alarm.$O\
	alloc.$O\
	allocb.$O\
	auth.$O\
	cache.$O\
	chan.$O\
	dev.$O\
	edf.$O\
	fault.$O\
	mul64fract.$O\
	page.$O\
	parse.$O\
	pgrp.$O\
	portclock.$O\
	print.$O\
	proc.$O\
	qio.$O\
	qlock.$O\
	rdb.$O\
	rebootcmd.$O\
	segment.$O\
	swap.$O\
	syscallfmt.$O\
	sysfile.$O\
	sysproc.$O\
	taslock.$O\
	tod.$O\
	xalloc.$O\

OBJ=\
	l.$O\
	lexception.$O\
	lproc.$O\
	arch.$O\
	clock.$O\
	fpi.$O\
	fpiarm.$O\
	fpimem.$O\
	main.$O\
	mmu.$O\
	random.$O\
	syscall.$O\
	trap.$O\
	$CONF.root.$O\
	$CONF.rootc.$O\
	$DEVS\
	$PORT\

# HFILES=

LIB=\
	/$objtype/lib/libmemlayer.a\
	/$objtype/lib/libmemdraw.a\
	/$objtype/lib/libdraw.a\
	/$objtype/lib/libip.a\
	/$objtype/lib/libsec.a\
	/$objtype/lib/libmp.a\
	/$objtype/lib/libc.a\

9:V: $p$CONF s$p$CONF

$p$CONF:DQ:	$CONF.c $OBJ $LIB mkfile
	$CC $CFLAGS '-DKERNDATE='`{date -n} $CONF.c
	echo '# linking raw kernel'	# H6: no headers, data segment aligned
	$LD -l -o $target -H6 -R4096 -T$loadaddr $OBJ $CONF.$O $LIB

s$p$CONF:DQ:	$CONF.$O $OBJ $LIB
	echo '# linking kernel with symbols'
	$LD -l -o $target -R4096 -T$loadaddr $OBJ $CONF.$O $LIB
	size $target

$p$CONF.gz:D:	$p$CONF
	gzip -9 <$p$CONF >$target

$OBJ: $HFILES

install:V: /$objtype/$p$CONF

/$objtype/$p$CONF:D: $p$CONF s$p$CONF
	cp -x $p$CONF s$p$CONF /$objtype/ &
	for(i in $EXTRACOPIES)
		{ 9fs $i && cp $p$CONF s$p$CONF /n/$i/$objtype && echo -n $i... & }
	wait
	echo
	touch $target

<../boot/bootmkfile
<../port/portmkfile
<|../port/mkbootrules $CONF

arch.$O clock.$O fpiarm.$O main.$O mmu.$O screen.$O syscall.$O trap.$O: \
	/$objtype/include/ureg.h

archbcm.$O devether.$0: etherif.h ../port/netif.h
archbcm.$O: ../port/flashif.h
fpi.$O fpiarm.$O fpimem.$O: ../port/fpi.h
l.$O lexception.$O lproc.$O mmu.$O: arm.s mem.h
main.$O: errstr.h init.h reboot.h
devmouse.$O mouse.$O screen.$O: screen.h
devusb.$O: ../port/usb.h
usbehci.$O usbohci.$O usbuhci.$O: ../port/usb.h usbehci.h uncached.h

init.h:D:	../port/initcode.c init9.s
	$CC ../port/initcode.c
	$AS init9.s
	$LD -l -R1 -s -o init.out init9.$O initcode.$O /$objtype/lib/libc.a
	{echo 'uchar initcode[]={'
	 xd -1x <init.out |
		sed -e 's/^[0-9a-f]+ //' -e 's/ ([0-9a-f][0-9a-f])/0x\1,/g'
	 echo '};'} > init.h

reboot.h:D:	rebootcode.s arm.s arm.h mem.h
	$AS rebootcode.s
	# -lc is only for memmove.  -T arg is PADDR(REBOOTADDR)
	$LD -l -s -T0x3400 -R4 -o reboot.out rebootcode.$O -lc
	{echo 'uchar rebootcode[]={'
	 xd -1x reboot.out |
		sed -e '1,2d' -e 's/^[0-9a-f]+ //' -e 's/ ([0-9a-f][0-9a-f])/0x\1,/g'
	 echo '};'} > reboot.h
