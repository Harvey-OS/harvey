/* POSIX DEPENDENT PROCEDURES */
#include "defs.h"
#include <sys/stat.h>
#include <ar.h>

#define NAMESPERBLOCK	32

/* DEFAULT RULES FOR POSIX */

char *dfltmacro[] =
	{
	".SUFFIXES : .o .c .y .l .a .sh .f",
	"MAKE=make",
	"AR=ar",
	"ARFLAGS=rv",
	"YACC=yacc",
	"YFLAGS=",
	"LEX=lex",
	"LFLAGS=",
	"LDFLAGS=",
	"CC=c89",
	"CFLAGS=-O",
	"FC=fort77",
	"FFLAGS=-O 1",
	0 };

char *dfltpat[] =
	{
	"%.o : %.c",
	"\t$(CC) $(CFLAGS) -c $<",

	"%.o : %.y",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c y.tab.c",
	"\trm y.tab.c",
	"\tmv y.tab.o $@",

	"%.o : %.l",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c lex.yy.c",
	"\trm lex.yy.c",
	"\tmv lex.yy.o $@",

	"%.c : %.y",
	"\t$(YACC) $(YFLAGS) $<",
	"\tmv y.tab.c $@",

	"%.c : %.l",
	"\t$(LEX) $(LFLAGS) $<",
	"\tmv lex.yy.c $@",

	"% : %.o",
	"\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<",

	"% : %.c",
	"\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<",

	0 };



char *dfltsuff[] =
	{
	".SUFFIXES : .o .c .y .l .a .sh .f",
	".c.o :",
	"\t$(CC) $(CFLAGS) -c $<",

	".f.o :",
	"\t$(FC) $(FFLAGS) -c $<",

	".y.o :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c y.tab.c",
	"\trm -f y.tab.c",
	"\tmv y.tab.o $@",

	".l.o :",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c lex.yy.c",
	"\trm -f lex.yy.c",
	"\tmv lex.yy.o $@",

	".y.c :",
	"\t$(YACC) $(YFLAGS) $<",
	"\tmv y.tab.c $@",

	".l.c :",
	"\t$(LEX) $(LFLAGS) $<",
	"\tmv lex.yy.c $@",

	".c.a:",
	"\t$(CC) -c $(CFLAGS) $<",
	"\t$(AR) $(ARFLAGS) $@ $*.o",
	"\trm -f $*.o",

	".f.a:",
	"\t$(FC) -c $(FFLAGS) $<",
	"\t$(AR) $(ARFLAGS) $@ $*.o",
	"\trm -f $*.o",

	".c:",
	"\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<",

	".f:",
	"\t$(FC) $(FFLAGS) $(LDFLAGS) -o $@ $<",

	".sh:",
	"\tcp $< $@",
	"\tchmod a+x $@",

	0 };


static struct dirhd	*opdir(char *, int);
static void		cldir(struct dirhd *, int);
static int		amatch(char *, char *);
static int		umatch(char *, char *);
static void		clarch(void);
static int		openarch(char *);
static int		getarch(void);

time_t
exists(char *filename)
{
struct stat buf;
char *s;

for(s = filename ; *s!='\0' && *s!='(' &&  *s!=')' ; ++s)
	;

if(*s != '\0')
	return lookarch(filename);

if(stat(filename,&buf) < 0) 
	return 0;
else	return buf.st_mtime;
}


time_t
prestime(void)
{
time_t t;
time(&t);
return t;
}

static char nmtemp[MAXNAMLEN+1];	/* guarantees a null after the name */
static char *tempend = nmtemp + MAXNAMLEN;



