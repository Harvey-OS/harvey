#include	"all.h"

int
fchar(void)
{
	if(uidgc.find >= uidgc.flen) {
		uidgc.find = 0;
		uidgc.flen = con_read(FID2, uidgc.uidbuf->iobuf, cons.offset, BUFSIZE);
		if(uidgc.flen <= 0)
			return -1;
		cons.offset += uidgc.flen;
	}
	return uidgc.uidbuf->iobuf[uidgc.find++] & 0xff;
}

int
fread(void *buf, int len)
{
	int n, c;
	char *b;

	b = buf;
	for(n = 0; n < len; n++) {
		c = fchar();
		if(c < 0)
			break;
		b[n] = c;
	}

	return n;
}

int
fname(char *name)
{
	int i, c;

	/*
	 * read a name and return first char known not
	 * to be in the name.
	 */
	memset(name, 0, NAMELEN);
	for(i=0;; i++) {
		c = fchar();
		switch(c) {
		case '#':
			for(;;) {
				c = fchar();
				if(c == -1 || c == '\n')
					break;
			}

		case ' ':
		case '\n':

		case ':':
		case ',':
		case '=':
		case 0:
			return c;

		case -1:
			return 0;

		case '\t':
			return ' ';
		}
		if(i < NAMELEN-1)
			name[i] = c;
	}
	return 0;
}

int
fpair(char *n1, char *n2)
{
	int c;

	do {
		c = fname(n1);
		if(c == 0)
			return 1;
	} while(*n1 == 0);


	while(c != '=') {
		c = fname(n2);
		if(c == 0)
			return 1;
		if(*n2 != 0)
			memmove(n1, n2, NAMELEN);
	}

	do {
		c = fname(n2);
		if(c == 0)
			return 1;
	} while(*n2 == 0);
	return 0;
}

int
chartoea(uchar *ea, char *cp)
{
	int i, h, c;

	h = 0;
	for(i=0; i<Easize*2; i++) {
		c = *cp++;
		if(c >= '0' && c <= '9')
			c = c - '0';
		else
		if(c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else
		if(c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else
			return 1;
		h = (h*16) + c;
		if(i & 1) {
			*ea++ = h;
			h = 0;
		}
	}
	if(*cp != 0)
		return 1;
	return 0;
}

int
chartoip(uchar *pa, char *cp)
{
	int i, c, h;

	for(i=0;;) {
		h = 0;
		for(;;) {
			c = *cp++;
			if(c < '0' || c > '9')
				break;
			h = (h*10) + (c-'0');
		}
		*pa++ = h;
		i++;
		if(i == Pasize) {
			if(c != 0)
				return 1;
			return 0;
		}
		if(c != '.')
			return 1;
	}
}

void
getipa(Ifc *ifc)
{

	memmove(ifc->ipa, sysip, Pasize);
	memmove(ifc->netgate, defgwip, Pasize);
	ifc->ipaddr = nhgetl(ifc->ipa);
	ifc->mask = nhgetl(defmask);
}
