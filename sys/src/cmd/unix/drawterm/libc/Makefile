ROOT=..
include ../Make.config
LIB=libc.a

OFILES=\
	charstod.$O\
	cleanname.$O\
	convD2M.$O\
	convM2D.$O\
	convM2S.$O\
	convS2M.$O\
	crypt.$O\
	dial.$O\
	dirfstat.$O\
	dirfwstat.$O\
	dirmodefmt.$O\
	dirstat.$O\
	dirwstat.$O\
	dofmt.$O\
	dorfmt.$O\
	encodefmt.$O\
	fcallfmt.$O\
	fltfmt.$O\
	fmt.$O\
	fmtfd.$O\
	fmtfdflush.$O\
	fmtlock.$O\
	fmtprint.$O\
	fmtquote.$O\
	fmtrune.$O\
	fmtstr.$O\
	fmtvprint.$O\
	fprint.$O\
	getfields.$O\
	getpid.$O\
	lock.$O\
	mallocz.$O\
	nan64.$O\
	netmkaddr.$O\
	nsec.$O\
	pow10.$O\
	pushssl.$O\
	pushtls.$O\
	read9pmsg.$O\
	readn.$O\
	rune.$O\
	runefmtstr.$O\
	runeseprint.$O\
	runesmprint.$O\
	runesnprint.$O\
	runesprint.$O\
	runestrchr.$O\
	runestrlen.$O\
	runestrstr.$O\
	runetype.$O\
	runevseprint.$O\
	runevsmprint.$O\
	runevsnprint.$O\
	seprint.$O\
	smprint.$O\
	snprint.$O\
	sprint.$O\
	strecpy.$O\
	strtod.$O\
	strtoll.$O\
	sysfatal.$O\
	time.$O\
	tokenize.$O\
	truerand.$O\
	u16.$O\
	u32.$O\
	u64.$O\
	utfecpy.$O\
	utflen.$O\
	utfnlen.$O\
	utfrrune.$O\
	utfrune.$O\
	utfutf.$O\
	vfprint.$O\
	vseprint.$O\
	vsmprint.$O\
	vsnprint.$O

default: $(LIB)
$(LIB): $(OFILES)
	$(AR) r $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

%.$O: %.c
	$(CC) $(CFLAGS) $*.c

