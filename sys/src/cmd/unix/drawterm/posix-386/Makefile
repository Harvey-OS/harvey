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
	$(AS) -o $*.$O $*.s

md5block.s: md5block.spp
	cpp md5block.spp >md5block.s

sha1block.s: sha1block.spp
	cpp sha1block.spp >sha1block.s



