</$objtype/mkfile

TARG=`{ls *.[cy] | sed '/\.tab\.c$/d;s/..$//'}
HFILES=/$objtype/include/u.h /sys/include/libc.h /sys/include/bio.h
BIN=/$objtype/bin
PROGS=${TARG:%=$O.%}
LDFLAGS=
YFLAGS=-d

NOTSYS=sml|dup|.+\..+
BUGGERED=unix
OUTOFDATE=old

NOMK=$NOTSYS|$BUGGERED|$OUTOFDATE

cpuobjtype=`{sed -n 's/^O=//p' /$cputype/mkfile}
DIRS=`{ls -l | sed '/^d/!d; s/.* //; /^('$NOMK')$/d'}
APEDIRS=awk bzip2 compress cvs eqn grap gs lp pic postscript spin \
	tex troff

none:VQ:
	echo usage: mk cmds, dirs, all, install, installall, '$O'.cmd, cmd.install, or cmd.installall

cmds:V:	$PROGS

ape:V: $APE
	for(i in $APEDIRS) @{
		cd $i
		echo mk $i
		mk $MKFLAGS all
	}

dirs:V:
	for(i in cc $DIRS) @{
		cd $i
		echo mk $i
		mk $MKFLAGS all
	}

all:V:	$PROGS dirs

^([$OS])\.(.*):R:	\2.\1
	$stem1^l $LDFLAGS -o $target $stem2.$stem1

.*\.[$OS]:R:	$HFILES

(.*)\.([$OS])'$':R:	\1.c
	$stem2^c $CFLAGS $stem1.c

&:n:	$O.&
	mv $O.$stem $stem

%.tab.h %.tab.c:	%.y
	$YACC $YFLAGS -s $stem $prereq

%.install:V: $BIN/%

$cpuobjtype._cp:	/bin/cp
	cp $prereq $target

%.safeinstall:	$O.% $cpuobjtype._cp
	test -e $BIN/$stem && mv $BIN/$stem $BIN/_$stem
	$cpuobjtype._cp $O.$stem $BIN/$stem

%.safeinstallall:
	for(objtype in $CPUS)
		mk $stem.safeinstall
	mk $stem.clean

$BIN/%:	$O.% $cpuobjtype._cp
	$cpuobjtype._cp $O.$stem $BIN/$stem

%.directories:V:
	for(i in $DIRS) @{
		cd $i
		echo mk $i
		mk $MKFLAGS $stem
	}

clean:V:	cleanfiles clean.directories

nuke:V:		cleanfiles nuke.directories

directories:V:	install.directories

cleanfiles:V:
	rm -f [$OS].out *.[$OS] y.tab.? y.debug y.output [$OS].$TARG [$OS].units.tab $TARG bc.c bc.tab.h units.tab.h units.c [$OS]._cp

%.clean:V:
	rm -f [$OS].$stem $stem.[$OS]

install:V:
	test -e $cpuobjtype._cp || cp /bin/cp $cpuobjtype._cp
	mk $MKFLAGS $TARG.install
	mk $MKFLAGS directories

installall:V:
	for(objtype in $CPUS)
		mk $MKFLAGS install

%.installall:	%.c
	test -e $cpuobjtype._cp || cp /bin/cp $cpuobjtype._cp
	for (objtype in $CPUS) {
		rfork e
		mk $stem.install &
	}
	wait
	rm -f $stem.[$OS] y.tab.? $stem.tab.? y.debug y.output [$OS].$stem bc.c bc.tab.h units.c

%.acid: %.$O $HFILES
	$CC $CFLAGS -a $stem.c >$target

(bc|units).c:R:	\1.tab.c
	mv $stem1.tab.c $stem1.c

$BIN/init:	$O.init
	cp $prereq /$objtype/init

$O.cj:	cj.$O
	$LD $LDFLAGS -o $O.cj cj.$O /$objtype/lib/libjpg.a

%.update:V:
	update $stem.c /386/bin/$stem

compilers:V:
	for(i in ?c)
		if(! ~ $i cc rc) @{
			cd $i
			mk clean
			objtype=$cputype mk install
			mk clean
		}
	for(i in ?c)
		if(! ~ $i cc rc) @{
			cd $i
			mk clean
			mk installall
			mk clean
		}
