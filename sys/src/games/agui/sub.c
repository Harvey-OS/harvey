#include	<audio.h>

void*
mal(long n)
{
	void *v;

	totalmal += n;
	v = malloc(n);
	if(v == 0) {
		fprint(2, "out of mem\n");
		exits("mem");
	}
	memset(v, 0, n);
	return v;
}

int
get2(Ibuf *bin, long o)
{
	int n;

	if(o != 0)
		n = Igetc1(bin, o);
	else
		n = Igetc(bin);
	return (n<<8) | Igetc(bin);
}

long
get4(Ibuf *bin, long o)
{
	int i;
	long n;

	if(o != 0)
		n = Igetc1(bin, o);
	else
		n = Igetc(bin);
	for(i=0; i<3; i++)
		n = (n<<8) | Igetc(bin);
	return n;
}

char*
gets(Ibuf *bin, long o)
{
	static char buf[500];
	int i, c;

	if(o != 0)
		c = Igetc1(bin, o);
	else
		c = Igetc(bin);
	for(i=0;; i++) {
		if(c < 0)
			c = 0;
		if(i < nelem(buf)-3)
			buf[i] = c;
		if(c == 0)
			break;
		c = Igetc(bin);
	}
	return buf;
}

/*
 * this is same as access in database
 * must be constant and portable
 */
int
srchash(char *s)
{
	ulong h;
	int c;

	h = 0;
	for(; c = *s; s++)
		h = h*3 + (c & 0xff);
	return h & 0xff;
}

long
bsearch(char *s)
{
	long p, pm, o, c, m;
	int t;

	t = srchash(s) * 2;
	p = strings + bschar[t]*4;
	c = bschar[t+1];

loop:
	if(c <= 0)
		return 0;
	m = c/2;
	pm = p + m*4;
	o = get4(bin, pm);
	t = strcmp(s, gets(bin, o));
	if(t <= 0) {
		if(t == 0)
			return o + strlen(s) + 1;
		c = m;
		goto loop;
	}
	p = pm+4;
	c = c-m-1;
	goto loop;
}

int
isonone(long o)
{
	if(o == Onone)
		return 1;
	return 0;
}

int
isoplaying(long o)
{
	if(o == Oplaying)
		return 1;
	return 0;
}

int
isomissing(long o)
{
	if(o == Omissing)
		return 1;
	return 0;
}

int
isoqueue(long o)
{
	if(o >= Oqueue && o < Oqueue+Npage)
		return 1;
	return 0;
}

int
isosearch(long o)
{
	if(o >= Osearch && o < Osearch+Nsearch*Npage)
		return 1;
	return 0;
}

int
isospecial(long o)
{
	if(o >= Onone)
		return 1;
	return 0;
}
