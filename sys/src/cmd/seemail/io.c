#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include <regexp.h>
#include "dat.h"


char	machineused[64];
char	userused[64];
int	depthused;

/*
 *  translate mail address into a file name
 */
char*
tryfile(char *machine, char *user, int ld)
{
	static char file[128];
	char *cp, *p;
	Biobuf *mf;

	sprint(file, "/lib/face/48x48x%d/.dict", 1<<ld);
	mf = Bopen(file, OREAD);
	if(mf == 0)
		return 0;

	sprint(file, "%s/%s", machine, user);
	while(cp = Brdline(mf, '\n')) {
		cp[Blinelen(mf)-1] = 0;
		p = strchr(cp, ' ');
		if(p == 0)
			continue;
		*p = 0;
		if(strcmp(cp, file) == 0) {
			sprint(file, "/lib/face/48x48x%d/%s", 1<<ld, p+1);
			strncpy(machineused, machine, sizeof(machineused));
			strncpy(userused, user, sizeof(userused));
			depthused = ld;
			Bterm(mf);
			return file;
		}
	}
	Bterm(mf);
	return 0;
}

char*
tryfiles(char *machine, char *user, int ld)
{
	char *cp;

	cp = tryfile(machine, user, ld);
	if(cp)
		return cp;
	if(ld == 1)
		cp = tryfile(machine, user, 0);
	return cp;
}

char*
getfile(char *machine, char *user, int ld)
{
	char *fp;
	Biobuf *mf;
	char *cp, *dp, *ep;
	Reprog *exp;
	char buf[200];
	static char alnum[]=	"abcdefghijklmnopqrstuvwxyz0123456789"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if(*user == 0)
		user = "unknown";
	if(*machine == 0)
		machine = "astro";	/* this is hopelessly provincial */

	fp = tryfiles(machine, user, ld);
	if(fp != 0)
		return fp;

	cp = strchr(machine, '.');
	ep = strrchr(machine, '.');
	while(cp != 0 && cp < ep){	/* try higher domain */
		if((fp = tryfiles(++cp, user, ld)) != 0)
			return fp;
		cp = strchr(cp, '.');
	}

	mf = Bopen("/lib/face/.machinelist", OREAD);
	if(mf == 0)
		return 0;

	while(cp = Brdline(mf, '\n')){
		int clean = 1;
		if(*cp == '#' || (ep=strpbrk(cp, " \t")) == 0)
			continue;
		cp[Blinelen(mf)-1] = 0;
		for(dp=&buf[1]; cp < ep; ){
			if(strchr(alnum, *cp) == 0)
				clean = 0;
			*dp++ = *cp++;
		}
		*dp = '\0';
		ep += strspn(ep, "\t ");

		/*
		 * skip expression processing for a simple compare
		 */
		if(clean && (strcmp(&buf[1], machine) == 0))
			return tryfiles(ep, user, ld);

		if(buf[1] != '^'){
			buf[0] = '^';
			cp = buf;
		} else
			cp = buf+1;
		if(*(dp-1) != '$')
			*dp++ = '$';
		*dp = '\0';

		exp = regcomp(cp);
		if(!exp)
			error("error in regular expression");
		if(regexec(exp, machine, (Resub *)0, 0) != 0){
			free(exp);
			return tryfiles(ep, user, ld);
		}
		free(exp);
	}
	Bterm(mf);
	return 0;
}

void
makelower(char *cp)
{
	int c;

	for(; (c=*cp)!=0; cp++)
		if('A'<=c && c<='Z')
			*cp += 'a'-'A';
}

void
geticon(SRC *into, char *machine, char *user)
{
	Biobuf *fd;
	int x, y;
	char *cp, *file;
	uchar *pp;
	ulong p, q, r;
	int depth;
	static uchar tab1[] = { 0, 85, 170, 255 };

	label = 0;

	makelower(machine);
	makelower(user);
	depth = screen.ldepth;
	if(depth > 1)
		depth = 1;

	file = getfile(machine, user, depth);
	if(file == 0)
		file = getfile(machine, "unknown", depth);
	if(file == 0)
		file = getfile("", "unknown", depth);

	if(file == 0){
    out:
		memset(into, 0, sizeof(SRC));
		label = machine;
		return;
	}

	strcpy(realmachine, machineused);

	fd = Bopen(file, OREAD);
	if(fd == 0)
		goto out;

	if(strcmp(userused, "unknown") == 0)
		label = machine;
	y = 0;
	while((cp=Brdline(fd, '\n')) != 0){
		p = strtoul(cp, &cp, 16);
		q = strtoul(cp+1, &cp, 16);	/* skip ',' */
		r = strtoul(cp+1, 0, 16);	/* skip ',' */
		pp = into->pix[y];
		for(x=0; x<16; x++){
			if(depthused == 1){
				pp[x] = tab1[(p&(3<<(30-2*x)))>>(30-2*x)];
				pp[16+x] = tab1[(q&(3<<(30-2*x)))>>(30-2*x)];
				pp[32+x] = tab1[(r&(3<<(30-2*x)))>>(30-2*x)];
			}else{
				pp[x]    = (p&(1<<(15-x)))?255:0;
				pp[16+x] = (q&(1<<(15-x)))?255:0;
				pp[32+x] = (r&(1<<(15-x)))?255:0;
			}
		}
		if(y++ >= MAXY)
			break;
	}
	Bterm(fd);
}
