/* Copyright 1990, AT&T Bell Labs */

#include "fsort.h"

#define	NF	15		/* 0 is global, 1 to NF-1 are field specs */
#define	NP	30		/* NP-1 is largest permitted field number */

extern	void	modifiers(Field*, char*, int);
extern	void	globalmods(Field*);

Field	fields[NF] =
{
	{ tcode, ident, all, 0, 0, 0, 0, { 0, 0 }, { NP, 0 } }
};
int	nfields = 0;

int	tab;
int	signedrflag;
int	simplekeyed;

#define	isdigit(c)	((c)<='9'&&(c)>='0')
#define	blank(p)	(*(p)==' ' || *(p)=='\t')

	/* interpret 0, 1, or 2 arguments and return how many */

int
fieldarg(char *argv1, char *argv2)
{
	char *av1 = argv1;
	char *av2 = argv2;
	Field *field;

	if(av1[0] == '+' && isdigit(av1[1])) {
		if(++nfields >= NF)
			fatal("too many fields", argv1, 0);
		field = &fields[nfields];
		field->end.fieldno = NP;

		field->begin.fieldno = atoi(++av1);
		if(field->begin.fieldno >= NP)
			fatal("field number too big", argv1, 0);
		while(isdigit(*av1))
			av1++;
		if(*av1 == '.') {
			field->begin.charno = atoi(++av1);
			while(isdigit(*av1))
				av1++;
		}
		modifiers(field, av1, 0);

		if(av2==0 || av2[0]!='-' || !isdigit(av2[1]))
			return 1;
		field->end.fieldno = atoi(++av2);
		while(isdigit(*av2))
			av2++;
		if(*av2== '.') {
			field->end.charno = atoi(++av2);
			while(isdigit(*av2))
				av2++;;
		}
		if(field->begin.fieldno > field->end.fieldno ||
		   field->begin.fieldno == field->end.fieldno &&
		   field->begin.charno >= field->end.charno)
			warn("empty field", argv1, 0);
		modifiers(field, av2, 1);
		return 2;
	} else
	if(av1[0] != '-' || av1[1] == 0)
		return 0;
	if(nfields > 0)
		warn("field precedes global option", av1, 0);
	modifiers(fields, &av1[1], 0);
	return 1;
}

/* keyed = 1 if there are fields present (+ options) or if
   numeric (-ng), translation (-f) or deletion (-idb) options
   are present.  In these cases, a separate key is constructed
   for rsort.  The key, however is not carried on 
   intermediate files.  (It would be interesting to try.)
   It must be reconstructed for the merge phase, and that
   may be expensive, since relatively few comparisons
   happen in that phase.  singlekeyed = 1 if there are options,
   so that pure ascii comparison won't work, but no fields, no
   months, no numerics.
 */

void
fieldwrapup(void)
{
	int i;

	for(i=1; i<=nfields; i++)
		globalmods(&fields[i]);
	signedrflag = fields->rflag? -1: 1; /* used only by merge.c*/
	simplekeyed = nfields==0 && fields->coder==tcode 
		      && (fields->trans!=ident || fields->keep!=all);
	if(nfields==0 && !keyed)	/* used only by rsort.c */
		rflag = fields->rflag;
	if(nfields > 0)
		keyed = 1;
	fields[nfields].lflag = 1;
}

void
modifiers(Field *field, char *argv1, int eflag)
{
	int trans = 0;
	int keep = 0;
	int nontext = 0;

	for( ; *argv1; argv1++) {
		switch(*argv1) {
		case 'c':
			cflag = 1;
			continue;
		case 'm':
			mflag = 1;
			continue;
		case 'u':
			uflag = 1;
			continue;
		case 'b':
			if(eflag)
				field->eflag = 1;
			else
				field->bflag = 1;
			continue;
		case 'r':
			field->rflag = 1;
			continue;
		case 'f':
			field->trans = fold;
			trans++;
			break;
		case 'd':
			field->keep = dict;
			keep++;
			break;
		case 'i':
			field->keep = ascii;
			keep++;
			break;
		case 'g':
			warn("option -g taken as -n", "", 0);
		case 'n':
			field->coder = ncode;
			nontext++;
			break;
		case 'M':
			field->coder = Mcode;
			nontext++;
			break;
		case 't':
			if(*(++argv1) == 0)
				fatal("no -t character", "" ,0);
			else
				tab = *argv1;
			continue;
		default:
			fatal("unknown option", argv1, 1);
		}
		keyed = 1;
	}
	if(nontext&&(trans||keep) || trans>1 || keep>1 || nontext>1)
		warn("duplicate or conflicting options", "", 0);
}

