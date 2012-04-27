ROOT=..
include ../Make.config
LIB=libdraw.a

OFILES=\
	alloc.$O\
	arith.$O\
	bytesperline.$O\
	chan.$O\
	defont.$O\
	drawrepl.$O\
	icossin.$O\
	icossin2.$O\
	rectclip.$O\
	rgb.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

