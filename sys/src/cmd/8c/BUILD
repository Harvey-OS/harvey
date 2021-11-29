rm -f *.o

(cd ../cc; $PLAN9/bin/yacc -D1 -d cc.y)
9c -O0 -g -I /../../../../include -I . -I ../cc  \
		-Wno-parentheses \
		-Wno-missing-braces \
		-Wno-switch \
		-Wno-comment \
		-Wno-sign-compare \
		-Wno-unknown-pragmas \
		-Wno-misleading-indentation \
		-Wno-stringop-truncation \
		-Wno-stringop-overflow \
		-Wno-format-truncation \
		-fno-omit-frame-pointer \
		-fsigned-char \
		-fcommon \
		-Wno-char-subscripts  \
		-Wno-builtin-declaration-mismatch \
		-Wno-implicit-function-declaration \
		-Wno-int-conversion \
		-Wno-maybe-uninitialized \
		-Wno-incompatible-pointer-types \
		-DRunemask=0x1FFFFF \
	../cc/acid.c \
	../cc/bits.c \
	../cc/com64.c \
	../cc/com.c \
	../6l/unixcompat.c \
	../cc/dcl.c \
	../cc/dpchk.c \
	../cc/funct.c \
	../cc/lex.c \
	../cc/mac.c \
	../cc/pgen.c \
	../cc/pickle.c \
	../cc/pswt.c \
	../cc/scon.c \
	../cc/sub.c \
	../cc/y.tab.c \
	cgen.c \
	cgen64.c \
	div.c \
	enam.c \
	list.c \
	machcap.c \
	mul.c \
	peep.c \
	reg.c \
	sgen.c \
	swt.c \
	txt.c \
&& 9l -o 8c -static *.o
cp 8c $HARVEY_LINUX_BIN
