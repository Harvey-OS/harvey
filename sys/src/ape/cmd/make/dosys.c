#include "defs.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static int	metas(char *);
static int	waitproc(int *);
static int	doshell(char *, int);
static int	doexec(char *);

int
dosys(char *comstring, int nohalt, int nowait, char *prefix)
{
int status;
struct process *procp;

/* make sure there is room in the process stack */
if(nproc >= MAXPROC)
	waitstack(MAXPROC-1);

/* make sure fewer than proclimit processes are running */
while(proclive >= proclimit)
	{
	enbint(SIG_IGN);
	waitproc(&status);
	enbint(intrupt);
	}

if(prefix)
	{
	fputs(prefix, stdout);
	fputs(comstring, stdout);
	}

procp = procstack + nproc;
procp->pid = (forceshell || metas(comstring) ) ?
	doshell(comstring,nohalt) : doexec(comstring);
if(procp->pid == -1)
	fatal("fork failed");
procstack[nproc].nohalt = nohalt;
procstack[nproc].nowait = nowait;
procstack[nproc].done = NO;
++proclive;
++nproc;

if(nowait)
	{
	printf(" &%d\n", procp->pid);
	fflush(stdout);
	return 0;
	}
if(prefix)
	{
	putchar('\n');
	fflush(stdout);
	}
return waitstack(nproc-1);
}

static int
metas(char *s)   /* Are there are any  Shell meta-characters? */
{
char c;

while( (funny[c = *s++] & META) == 0 )
	;
return( c );
}

static void
doclose(void)	/* Close open directory files before exec'ing */
{
struct dirhd *od;

for (od = firstod; od; od = od->nxtdirhd)
	if(od->dirfc)
		closedir(od->dirfc);
}

/*  wait till none of the processes in the stack starting at k is live */
int
waitstack(int k)
{
int npending, status, totstatus;
int i;

totstatus = 0;
npending = 0;
for(i=k ; i<nproc; ++i)
	if(! procstack[i].done)
		++npending;
enbint(SIG_IGN);
if(dbgflag > 1)
	printf("waitstack(%d)\n", k);

while(npending>0 && proclive>0)
	{
	if(waitproc(&status) >= k)
		--npending;
	totstatus |= status;
	}

if(nproc > k)
	nproc = k;
enbint(intrupt);
return totstatus;
}

static int
waitproc(int *statp)
{
pid_t pid;
int status;
int i;
struct process *procp;
char junk[50];
static int inwait = NO;

if(inwait)	/* avoid infinite recursions on errors */
	return MAXPROC;
inwait = YES;

pid = wait(&status);
if(dbgflag > 1)
	fprintf(stderr, "process %d done, status = %d\n", pid, status);
if(pid == -1)
	{
	if(errno == ECHILD)	/* multiple deaths, no problem */
		{
		if(proclive)
			{
			for(i=0, procp=procstack; i<nproc; ++i, ++procp)
				procp->done = YES;
			proclive = nproc = 0;
			}
		return MAXPROC;
		}
	fatal("bad wait code");
	}
for(i=0, procp=procstack; i<nproc; ++i, ++procp)
	if(procp->pid == pid)
		{
		--proclive;
		procp->done = YES;

		if(status)
			{
			if(procp->nowait)
				printf("%d: ", pid);
			if( WEXITSTATUS(status) )
				printf("*** Error code %d", WEXITSTATUS(status) );
			else	printf("*** Termination code %d", WTERMSIG(status));
		
			printf(procp->nohalt ? "(ignored)\n" : "\n");
			fflush(stdout);
			if(!keepgoing && !procp->nohalt)
				fatal(CHNULL);
			}
		*statp = status;
		inwait = NO;
		return i;
		}

sprintf(junk, "spurious return from process %d", pid);
fatal(junk);
/*NOTREACHED*/
return -1;
}

static int
doshell(char *comstring, int nohalt)
{
pid_t pid;

if((pid = fork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();

	execl(SHELLCOM, "sh", (nohalt ? "-c" : "-ce"), comstring, NULL);
	fatal("Couldn't load Shell");
	}

return pid;
}

static int
doexec(char *str)
{
char *t, *tend;
char **argv;
char **p;
int nargs;
pid_t pid;

while( *str==' ' || *str=='\t' )
	++str;
if( *str == '\0' )
	return(-1);	/* no command */

nargs = 1;
for(t = str ; *t ; )
	{
	++nargs;
	while(*t!=' ' && *t!='\t' && *t!='\0')
		++t;
	if(*t)	/* replace first white space with \0, skip rest */
		for( *t++ = '\0' ; *t==' ' || *t=='\t'  ; ++t)
			;
	}

/* now allocate args array, copy pointer to start of each string,
   then terminate array with a null
*/
p = argv = (char **) ckalloc(nargs*sizeof(char *));
tend = t;
for(t = str ; t<tend ; )
	{
	*p++ = t;
	while( *t )
		++t;
	do	{
		++t;
		} while(t<tend && (*t==' ' || *t=='\t') );
	}
*p = NULL;
/*TEMP  for(p=argv; *p; ++p)printf("arg=%s\n", *p);*/

if((pid = fork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();
	enbint(intrupt);
	execvp(str, argv);
	printf("\n");
	fatal1("Cannot load %s",str);
	}

free( (char *) argv);
return pid;
}

void
touch(int force, char *name)
{
struct stat stbuff;
char junk[1];
int fd;

if( stat(name,&stbuff) < 0)
	if(force)
		goto create;
	else
		{
		fprintf(stderr, "touch: file %s does not exist.\n", name);
		return;
		}

if(stbuff.st_size == 0)
	goto create;

if( (fd = open(name, O_RDWR)) < 0)
	goto bad;

if( read(fd, junk, 1) < 1)
	{
	close(fd);
	goto bad;
	}
lseek(fd, 0L, SEEK_SET);
if( write(fd, junk, 1) < 1 )
	{
	close(fd);
	goto bad;
	}
close(fd);
return;

bad:
	fprintf(stderr, "Cannot touch %s\n", name);
	return;

create:
	if( (fd = creat(name, 0666)) < 0)
		goto bad;
	close(fd);
}
