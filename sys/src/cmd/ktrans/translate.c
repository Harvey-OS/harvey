#include <u.h>
#include <libc.h>
#include "ktrans.h"

#define	LSIZE	256

int natural = 1;		/* not Japanese */
extern struct map *table, kata[], hira[], cyril[], greek[];
void	send(uchar *, int);
int	dotrans(void);
Rune	lbuf[LSIZE];
int	llen;
int	in, out;
int	noter(void *, char *);
	
void
main(int argc, char **argv)
{

	uchar *bp, *ep, buf[128];
	struct map *mp;
	int nchar, wantmore;
	int n, c;

	USED(argc);
	USED(argv);
	if ((in = open("/dev/kbd", OREAD)) < 0)
		exits("open /dev/kbd read");
	if ((out = open("/dev/kbd", OWRITE)) < 0)
		exits("open /dev/kbd/write");
	if (rfork(RFPROC))
		exits(0);
	atnotify(noter, 1);
	bp = ep = buf;
	wantmore = 0;
	for (;;) {
	getmore:
		if (bp>=ep || wantmore) {
			if (wantmore==0)
				bp = ep = buf;
			n = read(in, ep, &buf[sizeof(buf)]-ep);
			if (n<=0)
				exits("");
			ep += n;
			*ep = '\0';
		}
		while (bp<ep) {
			if (!fullrune((char *)bp, ep-bp)) {
				wantmore = 1;
				goto getmore;
			}
			wantmore = 0;
			if (*bp=='') {
				c = dotrans();
				if (c)
					*bp = c;
				else
					bp++;
				continue;
			}
			if (changelang(*bp)) {
				bp++;
				continue;
			}
			if (natural || *bp<=' ' || *bp>='{') {
				Rune r;
				int rlen = chartorune(&r, (char *)bp);
				send(bp, rlen);
				bp += rlen;
				continue;
			}
			mp = match(bp, &nchar, table);
			if (mp == 0) {
				if (nchar>0) {		/* match, longer possible */
					wantmore++;
					break;
				}
				send(bp++, 1);	/* no match, pass through */
			} else {
				send((uchar*)mp->kana, strlen(mp->kana));
				bp += nchar;
			}
		}
	}
}

int
noter(void *x, char *note)
{
	USED(x);
	if (strcmp(note, "interrupt")==0)
		return 1;
	else
		return 0;
}

int
min(int a, int b)
{
	return a<b? a: b;
}

void
send(uchar *p, int n)
{
	Rune r;
	uchar *ep;

	if (write(out, (char *)p, n) != n)
		exits("");
	if (llen>LSIZE-64) {
		memmove((char*)lbuf, (char*)lbuf+64, 64*sizeof(Rune));
		llen -= 64;
	}
	if (table!=hira || natural)
		return;
	ep = p+n;
	while (llen<LSIZE && p<ep) {
		p += chartorune(&r, (char*)p);
		if (r=='\b') {			/* handle backspace */
			if (llen>0)
				llen--;
			continue;
		}
		if (r==0x80)			/* ignore view key */
			continue;
		if (r<0x3041 || r>0x309e) {	/* reset if out of hiragana range */
			llen = 0;
			continue;
		}
		lbuf[llen++] = r;
	}
}

struct map *
match(uchar *p, int *nc, struct map *table)
{
	register struct map *longp = 0, *kp;
	static char last;
	int longest = 0;

	*nc = -1;
	/* special pleading for long katakana vowels */
	if (table==kata && strchr("aeiou", *p) && *p==last) {
		last = 0;
		*nc = 1;
		return &table[0];
	}
	for (kp=table; kp->roma; kp++) {
		if (*p == *kp->roma) {
			int lr = strlen(kp->roma);
			int len = min(lr, strlen((char *)p));
			if (strncmp(kp->roma, (char *)p, len)==0) {
				if (len<lr) {
					*nc = 1;
					return 0;
				}
				if (len>longest) {
					longest = len;
					longp = kp;
				}	
			}
		}
	}
	if (longp) {
		last = longp->roma[longest-1];
		*nc = longp->advance;
	}
	return(longp);
}

int
changelang(int c)
{
	if (c=='') {
		natural = 0;
		table = kata;
		llen = 0;
		return 1;
	}

	if (c=='') {
		natural = 0;
		table = hira;
		return 1;
	}

	if (c=='') {
		natural = 1;
		llen = 0;
		return 1;
	}

	if (c=='') {
		natural = 0;
		table = cyril;
		llen = 0;
		return 1;
	}
	if (c=='') {
		natural = 0;
		table = greek;
		llen = 0;
		return 1;
	}
	return 0;

}

int
dotrans()
{
	Rune *res;
	char v[128], *p, *ep;
	int j, lastlen;
	char ch;
	
	if (llen==0)
		return 0;
	lbuf[llen] = 0;
	res = hlook(lbuf);
	if (res==0)
		return 0;
	p = v;
	do {
		while (*res!=L' ' && *res!=L'\t' && *res!=L'\n')
			p += runetochar(p, res++);
		*p++ = '\0';
	} while (*res++!=L'\n');
	ep = p;
	p = v;
	for (;;) {
		lastlen = nrune(p);
		for (j=0; j<lastlen; j++)
			write(out, "\b", 1);
		p += strlen(p)+1;
		if (p>=ep)
			p = v;
		write(out, p, strlen(p));
		if (read(in, &ch, 1)<=0)
			exits(0);
		if (ch != '')
			break;
	}
	llen = 0;
	return ch;
}
