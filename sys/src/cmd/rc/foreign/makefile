# makefile for rc on unix
CFLAGS=-DUnix
O=o
TARG=rc
COMMONOFILES=\
	code.$O\
	exec.$O\
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
	havefork.$O

PLAN9OFILES=plan9.$O

UNIXOFILES=unix.$O\
	rune.$O\
	utfrune.$O\
	utfutf.$O

OFILES=$(COMMONOFILES) $(UNIXOFILES) y.tab.$O

HFILES=rc.h\
	x.tab.h\
	io.h\
	exec.h\
	fns.h

YFILES=syn.y

default:
	if test ! -d build; then mkdir build; fi
	if cp ../* * build 2>/dev/null; then echo copied, cd build; fi
	cd build && make $(TARG)

$(TARG): $(OFILES)
	$(CC) -o $(TARG) $(OFILES)
$(OFILES): $(HFILES)

x.tab.h: y.tab.h
	cmp -s x.tab.h y.tab.h || cp y.tab.h x.tab.h
y.tab.h y.tab.c: $(YFILES)
	$(YACC) -d $(YFILES)
clean:
	rm -f *.out *.o [xy].tab.? y.debug $(TARG)
