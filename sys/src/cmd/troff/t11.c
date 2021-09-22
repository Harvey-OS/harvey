#include "tdef.h"
#include "fns.h"
#include "ext.h"

#define	MAXCH NCHARS		/* maximum number of global char names */
char	*chnames[MAXCH];	/* chnames[n-ALPHABET] -> name of char n */
int	nchnames;		/* number of Cxy names currently seen */

#define	MAXPS	100		/* max number of point sizes */
int	pstab[MAXPS];		/* point sizes */
int	nsizes;			/* number in DESC */

Font	fonts[MAXFONTS+1];	/* font info + ptr to width info */

static int	getlig(char **);

getdesc(char *name)
{
	FILE *fin;
	char *cmd, *s;
	char ln[128];
	char *toks[50];
	int i, v, ntok;

	if ((fin = fopen(name, "r")) == NULL)
		return -1;
	while (fgets(ln, sizeof ln, fin) != NULL) {
		memset(toks, 0, sizeof toks);
		ntok = getfields(ln, toks, nelem(toks), 1, "\r\n\t ");
		if (ntok == 0)
			continue;
		cmd = toks[0];
		if (strcmp(cmd, "res") == 0)
			Inch = atoi(toks[1]);
		else if (strcmp(cmd, "hor") == 0)
			Hor = atoi(toks[1]);
		else if (strcmp(cmd, "vert") == 0)
			Vert = atoi(toks[1]);
		else if (strcmp(cmd, "unitwidth") == 0)
			Unitwidth = atoi(toks[1]);
		else if (strcmp(cmd, "sizes") == 0) {
			nsizes = 0;
			while (toks[nsizes+1] && (v = atoi(toks[nsizes+1])) != 0 &&
			    nsizes < MAXPS)
				pstab[nsizes++] = v;
		} else if (strcmp(cmd, "fonts") == 0) {
			nfonts = atoi(toks[1]);
			for (i = 1; i <= nfonts; i++) {
				s = toks[i+1];
				fontlab[i] = PAIR(s[0], s[1]);
			}
		} else if (strcmp(cmd, "charset") == 0) {  /* add any names */
			/* chew up any remaining lines */
			while (fgets(ln, sizeof ln, fin) != NULL) {
				ntok = getfields(ln, toks, nelem(toks), 1, "\r\n\t ");
				for (i = 0; i < ntok; i++)
					chadd(toks[i], Troffchar, Install);
			}
			break;
		}
		/* else 
			just skip anything else */
	}
	fclose(fin);
	return 1;
}

static int checkfont(char *name)
{		/* in case it's not really a font description file */
		/* really paranoid, but consider \f. */
	FILE *fp;
	char *buf2, *p;
	char buf[300];
	int i, status = -1;

	if ((fp = fopen(name, "r")) == NULL)
		return -1;
	for (i = 1; i <= 10; i++) {
		if (fgets(buf, sizeof buf, fp) == NULL)
			break;
		buf2 = skipspace(buf);
		if (buf2[0] == '#') {
			i--;
			continue;
		}
		p = skipword(buf2);
		*p = '\0';
		if (eq(buf2, "name") || eq(buf2, "fontname") ||
		    eq(buf2, "special") || eq(buf2, "charset")) {
			status = 1;
			break;
		}
	}
	fclose(fp);
	return status;
}

