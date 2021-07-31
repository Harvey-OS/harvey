#include <u.h>
#include <libc.h>
int ntrap;
#define	GLOB	((char)0200)
char *syssigname[]={
	"exit",		/* can't happen */
	"hangup",
	"interrupt",
	"quit",		/* can't happen */
	"alarm"
	"sys: bad address",
	"sys: odd address",
	"sys: bad sys call",
	"sys: odd stack",
	"sys: write on closed pipe",
	0
};
void
notifyf(void *a, char *s)
{
	int i;
	for(i=0;syssigname[i];i++) if(strcmp(s, syssigname[i])==0) goto Out;
	if(strncmp(s, "sys: trap:", 10)==0
	|| strncmp(s, "sys: fp:", 8)==0){
	Out:
		ntrap++;
		if(ntrap>=32){
			fprint(2, "junk: Too many traps (trap %s), aborting\n", s);
			abort();
		}
		noted(NCONT);
	}
	fprint(2, "junk: note: %s\n", s);
	noted(NDFLT);
}
void deglob(char *s)
{
	char *t=s;
	do{
		if(*t==GLOB) t++;
		*s++=*t;
	}while(*t++);
}
char bigbuf[2000000];
void main(void){
	char *s;
	for(s=bigbuf;s!=&bigbuf[2000000];s++) *s='a';
	notify(notifyf);
	deglob(bigbuf);
}
