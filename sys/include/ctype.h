#pragma	src	"/sys/src/libc/port"
#pragma	lib	"libc.a"

#define	_U	01
#define	_L	02
#define	_N	04
#define	_S	010
#define	_P	020
#define	_C	040
#define	_B	0100
#define	_X	0200

extern unsigned char	_ctype[];

#define	isalpha(c)	(_ctype[(unsigned char)(c)]&(_U|_L))
#define	isupper(c)	(_ctype[(unsigned char)(c)]&_U)
#define	islower(c)	(_ctype[(unsigned char)(c)]&_L)
#define	isdigit(c)	(_ctype[(unsigned char)(c)]&_N)
#define	isxdigit(c)	(_ctype[(unsigned char)(c)]&_X)
#define	isspace(c)	(_ctype[(unsigned char)(c)]&_S)
#define	ispunct(c)	(_ctype[(unsigned char)(c)]&_P)
#define	isalnum(c)	(_ctype[(unsigned char)(c)]&(_U|_L|_N))
#define	isprint(c)	(_ctype[(unsigned char)(c)]&(_P|_U|_L|_N|_B))
#define	isgraph(c)	(_ctype[(unsigned char)(c)]&(_P|_U|_L|_N))
#define	iscntrl(c)	(_ctype[(unsigned char)(c)]&_C)
#define	isascii(c)	((unsigned char)(c)<=0177)
#define	_toupper(c)	((c)-'a'+'A')
#define	_tolower(c)	((c)-'A'+'a')
#define	toascii(c)	((c)&0177)