getfont(char *name, int pos)	/* create width tab for font */
{
	FILE *fin;
	Font *ftemp = &fonts[pos];
	Chwid chtemp[MAXCH];
	static Chwid chinit;
	int i, nw, n, wid, kern, code, ntok;
	wchar_t wc;
	char buf[128], ln[300], dummy[32];
	char *toks[30];
	char *ch, *cmd;

	dummy[0] = '\0';
	ch = dummy;
	/* fprintf(stderr, "read font %s onto %d\n", name, pos); */
	if (checkfont(name) == -1)
		return -1;
	if ((fin = fopen(name, "r")) == NULL)
		return -1;
	for (i = 0; i < ALPHABET; i++)
		chtemp[i] = chinit;	/* zero out to begin with */
	ftemp->specfont = ftemp->ligfont = 0;
	ftemp->defaultwidth = ftemp->spacewidth = Inch * Unitwidth / 72 / 3;
					/* should be rounded */
	nw = code = kern = wid = 0;
	while (fgets(ln, sizeof ln, fin) != NULL) {
		memset(toks, 0, sizeof toks);
		ntok = getfields(ln, toks, nelem(toks), 1, "\r\n\t ");
		if (ntok == 0)
			continue;
		cmd = toks[0];
		if (strcmp(cmd, "name") == 0)
			strcpy(ftemp->longname, toks[1]);
		else if (strcmp(cmd, "special") == 0)
			ftemp->specfont = 1;
		else if (strcmp(cmd, "ligatures") == 0)
			ftemp->ligfont = getlig(&toks[1]);
		else if (strcmp(cmd, "spacewidth") == 0)
			ftemp->spacewidth = atoi(toks[1]);
		else if (strcmp(cmd, "defaultwidth") == 0)
			ftemp->defaultwidth = atoi(toks[1]);
		else if (strcmp(cmd, "charset") == 0) {
			nw = ALPHABET;
			while (fgets(buf, sizeof buf, fin) != NULL) {
				memset(toks, 0, sizeof toks);
				ntok = getfields(buf, toks, 5, 1, "\r\n\t ");
				if (ntok >= 4 &&
				    toks[1][0] != '"') { /* genuine new character */
					ch = toks[0];
					wid = atoi(toks[1]);
					kern = atoi(toks[2]);
					code = atoi(toks[3]);  /* dec/oct/hex */
					/* toks[4] is char name */
				}
				/*
				 * otherwise it's a synonym for prev character,
				 * so leave previous values intact.
				 */

				/* decide what kind of alphabet it might come from here */

				if (ch[0] && ch[1] == '\0') {	/* it's ascii */
					n = ch[0] & 0xff; /* origin includes non-graphics */
					chtemp[n].num = n;
				} else if (ch[0] == '\\' && ch[1] == '0') {
					n = strtol(ch+1, 0, 0);	/* \0octal or \0xhex */
					chtemp[n].num = n;
#ifdef UNICODE
				} else if (mbtowc(&wc, ch, strlen(ch)) > 1) {
					chtemp[nw].num = chadd(ch, MBchar, Install);
					n = nw;
					nw++;
#endif	/*UNICODE*/
				} else {
					if (strcmp(ch, "---") == 0) { /* no name */
						snprintf(dummy, sizeof dummy,
							"%d", code);
						ch = dummy;
						n = chadd(ch, Number, Install);
					} else
						n = chadd(ch, Troffchar, Install);
					chtemp[nw].num = n;
					n = nw;
					nw++;
				}
				chtemp[n].wid = wid;
				chtemp[n].kern = kern;
				chtemp[n].code = code;
				/*fprintf(stderr, "font %2.2s char %4.4s num %3d wid %2d code %3d\n",
					ftemp->longname, ch, n, wid, code);
				*/
			}
			break;
		}
	}
	fclose(fin);
	chtemp[' '].wid = ftemp->spacewidth;  /* width of space on this font */
	ftemp->nchars = nw;
	if (ftemp->wp)
		free(ftemp->wp);  /* god help us if this wasn't allocated */
	ftemp->wp = (Chwid *) malloc(nw * sizeof(Chwid));
	if (ftemp->wp == NULL)
		return -1;
	for (i = 0; i < nw; i++)
		ftemp->wp[i] = chtemp[i];
/*
 *	printf("%d chars: ", nw);
 *	for (i = 0; i < nw; i++)
 *		if (ftemp->wp[i].num > 0 && ftemp->wp[i].num < ALPHABET) {
 *			printf("%c %d ", ftemp->wp[i].num, ftemp->wp[i].wid);
 *		else if (i >= ALPHABET)
 *			printf("%d (%s) %d ", ftemp->wp[i].num,
 *				chnames[ftemp->wp[i].num-ALPHABET], ftemp->wp[i].wid);
 *	}
 *	printf("\n");
 */
	return 1;
}

chadd(char *s, int type, int install)	/* add s to global character name table; */
{					/* or just look it up */

	/* a temporary kludge:  store the "type" as the first character */
	/* of the string, so we can remember from whence it came */

	char *p;
	int i;

/* fprintf(stderr, "into chadd %s %c %c\n", s, type, install); /* */
	for (i = 0; i < nchnames; i++)
		/* +1 since type at front */
		if (type == chnames[i][0] && eq(s, chnames[i]+1))
			break;
/* fprintf(stderr, "i %d, nchnames %d\n", i, nchnames); /* */
	if (i < nchnames)	/* found same type and bytes at position i */
		return ALPHABET + i;
	else if (install == Lookup)  /* not found, and we were just looking */
		return -1;

	chnames[nchnames] = p = (char *) malloc(strlen(s)+1+1);	/* type + \0 */
	if (p == NULL) {
		ERROR "out of space adding character %s", s WARN;
		return LEFTHAND;
	}
	if (nchnames >= NCHARS - ALPHABET) {
		ERROR "out of table space adding character %s", s WARN;
		return LEFTHAND;
	}
	strcpy(chnames[nchnames]+1, s);
	chnames[nchnames][0] = type;
/* fprintf(stderr, "installed %c%s at %d\n", type, s, nchnames); /* */
	return nchnames++ + ALPHABET;
}

char *chname(int n)	/* return string for char with index n */
{			/* includes type char at front, to be peeled off elsewhere */
	if (n >= ALPHABET && n < nchnames + ALPHABET)
		return chnames[n-ALPHABET];
	else
		return "";
}

static int
getlig(char **toks)	/* pick up ligature list. e.g., "ligatures ff fi 0" */
{
	int lig, i;
	char *temp;

	lig = 0;
	for (i = 0; toks[i] && strcmp(toks[i], "0") != 0; i++) {
		temp = toks[i];
		if (strcmp(temp, "fi") == 0)
			lig |= LFI;
		else if (strcmp(temp, "fl") == 0)
			lig |= LFL;
		else if (strcmp(temp, "ff") == 0)
			lig |= LFF;
		else if (strcmp(temp, "ffi") == 0)
			lig |= LFFI;
		else if (strcmp(temp, "ffl") == 0)
			lig |= LFFL;
		else
			fprintf(stderr, "illegal ligature %s ignored\n", temp);
	}
	return lig;
}
