/*
 * wc -- count things in utf-encoded text files
 * Bugs:
 *	Codes higher than F7 65 B7 (the utf encoding of FFFF) are not flagged as illegal.
 *	The only white space characters recognized are ' ', '\t' and '\n', even though
 *	iso 10646 has many more blanks scattered through it.
 */
#include <u.h>
#include <libc.h>

#define	NBUF	(8*1024)

long nline, tnline, pline;
long nword, tnword, pword;
long nrune, tnrune, prune;
long nbadr, tnbadr, pbadr;
long nchar, tnchar, pchar;

void count(int, char *);
void report(long, long, long, long, long, char *);

void
main(int argc, char *argv[])
{
	char *status = "";
	int i, f;

	ARGBEGIN {
	case 'l':
		pline++;
		break;
	case 'w':
		pword++;
		break;
	case 'r':
		prune++;
		break;
	case 'b':
		pbadr++;
		break;
	case 'c':
		pchar++;
		break;
	default:
		fprint(2, "Usage: %s [-lwrbc] [file ...]\n", argv0);
		exits("usage");
	} ARGEND

	if(pline+pword+prune+pbadr+pchar == 0)
		pline = pword = pchar = 1;
	if(argc == 0)
		count(0, 0);
	else {
		for(i=0; i<argc; i++) {
			f = open(argv[i], OREAD);
			if(f < 0) {
				perror(argv[i]);
				status = "can't open";
			} else {
				count(f, argv[i]);
				tnline += nline;
				tnword += nword;
				tnrune += nrune;
				tnbadr += nbadr;
				tnchar += nchar;
				close(f);
			}
		}
		if(argc > 1)
			report(tnline, tnword, tnrune, tnbadr, tnchar, "total");
	}
	exits(status);
}

void
report(long nline, long nword, long nrune, long nbadr, long nchar, char *fname)
{
	char line[1024], word[128];

	line[0] = '\0';
	if(pline) {
		sprint(word, " %7ld", nline);
		strcat(line, word);
	}
	if(pword) {
		sprint(word, " %7ld", nword);
		strcat(line, word);
	}
	if(prune) {
		sprint(word, " %7ld", nrune);
		strcat(line, word);
	}
	if(pbadr) {
		sprint(word, " %7ld", nbadr);
		strcat(line, word);
	}
	if(pchar) {
		sprint(word, " %7ld", nchar);
		strcat(line, word);
	}
	if(fname) {
		sprint(word, " %s",   fname);
		strcat(line, word);
	}
	print("%s\n", line+1);
}

enum
{
	NEWL	= 0,	/* in any state, read a newline */
	WHIT,		/* in any state, read blank or tab */
	BEGL,		/* in space state, read first byte of an escaped latin-1 char */
	BEG1,		/* in space state, read 1-byte non-space utf code */
	BEG2,		/* in space state, read first byte of 2-byte utf code */
	BEG3,		/* in space state, read first byte of 3-byte utf code */
	BEGB,		/* in space state, read illegal utf code */
	BADU,		/* in word* state, read bad non-space byte in multi-byte utf code */
	BADW,		/* in word* state, read bad blank or tab in multi-byte utf code */
	BADN,		/* in word* state, read bad newline in multi-byte utf code */
	ASCI,		/* in word0 state, read a one-byte utf code */
	LAT1,		/* in word0 state, read the first byte of an escaped latin-1 char */
	UTF2,		/* in word0 state, read first byte in 2-byte utf code */
	UTF3,		/* in word0 state, read first byte in 3-byte utf code */
	BADC,		/* in word0 state, read illegal utf code */
	WRD0,		/* in word1 state, read the last byte of multi-byte utf code */
	WRD1,		/* in word2 state, read the second byte of a multi-byte utf code */
};

