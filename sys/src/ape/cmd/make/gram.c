
#line	1	"/sys/src/ape/cmd/make/gram.y"
#include "defs.h"

#line	5	"/sys/src/ape/cmd/make/gram.y"
typedef union 
	{
	struct shblock *yshblock;
	depblkp ydepblock;
	nameblkp ynameblock;
	} YYSTYPE;
extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;

#line	20	"/sys/src/ape/cmd/make/gram.y"
struct depblock *pp;
static struct shblock *prevshp;

static struct nameblock *lefts[NLEFTS];
struct nameblock *leftp;
static int nlefts;

struct lineblock *lp, *lpp;
static struct depblock *prevdep;
static int sepc;
static int allnowait;

static struct fstack
	{
	FILE *fin;
	char *fname;
	int lineno;
	} filestack[MAXINCLUDE];
static int ninclude = 0;
#define	NAME	57346
#define	SHELLINE	57347
#define	START	57348
#define	MACRODEF	57349
#define	COLON	57350
#define	DOUBLECOLON	57351
#define	GREATER	57352
#define	AMPER	57353
#define	AMPERAMPER	57354
#define YYEOFCODE 1
#define YYERRCODE 2

#line	149	"/sys/src/ape/cmd/make/gram.y"


static char *zznextc;	/* null if need another line;
			   otherwise points to next char */
static int yylineno;
static FILE * fin;
static int retsh(char *);
static int nextlin(void);
static int isinclude(char *);

int yyparse(void);

int
parse(char *name)
{
FILE *stream;

if(name == CHNULL)
	{
	stream = NULL;
	name = "(builtin-rules)";
	}
else if(equal(name, "-"))
	{
	stream = stdin;
	name = "(stdin)";
	}
else if( (stream = fopen(name, "r")) == NULL)
	return NO;
filestack[0].fname = copys(name);
ninclude = 1;
fin = stream;
yylineno = 0;
zznextc = 0;

if( yyparse() )
	fatal("Description file error");

if(fin)
	fclose(fin);
return YES;
}

int
yylex(void)
{
char *p;
char *q;
char word[INMAX];

if(! zznextc )
	return nextlin() ;

while( isspace(*zznextc) )
	++zznextc;
switch(*zznextc)
	{
	case '\0':
		return nextlin() ;

	case '|':
		if(zznextc[1]==':')
			{
			zznextc += 2;
			return DOUBLECOLON;
			}
		break;
	case ':':
		if(*++zznextc == ':')
			{
			++zznextc;
			return DOUBLECOLON;
			}
		return COLON;
	case '>':
		++zznextc;
		return GREATER;
	case '&':
		if(*++zznextc == '&')
			{
			++zznextc;
			return AMPERAMPER;
			}
		return AMPER;
	case ';':
		return retsh(zznextc) ;
	}

p = zznextc;
q = word;

while( ! ( funny[*p] & TERMINAL) )
	*q++ = *p++;

if(p != zznextc)
	{
	*q = '\0';
	if((yylval.ynameblock=srchname(word))==0)
		yylval.ynameblock = makename(word);
	zznextc = p;
	return NAME;
	}

else	{
	char junk[100];
	sprintf(junk, "Bad character %c (octal %o), line %d of file %s",
		*zznextc, *zznextc, yylineno, filestack[ninclude-1].fname);
	fatal(junk);
	}
return 0;	/* never executed */
}




static int
retsh(char *q)
{
register char *p;
struct shblock *sp;

for(p=q+1 ; *p==' '||*p=='\t' ; ++p)  ;

sp = ALLOC(shblock);
sp->nxtshblock = NULL;
sp->shbp = (fin ? copys(p) : p );
yylval.yshblock = sp;
zznextc = 0;
return SHELLINE;
}

