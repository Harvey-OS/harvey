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

typedef struct Attachment Attachment;
struct Attachment
{
	char	*gentype;
	char	*spectype;
	char	*compression;
	int	charset;
	char	*body;
	int	len;
	int	fd;			/* temp file if body > 4k */
};
 
void	fatal(char *fmt, ...);
void	usage(void);
int	enc64(char*, int, uchar*, int);
int	classify(char*, int*, int*, int*);

void
usage(void)
{
	fprint(2, "usage: %s address-list\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	ARGBEGIN{
	}ARGEND;

	if(argc < 1)
		usage();
}

static uchar t64d[256];
static char t64e[64];

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
enc64(char *out, int lim, uchar *in, int n)
{
	int i;
	ulong b24;
	char *start = out;
	char *e = out + lim;

	if(t64e[0] == 0)
		init64();
	for(i = n/3; i > 0; i--){
		b24 = (*in++)<<16;
		b24 |= (*in++)<<8;
		b24 |= *in++;
		if(out + 5 >= e)
			goto exhausted;
		*out++ = t64e[(b24>>18)];
		*out++ = t64e[(b24>>12)&0x3f];
		*out++ = t64e[(b24>>6)&0x3f];
		*out++ = t64e[(b24)&0x3f];
	}

	switch(n%3){
	case 2:
		b24 = (*in++)<<16;
		b24 |= (*in)<<8;
		if(out + 4 >= e)
			goto exhausted;
		*out++ = t64e[(b24>>18)];
		*out++ = t64e[(b24>>12)&0x3f];
		*out++ = t64e[(b24>>6)&0x3f];
		break;
	case 1:
		b24 = (*in)<<16;
		if(out + 4 >= e)
			goto exhausted;
		*out++ = t64e[(b24>>18)];
		*out++ = t64e[(b24>>12)&0x3f];
		*out++ = '=';
		break;
	}
exhausted:
	*out++ = '=';
	*out = 0;
	return out - start;
}

typedef struct Suffix Suffix;
struct Suffix
{
	Suffix	*next;
	char	*suffix;
	char	*
};
Suffix suftab;

/* read table of suffixes */
void


/* classify a file */
int
classify(char *file, Attachment *a)
{
	if(suftab == 0)
		readsuftab();
}
