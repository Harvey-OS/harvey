#include <u.h>
#include <libc.h>
#include <libg.h>
#include "ikana.h"

void changelang(Event *);
int natural = 1;		/* not Japanese */
extern struct map *table, kata[], hira[], cyril[], greek[];
	
void
main(int argc, char **argv)
{

	uchar *bp, *ep, buf[32];
	struct map *mp;
	int nchar, wantmore;
	int fd, cfd, kfd, ckey, kkey, rkey;
	struct Event e;

	USED(argc, argv);
	if(bind("#|", "/mnt/kana", MREPL) < 0
	|| bind("/mnt/kana/data1", "/dev/cons", MREPL) < 0){
		fprint(2, "error simulating cons\n");
		exits("/mnt/kana");
	}
	fd = open("/mnt/kana/data", OWRITE);
	if(bind("#|", "/mnt/kanactl", MREPL) < 0) {
		fprint(2, "error binding /mnt/kanactl\n");
		exits("/mnt/kanactl");
	}
	cfd = open("/mnt/kanactl/data1", OREAD);
	open("/mnt/kanactl/data", OWRITE);	/* just to keep it open */
	kfd = 0;
	einit(0);
	kkey = estart(0, kfd, sizeof(e.data));
	ckey = estart(0, cfd, sizeof(e.data));
	bp = ep = buf;
	wantmore = 0;
	for (;;) {
		if (bp>=ep || wantmore) {
			if (wantmore==0)
				bp = ep = buf;
			rkey = eread(kkey|ckey, &e);
			if (rkey & ckey) {
				changelang(&e);
				continue;
			}
			if ((rkey & kkey)==0)
				continue;
			memmove(ep, e.data, e.n);
			ep += e.n;
			*ep = '\0';
		}
		while (bp<ep) {
			if (!fullrune((char *)bp, ep-bp)) {
				wantmore = 1;
				continue;
			}
			wantmore = 0;
			if (natural || *bp<=' ' || *bp>='{') {
				Rune r;
				int rlen = chartorune(&r, (char *)bp);
				write(fd, bp, rlen);
				bp += rlen;
				continue;
			}
			mp = match(bp, &nchar, table);
			if (mp == 0) {
				if (nchar>0) {		/* match, longer possible */
					wantmore++;
					break;
				}
				write(fd, bp++, 1);	/* no match, pass through */
			} else {
				write(fd, mp->kana, strlen(mp->kana));
				bp += nchar;
			}
		}
	}
}

int
min(int a, int b)
{
	return a<b? a: b;
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

void
changelang(Event *e)
{
	if (strncmp((char *)e->data, "katakana", 8)==0) {
		natural = 0;
		table = kata;
		return;
	}

	if (strncmp((char *)e->data, "hiragana", 8)==0) {
		natural = 0;
		table = hira;
		return;
	}

	if (strncmp((char *)e->data, "english", 7)==0) {
		natural = 1;
		return;
	}

	if (strncmp((char *)e->data, "russian", 7)==0) {
		natural = 0;
		table = cyril;
		return;
	}
	if (strncmp((char *)e->data, "greek", 5)==0) {
		natural = 0;
		table = greek;
		return;
	}

}

void ereshaped(Rectangle r)
{
	USED(r);
}

