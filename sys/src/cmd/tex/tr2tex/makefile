# Use makefile.msc if you are compiling with MS-DOS

# add -Dtops20 to CFLAGS if you're running it under tops20
# and add -DANSI if you're using ANSI C
CFLAGS =
LINTFLAGS = -abchnpux
CFILES = tr2tex.c tr.c subs.c
HFILES = setups.h simil.h greek.h macros.h maths.h flip.h forbid.h
B =

default: tr2tex

tr2tex: tr2tex.o tr.o subs.o
	cc $(CFLAGS) tr2tex.o tr.o subs.o -o $(B)tr2tex

tr2tex.o: tr2tex.c setups.h
	cc $(CFLAGS) -c tr2tex.c
tr.o: tr.c setups.h
	cc $(CFLAGS) -c tr.c
subs.o: subs.c $(HFILES)
	cc $(CFLAGS) -c subs.c

lint:
	lint $(LINTFLAGS) $(CFILES) > lint.lst

clean:
	\rm -f *.o core *junk* tr2tex lint.lst

