# CFLAGS=-Wall -D_GNU_SOURCE
CFLAGS=-Wall
LD=cc
LDFLAGS=-static

all: authsrv9 passtokey authsrv9.0

authsrv9: authsrv9.o util.o util.h des.o des.h
	$(LD) $(LDFLAGS) -o authsrv9 authsrv9.o util.o des.o

passtokey: passtokey.o util.o util.h des.o des.h
	$(LD) $(LDFLAGS) -o passtokey passtokey.o util.o des.o

authsrv9.0: authsrv9.8
	nroff -mandoc authsrv9.8 >authsrv9.0

clean:
	-rm -f authsrv9 authsrv9.o passtokey passtokey.o util.o authsrv9.0 des.o
