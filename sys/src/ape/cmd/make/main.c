# include "defs.h"
/*
command make to update programs.
Posix Flags:
	'e'  use environment macros after rather than before makefiles
	'f'  the next argument is the name of the description file;
	     "makefile" is the default
	'i'  ignore error codes from the shell
	'k'  continue to update other targets that don't depend
	     on target if error occurs making a target
	'n'  don't issue, just print, commands
	'p'  print out a version of the input graph
	'q'  don't do anything, but check if object is up to date;
	     returns exit code 0 if up to date, 1 if not
	'r'  clear the builtin suffix list and don't use built-in rules
	's'  silent mode--don't print out commands
	'S'  stop after any command fails (default; opposite of -k)
	't'  touch (update time of) files but don't issue command

Nonposix Flags:
	'd'  print out debugging comments
	'N'  use % patterns instead of old suffix rules
	'Pn' set process limit to n
	'z'  always use shell, never issue commands directly

*/

nameblkp mainname	= NULL;
nameblkp firstname	= NULL;
lineblkp sufflist	= NULL;
struct varblock *firstvar	= NULL;
struct pattern *firstpat	= NULL;
struct dirhd *firstod		= NULL;
wildp firstwild			= NULL;
wildp lastwild			= NULL;
nameblkp *hashtab;
int nhashed;
int hashsize;
int hashthresh;

int proclimit	= PROCLIMIT;
int nproc	= 0;
int proclive	= 0;
struct process procstack[MAXPROC];

int sigivalue	= 0;
int sigqvalue	= 0;

int dbgflag	= NO;
int prtrflag	= NO;
int silflag	= NO;
int noexflag	= NO;
int keepgoing	= NO;
int noruleflag	= NO;
int touchflag	= NO;
int questflag	= NO;
int oldflag	= YES;
int ndocoms	= NO;
int ignerr	= NO;    /* default is to stop on error */
int forceshell	= NO;
int okdel	= YES;
int envlast	= NO;
int inarglist	= NO;
char **envpp	= NULL;

extern char *dfltmacro[];
extern char *dfltpat[];
extern char *dfltsuff[];
extern char **environ;
char **linesptr;

char *prompt	= "";
int nopdir	= 0;
char funny[128];

static void	loadenv(void);
static int	isprecious(char *);
static int	rddescf(char *);
static void	rdarray(char **);
static void	printdesc(int);

void
main(int argc, char **argv)
{
nameblkp p;
int i, j;
int descset, nfargs;
int nowait = NO;
time_t tjunk;
char c, *s, *mkflagp;
static char makeflags[30] = "-";
static char onechar[2] = "X";

descset = 0;
mkflagp = makeflags+1;

funny['\0'] = (META | TERMINAL);
for(s = "=|^();&<>*?[]:$`'\"\\\n" ; *s ; ++s)
	funny[*s] |= META;
for(s = "\n\t :;&>|" ; *s ; ++s)
	funny[*s] |= TERMINAL;


newhash(HASHSIZE);

inarglist = YES;
for(i=1; i<argc; ++i)
	if(argv[i]!=0 && argv[i][0]!='-' && eqsign(argv[i]))
		argv[i] = 0;

setvar("$", "$", NO);
inarglist = NO;

for(i=1; i<argc; ++i)
    if(argv[i]!=0 && argv[i][0]=='-')
	{
	for(j=1 ; (c=argv[i][j])!='\0' ; ++j)  switch(c)
		{
		case 'd':
			++dbgflag;
			*mkflagp++ = 'd';
			break;

		case 'e':
			envlast = YES;
			*mkflagp++ = 'e';
			break;

		case 'f':
			if(i >= argc-1)
			  fatal("No description argument after -f flag");
			if( ! rddescf(argv[i+1]) )
				fatal1("Cannot open %s", argv[i+1]);
			argv[i+1] = 0;
			++descset;
			break;

		case 'i':
			ignerr = YES;
			*mkflagp++ = 'i';
			break;

		case 'k':
			keepgoing = YES;
			*mkflagp++ = 'k';
			break;

		case 'n':
			noexflag = YES;
			*mkflagp++ = 'n';
			break;

		case 'N':
			oldflag = NO;
			*mkflagp++ = 'N';
			break;
	
		case 'p':
			prtrflag = YES;
			break;

		case 'P':
			if(isdigit(argv[i][j+1]))
				{
				proclimit = argv[i][++j] - '0';
				if(proclimit < 1)
					proclimit = 1;
				}
			else
				fatal("illegal proclimit parameter");
			*mkflagp++ = 'P';
			*mkflagp++ = argv[i][j];
			break;

		case 'q':
			questflag = YES;
			*mkflagp++ = 'q';
			break;

		case 'r':
			noruleflag = YES;
			*mkflagp++ = 'r';
			break;

		case 's':
			silflag = YES;
			*mkflagp++ = 's';
			break;

		case 'S':
			keepgoing = NO;
			*mkflagp++ = 'S';
			break;

		case 't':
			touchflag = YES;
			*mkflagp++ = 't';
			break;

		case 'z':
			forceshell = YES;
			*mkflagp++ = 'z';
			break;

		default:
			onechar[0] = c;	/* to make lint happy */
			fatal1("Unknown flag argument %s", onechar);
		}

	argv[i] = NULL;
	}

if(mkflagp > makeflags+1)
	setvar("MAKEFLAGS", makeflags, NO);

if( !descset )
if(	!rddescf("makefile") &&
	!rddescf("Makefile") &&
	(exists(s = "s.makefile") || exists(s = "s.Makefile")) )
		{
		char junk[20];
		concat("get ", s, junk);
		(void) dosys(junk, NO, NO, junk);
		rddescf(s+2);
		unlink(s+2);
		}


if(envlast)
	loadenv();
if(!noruleflag && !oldflag)
	rdarray(dfltpat);

if(prtrflag) printdesc(NO);

if( srchname(".IGNORE") )
	ignerr = YES;
if( srchname(".SILENT") )
	silflag = YES;
if( srchname(".OLDFLAG") )
	oldflag = YES;
if( p=srchname(".SUFFIXES") )
	sufflist = p->linep;
if( !sufflist && !firstwild)
	fprintf(stderr,"No suffix or %% pattern list.\n");
/*
if(sufflist && !oldflag)
	fprintf(stderr, "Suffix lists are old-fashioned. Use %% patterns\n);
*/

	sigivalue = (int) signal(SIGINT, SIG_IGN);
	sigqvalue = (int) signal(SIGQUIT, SIG_IGN);
	enbint(intrupt);

nfargs = 0;

for(i=1; i<argc; ++i)
	if(s = argv[i])
		{
		if((p=srchname(s)) == NULL)
			p = makename(s);
		++nfargs;
		if(i+1<argc && argv[i+1] != 0 && equal(argv[i+1], "&") )
			{
			++i;
			nowait = YES;
			}
		else
			nowait = NO;
		doname(p, 0, &tjunk, nowait);
		if(dbgflag) printdesc(YES);
		}

/*
If no file arguments have been encountered, make the first
name encountered that doesn't start with a dot
*/

if(nfargs == 0)
	if(mainname == 0)
		fatal("No arguments or description file");
	else	{
		doname(mainname, 0, &tjunk, NO);
		if(dbgflag) printdesc(YES);
		}

if(!nowait)
	waitstack(0);
exit(0);
}


