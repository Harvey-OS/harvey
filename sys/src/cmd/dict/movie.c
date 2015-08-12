/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dict.h"

/* Possible tags */
enum {
	BEG, /* beginning of entry */
	AB,  /* abstract */
	AN,  /* database serial number */
	AS,  /* author (one at a time) */
	AU,  /* all authors */
	AW,  /* award_awardee */
	BW,  /* bw or c */
	CA,  /* cast: character_actor */
	CN,  /* cinematography */
	CO,  /* country */
	CR,  /* miscellaneous job_name */
	DE,  /* topic keyword */
	DR,  /* director */
	ED,  /* editor */
	MP,  /* MPAA rating (R, PG, etc.) */
	NT,  /* note */
	PR,  /* producer and for ...*/
	PS,  /* producer (repeats info in PR) */
	RA,  /* rating (letter) */
	RD,  /* release date */
	RT,  /* running time */
	RV,  /* review citation */
	ST,  /* production or release company (repeats info in PR) */
	TI,  /* title[; original foreign title] */
	TX,  /* paragraph of descriptive text */
	VD,  /* video information (format_time_company; or "Not Avail.") */
	NTAG /* number of tags */
};

/* Assoc tables must be sorted on first field */

static char *tagtab[] = {
	[BEG] "$$",
	[AB] "AB",
	[AN] "AN",
	[AS] "AS",
	[AU] "AU",
	[AW] "AW",
	[BW] "BW",
	[CA] "CA",
	[CN] "CN",
	[CO] "CO",
	[CR] "CR",
	[DE] "DE",
	[DR] "DR",
	[ED] "ED",
	[MP] "MP",
	[NT] "NT",
	[PR] "PR",
	[PS] "PS",
	[RA] "RA",
	[RD] "RD",
	[RT] "RT",
	[RV] "RV",
	[ST] "ST",
	[TI] "TI",
	[TX] "TX",
	[VD] "VD",
};

static char *mget(int, char *, char *, char **);
static void moutall(int, char *, char *);
static void moutall2(int, char *, char *);

void
movieprintentry(Entry ent, int cmd)
{
	char *p, *e, *ps, *pe, *pn;
	int n;

	ps = ent.start;
	pe = ent.end;
	if(cmd == 'r') {
		Bwrite(bout, ps, pe - ps);
		return;
	}
	p = mget(TI, ps, pe, &e);
	if(p) {
		outpiece(p, e);
		outnl(0);
	}
	if(cmd == 'h')
		return;
	outnl(2);
	n = 0;
	p = mget(RD, ps, pe, &e);
	if(p) {
		outchars("Released: ");
		outpiece(p, e);
		n++;
	}
	p = mget(CO, ps, pe, &e);
	if(p) {
		if(n)
			outchars(", ");
		outpiece(p, e);
		n++;
	}
	p = mget(RT, ps, pe, &e);
	if(p) {
		if(n)
			outchars(", ");
		outchars("Running time: ");
		outpiece(p, e);
		n++;
	}
	p = mget(MP, ps, pe, &e);
	if(p) {
		if(n)
			outchars(", ");
		outpiece(p, e);
		n++;
	}
	p = mget(BW, ps, pe, &e);
	if(p) {
		if(n)
			outchars(", ");
		if(*p == 'c' || *p == 'C')
			outchars("Color");
		else
			outchars("B&W");
		n++;
	}
	if(n) {
		outchar('.');
		outnl(1);
	}
	p = mget(VD, ps, pe, &e);
	if(p) {
		outchars("Video: ");
		outpiece(p, e);
		outnl(1);
	}
	p = mget(AU, ps, pe, &e);
	if(p) {
		outchars("By: ");
		moutall2(AU, ps, pe);
		outnl(1);
	}
	p = mget(DR, ps, pe, &e);
	if(p) {
		outchars("Director: ");
		outpiece(p, e);
		outnl(1);
	}
	p = mget(PR, ps, pe, &e);
	if(p) {
		outchars("Producer: ");
		outpiece(p, e);
		outnl(1);
	}
	p = mget(CN, ps, pe, &e);
	if(p) {
		outchars("Cinematograpy: ");
		outpiece(p, e);
		outnl(1);
	}
	p = mget(CR, ps, pe, &e);
	if(p) {
		outchars("Other Credits: ");
		moutall2(CR, ps, pe);
	}
	outnl(2);
	p = mget(CA, ps, pe, &e);
	if(p) {
		outchars("Cast: ");
		moutall2(CA, ps, pe);
	}
	outnl(2);
	p = mget(AW, ps, pe, &e);
	if(p) {
		outchars("Awards: ");
		moutall2(AW, ps, pe);
		outnl(2);
	}
	p = mget(NT, ps, pe, &e);
	if(p) {
		outpiece(p, e);
		outnl(2);
	}
	p = mget(AB, ps, pe, &e);
	if(p) {
		outpiece(p, e);
		outnl(2);
	}
	pn = ps;
	n = 0;
	while((p = mget(TX, pn, pe, &pn)) != 0) {
		if(n++)
			outnl(1);
		outpiece(p, pn);
	}
	outnl(0);
}

