#include "u.h"
#include "lib.h"
#include "kern/dat.h"
#include "kern/fns.h"
#include "user.h"

#include "drawterm.h"

char *argv0;
char *user;

extern int errfmt(Fmt*);
void
sizebug(void)
{
	/*
	 * Needed by various parts of the code.
	 * This is a huge bug.
	 */
	assert(sizeof(char)==1);
	assert(sizeof(short)==2);
	assert(sizeof(ushort)==2);
	assert(sizeof(int)==4);
	assert(sizeof(uint)==4);
	assert(sizeof(long)==4);
	assert(sizeof(ulong)==4);
	assert(sizeof(vlong)==8);
	assert(sizeof(uvlong)==8);
}

int
main(int argc, char **argv)
{
	eve = getuser();
	if(eve == nil)
		eve = "drawterm";

	sizebug();

	osinit();
	procinit0();
	printinit();
	screeninit();

	chandevreset();
	chandevinit();
	quotefmtinstall();

	if(bind("#c", "/dev", MBEFORE) < 0)
		panic("bind #c: %r");
	if(bind("#m", "/dev", MBEFORE) < 0)
		panic("bind #m: %r");
	if(bind("#i", "/dev", MBEFORE) < 0)
		panic("bind #i: %r");
	if(bind("#I", "/net", MBEFORE) < 0)
		panic("bind #I: %r");
	if(bind("#U", "/", MAFTER) < 0)
		panic("bind #U: %r");
	bind("#A", "/dev", MAFTER);

	if(open("/dev/cons", OREAD) != 0)
		panic("open0: %r");
	if(open("/dev/cons", OWRITE) != 1)
		panic("open1: %r");
	if(open("/dev/cons", OWRITE) != 2)
		panic("open2: %r");

	cpumain(argc, argv);
	return 0;
}

char*
getkey(char *user, char *dom)
{
	char buf[1024];

	snprint(buf, sizeof buf, "%s@%s password", user, dom);
	return readcons(buf, nil, 1);
}

char*
findkey(char **puser, char *dom)
{
	char buf[1024], *f[50], *p, *ep, *nextp, *pass, *user;
	int nf, haveproto,  havedom, i;

	for(p=secstorebuf; *p; p=nextp){
		nextp = strchr(p, '\n');
		if(nextp == nil){
			ep = p+strlen(p);
			nextp = "";
		}else{
			ep = nextp++;
		}
		if(ep-p >= sizeof buf){
			print("warning: skipping long line in secstore factotum file\n");
			continue;
		}
		memmove(buf, p, ep-p);
		buf[ep-p] = 0;
		nf = tokenize(buf, f, nelem(f));
		if(nf == 0 || strcmp(f[0], "key") != 0)
			continue;
		pass = nil;
		haveproto = havedom = 0;
		user = nil;
		for(i=1; i<nf; i++){
			if(strncmp(f[i], "user=", 5) == 0)
				user = f[i]+5;
			if(strncmp(f[i], "!password=", 10) == 0)
				pass = f[i]+10;
			if(strncmp(f[i], "dom=", 4) == 0 && strcmp(f[i]+4, dom) == 0)
				havedom = 1;
			if(strcmp(f[i], "proto=p9sk1") == 0)
				haveproto = 1;
		}
		if(!haveproto || !havedom || !pass || !user)
			continue;
		*puser = strdup(user);
		pass = strdup(pass);
		memset(buf, 0, sizeof buf);
		return pass;
	}
	return nil;
}

