ROOT=..
include ../Make.config
LIB=libmemdraw.a

OFILES=\
	alloc.$O\
	arc.$O\
	cload.$O\
	cmap.$O\
	cread.$O\
	defont.$O\
	draw.$O\
	ellipse.$O\
	fillpoly.$O\
	hwdraw.$O\
	line.$O\
	load.$O\
	openmemsubfont.$O\
	poly.$O\
	read.$O\
	string.$O\
	subfont.$O\
	unload.$O\
	write.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

