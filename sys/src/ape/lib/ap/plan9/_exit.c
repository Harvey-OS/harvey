#include "lib.h"
#include "sys9.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int _finishing = 0;
int _sessleader = 0;

#define CPIDMAX		100	/* for keeping track of child pids */
static int cpids[CPIDMAX+1];
static int ncpid = 0;
static char exitstatus[ERRLEN];

static void killcpids(int);

void
_exit(int status)
{
	_finish(status, 0);
}

void
_finish(int status, char *term)
{
	int i, nalive;
	char *cp;

	if(_finishing)
		_EXITS(exitstatus);
	_finishing = 1;
	if(status){
		cp = _ultoa(exitstatus, status & 0xFF);
		*cp = 0;
	}else if(term){
		strncpy(exitstatus, term, ERRLEN);
	}
	if(_sessleader) {
		for(i = 0, nalive=0; i < ncpid; i++)
			if(cpids[i])
				nalive++;
		if(nalive)
			killcpids(nalive);
	}
	_EXITS(exitstatus);
}

/*
 * register a child pid; will be hup'd on exit or termination
 * if pid < 0, just clear the table (done on fork)
 */

void
_newcpid(int pid)
{
	int i;

	if(pid < 0){
		ncpid = 0;
	}else{
		for(i = 0; i < ncpid; i++)
			if(cpids[i] == 0) {
				cpids[i] = pid;
				return;
			}
		if(ncpid < CPIDMAX)
			cpids[ncpid++] = pid;
	}
}

void
_delcpid(int pid)
{
	int i;

	for(i = 0; i < ncpid; i++)
		if(cpids[i] == pid) {
			cpids[i] = 0;
			if(i == ncpid-1)
				ncpid--;
			return;
		}
}

/*
 * What a pain this is.  Children might not die because
 * we're still here.  So fork to get an independent
 * child, and use it to kill everyone
 */
static void
killcpids(int nalive)
{
	int i, p, fd, niters, wpid;
	Waitmsg w;
	char buf[100];
	char *cp;

	for(i = 0; i<OPEN_MAX; i++)
		_CLOSE(i);
	cpids[ncpid++] = getpid();	/* kill ourselves, too */
	nalive++;
	i = _RFORK(FORKPCS|FORKFDG|FORKNOW);
	if(i > 0)
		return;

	for(niters=0; nalive > 0 && niters < 5; niters++){
		for(i = 0; i < ncpid; i++){
			if(!cpids[i])
				continue;
			strcpy(buf, "/proc/");
			cp = _ultoa(buf+6, cpids[i]);
			strcpy(cp, "/note");
			fd = _OPEN(buf, 1);
			if(fd >= 0){
				if(_WRITE(fd, "hangup", 6) < 0){
					cpids[i] = 0;
					nalive--;
				}
				_CLOSE(fd);
			}else{
				cpids[i] = 0;
				nalive--;
			}
		}
	}
}

/* emulate: return p+sprintf(p, "%uld", v) */
#define IDIGIT 15
char *
_ultoa(char *p, unsigned long v)
{
	char s[IDIGIT];
	int n, i;

	s[IDIGIT-1] = 0;
	for(i = IDIGIT-2; i; i--){
		n = v % 10;
		s[i] = n + '0';
		v = v / 10;
		if(v == 0)
			break;
	}
	strcpy(p, s+i);
	return p + (IDIGIT-1-i);
}
