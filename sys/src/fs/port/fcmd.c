#include	"all.h"

int
fchar(void)
{
	int n;

	n = BUFSIZE;
	if(n > MAXDAT)
		n = MAXDAT;
	if(uidgc.find >= uidgc.flen) {
		uidgc.find = 0;
		uidgc.flen = con_read(FID2, uidgc.uidbuf->iobuf, cons.offset, n);
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
