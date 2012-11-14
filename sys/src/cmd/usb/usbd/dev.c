/*
 * Framework for USB devices.
 * Some of them may be embedded into usbd and some of
 * them may exist as /bin/usb/* binaries on their own.
 *
 * When embedded, devmain() is given a ref of an already
 * configured and open Dev. If devmain()
 * does not fail it should release this ref when done and
 * use incref to add further refs to it.
 */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbd.h"

static Lock masklck;
extern Devtab devtab[];
static char* cputype;

int
getdevnb(uvlong *maskp)
{
	int i;

	lock(&masklck);
	for(i = 0; i < 8 * sizeof *maskp; i++)
		if((*maskp & (1ULL<<i)) == 0){
			*maskp |= 1ULL<<i;
			unlock(&masklck);
			return i;
		}
	unlock(&masklck);
	return -1;
}

void
putdevnb(uvlong *maskp, int id)
{
	lock(&masklck);
	if(id >= 0)
		*maskp &= ~(1ULL<<id);
	unlock(&masklck);
}
		
static int
cspmatch(Devtab *dt, int dcsp)
{
	int	i;
	int	csp;

	for(i = 0; i < nelem(dt->csps); i++)
		if((csp=dt->csps[i]) != 0)
		if(csp == dcsp)
			return 1;
		else if((csp&DCL) && (csp&~DCL) == Class(dcsp))
			return 1;
	return 0;
}

static int
devmatch(Devtab *dt, Usbdev *d)
{
	int i;
	int c;
	Conf *cp;

	if(dt->noauto)
		return 0;
	if(dt->vid != -1 && d->vid != dt->vid)
		return 0;
	if(dt->did != -1 && d->did != dt->did)
		return 0;
	if(cspmatch(dt, d->csp))
		return 1;
	for(c = 0; c < Nconf; c++)
		if((cp=d->conf[c]) != nil)
			for(i = 0; i < Niface; i++)
				if(cp->iface[i] != nil)
					if(cspmatch(dt, cp->iface[i]->csp))
						return 1;
	return 0;
}

/* We can't use procexec to execute drivers, because
 * procexec mounts #| at /mnt/temp and we do *not*
 * have /mnt/temp at boot time.
 * Instead, we use access to guess if we can execute the file.
 * and reply as procexec. Be careful that the child inherits
 * all the shared state of the thread library. It should run unnoticed.
 */
static void
xexec(Channel *c, char *nm, char *args[])
{
	int	pid;

	if(access(nm, AEXEC) == 0){
		pid = rfork(RFFDG|RFREND|RFPROC);
		switch(pid){
		case 0:
			exec(nm, args);
			_exits("exec");
		case -1:
			break;
		default:
			sendul(c, pid);
			threadexits(nil);
		}
	}
}

typedef struct Sarg Sarg;
struct Sarg{
	Port *pp;
	Devtab* dt;
	Channel*rc;
	char fname[80];
	char	args[128];
	char	*argv[40];
};

