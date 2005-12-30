ROOT=..
include ../Make.config

LIB=libauth.a
OFILES=\
	attr.$O\
	auth_attr.$O\
	auth_challenge.$O\
	auth_getuserpasswd.$O\
	auth_proxy.$O\
	auth_respond.$O\
	auth_rpc.$O\
	auth_userpasswd.$O\

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