static int
nextlin(void)
{
static char yytext[INMAX];
static char *yytextl	= yytext+INMAX;
char *text, templin[INMAX];
char c;
char *p, *t;
char lastch, *lastchp;
extern char **linesptr;
int incom;
int kc;

again:

	incom = NO;
	zznextc = 0;

if(fin == NULL)
	{
	if( (text = *linesptr++) == 0)
		return 0;
	++yylineno;
	}

else	{
	for(p = text = yytext ; p<yytextl ; *p++ = kc)
		switch(kc = getc(fin))
			{
			case '\t':
				if(p == yytext)
					incom = YES;
				break;

			case ';':
				incom = YES;
				break;

			case '#':
				if(! incom)
					kc = '\0';
				break;

			case '\n':
				++yylineno;
				if(p==yytext || p[-1]!='\\')
					{
					*p = '\0';
					goto endloop;
					}
				p[-1] = ' ';
				while( (kc=getc(fin))=='\t' || kc==' ' || kc=='\n')
					if(kc == '\n')
						++yylineno;
	
				if(kc != EOF)
					break;
			case EOF:
				*p = '\0';
				if(ninclude > 1)
					{
					register struct fstack *stp;
					fclose(fin);
					--ninclude;
					stp = filestack + ninclude;
					fin = stp->fin;
					yylineno = stp->lineno;
					free(stp->fname);
					goto again;
					}
				return 0;
			}

	fatal("line too long");
	}

endloop:

	if((c = text[0]) == '\t')
		return retsh(text) ;
	
	if(isalpha(c) || isdigit(c) || c==' ' || c=='.'|| c=='_')
		for(p=text+1; *p!='\0'; )
			if(*p == ':')
				break;
			else if(*p++ == '=')
				{
				eqsign(text);
				return MACRODEF;
				}

/* substitute for macros on dependency line up to the semicolon if any */

for(t = yytext ; *t!='\0' && *t!=';' ; ++t)
	;

lastchp = t;
lastch = *t;
*t = '\0';	/* replace the semi with a null so subst will stop */

subst(yytext, templin);		/* Substitute for macros on dependency lines */

if(lastch)	/* copy the stuff after the semicolon */
	{
	*lastchp = lastch;
	strcat(templin, lastchp);
	}

strcpy(yytext, templin);

/* process include files after macro substitution */
if(strncmp(text, "include", 7) == 0) {
 	if (isinclude(text+7))
		goto again;
}

for(p = zznextc = text ; *p ; ++p )
	if(*p!=' ' && *p!='\t')
		return START;
goto again;
}


static int
isinclude(char *s)
{
char *t;
struct fstack *p;

for(t=s; *t==' ' || *t=='\t' ; ++t)
	;
if(t == s)
	return NO;

for(s = t; *s!='\n' && *s!='#' && *s!='\0' ; ++s)
	if(*s == ':')
		return NO;
*s = '\0';

if(ninclude >= MAXINCLUDE)
	fatal("include depth exceeded");
p = filestack + ninclude;
p->fin = fin;
p->lineno = yylineno;
p->fname = copys(t);
if( (fin = fopen(t, "r")) == NULL)
	fatal1("Cannot open include file %s", t);
yylineno = 0;
++ninclude;
return YES;
}


int
yyerror(char *s, ...)
{
char buf[100];

sprintf(buf, "line %d of file %s: %s",
		yylineno, filestack[ninclude-1].fname, s);
fatal(buf);
}
short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 0,
};
#define	YYNPROD	22
#define	YYPRIVATE 57344
#define	YYLAST	21
short	yyact[] =
{
  17,  20,  21,  16,   7,   9,  11,  18,  19,  12,
  13,   5,   2,   1,  10,   3,   4,   8,   6,  14,
  15
};
short	yypact[] =
{
-1000,   9,-1000,   0,-1000,-1000,   1,-1000,  -2,-1000,
  -4, -10,-1000,-1000,-1000,  -3,-1000,-1000,-1000,-1000,
-1000,-1000
};
short	yypgo[] =
{
   0,  20,  19,  18,  17,  14,  13,  12,   6
};
short	yyr1[] =
{
   0,   6,   6,   7,   7,   7,   7,   3,   3,   4,
   4,   5,   5,   5,   5,   5,   8,   8,   2,   2,
   1,   1
};
short	yyr2[] =
{
   0,   0,   2,   1,   1,   4,   1,   1,   2,   0,
   1,   1,   2,   2,   2,   2,   1,   1,   0,   1,
   1,   2
};
short	yychk[] =
{
-1000,  -6,  -7,   6,   7,   2,  -3,   4,  -4,   4,
  -5,  -8,   8,   9,  -2,  -1,   5,   4,  11,  12,
  11,   5
};
short	yydef[] =
{
   1,  -2,   2,   3,   4,   6,   9,   7,  18,   8,
  10,  11,  16,  17,   5,  19,  20,  13,  14,  15,
  12,  21
};
short	yytok1[] =
{
   1
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12
};
long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define	yyclearin	yychar = -1
#define	yyerrok		yyerrflag = 0

#ifdef	yydebug
#include	"y.debug"
#else
#define	yydebug		0
char*	yytoknames[1];		/* for debugging */
char*	yystates[1];		/* for debugging */
#endif

