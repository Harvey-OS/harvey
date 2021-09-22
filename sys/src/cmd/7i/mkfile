</$objtype/mkfile

TARG=7i
OFILES= 7i.$O\
	run.$O\
	mem.$O\
	syscall.$O\
	stats.$O\
	icache.$O\
	symbols.$O\
	cmd.$O\
	bpt.$O\
	float.$O\

HFILES=arm64.h\

BIN=/$objtype/bin

UPDATE=\
	mkfile\
	$HFILES\
	${OFILES:%.$O=%.c}\

</sys/src/cmd/mkone

acid:	
	$CC -a run.c > acid.def
