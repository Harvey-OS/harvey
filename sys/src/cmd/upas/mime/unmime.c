#include "common.h"

enum
{
	NONE=	0,
	BASE64,
	QUOTED,

	Self=	1,
	Hex=	2,

	/* disposition possibilities */
	Dnone=	0,
	Dinline,
	Dfile,
	Dignore,

	/* content type possibilities */
	Tnone=	0,
	Ttext,
	Tother,

	/* character sets */
	USASCII,
	UTF8,
	ISO8859_1,

	PAD64=	'=',
};

uchar	table64[256];
uchar	tableqp[256];

typedef struct Header Header;

void	ctype(Header*, char*);
void	cencoding(Header*, char*);
void	cdisposition(Header*, char*);
void	cdescription(Header*, char*);

struct Header {
	char *type;
	void (*f)(Header*, char*);
	int len;
};

Header head[] =
{
	{ "content-type:", ctype, },
	{ "content-transfer-encoding:", cencoding, },
	{ "content-disposition:", cdisposition, },
	{ "content-description:", cdescription, },
	{ 0, },
};

typedef struct Ignorance Ignorance;
struct Ignorance
{
	Ignorance *next;
	char	*str;		/* string */
	int	partial;	/* true if not exact match */
};
Ignorance *ignore;

char	boundary[1024];	/* boundary twixt pieces */
char	currentfile[1024];
Biobuf	bout;
int	encoding;
char	*dir;
int	debug;
int	ask;
int	delete;
int	disposition;
int	contenttype;
int	inheader;
int	in822header;
int	charset;
 
static uchar t64d[256];
static char t64e[64];

void	fatal(char *fmt, ...);

static void
init64(void)
{
	int c, i;

	memset(t64d, 255, 256);
	memset(t64e, '=', 64);
	i = 0;
	for(c = 'A'; c <= 'Z'; c++){
		t64e[i] = c;
		t64d[c] = i++;
	}
	for(c = 'a'; c <= 'z'; c++){
		t64e[i] = c;
		t64d[c] = i++;
	}
	for(c = '0'; c <= '9'; c++){
		t64e[i] = c;
		t64d[c] = i++;
	}
	t64e[i] = '+';
	t64d['+'] = i++;
	t64e[i] = '/';
	t64d['/'] = i;
}

int
dec64(uchar *out, int lim, char *in, int n)
{
	ulong b24;
	uchar *start = out;
	uchar *e = out + lim;
	int i, c;

	if(t64e[0] == 0)
		init64();

	b24 = 0;
	i = 0;
	while(n-- > 0){
 
		c = t64d[*in++];
		if(c == 255)
			continue;
		switch(i){
		case 0:
			b24 = c<<18;
			break;
		case 1:
			b24 |= c<<12;
			break;
		case 2:
			b24 |= c<<6;
			break;
		case 3:
			if(out + 3 > e)
				goto exhausted;

			b24 |= c;
			*out++ = b24>>16;
			*out++ = b24>>8;
			*out++ = b24;
			i = -1;
			break;
		}
		i++;
	}
	switch(i){
	case 2:
		if(out + 1 > e)
			goto exhausted;
		*out++ = b24>>16;
		break;
	case 3:
		if(out + 2 > e)
			goto exhausted;
		*out++ = b24>>16;
		*out++ = b24>>8;
		break;
	}
exhausted:
	return out - start;
}

void
initquoted(void)
{
	int c;

	memset(tableqp, 0, 256);
	for(c = ' '; c <= '<'; c++)
		tableqp[c] = Self;
	for(c = '>'; c <= '~'; c++)
		tableqp[c] = Self;
	tableqp['='] = Hex;
	tableqp['\t'] = Self;
}

int
hex2int(int x)
{
	if(x >= '0' && x <= '9')
		return x - '0';
	if(x >= 'A' && x <= 'F')
		return (x - 'A') + 10;
	if(x >= 'a' && x <= 'f')
		return (x - 'a') + 10;
}

char*
decquoted(char *out, char *in, int n)
{
	char *e;
	int c, soft;

	e = in + n - 1;

	/* dump trailing cr */
	if(*e == '\r')
		e--;

	/* dump trailing white space */
	while(*e == ' ' || *e == '\t')
		e--;

	/* trailing '=' means no newline */
	if(*e == '='){
		soft = 1;
		e--;
	} else
		soft = 0;

	while(in <= e){
		c = *in++;
		switch(tableqp[c]){
		case Self:
			*out++ = c;
			break;
		case Hex:
			c = hex2int(*in++)<<4;
			c |= hex2int(*in++);
			*out++ = c;
			break;
		}
	}
	if(!soft)
		*out++ = '\n';
	*out = 0;

	return out;
}

