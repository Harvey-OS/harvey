/*
 * pANS stdio -- tmpfile
 *
 * Bug: contains a critical section.  Two executions by the same
 * user could interleave as follows, both yielding the same file:
 *	access fails
 *			access fails
 *	fopen succeeds
 *			fopen succeeds
 *	unlink succeeds
 *			unlink fails
 * As I read the pANS, this can't reasonably use tmpnam to generate
 * the name, so that code is duplicated.
 */
#include "iolib.h"

static char tmpsmade[FOPEN_MAX][L_tmpnam+1];
static int ntmps = 0;

static void rmtmps(void);

FILE *tmpfile(void){
	FILE *f;
	static char name[]="/tmp/tf0000000000000";
	char *p;
	while(access(name, 0)==0){
		p=name+7;
		while(*p=='9') *p++='0';
		if(*p=='\0') return NULL;
		++*p;
	}
	f=fopen(name, "wb+");
	if(f && ntmps<FOPEN_MAX){
		if(ntmps==0)
			atexit(rmtmps);
		strcpy(tmpsmade[ntmps++], name);
	}
	return f;
}

static void
rmtmps(void)
{
	int i;
	
	for(i=0; i<ntmps; i++)
		remove(tmpsmade[i]);
}