int32_t
movienextoff(int32_t fromoff)
{
	int32_t a;
	char *p;

	a = Bseek(bdict, fromoff, 0);
	if(a < 0)
		return -1;
	for(;;) {
		p = Brdline(bdict, '\n');
		if(!p)
			break;
		if(p[0] == '$' && p[1] == '$')
			return (Boffset(bdict) - Blinelen(bdict));
	}
	return -1;
}

void
movieprintkey(void)
{
	Bprint(bout, "No key\n");
}

/*
 * write a comma-separated list of all tag values between b and e
 */
static void
moutall(int tag, char *b, char *e)
{
	char *p, *pn;
	int n;

	n = 0;
	pn = b;
	while((p = mget(tag, pn, e, &pn)) != 0) {
		if(n++)
			outchars(", ");
		outpiece(p, pn);
	}
}

/*
 * like moutall, but values are expected to have form:
 *    field1_field2
 * and we are to output 'field2 (field1)' for each
 * (sometimes field1 has underscores, so search from end)
 */
static void
moutall2(int tag, char *b, char *e)
{
	char *p, *pn, *us, *q;
	int n;

	n = 0;
	pn = b;
	while((p = mget(tag, pn, e, &pn)) != 0) {
		if(n++)
			outchars(", ");
		us = 0;
		for(q = pn - 1; q >= p; q--)
			if(*q == '_') {
				us = q;
				break;
			}
		if(us) {
			/*
			 * Hack to fix cast list Himself/Herself
			 */
			if(strncmp(us + 1, "Himself", 7) == 0 ||
			   strncmp(us + 1, "Herself", 7) == 0) {
				outpiece(p, us);
				outchars(" (");
				outpiece(us + 1, pn);
				outchar(')');
			} else {
				outpiece(us + 1, pn);
				outchars(" (");
				outpiece(p, us);
				outchar(')');
			}
		} else {
			outpiece(p, pn);
		}
	}
}

/*
 * Starting from b, find next line beginning with tagtab[tag].
 * Don't go past e, but assume *e==0.
 * Return pointer to beginning of value (after tag), and set
 * eptr to point at newline that ends the value
 */
static char *
mget(int tag, char *b, char *e, char **eptr)
{
	char *p, *t, *ans;

	if(tag < 0 || tag >= NTAG)
		return 0;
	t = tagtab[tag];
	ans = 0;
	for(p = b;;) {
		p = strchr(p, '\n');
		if(!p || ++p >= e) {
			if(ans)
				*eptr = e - 1;
			break;
		}
		if(!ans) {
			if(p[0] == t[0] && p[1] == t[1])
				ans = p + 3;
		} else {
			if(p[0] != ' ') {
				*eptr = p - 1;
				break;
			}
		}
	}
	return ans;
}
