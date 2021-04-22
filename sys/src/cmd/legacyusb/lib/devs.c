#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

typedef struct Parg Parg;

enum {
	Ndevs = 32,
	Arglen = 500,
	Nargs = 64,
	Stack = 16 * 1024,
};

struct Parg {
	char*	args;
	Dev*	dev;
	int 	(*f)(Dev*,int,char**);
	Channel*rc;
};

static void
workproc(void *a)
{
	Parg *pa;
	char args[Arglen];
	char *argv[Nargs];
	int argc;
	Channel *rc;
	Dev *d;
	int (*f)(Dev*,int,char**);

	pa = a;
	threadsetname("workproc %s", pa->dev->dir);
	strecpy(args, args+sizeof(args), pa->args);	/* don't leak */
	d = pa->dev;
	f = pa->f;
	rc = pa->rc;
	free(pa->args);
	free(pa);
	argc = tokenize(args, argv, nelem(argv)-1);
	argv[argc] = nil;
	if(f(d, argc, argv) < 0){
		closedev(d);
		fprint(2, "%s: devmain: %r\n", argv0);
		sendul(rc, -1);
		threadexits("devmain: %r");
	}
	sendul(rc, 0);
	threadexits(nil);
	
}

int
matchdevcsp(char *info, void *a)
{
	char sbuf[40];
	int *csps;

	csps = a;
	for(; *csps != 0; csps++){
		snprint(sbuf, sizeof(sbuf), "csp %#08ux", *csps);
		if(strstr(info, sbuf) != nil)
			return 0;
	}
	return -1;
}

int
finddevs(int (*matchf)(char*,void*), void *farg, char** dirs, int ndirs)
{
	int fd, i, n, nd, nr;
	char *nm;
	char dbuf[512], fbuf[40];
	Dir *d;

	fd = open("/dev/usb", OREAD);
	if(fd < 0)
		sysfatal("/dev/usb: %r");
	nd = dirreadall(fd, &d);
	close(fd);
	if(nd < 2)
		sysfatal("/dev/usb: no devs");
	for(i = n = 0; i < nd && n < ndirs; i++){
		nm = d[i].name;
		if(strcmp(nm, "ctl") == 0 || strstr(nm, ".0") == nil)
			continue;
		snprint(fbuf, sizeof(fbuf), "/dev/usb/%s/ctl", nm);
		fd = open(fbuf, OREAD);
		if(fd < 0)
			continue;	/* may be gone */
		nr = read(fd, dbuf, sizeof(dbuf)-1);
		close(fd);
		if(nr < 0)
			continue;
		dbuf[nr] = 0;
		if(strstr(dbuf, "enabled ") != nil && strstr(dbuf, " busy") == nil)
			if(matchf(dbuf, farg) == 0)
				dirs[n++] = smprint("/dev/usb/%s", nm);
	}
	free(d);
	if(usbdebug > 1)
		for(nd = 0; nd < n; nd++)
			fprint(2, "finddevs: %s\n", dirs[nd]);
	return n;
}

void
startdevs(char *args, char *argv[], int argc, int (*mf)(char*, void*),
	void *ma, int (*df)(Dev*, int, char**))
{
	int i, ndirs, ndevs;
	char *dirs[Ndevs];
	char **dp;
	Parg *parg;
	Dev *dev;
	Channel *rc;

	if(access("/dev/usb", AEXIST) < 0 && bind("#u", "/dev", MBEFORE) < 0)
		sysfatal("#u: %r");

	if(argc > 0){
		ndirs = argc;
		dp = argv;
	}else{
		dp = dirs;
		ndirs = finddevs(mf, ma, dp, Ndevs);
		if(ndirs == nelem(dirs))
			fprint(2, "%s: too many devices\n", argv0);
	}
	ndevs = 0;
	rc = chancreate(sizeof(ulong), 0);
	if(rc == nil)
		sysfatal("no memory");
	for(i = 0; i < ndirs; i++){
		fprint(2, "%s: startdevs: opening #%d %s\n", argv0, i, dp[i]);
		dev = opendev(dp[i]);
		if(dev == nil)
			fprint(2, "%s: %s: %r\n", argv0, dp[i]);
		else if(configdev(dev) < 0){
			fprint(2, "%s: %s: config: %r\n", argv0, dp[i]);
			closedev(dev);
		}else{
			dprint(2, "%s: %U", argv0, dev);
			parg = emallocz(sizeof(Parg), 0);
			parg->args = estrdup(args);
			parg->dev = dev;
			parg->rc = rc;
			parg->f = df;
			proccreate(workproc, parg, Stack);
			if(recvul(rc) == 0)
				ndevs++;
		}
		if(dp != argv)
			free(dirs[i]);
	}
	chanfree(rc);
	if(ndevs == 0)
		sysfatal("no unhandled devices found");
}
