ROOT=..
include ../Make.config
LIB=libkern.a

OFILES=\
	allocb.$O\
	cache.$O\
	chan.$O\
	data.$O\
	dev.$O\
	devaudio.$O\
	devaudio-$(AUDIO).$O\
	devcons.$O\
	devdraw.$O\
	devfs-$(OS).$O\
	devip.$O\
	devip-$(OS).$O\
	devlfd.$O\
	devmnt.$O\
	devmouse.$O\
	devpipe.$O\
	devroot.$O\
	devssl.$O\
	devtls.$O\
	devtab.$O\
	error.$O\
	parse.$O\
	pgrp.$O\
	procinit.$O\
	rwlock.$O\
	sleep.$O\
	smalloc.$O\
	stub.$O\
	sysfile.$O\
	sysproc.$O\
	qio.$O\
	qlock.$O\
	term.$O\
	uart.$O\
	waserror.$O\
	$(OS).$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