depblkp
srchdir(char *pat, int mkchain, depblkp nextdbl)
{
DIR *dirf;
struct dirhd *dirptr;
char *dirname, *dirpref, *endir, *filepat, *p, temp[100];
char fullname[100];
nameblkp q;
depblkp thisdbl;
struct pattern *patp;

struct dirent *dptr;

thisdbl = 0;

if(mkchain == NO)
	for(patp=firstpat ; patp ; patp = patp->nxtpattern)
		if(equal(pat, patp->patval)) return 0;

patp = ALLOC(pattern);
patp->nxtpattern = firstpat;
firstpat = patp;
patp->patval = copys(pat);

endir = 0;

for(p=pat; *p!='\0'; ++p)
	if(*p=='/') endir = p;

if(endir==0)
	{
	dirname = ".";
	dirpref = "";
	filepat = pat;
	}
else	{
	dirname = pat;
	*endir = '\0';
	dirpref = concat(dirname, "/", temp);
	filepat = endir+1;
	}

dirptr = opdir(dirname,YES);
dirf = dirptr->dirfc;

for( dptr = readdir(dirf) ; dptr ; dptr = readdir(dirf) )
	{
	char *p1, *p2;
	p1 = dptr->d_name;
	p2 = nmtemp;
	while( (p2<tempend) && (*p2++ = *p1++)!='\0')
		;
	if( amatch(nmtemp,filepat) )
		{
		concat(dirpref,nmtemp,fullname);
		if( (q=srchname(fullname)) ==0)
			q = makename(copys(fullname));
		if(mkchain)
			{
			thisdbl = ALLOC(depblock);
			thisdbl->nxtdepblock = nextdbl;
			thisdbl->depname = q;
			nextdbl = thisdbl;
			}
		}
	}


if(endir)
	*endir = '/';

cldir(dirptr, YES);

return thisdbl;
}

static struct dirhd *
opdir(char *dirname, int stopifbad)
{
struct dirhd *od;

for(od = firstod; od; od = od->nxtdirhd)
	if(equal(dirname, od->dirn) )
		break;

if(od == NULL)
	{
	++nopdir;
	od = ALLOC(dirhd);
	od->nxtdirhd = firstod;
	firstod = od;
	od->dirn = copys(dirname);
	}

if(od->dirfc==NULL && (od->dirfc = opendir(dirname)) == NULL && stopifbad)
	{
	fprintf(stderr, "Directory %s: ", dirname);
	fatal("Cannot open");
	}

return od;
}


static void
cldir(struct dirhd *dp, int used)
{
if(nopdir >= MAXDIR)
	{
	closedir(dp->dirfc);
	dp->dirfc = NULL;
	}
else if(used)
	rewinddir(dp->dirfc); /* start over at the beginning  */
}

/* stolen from glob through find */

static int
amatch(char *s, char *p)
{
	int cc, scc, k;
	int c, lc;

	scc = *s;
	lc = 077777;
	switch (c = *p) {

	case '[':
		k = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (k)
					return amatch(++s, ++p);
				else
					return 0;

			case '-':
				k |= (lc <= scc)  & (scc <= (cc=p[1]) ) ;
			}
			if (scc==(lc=cc)) k++;
		}
		return 0;

	case '?':
	caseq:
		if(scc) return amatch(++s, ++p);
		return 0;
	case '*':
		return umatch(s, ++p);
	case 0:
		return !scc;
	}
	if (c==scc) goto caseq;
	return 0;
}

static int
umatch(char *s, char *p)
{
	if(*p==0) return 1;
	while(*s)
		if (amatch(s++,p)) return 1;
	return 0;
}

#ifdef METERFILE
#include <pwd.h>
int meteron	= 0;	/* default: metering off */

extern void meter(char *file)
{
time_t tvec;
char *p;
FILE * mout;
struct passwd *pwd;

if(file==0 || meteron==0) return;

pwd = getpwuid(getuid());

time(&tvec);

if( mout = fopen(file,"a") )
	{
	p = ctime(&tvec);
	p[16] = '\0';
	fprintf(mout, "User %s, %s\n", pwd->pw_name, p+4);
	fclose(mout);
	}
}
#endif


/* look inside archives for notation a(b)
	a(b)	is file member   b   in archive a
*/

static long arflen;
static long arfdate;
static char arfname[16];
FILE *arfd;
long int arpos, arlen;

