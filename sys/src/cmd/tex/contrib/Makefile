# Makefile for MkTeX... programs which emulate the shell scripts.

srcdir = .

EXE = 
O = .o
LN_S = ln -s
RM = rm -f

mktex_main = mktex$(EXE)
mktex_links = mktexpk$(EXE) mktexlsr$(EXE) mktexnam$(EXE) \
		mktexupd$(EXE) mktexdir$(EXE) mktexmf$(EXE) mktextfm$(EXE)
mktex_progs = $(mktex_main) $(mktex_links)

DEFS = -D_IMPORT $(XDEFS)
CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../kpathsea $(XCPPFLAGS)
CFLAGS = -Wall -g -O2 $(XCFLAGS)
CC = gcc

# Kpathsea needs this for compiling, programs need it for linking.
LIBTOOL = $(srcdir)/../klibtool

# You can change [X]CPPFLAGS, [X]CFLAGS, or [X]DEFS, but
# please don't change ALL_CPPFLAGS or ALL_CFLAGS.
ALL_CPPFLAGS = $(DEFS) -I. -I$(srcdir) $(CPPFLAGS)
ALL_CFLAGS = $(ALL_CPPFLAGS) $(CFLAGS) -c
compile = $(CC) $(ALL_CFLAGS)

LDFLAGS = $(XLDFLAGS)
LOADLIBES = ../kpathsea/libkpathsea.la -lm $(XLOADLIBES)
link_command = $(CC) -o $@ $(LDFLAGS) 
kpathsea_link = $(LIBTOOL) link $(link_command)

.c.o:
	$(compile) $<


default all do-mktex: mktexprogs

mktexprogs: $(mktex_progs) makempx$(EXE) dvihp$(EXE)

$(mktex_main): mktex$O stackenv$O variables$O regex$O $(kpathsea)
	$(kpathsea_link) mktex$O stackenv$O variables$O regex$O $(LOADLIBES)

$(mktex_links):
	$(RM) $@
	$(LN_S) $(mktex_main) $@
	touch $@

makempx: makempx$O stackenv$O $(kpathsea)
	$(kpathsea_link) makempx$O stackenv$O $(LOADLIBES)

dvihp: dvihp$O $(kpathsea)
	$(kpathsea_link) dvihp$O stackenv$O $(LOADLIBES)

mktex$O makempx$O dvihp$O stackenv$O variables$O: stackenv.h
mktex$O: variables.h
mktex$O variables$O regex$O: regex.h

clean distclean:
	-$(RM) *$O $(mktex_progs) makempx$(EXE) dvihp$(EXE) *.exe

check:

#Local Variables:
#mode: Makefile
#End:
