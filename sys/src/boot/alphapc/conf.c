#include	"u.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"lib.h"

static char *confname[MAXCONF];
static char *confval[MAXCONF];
static int nconf;
static char bootargs[BOOTARGSLEN];

char*
getconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(strcmp(confname[i], name) == 0)
			return confval[i];
	return 0;
}

void
setconf(char *buf)
{
	char *cp, *line[MAXCONF];
	int i, n;

	/*
	 * Keep a pristine copy.
	 * Should change this to pass the parsed strings
	 * to the booted programme instead of the raw
	 * string, then it only gets done once.
	 */
	strcpy(bootargs, buf);
	/* print("boot: stashing /alpha/conf boot args at 0x%lux\n",
		bootargs);			/* DEBUG */
	conf.bootargs = bootargs;

	n = getcfields(buf, line, MAXCONF, "\n");
	for(i = 0; i < n; i++){
		if(*line[i] == '#')
			continue;
		cp = strchr(line[i], '=');
		if(cp == nil)
			continue;
		*cp++ = 0;
		if(cp - line[i] >= NAMELEN+1)
			*(line[i]+NAMELEN-1) = 0;
		confname[nconf] = line[i];
		confval[nconf] = cp;
		nconf++;
	}
}

int
getcfields(char* lp, char** fields, int n, char* sep)
{
	int i;

	for(i = 0; lp && *lp && i < n; i++){
		while(*lp && strchr(sep, *lp) != 0)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && strchr(sep, *lp) == 0){
			if(*lp == '\\' && *(lp+1) == '\n')
				*lp++ = ' ';
			lp++;
		}
	}

	return i;
}