int
cistrncmp(char *a, char *b, int n)
{
	while(n-- > 0){
		if(tolower(*a++) != tolower(*b++))
			return -1;
	}
	return 0;
}

char*
skipwhite(char *p)
{
	while(*p == ' ' || *p == '\t' || *p == '\n')
		p++;
	return p;
}

char*
skiptosemi(char *p)
{
	while(*p && *p != ';')
		p++;
	while(*p == ';' || *p == ' ' || *p == '\t' || *p == '\n')
		p++;
	return p;
}

char*
getstring(char *p, char *buf, int len)
{
	char *op = buf;

	p = skipwhite(p);
	if(*p == '"'){
		p++;
		for(;*p && *p != '"'; p++)
			if(op-buf < len-1)
				*op++ = *p;
		*op = 0;
		if(*p == '"')
			p++;
		return p;
	}

	for(;*p && *p != ' ' && *p != '\t' && *p != ';'; p++)
		if(op-buf < len-1)
			*op++ = *p;
	*op = 0;
	return p;
}

int
getanswer(int fd, char *buf, int len)
{
	int i, n;
	char *p;

	buf[0] = 0;
	for(i = 0; i < len-1; i += n){
		n = read(fd, buf+i, len-i-1);
		if(n <= 0)
			break;
		buf[i+n] = 0;
		p = strchr(buf, '\n');
		if(p){
			*p = 0;
			break;
		}
	}
	return i;
}

int
askabout(char *f, int len)
{
	int fd, fd2;
	char buf[128];
	char *p;
	static int beenhere;

	fd = sysopentty();
	if(fd < 0)
		return -1;

	if(beenhere == 0){
		fprint(fd, "!there are attachments, ignore them (y or n)[default = y]? ");
		getanswer(fd, buf, sizeof(buf));
		for(p = buf; *p == ' ' || *p == '\t'; p++)
			;
		if(*p == 'y' || *p == 0){
			beenhere = -1;
			return open("/dev/null", OWRITE);
		}
		beenhere = 1;
	}
	if(beenhere < 0)
		return open("/dev/null", OWRITE);

	for(;;){
		fprint(fd, "!attachment %s, p(rint), s(ave) [filename], or i(gnore)? ", f);
		getanswer(fd, buf, sizeof(buf));
		for(p = buf; *p == ' ' || *p == '\t'; p++)
			;

		switch(*p){
		case 'p':
			close(fd);
			return 1;
		case 'd':
		case 'i':
			close(fd);
			return open("/dev/null", OWRITE);
		case 's':
			for(p++; *p == ' ' || *p == '\t'; p++)
				;
			if(*p){
				if(*p == '/')
					snprint(f, len, "%s", p);
				else
					snprint(f, len, "%s/%s", dir, p);
			}
			fd2 = create(f, OWRITE, 0664);
			if(fd2 >= 0){
				print("!creating %s for attachment\n", f);
				close(fd);
				return fd2;
			}
			print("!can't create %s for attachment\n", f);
		}
	}
	return -1;
}

void
setfilename(char *p)
{
	int i;
	Dir d;
	static int tmpno;
	char file[1024];
	char name[256];

	if(dir == 0)
		return;

	if(p == 0 || *p == 0)
		sprint(name, "unmime.%d", tmpno++);
	else
		getstring(p, name, sizeof name);
	if(*name == 0)
		sprint(name, "unmime.%d", tmpno++);

	for(p = name; *p; p++)
		if(*p == ' ' || *p == '\t' || *p == ';')
			*p = '_';

	p = strrchr(name, '/');
	if(p)
		p = p + 1;
	else
		p = name;
	p[NAMELEN-4] = 0;

	snprint(file, sizeof(file), "%s/%s", dir, p);

	i = 0;
	while(dirstat(file, &d) >= 0 && i < 100)
		snprint(file, sizeof(file), "%s/%d.%s", dir, i++, p);
	strcpy(currentfile, file);
}

void
createfile(void)
{
	int fd, ofd;

	Bflush(&bout);
	if(delete){
		fd = open("/dev/null", OWRITE);
		if(fd < 0)
			return;
	} else if(ask){
		fd = askabout(currentfile, sizeof(currentfile));
		if(fd < 0)
			return;
	} else {
		print("!creating %s for attachment\n", currentfile);
		fd = create(currentfile, OWRITE, 0664);
		if(fd < 0){
			fprint(2, "unmime: failed creating %s: %r\n", currentfile);
			if(fd < 0)
				fd = 1;
			currentfile[0] = 0;
		}
	}
	ofd = Bfildes(&bout);
	Bterm(&bout);
	if(ofd != 1)
		close(ofd);
	Binit(&bout, fd, OWRITE);

	return;
}

