LIB=/usr/lib/upas
EDSRC= edmail.c message.c cmds.c ../common/*.c
EDOBJ= edmail.o message.o cmds.o ../config/config.o ../common/common.a\
	 ../libc/libc.a
CFLAGS=${UNIX} -g -I. -I../libc -I../common ${SCFLAGS}
LFLAGS=-g ${SLFLAGS}
.c.o: ; $(CC) -c $(CFLAGS) $*.c

all: edmail

edmail: $(EDOBJ)
	$(CC) $(EDOBJ) $(LFLAGS) -o edmail

cmds.o: ../common/mail.h message.h ../common/string.h
message.o: ../common/mail.h message.h ../common/string.h ../common/aux.h
edmail.o: ../common/mail.h message.h ../common/string.h

clean:
	-rm -f edmail *.[oO] core a.out *.sL

cyntax:
	cyntax $(CFLAGS) $(EDSRC)

install:	edmail
	cp edmail $(LIB)/edmail
	strip $(LIB)/edmail