static void
startdevproc(void *a)
{
	Sarg	*sa = a;
	Dev	*d;
	Devtab *dt;
	int	argc;
	char *args, *argse, **argv;
	char *fname;

	threadsetgrp(threadid());
	d = sa->pp->dev;
	dt = sa->dt;
	args = sa->args;
	argse = sa->args + sizeof sa->args;
	argv = sa->argv;
	fname = sa->fname;
	sa->pp->devmaskp = &dt->devmask;
	sa->pp->devnb = getdevnb(&dt->devmask);
	if(sa->pp->devnb < 0){
		sa->pp->devmaskp = nil;
		sa->pp->devnb = 0;
	}else
		args = seprint(args, argse, "-N %d", sa->pp->devnb);
	if(dt->args != nil)
		seprint(args, argse, " %s", dt->args);
	args = sa->args;
	dprint(2, "%s: start: %s %s\n", argv0, dt->name, args);
	argv[0] = dt->name;
	argc = 1;
	if(args[0] != 0)
		argc += tokenize(args, argv+1, nelem(sa->argv)-2);
	argv[argc] = nil;
	if(dt->init == nil){
		if(d->dfd > 0 ){
			close(d->dfd);
			d->dfd = -1;
		}
		rfork(RFCFDG);
		open("/dev/null", OREAD);
		open("/dev/cons", OWRITE);
		open("/dev/cons", OWRITE);

		xexec(sa->rc, argv[0], argv);
		snprint(fname, sizeof(sa->fname), "/bin/usb/%s", dt->name);
		xexec(sa->rc, fname, argv);
		snprint(fname, sizeof(sa->fname), "/boot/%s", dt->name);
		xexec(sa->rc, fname, argv);
		if(cputype == nil)
			cputype = getenv("cputype");
		if(cputype != nil){
			snprint(fname, sizeof(sa->fname), "/%s/bin/%s",
				cputype, dt->name);
			argv[0] = fname;
			xexec(sa->rc, fname, argv);
		}
		fprint(2, "%s: %s: not found. can't exec\n", argv0, dt->name);
		sendul(sa->rc, -1);
		threadexits("exec");
	}else{
		sa->pp->dev = opendev(d->dir);
		sendul(sa->rc, 0);
		if(dt->init(d, argc, argv) < 0)
			fprint(2, "%s: %s: %r\n", argv0, dt->name);
		closedev(d);
		free(sa);
	}
	threadexits(nil);
}

static void
writeinfo(Dev *d)
{
	char buf[128];
	char *s;
	char *se;
	Usbdev *ud;
	Conf *c;
	Iface *ifc;
	int i, j;

	ud = d->usb;
	s = buf;
	se = buf+sizeof(buf);
	s = seprint(s, se, "info %s csp %#08ulx", classname(ud->class), ud->csp);
	for(i = 0; i < ud->nconf; i++){
		c = ud->conf[i];
		if(c == nil)
			break;
		for(j = 0; j < nelem(c->iface); j++){
			ifc = c->iface[j];
			if(ifc == nil)
				break;
			if(ifc->csp != ud->csp)
				s = seprint(s, se, " csp %#08ulx", ifc->csp);
		}
	}
	s = seprint(s, se, " vid %06#x did %06#x", ud->vid, ud->did);
	seprint(s, se, " %q %q", ud->vendor, ud->product);
	devctl(d, "%s", buf);
}

int
startdev(Port *pp)
{
	Dev *d;
	Usbdev *ud;
	Devtab *dt;
	Sarg *sa;
	Channel *rc;

	d = pp->dev;
	assert(d);
	ud = d->usb;
	assert(ud != nil);

	writeinfo(d);

	if(ud->class == Clhub){
		/*
		 * Hubs are handled directly by this process avoiding
		 * concurrent operation so that at most one device
		 * has the config address in use.
		 * We cancel kernel debug for these eps. too chatty.
		 */
		pp->hub = newhub(d->dir, d);
		if(pp->hub == nil)
			fprint(2, "%s: %s: %r\n", argv0, d->dir);
		else
			fprint(2, "usb/hub... ");
		if(usbdebug > 1)
			devctl(d, "debug 0");	/* polled hubs are chatty */
		return pp->hub == nil ? -1 : 0;
	}

	for(dt = devtab; dt->name != nil; dt++)
		if(devmatch(dt, ud))
			break;
	/*
	 * From here on the device is for the driver.
	 * When we return pp->dev contains a Dev just for us
	 * with only the ctl open. Both devs are released on the last closedev:
	 * driver's upon I/O errors and ours upon port dettach.
	 */
	if(dt->name == nil){
		dprint(2, "%s: no configured entry for %s (csp %#08lx)\n",
			argv0, d->dir, ud->csp);
		close(d->dfd);
		d->dfd = -1;
		return 0;
	}
	sa = emallocz(sizeof(Sarg), 1);
	sa->pp = pp;
	sa->dt = dt;
	rc = sa->rc = chancreate(sizeof(ulong), 1);
	procrfork(startdevproc, sa, Stack, RFNOTEG);
	if(recvul(rc) != 0)
		free(sa);
	chanfree(rc);
	fprint(2, "usb/%s... ", dt->name);

	sleep(Spawndelay);		/* in case we re-spawn too fast */
	return 0;
}
