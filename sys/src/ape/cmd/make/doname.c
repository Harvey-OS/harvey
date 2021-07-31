#include "defs.h"

static int docom1(char *, int, int, int, int);
static void expand(depblkp);

/*  BASIC PROCEDURE.  RECURSIVE.  */

/*
p->done = 0   don't know what to do yet
p->done = 1   file in process of being updated
p->done = 2   file already exists in current state
p->done = 3   file make failed
*/

int
doname(nameblkp p, int reclevel, time_t *tval, int nowait)
{
int errstat;
int okdel1;
int didwork;
int len;
time_t td, td1, tdep, ptime, ptime1;
depblkp q;
depblkp qtemp, suffp, suffp1;
nameblkp p1, p2;
struct shblock *implcom, *explcom;
lineblkp lp;
lineblkp lp1, lp2;
char sourcename[100], prefix[100], temp[100], concsuff[20];
char *stem;
char *pnamep, *p1namep;
chainp allchain, qchain;
char qbuf[QBUFMAX], tgsbuf[QBUFMAX];
wildp wp;
int nproc1;
char *lastslash, *s;

if(p == 0)
	{
	*tval = 0;
	return 0;
	}

if(dbgflag)
	{
	printf("doname(%s,%d)\n",p->namep,reclevel);
	fflush(stdout);
	}

if(p->done > 0)
	{
	*tval = p->modtime;
	return (p->done == 3);
	}

errstat = 0;
tdep = 0;
implcom = 0;
explcom = 0;
ptime = exists(p->namep);
ptime1 = 0;
didwork = NO;
p->done = 1;	/* avoid infinite loops */
nproc1 = nproc;	/* current depth of process stack */

qchain = NULL;
allchain = NULL;

/* define values of Bradford's $$@ and $$/ macros */
for(s = lastslash = p->namep; *s; ++s)
	if(*s == '/')
		lastslash = s;
setvar("$@", p->namep, YES);
setvar("$/", lastslash, YES);


/* expand any names that have embedded metacharacters */

for(lp = p->linep ; lp ; lp = lp->nxtlineblock)
	for(q = lp->depp ; q ; q=qtemp )
		{
		qtemp = q->nxtdepblock;
		expand(q);
		}

/* make sure all dependents are up to date */

for(lp = p->linep ; lp ; lp = lp->nxtlineblock)
	{
	td = 0;
	for(q = lp->depp ; q ; q = q->nxtdepblock)
		if(q->depname)
			{
			errstat += doname(q->depname, reclevel+1, &td1, q->nowait);
			if(dbgflag)
				printf("TIME(%s)=%ld\n",q->depname->namep, td1);
			if(td1 > td)
				td = td1;
			if(ptime < td1)
				qchain = appendq(qchain, q->depname->namep);
			allchain = appendq(allchain, q->depname->namep);
			}
	if(p->septype == SOMEDEPS)
		{
		if(lp->shp)
		     if( ptime<td || (ptime==0 && td==0) || lp->depp==0)
			{
			okdel1 = okdel;
			okdel = NO;
			set3var("@", p->namep);
			setvar("?", mkqlist(qchain,qbuf), YES);
			setvar("^", mkqlist(allchain,tgsbuf), YES);
			qchain = NULL;
			if( !questflag )
				errstat += docom(lp->shp, nowait, nproc1);
			set3var("@", CHNULL);
			okdel = okdel1;
			ptime1 = prestime();
			didwork = YES;
			}
		}

	else	{
		if(lp->shp != 0)
			{
			if(explcom)
				fprintf(stderr, "Too many command lines for `%s'\n",
					p->namep);
			else	explcom = lp->shp;
			}

		if(td > tdep) tdep = td;
		}
	}



/* Look for implicit dependents, using suffix rules */

for(lp = sufflist ; lp ; lp = lp->nxtlineblock)
    for(suffp = lp->depp ; suffp ; suffp = suffp->nxtdepblock)
	{
	pnamep = suffp->depname->namep;
	if(suffix(p->namep , pnamep , prefix))
		{
		(void)srchdir(concat(prefix,"*",temp), NO, (depblkp) NULL);
		for(lp1 = sufflist ; lp1 ; lp1 = lp1->nxtlineblock)
		    for(suffp1=lp1->depp; suffp1 ; suffp1 = suffp1->nxtdepblock)
			{
			p1namep = suffp1->depname->namep;
			if( (p1=srchname(concat(p1namep, pnamep ,concsuff))) &&
			    (p2=srchname(concat(prefix, p1namep ,sourcename))) )
				{
				errstat += doname(p2, reclevel+1, &td, NO);
				if(ptime < td)
					qchain = appendq(qchain, p2->namep);
if(dbgflag) printf("TIME(%s)=%ld\n", p2->namep, td);
				if(td > tdep) tdep = td;
				set3var("*", prefix);
				set3var("<", copys(sourcename));
				for(lp2=p1->linep ; lp2 ; lp2 = lp2->nxtlineblock)
					if(implcom = lp2->shp) break;
				goto endloop;
				}
			}
		}
	}

/* Look for implicit dependents, using pattern matching rules */

len = strlen(p->namep);
for(wp = firstwild ; wp ; wp = wp->next)
	if(stem = wildmatch(wp, p->namep, len) )
		{
		lp = wp->linep;
		for(q = lp->depp; q; q = q->nxtdepblock)
			{
			if(dbgflag>1 && q->depname)
				fprintf(stderr,"check dep of %s on %s\n", p->namep,
					wildsub(q->depname->namep,stem));
			if(q->depname &&
				! chkname(wildsub(q->depname->namep,stem)))
					break;
			}

		if(q)	/* some name not found, go to next line */
			continue;

		for(q = lp->depp; q; q = q->nxtdepblock)
			{
			nameblkp tamep;
			if(q->depname == NULL)
				continue;
			tamep = srchname( wildsub(q->depname->namep,stem));
/*TEMP fprintf(stderr,"check dep %s on %s =>%s\n",p->namep,q->depname->namep,tamep->namep);*/
/*TEMP*/if(dbgflag) printf("%s depends on %s. stem=%s\n", p->namep,tamep->namep, stem);
			errstat += doname(tamep, reclevel+1, &td, q->nowait);
			if(ptime < td)
				qchain = appendq(qchain, tamep->namep);
			allchain = appendq(allchain, tamep->namep);
			if(dbgflag) printf("TIME(%s)=%ld\n", tamep->namep, td);
			if(td > tdep)
				tdep = td;
			set3var("<", copys(tamep->namep) );
			}
		set3var("*", stem);
		setvar("%", stem, YES);
		implcom = lp->shp;
		goto endloop;
		}

endloop:


if(errstat==0 && (ptime<tdep || (ptime==0 && tdep==0) ) )
	{
	ptime = (tdep>0 ? tdep : prestime() );
	set3var("@", p->namep);
	setvar("?", mkqlist(qchain,qbuf), YES);
	setvar("^", mkqlist(allchain,tgsbuf), YES);
	if(explcom)
		errstat += docom(explcom, nowait, nproc1);
	else if(implcom)
		errstat += docom(implcom, nowait, nproc1);
	else if(p->septype == 0)
		if(p1=srchname(".DEFAULT"))
			{
			set3var("<", p->namep);
			for(lp2 = p1->linep ; lp2 ; lp2 = lp2->nxtlineblock)
				if(implcom = lp2->shp)
					{
					errstat += docom(implcom, nowait,nproc1);
					break;
					}
			}
		else if(keepgoing)
			{
			printf("Don't know how to make %s\n", p->namep);
			++errstat;
			}
		else
			fatal1(" Don't know how to make %s", p->namep);

	set3var("@", CHNULL);
	if(noexflag || nowait || (ptime = exists(p->namep)) == 0 )
		ptime = prestime();
	}

else if(errstat!=0 && reclevel==0)
	printf("`%s' not remade because of errors\n", p->namep);

else if(!questflag && reclevel==0  &&  didwork==NO)
	printf("`%s' is up to date.\n", p->namep);

if(questflag && reclevel==0)
	exit(ndocoms>0 ? -1 : 0);

p->done = (errstat ? 3 : 2);
if(ptime1 > ptime)
	ptime = ptime1;
p->modtime = ptime;
*tval = ptime;
return errstat;
}

