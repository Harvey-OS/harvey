/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* lex [-[dynvt]] [file] ... [file] */

/* Copyright 1976, Bell Telephone Laboratories, Inc.,
   written by Eric Schmidt, August 27, 1976   */

# include "ldefs.h"
Biobuf	fout;
int	foutopen;
int	errorf = 1;
int	sect = DEFSECTION;
int	prev = '\n';	/* previous input character */
int	pres = '\n';	/* present input character */
int	peek = '\n';	/* next input character */
uint8_t	*pushptr = pushc;
uint8_t	*slptr = slist;

char	*cname = "/sys/lib/lex/ncform";

int nine;
int ccount = 1;
int casecount = 1;
int aptr = 1;
int nstates = NSTATES, maxpos = MAXPOS;
int treesize = TREESIZE, ntrans = NTRANS;
int yytop;
int outsize = NOUTPUT;
int sptr = 1;
int report = 2;
int debug;		/* 1 = on */
int charc;
int sargc;
char **sargv;
uint8_t buf[520];
int yyline;		/* line number of file */
char *yyfile;		/* filename for error messages */
int eof;
int lgatflg;
int divflg;
int funcflag;
int pflag;
int chset;	/* 1 = char set modified */
Biobuf *fin = 0, *fother;
int fptr;
int *name;
int *left;
int *right;
int *parent;
uint8_t *nullstr;
uint8_t **ptr;
int tptr;
uint8_t pushc[TOKENSIZE];
uint8_t slist[STARTSIZE];
uint8_t **def, **subs, *dchar;
uint8_t **sname, *stchar;
uint8_t *ccl;
uint8_t *ccptr;
uint8_t *dp, *sp;
int dptr;
uint8_t *bptr;		/* store input position */
uint8_t *tmpstat;
int count;
int **foll;
int *nxtpos;
int *positions;
int *gotof;
int *nexts;
uint8_t *nchar;
int **state;
int *sfall;		/* fallback state num */
uint8_t *cpackflg;		/* true if state has been character packed */
int *atable;
int nptr;
uint8_t symbol[NCH];
uint8_t cindex[NCH];
int xstate;
int stnum;
uint8_t match[NCH];
uint8_t extra[NACTIONS];
uint8_t *pchar, *pcptr;
int pchlen = TOKENSIZE;
int32_t rcount;
int *verify, *advance, *stoff;
int scon;
uint8_t *psave;

static void	free1core(void);
static void	free2core(void);
# ifdef DEBUG
static void	free3core(void);
# endif
static void	get1core(void);
static void	get2core(void);
static void	get3core(void);

void
main(int argc, char **argv)
{
	int i;

	ARGBEGIN {
# ifdef DEBUG
		case 'd': debug++; break;
		case 'y': yydebug = TRUE; break;
# endif
		case 't': case 'T':
			Binit(&fout, 1, OWRITE);
			errorf= 2;
			foutopen = 1;
			break;
		case 'v': case 'V':
			report = 1;
			break;
		case 'n': case 'N':
			report = 0;
			break;
		case '9':
			nine = 1;
			break;
		default:
			warning("Unknown option %c", ARGC());
	} ARGEND
	sargc = argc;
	sargv = argv;
	if (argc > 0){
		yyfile = argv[fptr++];
		fin = Bopen(yyfile, OREAD);
		if(fin == 0)
			error ("%s - can't open file: %r", yyfile);
		sargc--;
		sargv++;
	}
	else {
		yyfile = "/fd/0";
		fin = myalloc(sizeof(Biobuf), 1);
		if(fin == 0)
			exits("core");
		Binit(fin, 0, OREAD);
	}
	if(Bgetc(fin) == Beof)		/* no input */
		exits(0);
	Bseek(fin, 0, 0);
	gch();
		/* may be gotten: def, subs, sname, stchar, ccl, dchar */
	get1core();
		/* may be gotten: name, left, right, nullstr, parent, ptr */
	strcpy((char*)sp, "INITIAL");
	sname[0] = sp;
	sp += strlen("INITIAL") + 1;
	sname[1] = 0;
	if(yyparse()) exits("error");	/* error return code */
		/* may be disposed of: def, subs, dchar */
	free1core();
		/* may be gotten: tmpstat, foll, positions, gotof, nexts, nchar, state, atable, sfall, cpackflg */
	get2core();
	ptail();
	mkmatch();
# ifdef DEBUG
	if(debug) pccl();
# endif
	sect  = ENDSECTION;
	if(tptr>0)cfoll(tptr-1);
# ifdef DEBUG
	if(debug)pfoll();
# endif
	cgoto();
# ifdef DEBUG
	if(debug){
		print("Print %d states:\n",stnum+1);
		for(i=0;i<=stnum;i++)stprt(i);
		}
# endif
		/* may be disposed of: positions, tmpstat, foll, state, name, left, right, parent, ccl, stchar, sname */
		/* may be gotten: verify, advance, stoff */
	free2core();
	get3core();
	layout();
		/* may be disposed of: verify, advance, stoff, nexts, nchar,
			gotof, atable, ccpackflg, sfall */
# ifdef DEBUG
	free3core();
# endif
	fother = Bopen(cname,OREAD);
	if(fother == 0)
		error("Lex driver missing, file %s: %r",cname);
	while ( (i=Bgetc(fother)) != Beof)
		Bputc(&fout, i);

	Bterm(fother);
	Bterm(&fout);
	if(
# ifdef DEBUG
		debug   ||
# endif
			report == 1)statistics();
	if (fin)
		Bterm(fin);
	exits(0);	/* success return code */
}

