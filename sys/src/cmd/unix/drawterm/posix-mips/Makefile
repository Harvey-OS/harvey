ROOT=..
include ../Make.config
LIB=../libmachdep.a

OFILES=\
	getcallerpc.$O\
	md5block.$O\
	sha1block.$O\
	tas.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

%.$O: %.s
	$(AS) $(ASFLAGS) -o $*.$O $*.s

%.s: %.spp
	cpp $*.spp >$*.s