void
cdisposition(Header *h, char *p)
{
	p += h->len;
	p = skipwhite(p);
	if(debug)
		fprint(2, "!disposition %s\n", p);
	while(*p){
		if(cistrncmp(p, "inline", 6) == 0){
			disposition = Dinline;
		} else if(cistrncmp(p, "attachment", 10) == 0){
			disposition = Dfile;
		} else if(cistrncmp(p, "filename=", 9) == 0){
			p += 9;
			setfilename(p);
		}
		p = skiptosemi(p);
	}

}

void
cdescription(Header *h, char *p)
{
	USED(h);
	Bprint(&bout, "%s\n", p);
}

void
ctype(Header *h, char *p)
{
	Bprint(&bout, "%s\n", p);
	p += h->len;
	p = skipwhite(p);
	if(debug)
		fprint(2, "!ctype %s\n", p);
	if(cistrncmp(p, "text", 4) == 0)
		contenttype = Ttext;
	while(*p){
		if(cistrncmp(p, "boundary=", 9) == 0){
			p += 9;
			p = getstring(p, boundary, sizeof boundary);
			if(debug)
				fprint(2, "!boundary %s\n", boundary);
		} else if(cistrncmp(p, "multipart", 9) == 0){
			/*
			 *  the first unbounded part of a multipart message,
			 *  the preamble, is not displayed or saved
			 */
			if(disposition == Dnone)
				disposition = Dignore;
		} else if(cistrncmp(p, "name=", 5) == 0){
			if(debug)
				fprint(2, "!name %s\n", p+5);
			p += 5;
			if(currentfile[0] == 0)
				setfilename(p);
		} else if(cistrncmp(p, "charset=", 8) == 0){
			p += 8;
			if(*p == '"')
				p++;
			if(cistrncmp(p, "us-ascii", 8) == 0)
				charset = USASCII;
			else if(cistrncmp(p, "iso-8859-1", 10) == 0)
				charset = ISO8859_1;
		}
		
		p = skiptosemi(p);
	}

	if(contenttype != Ttext && currentfile[0] == 0)
		setfilename(0);
}

void
cencoding(Header *h, char *p)
{
	p += h->len;
	p = skipwhite(p);
	if(debug)
		fprint(2, "!encoding %s\n", p);
	if(cistrncmp(p, "base64", 6) == 0)
		encoding = BASE64;
	else if(cistrncmp(p, "quoted-printable", 16) == 0){
		if(charset < 0)
			charset = ISO8859_1;
		encoding = QUOTED;
	}
}

static long maptab[] = 
{
0x2022, 0x2022,	0x2022,	0x201a,	0x0192,	0x201e,	0x2026,	0x2020,	0x2021,
0x02c6,	0x2030,	0x0160,	0x2039,	0x0152,	0x2022,	0x2022,	0x2022,
0x2022, 0x2018,	0x2019,	0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x2022, 0x2022, 0x0178,
};

/*
 *  write out buffer, converting characters if necessary
 */
void
output(char *p, int n)
{
	char arena[1024];
	char *ep, *ap, *eap;
	Rune r;

	if(contenttype != Ttext){
		Bwrite(&bout, p, n);
		return;
	}
	switch(charset){
	case USASCII:
	case ISO8859_1:
		/* convert to UTF */
		ep = p + n;
		ap = arena;
		eap = ap + sizeof(arena);
		for(; p < ep; p++){
			if(eap-ap < 2){
				Bwrite(&bout, arena, ap-arena);
				ap = arena;
			}
			r = (*p) & 0xff;
				/* map the microsoft 'extensions' to iso 8859 */
			if (charset == ISO8859_1 && 0x7f <= r && r <= 0x9f)
				r = maptab[r-0x7f];
			ap += runetochar(ap, &r);
		}
		if(ap > arena)
			Bwrite(&bout, arena, ap-arena);
		break;
	default:
		Bwrite(&bout, p, n);
		break;
	}
}

char saved64[4];
int nsaved64;

void
decode(char *p, int len)
{
	char obuf[2*2048];
	int n;

	switch(encoding){
	case BASE64:
		if((len*3)/4 > sizeof obuf)
			len = sizeof(obuf)*4/3;

		/* cruft to handle lines that aren't a multiple of 4 long */
		if(nsaved64){
			while(nsaved64 < 4 && len > 0){
				saved64[nsaved64++] = *p++;
				len--;
			}
			if(nsaved64 != 4)
				return;
			n = dec64((uchar*)obuf, sizeof(obuf), saved64, 4);
			output(obuf, n);
			nsaved64 = 0;
		}
		n = len % 4;
		if(n){
			len -= n;
			memmove(saved64, p+len, n);
			nsaved64 = n;
		}

		len = dec64((uchar*)obuf, sizeof(obuf), p, len);
		output(obuf, len);
		break;
	case QUOTED:
		if(len > sizeof(obuf) - 1)
			len = sizeof(obuf) - 1;
		len = decquoted(obuf, p, len) - obuf;
		output(obuf, len);
		break;
	case NONE:
		output(p, strlen(p));
		Bprint(&bout, "\n");
		break;
	}
}