uchar	space[256] =	/* skipping over white space */
{
/*x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xa   xb   xc   xd   xe   xf */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,WHIT,NEWL,BEG1,BEG1,BEG1,BEG1,BEG1, /* 0x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 1x */
	WHIT,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 2x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 3x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 4x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 5x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 6x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 7x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 8x */
	BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1,BEG1, /* 9x */
	BEGL,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2, /* ax */
	BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2, /* bx */
	BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2, /* cx */
	BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2, /* dx */
	BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG2, /* ex */
	BEG2,BEG2,BEG2,BEG2,BEG2,BEG2,BEG3,BEG3,BEGB,BEGB,BEGB,BEGB,BEGB,BEGB,BEGB,BEGB, /* fx */
};
uchar	latin[256] =	/* in the middle of an escaped latin-1 character */
{
	/*x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xa   xb   xc   xd   xe   xf */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADW,BADN,BADU,BADU,BADU,BADU,BADU, /* 0x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 1x */
	BADW,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 2x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 3x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 4x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 5x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 6x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 7x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 8x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 9x */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* ax */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* bx */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* cx */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* dx */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* ex */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* fx */
};
uchar	word2[256] =	/* need 2 more bytes for a complete utf code */
{
	/*x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xa   xb   xc   xd   xe   xf */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADW,BADN,BADU,BADU,BADU,BADU,BADU, /* 0x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 1x */
	BADW,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* 2x */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* 3x */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* 4x */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* 5x */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* 6x */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,BADU, /* 7x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 8x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 9x */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* ax */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* bx */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* cx */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* dx */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* ex */
	WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1,WRD1, /* fx */
};
uchar	word1[256] =	/* need 1 more byte for a complete utf code */
{
	/*x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xa   xb   xc   xd   xe   xf */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADW,BADN,BADU,BADU,BADU,BADU,BADU, /* 0x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 1x */
	BADW,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* 2x */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* 3x */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* 4x */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* 5x */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* 6x */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,BADU, /* 7x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 8x */
	BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU,BADU, /* 9x */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* ax */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* bx */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* cx */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* dx */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* ex */
	WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0,WRD0, /* fx */
};
uchar	word0[256] =	/* at the end of a non-space utf code */
{
	/*x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xa   xb   xc   xd   xe   xf */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,WHIT,NEWL,ASCI,ASCI,ASCI,ASCI,ASCI, /* 0x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 1x */
	WHIT,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 2x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 3x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 4x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 5x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 6x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 7x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 8x */
	ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI,ASCI, /* 9x */
	LAT1,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2, /* ax */
	UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2, /* bx */
	UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2, /* cx */
	UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2, /* dx */
	UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF2, /* ex */
	UTF2,UTF2,UTF2,UTF2,UTF2,UTF2,UTF3,UTF3,BADC,BADC,BADC,BADC,BADC,BADC,BADC,BADC, /* fx */
};

void
count(int f, char *name)
{
	int n;
	uchar buf[NBUF];
	uchar *bufp, *ebuf;
	uchar *state = space;

	nline = nword = nrune = nbadr = nchar = 0;
	for(;;) {
		n = read(f, buf, NBUF);
		if(n <= 0)
			break;
		nchar += n;
		nrune += n;	/* might be too large, gets decreased later */
		bufp = buf;
		ebuf = buf+n;
		do {
			switch(state[*bufp]) {
			case NEWL:
				state = space;
				nline++;
				break;
			case WHIT:
				state = space;
				break;
			case BEGL:
				state = latin;
				nword++;
				break;
			case BEG1:
				state = word0;
				nword++;
				break;
			case BEG2:
				state = word1;
				nword++;
				break;
			case BEG3:
				state = word2;
				nword++;
				break;
			case BEGB:
				state = word0;
				nword++;
				nbadr++;
				break;
			case BADU:
				state = word0;
				nbadr++;
				break;
			case BADW:
				state = space;
				nbadr++;
				break;
			case BADN:
				state = space;
				nline++;
				nbadr++;
				break;
			case ASCI:
				break;
			case LAT1:
				state = latin;
				break;
			case UTF2:
				state = word1;
				break;
			case UTF3:
				state = word2;
				break;
			case BADC:
				nbadr++;
				break;
			case WRD0:
				state = word0;
				nrune--;
				break;
			case WRD1:
				state = word1;
				nrune--;
				break;
			}
		} while(++bufp != ebuf);
	}
	if(state!=space && state!=word0)
		nbadr++;
	if(n < 0)
		perror(name);
	report(nline, nword, nrune, nbadr, nchar, name);
}