docom(struct shblock *q, int nowait, int nproc1)
{
char *s;
int ign, nopr, doit;
char string[OUTMAX];

++ndocoms;
if(questflag)
	return NO;

if(touchflag)
	{
	s = varptr("@")->varval;
	if(!silflag)
		printf("touch(%s)\n", s);
	if(!noexflag)
		touch(YES, s);
	return NO;
	}

if(nproc1 < nproc)
	waitstack(nproc1);

for( ; q ; q = q->nxtshblock )
	{
	subst(q->shbp,string);
	ign = ignerr;
	nopr = NO;
	doit = NO;
	for(s = string ; ; ++s)
		{
		switch(*s)
			{
			case '-':
				ign = YES;
				continue;
			case '@':
				nopr = YES;
				continue;
			case '+':
				doit = YES;
				continue;
			default:
				break;
			}
		break;
		}

	if( docom1(s, ign, nopr, doit||!noexflag, nowait&&!q->nxtshblock) && !ign)
		return YES;
	}
return NO;
}


static int
docom1(char *comstring, int nohalt, int noprint, int doit, int nowait)
{
int status;
char *prefix;

if(comstring[0] == '\0')
	return 0;

if(!silflag && (!noprint || !doit) )
	prefix = doit ? prompt : "" ;
else
	prefix = CHNULL;

if(dynmacro(comstring) || !doit)
	{
	if(prefix)
		{
		fputs(prefix, stdout);
		puts(comstring);	/* with a newline */
		fflush(stdout);
		}
	return 0;
	}

status = dosys(comstring, nohalt, nowait, prefix);
baddirs();	/* directories may have changed */
return status;
}


/*
   If there are any Shell meta characters in the name,
   expand into a list, after searching directory
*/

static void
expand(depblkp q)
{
char *s;
char *s1;
depblkp p;

s1 = q->depname->namep;
for(s=s1 ; ;) switch(*s++)
	{
	case '\0':
		return;

	case '*':
	case '?':
	case '[':
		if( p = srchdir(s1 , YES, q->nxtdepblock) )
			{
			q->nxtdepblock = p;
			q->depname = 0;
			}
		return;
	}
}