/*
 *  read the file of headers to ignore 
 */
void
readignore(void)
{
	char *p;
	Ignorance *i;
	Biobuf *b;
	char buf[128];

	if(ignore)
		return;

	snprint(buf, sizeof(buf), "%s/ignore", UPASLIB); 
	b = Bopen(buf, OREAD);
	if(b == 0)
		return;
	while(p = Brdline(b, '\n')){
		p[Blinelen(b)-1] = 0;
		while(*p && (*p == ' ' || *p == '\t'))
			p++;
		if(*p == '#')
			continue;
		i = malloc(sizeof(Ignorance));
		if(i == 0)
			break;
		i->partial = strlen(p);
		i->str = strdup(p);
		if(i->str == 0){
			free(i);
			break;
		}
		i->next = ignore;
		ignore = i;
	}
	Bterm(b);
}

void
startheader(void)
{
	inheader = 1;
	disposition = Dnone;
	contenttype = Tnone;
	charset = -1;
	nsaved64 = 0;
	memset(currentfile, 0, sizeof(currentfile));
}

void
startbody(void)
{
	int fd;

	inheader = 0;
	in822header = 0;

	/* try not to print crap on the screen */
	if(contenttype != Ttext && disposition == Dinline)
		disposition = Dnone;

	switch(disposition){
	case Dnone:
		if(*currentfile && dir)
			createfile();
		else
			Bprint(&bout, "\n");
		break;
	case Dignore:
		fd = open("/dev/null", OWRITE);
		Bterm(&bout);
		Binit(&bout, fd, OWRITE);
		break;
	case Dinline:
		Bprint(&bout, "\n");
		break;
	case Dfile:
		if(dir)
			createfile();
		else
			Bprint(&bout, "\n");
		break;
	}
}

void
main(int argc, char *argv[])
{
	int n, c, seenheader;
	Biobuf b;
	char *p;
	Header *h;
	Ignorance *i;

	char buf[4096];

	ARGBEGIN {
	case 'd':
		debug++;
		break;
	case 'a':
		ask++;
		break;
	case 'D':
		dir = "/dev/null";
		delete++;
		break;
	} ARGEND;

	inheader = 1;
	in822header = 1;
	seenheader = 0;
	encoding = NONE;
	Binit(&b, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	initquoted();
	readignore();
	for(h = head; h->type; h++)
		h->len = strlen(h->type);

	if(argc == 1){
		dir = argv[0];
		p = dir+strlen(dir)-1;
		if(*p == '/')
			*p = 0;
	}
	while(p = Brdline(&b, '\n')){
		n = Blinelen(&b)-1;
		p[n] = 0;
		if(n > 0 && p[n-1] == '\r')
			p[n-1] = 0;

		if(*boundary && strstr(p, boundary)){
			if(Bfildes(&bout) != 1){
				memset(currentfile, 0, sizeof(currentfile));
				Bterm(&bout);
				Binit(&bout, 1, OWRITE);
			}
			encoding = NONE;
			startheader();
			continue;
		}

		if(!inheader){
			decode(p, Blinelen(&b)-1);
			continue;
		}

		if(seenheader && *p == 0){
			startbody();
			continue;
		}

		strncpy(buf, p, sizeof(buf)-1);
		buf[sizeof(buf)-1] = 0;
		n = strlen(buf);

		while(n < sizeof(buf)-2){
			c = Bgetc(&b);
			if(c == ' ' || c == '\t'){
				p = Brdline(&b, '\n');
				if(p == 0)
					break;
				p[Blinelen(&b)-1] = 0;
				buf[n++] = '\n';
				strncpy(buf+n, p, sizeof(buf)-n);
				buf[sizeof(buf)-1] = 0;
				n = strlen(buf);

				/* we've seen mime messages that separate the header
				 * with lines containing whitespace
				 */
				for(; *p; p++)
					if(*p != ' ' && *p != '\t')
						break;
				if(*p == 0){
					inheader = 0; /* see startbody below */
					break;
				}
			} else {
				Bungetc(&b);
				break;
			}
		}

		for(h = head; h->type; h++)
			if(cistrncmp(buf, h->type, h->len) == 0){
				seenheader = 1;
				(*h->f)(h, buf);
				break;
			}

		/*
		 * print only those 822 headers we've been
		 * not told to ignore
		 */
		if(h->type == 0 && in822header){
			for(i = ignore; i; i = i->next){
				if(cistrncmp(i->str, buf, i->partial) == 0)
					break;
			}
			if(i == 0)
				Bprint(&bout, "%s\n", buf);
		}

		if(inheader == 0)
			startbody();
	}
}
