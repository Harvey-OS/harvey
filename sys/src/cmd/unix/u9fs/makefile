#
# The goal is to keep as much per-system stuff autodetected in plan9.h
# as possible.  Still, sometimes you can't help it.  Look for your system.
#

# SGI
#
# To correctly handle 64-bit files and offsets, add -64 to CFLAGS and LDFLAGS
# On Irix 5.X, add -DIRIX5X to hack around their own #include problems (see plan9.h).
#
# SunOS
#
# SunOS 5.5.1 does not provide inttypes.h; add -lsunos to CFLAGS and
# change CC and LD to gcc.  Add -lsocket, -lnsl to LDTAIL.
# If you need <inttypes.h> copy sun-inttypes.h to inttypes.h.
#
#CC=cc
CFLAGS=-g -I.
LD=cc
LDFLAGS=
LDTAIL=

OFILES=\
	authnone.o\
	authrhosts.o\
	authp9any.o\
	convD2M.o\
	convM2D.o\
	convM2S.o\
	convS2M.o\
	des.o\
	dirmodeconv.o\
	doprint.o\
	fcallconv.o\
	oldfcall.o\
	print.o\
	random.o\
	readn.o\
	remotehost.o\
	rune.o\
	safecpy.o\
	strecpy.o\
	tokenize.o\
	u9fs.o\
	utfrune.o

HFILES=\
	fcall.h\
	plan9.h

u9fs: $(OFILES)
	$(LD) $(LDFLAGS) -o u9fs $(OFILES) $(LDTAIL)

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f *.o u9fs

install: u9fs
	cp u9fs ../../bin

.PHONY: clean install
