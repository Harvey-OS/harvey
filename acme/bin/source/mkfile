</$objtype/mkfile

TARG=\
	mkwnew\
	spout\

OFILES=
HFILES=
LIB=

DIRS=win

BIN=../$objtype

</sys/src/cmd/mkmany

all:V:		all.dirs
install:V:	install.dirs
clean:V:	clean.dirs
nuke:V:		nuke.dirs

%.dirs:VQ:
	for (i in $DIRS) @{
		echo mk $i
		cd $i
		mk $stem
	}
