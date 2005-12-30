ROOT=..
include ../Make.config
LIB=libauthsrv.a

OFILES=\
	_asgetticket.$O\
	_asrdresp.$O\
	convA2M.$O\
	convM2A.$O\
	convM2PR.$O\
	convM2T.$O\
	convM2TR.$O\
	convPR2M.$O\
	convT2M.$O\
	convTR2M.$O\
	nvcsum.$O\
	opasstokey.$O\
	passtokey.$O\

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

