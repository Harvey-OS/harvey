#include	"all.h"

static	linep;

int
readline(void)
{
	int i, c;

	for(i=0; i<nelem(line); i++) {
		c = Bgetc(&bin);
		if(c < 0) {
			return 1;
		}
		if(c == '\n') {
			line[i] = 0;
			linep = 0;
			return 0;
		}
		line[i] = c;
	}
	fprint(2, "nelem(line) too small: %d\n", nelem(line));
	exits("bad");
	return 1;
}

int
getword(void)
{
	int i, c;

	for(;;) {
		c = line[linep];
		if(c == 0) {
			if(readline())
				return 1;
			continue;
		}
		if(c != ' ' && c != '\t')
			break;
		linep++;
	}

	for(i=0; i<nelem(word); i++) {
		c = line[linep];
		if(c == 0 || c == ' ' || c == '\t') {
			word[i] = 0;
			return 0;
		}
		linep++;
		word[i] = c;
	}
	fprint(2, "nelem(word) too small: %d\n", nelem(word));
	exits("bad");
	return 1;
}

int
eol(void)
{
	if(line[linep] == 0)
		return 1;
	return 0;
}

int
getdev(char *s)
{
	if(eol())
		print("%s: ", s);
	getword();
	if(strcmp(word, "tosh") == 0)
		return Dtosh;
	if(strcmp(word, "nec") == 0)
		return Dnec;
	if(strcmp(word, "plex") == 0)
		return Dplex;
	if(strcmp(word, "phil") == 0)
		return Dphil;
	if(strcmp(word, "file") == 0)
		return Dfile;
	if(strcmp(word, "disk") == 0)
		return Ddisk;
	return Dunk;
}

int
gettype(char *s)
{
	if(eol())
		print("%s: ", s);
	getword();
	if(strcmp(word, "cdda") == 0)
		return Tcdda;
	if(strcmp(word, "cdrom") == 0)
		return Tcdrom;
	if(strcmp(word, "cdi") == 0)
		return Tcdi;
	return Tunk;
}

int
gettrack(char *s)
{
	int n;

	if(eol())
		print("%s: ", s);
	getword();
	if(word[0] >= '0' && word[0] <= '9') {
		n = atoi(word);
		if(n >= 0 || n < Ntrack)
			return n;
	}
	if(strcmp(word, "*") == 0)
		return Trackall;
	return -1;
}

void
swab(uchar *a, int n)
{
	ulong *p, b;

	p = (ulong*)a;
	while(n >= 4) {
		b = *p;
		b =	((b&0xff00ff00) >> 8) |
			((b&0x00ff00ff) << 8);
		*p++ = b;
		n -= 4;
	}
}

long
bige(uchar *p)
{
	long v;

	v = (long)p[0] << 24;
	v |= (long)p[1] << 16;
	v |= (long)p[2] << 8;
	v |= (long)p[3] << 0;
	return v;
}
