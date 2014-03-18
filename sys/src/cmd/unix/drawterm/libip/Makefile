ROOT=..
include ../Make.config
LIB=libip.a

OFILES=\
	eipfmt.$O\
	parseip.$O\
	classmask.$O\
	bo.$O\
	ipaux.$O\

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

