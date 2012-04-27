ROOT=..
include ../Make.config
# N.B.  This is used only for secstore.  It needn't be fast.

LIB=libmp.a

OFILES=\
	betomp.$O\
	crt.$O\
	letomp.$O\
	mpadd.$O\
	mpaux.$O\
	mpcmp.$O\
	mpdigdiv.$O\
	mpdiv.$O\
	mpeuclid.$O\
	mpexp.$O\
	mpextendedgcd.$O\
	mpfmt.$O\
	mpinvert.$O\
	mpleft.$O\
	mpmod.$O\
	mpmul.$O\
	mprand.$O\
	mpright.$O\
	mpsub.$O\
	mptobe.$O\
	mptoi.$O\
	mptole.$O\
	mptoui.$O\
	mptouv.$O\
	mptov.$O\
	mpvecadd.$O\
	mpveccmp.$O\
	mpvecdigmuladd.$O\
	mpvecsub.$O\
	strtomp.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

