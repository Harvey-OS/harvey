</$objtype/mkfile

LIB=/$objtype/lib/libavl.a
OFILES=\
	avl.$O\

HFILES=/sys/include/avl.h

UPDATE=\
	mkfile\
	$HFILES\
	${OFILES:%.$O=%.c}\
	${LIB:/$objtype/%=/386/%}\

</sys/src/cmd/mksyslib