/*	parser for yacc output	*/

int	yynerrs = 0;		/* number of errors */
int	yyerrflag = 0;		/* error recovery flag */

extern	int	fprint(int, char*, ...);
extern	int	sprint(char*, char*, ...);

char*
yytokname(int yyc)
{
	static char x[10];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprintf(x, "<%d>", yyc);
	return x;
}

char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprintf(x, "<%d>\n", yys);
	return x;
}

long
yylex1(void)
{
	long yychar;
	long *t3p;
	int c;

	yychar = yylex();
	if(yychar <= 0) {
		c = yytok1[0];
		goto out;
	}
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
		c = yytok1[yychar];
		goto out;
	}
	if(yychar >= YYPRIVATE)
		if(yychar < YYPRIVATE+sizeof(yytok2)/sizeof(yytok2[0])) {
			c = yytok2[yychar-YYPRIVATE];
			goto out;
		}
	for(t3p=yytok3;; t3p+=2) {
		c = t3p[0];
		if(c == yychar) {
			c = t3p[1];
			goto out;
		}
		if(c == 0)
			break;
	}
	c = 0;

out:
	if(c == 0)
		c = yytok2[1];	/* unknown char */
	if(yydebug >= 3)
		printf("lex %.4X %s\n", yychar, yytokname(c));
	return c;
}

int
yyparse(void)
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	short *yyxi;
	int yyj, yym, yystate, yyn, yyg;
	YYSTYPE save1, save2;
	int save3, save4;
	long yychar;

	save1 = yylval;
	save2 = yyval;
	save3 = yynerrs;
	save4 = yyerrflag;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[-1];
	goto yystack;

ret0:
	yyn = 0;
	goto ret;

ret1:
	yyn = 1;
	goto ret;

ret:
	yylval = save1;
	yyval = save2;
	yynerrs = save3;
	yyerrflag = save4;
	return yyn;

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		printf("char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) { 
		yyerror("yacc stack overflow"); 
		goto ret1; 
	}
	yyp->yys = yystate;
	yyp->yyv = yyval;

yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG)
		goto yydefault; /* simple state */
	if(yychar < 0)
		yychar = yylex1();
	yyn += yychar;
	if(yyn < 0 || yyn >= YYLAST)
		goto yydefault;
	yyn = yyact[yyn];
	if(yychk[yyn] == yychar) { /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if(yyerrflag > 0)
			yyerrflag--;
		goto yystack;
	}