void
globalmods(Field *field)
{
	int flagged = 0;

	if(!field->coder)
		field->coder = tcode;
	else
		flagged++;
	if(!field->trans)
		field->trans = ident;
	else
		flagged++;
	if(!field->keep)
		field->keep = all;
	else
		flagged++;
	if(!flagged && !field->bflag && !field->rflag) {
		field->coder = fields->coder;
		field->trans = fields->trans;
		field->keep = fields->keep;
		field->bflag = fields->bflag;
		field->rflag = fields->rflag;
	}
}

int
fieldcode(uchar *dp, uchar *kp, int len, uchar *bound)
{
	uchar *posns[NP+1];	/* field start positions */
	uchar *cp;
	Field *field;
	uchar *op = kp;
	uchar *ep;
	int np, i;

	posns[0] = dp;
	if(tab)
		for(np=1, i=len, cp=dp; i>0 && np<NP; cp++, i--) {
			if(*cp != tab)
				continue;
			posns[np++] = cp;
		}
	else
		for(np=1, i=len, cp=dp; i>0 && np<NP; ) {
			while(blank(cp) && i>0)
				cp++, i--;
			while(!blank(cp) && i>0)
				cp++, i--;
			posns[np++] = cp;
		}

	if(nfields > 0)
		field = &fields[1];
	else
		field = &fields[0];
	i = 1;
	do {
		int t = field->begin.fieldno;
		uchar *xp = dp + len;

		if(t < np) {
			cp = posns[t];
			if(tab && t)
				cp++;
			if(field->bflag && nfields>0)
				while(cp<xp && blank(cp))
					cp++;
			cp += field->begin.charno;
			if(cp > xp)
				cp = xp;
			else
			if(cp < dp)
				cp = dp;
		} else
			cp = xp;
		t = field->end.fieldno;
		if(t < np) {
			int skipped = 0;

			ep = posns[t];
			if(field->eflag) {
				if(tab && t && blank(ep+1))
					ep++, skipped++;
				while(ep<xp && blank(ep))
					ep++;
			}
			ep += field->end.charno;
			if(tab && field->end.charno && t && !skipped)
				ep ++;
			if(ep >	xp)
				ep = xp;
			else
			if(ep < cp)
				ep = cp;
		} else
			ep = xp;
		t = ep - cp;
		if(op+room(t) > bound)
			return -1;
		op += (*field->coder)(cp, op, ep-cp, field);
		field++;
	} while(++i <= nfields);
	return op - kp;
}

/* Encode text field subject to options -r -fdi -b.
   Fields are separated by 0 (or 255 if rflag is set)
           the anti-ambiguity stuff prevents such codes from
   happening otherwise by coding real zeros and ones
   as 0x0101 and 0x0102, and similarly for complements
 */

int
tcode(uchar *dp, uchar *kp, int len, Field *f)
{
	uchar *cp = kp;
	uchar *keep = f->keep;
	uchar *trans = f->trans;
	int c, reverse = f->rflag? ~0: 0;

	while(--len >= 0) {
		c = *dp++;
		if(keep[c]) {
			c = trans[c];
			if(c <= 1) {	/* anti-ambiguity */
				*cp++ = 1^reverse;
				c++;
			} else
			if(c >= 254) {
				*cp++ = 255^reverse;
				c--;
			}
			*cp++ = c^reverse;
		}
	}
	*cp++ = reverse;
	return cp - kp;
}

static
char*	month[] =
{
	"jan", "feb", "mar", "apr", "may", 
	"jun", "jul", "aug", "sep", "oct", "nov", "dec"
};

int
Mcode(uchar *dp, uchar *kp, int len, Field *f)
{
	int i, j = -1;
	uchar *cp;

	for(; len>0; dp++, len--) {
		if(*dp!=' ' && *dp!='\t')
			break;
	}
	if(len >= 3)
		while(++j < 12) {
			cp = (uchar*)month[j];
			for(i=0; i<3; i++)
				if((dp[i]|('a'-'A')) != *cp++)
					break;
			if(i >= 3)
				break;
		}
	*kp = j>=12? 0: j+1;
	if(f->rflag)
		*kp ^= ~0;
	return 1;
}
