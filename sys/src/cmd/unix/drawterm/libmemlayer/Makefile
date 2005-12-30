ROOT=..
include ../Make.config
LIB=libmemlayer.a

OFILES=\
	draw.$O\
	lalloc.$O\
	layerop.$O\
	ldelete.$O\
	lhide.$O\
	line.$O\
	load.$O\
	lorigin.$O\
	lsetrefresh.$O\
	ltofront.$O\
	ltorear.$O\
	unload.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

