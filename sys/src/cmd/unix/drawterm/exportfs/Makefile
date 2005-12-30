ROOT=..
include ../Make.config
LIB=libexportfs.a

OFILES=\
	exportfs.$O\
	exportsrv.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

