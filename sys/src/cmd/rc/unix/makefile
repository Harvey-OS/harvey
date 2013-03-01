O=o
TARG=rc
COMMONOFILES=\
	code.$O\
	exec.$O\
	getflags.$O\
	glob.$O\
	here.$O\
	io.$O\
	lex.$O\
	pcmd.$O\
	pfnc.$O\
	simple.$O\
	subr.$O\
	trap.$O\
	tree.$O\
	var.$O\
	havefork.$O\

PLAN9OFILES=plan9.$O\

UNIXOFILES=unix.$O\

OFILES=$(COMMONOFILES) $(UNIXOFILES) y.tab.$O

HFILES=rc.h\
	x.tab.h\
	io.h\
	exec.h\
	fns.h\

YFILES=syn.y

BIN=/$objtype/bin

UPDATE=\
	mkfile\
	$HFILES\
	${COMMONOFILES:%.$O=%.c}\
	${UNIXOFILES:%.$O=%.c}\
	${PLAN9OFILES:%.$O=%.c}\
	$YFILES\
	${TARG:%=/386/bin/%}\

CC=cc
CFLAGS= -c -w -Iinclude
LD=cc

YFLAGS=-d -S

rc: $(OFILES)
	$(CC) -o rc $(OFILES)
x.tab.h: y.tab.h
	cmp -s x.tab.h y.tab.h || cp y.tab.h x.tab.h

clean:
	rm -f [$OS].out *.[$OS] [xy].tab.? y.debug $TARG

regress: $O.out
	cd test
	mk

unregress:
	for(test in test/*.test) rc $test >$test.out