yydefault:
	/* default state action */
	yyn = yydef[yystate];
	if(yyn == -2) {
		if(yychar < 0)
			yychar = yylex1();

		/* look through exception table */
		for(yyxi=yyexca;; yyxi+=2)
			if(yyxi[0] == -1 && yyxi[1] == yystate)
				break;
		for(yyxi += 2;; yyxi += 2) {
			yyn = yyxi[0];
			if(yyn < 0 || yyn == yychar)
				break;
		}
		yyn = yyxi[1];
		if(yyn < 0)
			goto ret0;
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			if(yydebug >= 1) {
				printf("%s", yystatname(yystate));
				printf("saw %s\n", yytokname(yychar));
			}
yyerrlab:
			yynerrs++;

		case 1:
		case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */
			while(yyp >= yys) {
				yyn = yypact[yyp->yys] + YYERRCODE;
				if(yyn >= 0 && yyn < YYLAST) {
					yystate = yyact[yyn];  /* simulate a shift of "error" */
					if(yychk[yystate] == YYERRCODE)
						goto yystack;
				}

				/* the current yyp has no shift onn "error", pop stack */
				if(yydebug >= 2)
					printf("error recovery pops state %d, uncovers %d\n",
						yyp->yys, (yyp-1)->yys );
				yyp--;
			}
			/* there is no state on the stack with an error shift ... abort */
			goto ret1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= YYEOFCODE)
				printf("error recovery discards %s\n", yytokname(yychar));
			if(yychar == YYEOFCODE)
				goto ret1;
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
	if(yydebug >= 2)
		printf("reduce %d in:\n\t%s", yyn, yystatname(yystate));

	yypt = yyp;
	yyp -= yyr2[yyn];
	yyval = (yyp+1)->yyv;
	yym = yyn;

	/* consult goto table to find next state */
	yyn = yyr1[yyn];
	yyg = yypgo[yyn];
	yyj = yyg + yyp->yys + 1;

	if(yyj >= YYLAST || yychk[yystate=yyact[yyj]] != -yyn)
		yystate = yyact[yyg];
	switch(yym) {
		
case 5:
#line	48	"/sys/src/ape/cmd/make/gram.y"
 {
	    while( --nlefts >= 0)
		{
		wildp wp;

		leftp = lefts[nlefts];
		if(wp = iswild(leftp->namep))
			{
			leftp->septype = SOMEDEPS;
			if(lastwild)
				lastwild->next = wp;
			else
				firstwild = wp;
			lastwild = wp;
			}

		if(leftp->septype == 0)
			leftp->septype = sepc;
		else if(leftp->septype != sepc)
			{
			if(! wp)
				fprintf(stderr,
					"Inconsistent rules lines for `%s'\n",
					leftp->namep);
			}
		else if(sepc==ALLDEPS && leftp->namep[0]!='.' && yypt[-0].yyv.yshblock!=0)
			{
			for(lp=leftp->linep; lp->nxtlineblock; lp=lp->nxtlineblock)
			if(lp->shp)
				fprintf(stderr,
					"Multiple rules lines for `%s'\n",
					leftp->namep);
			}

		lp = ALLOC(lineblock);
		lp->nxtlineblock = NULL;
		lp->depp = yypt[-1].yyv.ydepblock;
		lp->shp = yypt[-0].yyv.yshblock;
		if(wp)
			wp->linep = lp;

		if(equal(leftp->namep, ".SUFFIXES") && yypt[-1].yyv.ydepblock==0)
			leftp->linep = 0;
		else if(leftp->linep == 0)
			leftp->linep = lp;
		else	{
			for(lpp = leftp->linep; lpp->nxtlineblock;
				lpp = lpp->nxtlineblock) ;
				if(sepc==ALLDEPS && leftp->namep[0]=='.')
					lpp->shp = 0;
			lpp->nxtlineblock = lp;
			}
		}
	} break;
case 7:
#line	105	"/sys/src/ape/cmd/make/gram.y"
 { lefts[0] = yypt[-0].yyv.ynameblock; nlefts = 1; } break;
case 8:
#line	106	"/sys/src/ape/cmd/make/gram.y"
 { lefts[nlefts++] = yypt[-0].yyv.ynameblock;
	    	if(nlefts>=NLEFTS) fatal("Too many lefts"); } break;
case 9:
#line	111	"/sys/src/ape/cmd/make/gram.y"
{
		char junk[100];
		sprintf(junk, "%s:%d", filestack[ninclude-1].fname, yylineno);
		fatal1("Must be a separator on rules line %s", junk);
		} break;
case 11:
#line	119	"/sys/src/ape/cmd/make/gram.y"
 { prevdep = 0;  yyval.ydepblock = 0; allnowait = NO; } break;
case 12:
#line	120	"/sys/src/ape/cmd/make/gram.y"
 { prevdep = 0; yyval.ydepblock = 0; allnowait = YES; } break;
case 13:
#line	121	"/sys/src/ape/cmd/make/gram.y"
 {
			  pp = ALLOC(depblock);
			  pp->nxtdepblock = NULL;
			  pp->depname = yypt[-0].yyv.ynameblock;
			  pp->nowait = allnowait;
			  if(prevdep == 0) yyval.ydepblock = pp;
			  else  prevdep->nxtdepblock = pp;
			  prevdep = pp;
			  } break;
case 14:
#line	130	"/sys/src/ape/cmd/make/gram.y"
 { if(prevdep) prevdep->nowait = YES; } break;
case 16:
#line	134	"/sys/src/ape/cmd/make/gram.y"
 { sepc = ALLDEPS; } break;
case 17:
#line	135	"/sys/src/ape/cmd/make/gram.y"
 { sepc = SOMEDEPS; } break;
case 18:
#line	138	"/sys/src/ape/cmd/make/gram.y"
 {yyval.yshblock = 0; } break;
case 19:
#line	139	"/sys/src/ape/cmd/make/gram.y"
 { yyval.yshblock = yypt[-0].yyv.yshblock; } break;
case 20:
#line	142	"/sys/src/ape/cmd/make/gram.y"
 { yyval.yshblock = yypt[-0].yyv.yshblock;  prevshp = yypt[-0].yyv.yshblock; } break;
case 21:
#line	143	"/sys/src/ape/cmd/make/gram.y"
 { yyval.yshblock = yypt[-1].yyv.yshblock;
			prevshp->nxtshblock = yypt[-0].yyv.yshblock;
			prevshp = yypt[-0].yyv.yshblock;
			} break;
	}
	goto yystack;  /* stack new state and value */
}