time_t
lookarch(char *filename)
{
char *p, *q, *send, s[15], pad;
int i, nc, nsym;

for(p = filename; *p!= '(' ; ++p)
	;

*p = '\0';
if( ! openarch(filename) )
	{
	*p = '(';
	return 0L;
	}
*p++ = '(';
nc = 14;
pad = ' ';

send = s + nc;
for( q = s ; q<send && *p!='\0' && *p!=')' ; *q++ = *p++ )
	;
if(p[0]==')' && p[1]!='\0')	/* forbid stuff after the paren */
	{
	clarch();
	return 0L;
	}
while(q < send)
	*q++ = pad;
while(getarch())
	{
	if( !strncmp(arfname, s, nc))
		{
		clarch();
/*TEMP fprintf(stderr, "found archive member %14s, time=%d\n", s, arfdate); */
		return arfdate;
		}
	}

clarch();
return  0L;
}

static void
clarch(void)
{
fclose( arfd );
}

static int
openarch(char *f)
{
char magic[SARMAG];
int word;
struct stat buf;
nameblkp p;

stat(f, &buf);
arlen = buf.st_size;

arfd = fopen(f, "r");
if(arfd == NULL)
	return NO;
	/* fatal1("cannot open %s", f); */

fread( (char *) &word, sizeof(word), 1, arfd);

fseek(arfd, 0L, 0);
fread(magic, SARMAG, 1, arfd);
arpos = SARMAG;
if( strncmp(magic, ARMAG, SARMAG) )
	fatal1("%s is not an archive", f);

if( !(p = srchname(f)) )
	p = makename( copys(f) );
p->isarch = YES;
arflen = 0;
return YES;
}


static int
getarch(void)
{
struct ar_hdr arhead;

arpos += (arflen + 1) & ~1L;	/* round archived file length up to even */
if(arpos >= arlen)
	return 0;
fseek(arfd, arpos, 0);

fread( (char *) &arhead, sizeof(arhead), 1, arfd);
arpos += sizeof(arhead);
arflen = atol(arhead.ar_size);
arfdate = atol(arhead.ar_date);
strncpy(arfname, arhead.ar_name, sizeof(arhead.ar_name));
return 1;
}

/* find the directory containing name.
   read it into the hash table if it hasn't been used before or if
   if might have changed since last reference
*/

void
dirsrch(char *name)
{
DIR *dirf;
struct dirhd *dirp;
time_t dirt, objt;
int dirused, hasparen;
char *dirname, *lastslash;
char *fullname, *filepart, *fileend, *s;
struct dirent *dptr;

lastslash = NULL;
hasparen = NO;

for(s=name; *s; ++s)
	if(*s == '/')
		lastslash = s;
	else if(*s=='(' || *s==')')
		hasparen = YES;

if(hasparen)
	{
	if(objt = lookarch(name))
		makename(name)->modtime = objt;
	return;
	}

if(lastslash)
	{
	dirname = name;
	*lastslash = '\0';
	}
else
	dirname = ".";

dirused = NO;
dirp = opdir(dirname, NO);
dirf = dirp->dirfc;
if(dirp->dirok || !dirf)
	goto ret;
dirt = exists(dirname);
if(dirp->dirtime == dirt)
	goto ret;

dirp->dirok = YES;
dirp->dirtime = dirt;
dirused = YES;

/* allocate buffer to hold full file name */
if(lastslash)
	{
	fullname = (char *) ckalloc(strlen(dirname)+MAXNAMLEN+2);
	concat(dirname, "/", fullname);
	filepart = fullname + strlen(fullname);
	}
else
	filepart = fullname = (char *) ckalloc(MAXNAMLEN+1);


fileend = filepart + MAXNAMLEN;
*fileend = '\0';
for(dptr = readdir(dirf) ; dptr ; dptr = readdir(dirf) )
	{
	char *p1, *p2;
	p1 = dptr->d_name;
	p2 = filepart;
	while( (p2<fileend) && (*p2++ = *p1++)!='\0')
		;
	if( ! srchname(fullname) )
		(void) makename(copys(fullname));
	}

free(fullname);

ret:
	cldir(dirp, dirused);
	if(lastslash)
		*lastslash = '/';
}



void
baddirs(void)
{
struct dirhd *od;

for(od = firstod; od; od = od->nxtdirhd)
	od->dirok = NO;
}