void
intrupt(int sig)
{
char *p;

if(okdel && !noexflag && !touchflag &&
	(p = varptr("@")->varval) && exists(p)>0 && !isprecious(p) )
		{
		fprintf(stderr, "\n***  %s removed.", p);
		remove(p);
		}

fprintf(stderr, "\n");
exit(2);
}



static int
isprecious(char *p)
{
lineblkp lp;
depblkp dp;
nameblkp np;

if(np = srchname(".PRECIOUS"))
	for(lp = np->linep ; lp ; lp = lp->nxtlineblock)
		for(dp = lp->depp ; dp ; dp = dp->nxtdepblock)
			if(equal(p, dp->depname->namep))
				return YES;

return NO;
}


void
enbint(void (*k)(int))
{
if(sigivalue == 0)
	signal(SIGINT,k);
if(sigqvalue == 0)
	signal(SIGQUIT,k);
}

static int
rddescf(char *descfile)
{
static int firstrd = YES;

/* read and parse description */

if(firstrd)
	{
	firstrd = NO;
	if( !noruleflag )
		{
		rdarray(dfltmacro);
		if(oldflag)
			rdarray(dfltsuff);
		}
	if(!envlast)
		loadenv();

	}

return  parse(descfile);
}

static void
rdarray(char **s)
{
linesptr = s;
parse(CHNULL);
}

static void
loadenv(void)
{
for(envpp = environ ; *envpp ; ++envpp)
	eqsign(*envpp);
envpp = NULL;
}

static void
printdesc(int prntflag)
{
nameblkp p;
depblkp dp;
struct varblock *vp;
struct dirhd *od;
struct shblock *sp;
lineblkp lp;

if(prntflag)
	{
	printf("Open directories:\n");
	for (od = firstod; od; od = od->nxtdirhd)
		printf("\t%s\n", od->dirn);
	}

if(firstvar != 0) printf("Macros:\n");
for(vp = firstvar; vp ; vp = vp->nxtvarblock)
	printf("\t%s = %s\n" , vp->varname , vp->varval ? vp->varval : "(null)");

for(p = firstname; p; p = p->nxtnameblock)
	{
	printf("\n\n%s",p->namep);
	if(p->linep != 0) printf(":");
	if(prntflag) printf("  done=%d",p->done);
	if(p==mainname) printf("  (MAIN NAME)");
	for(lp = p->linep ; lp ; lp = lp->nxtlineblock)
		{
		if( dp = lp->depp )
			{
			printf("\n depends on:");
			for(; dp ; dp = dp->nxtdepblock)
				if(dp->depname != 0)
					printf(" %s ", dp->depname->namep);
			}
	
		if(sp = lp->shp)
			{
			printf("\n commands:\n");
			for( ; sp ; sp = sp->nxtshblock)
				printf("\t%s\n", sp->shbp);
			}
		}
	}
printf("\n");
fflush(stdout);
}
