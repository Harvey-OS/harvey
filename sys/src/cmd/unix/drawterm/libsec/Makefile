ROOT=..
include ../Make.config
LIB=libsec.a

OFILES=\
	aes.$O\
	blowfish.$O\
	decodepem.$O\
	des.$O\
	des3CBC.$O\
	des3ECB.$O\
	desCBC.$O\
	desECB.$O\
	desmodes.$O\
	dsaalloc.$O\
	dsagen.$O\
	dsaprimes.$O\
	dsaprivtopub.$O\
	dsasign.$O\
	dsaverify.$O\
	egalloc.$O\
	egdecrypt.$O\
	egencrypt.$O\
	eggen.$O\
	egprivtopub.$O\
	egsign.$O\
	egverify.$O\
	fastrand.$O\
	genprime.$O\
	genrandom.$O\
	gensafeprime.$O\
	genstrongprime.$O\
	hmac.$O\
	md4.$O\
	md5.$O\
	md5pickle.$O\
	nfastrand.$O\
	prng.$O\
	probably_prime.$O\
	rc4.$O\
	rsaalloc.$O\
	rsadecrypt.$O\
	rsaencrypt.$O\
	rsafill.$O\
	rsagen.$O\
	rsaprivtopub.$O\
	sha1.$O\
	sha1pickle.$O\
	smallprimes.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