static void
get1core(void)
{
	ccptr =	ccl = myalloc(CCLSIZE,sizeof(*ccl));
	pcptr = pchar = myalloc(pchlen, sizeof(*pchar));
	def = myalloc(DEFSIZE,sizeof(*def));
	subs = myalloc(DEFSIZE,sizeof(*subs));
	dp = dchar = myalloc(DEFCHAR,sizeof(*dchar));
	sname = myalloc(STARTSIZE,sizeof(*sname));
	sp = stchar = myalloc(STARTCHAR,sizeof(*stchar));
	if(ccl == 0 || def == 0 || subs == 0 || dchar == 0 || sname == 0 || stchar == 0)
		error("Too little core to begin");
}

static void
free1core(void)
{
	free(def);
	free(subs);
	free(dchar);
}

static void
get2core(void)
{
	int i;

	gotof = myalloc(nstates,sizeof(*gotof));
	nexts = myalloc(ntrans,sizeof(*nexts));
	nchar = myalloc(ntrans,sizeof(*nchar));
	state = myalloc(nstates,sizeof(*state));
	atable = myalloc(nstates,sizeof(*atable));
	sfall = myalloc(nstates,sizeof(*sfall));
	cpackflg = myalloc(nstates,sizeof(*cpackflg));
	tmpstat = myalloc(tptr+1,sizeof(*tmpstat));
	foll = myalloc(tptr+1,sizeof(*foll));
	nxtpos = positions = myalloc(maxpos,sizeof(*positions));
	if(tmpstat == 0 || foll == 0 || positions == 0 ||
		gotof == 0 || nexts == 0 || nchar == 0 || state == 0 || atable == 0 || sfall == 0 || cpackflg == 0 )
		error("Too little core for state generation");
	for(i=0;i<=tptr;i++)foll[i] = 0;
}

static void
free2core(void)
{
	free(positions);
	free(tmpstat);
	free(foll);
	free(name);
	free(left);
	free(right);
	free(parent);
	free(nullstr);
	free(ptr);
	free(state);
	free(sname);
	free(stchar);
	free(ccl);
}

static void
get3core(void)
{
	verify = myalloc(outsize,sizeof(*verify));
	advance = myalloc(outsize,sizeof(*advance));
	stoff = myalloc(stnum+2,sizeof(*stoff));
	if(verify == 0 || advance == 0 || stoff == 0)
		error("Too little core for final packing");
}
# ifdef DEBUG
static void
free3core(void){
	free(advance);
	free(verify);
	free(stoff);
	free(gotof);
	free(nexts);
	free(nchar);
	free(atable);
	free(sfall);
	free(cpackflg);
}
# endif
void *
myalloc(int a, int b)
{
	void *i;
	i = calloc(a, b);
	if(i==0)
		warning("OOPS - calloc returns a 0");
	return(i);
}

void
yyerror(char *s)
{
	fprint(2, "%s:%d %s\n", yyfile, yyline, s);
}
