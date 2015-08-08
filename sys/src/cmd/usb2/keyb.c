/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Usbkeyb Usbkeyb;
typedef struct Dirtab Dirtab;

enum { Qdir = 0,
       Qkeyb,
       Qmouse,
       Qmax,
};

struct Usbkeyb {
	Usbkeyb* next;
	Dev* dev;
	char name[32];
	int pid;
};

struct Dirtab {
	char* name;
	Qid qid;
	int64_t length;
	int32_t mode;
};

static uint32_t time0;
static Usbkeyb* keybs;
static Dirtab dirtab[] = {
    {".", {Qdir, 0, QTDIR}, 0, 0555},
    {"keyb", {Qkeyb, 0, 0}, 0, 0444},
    {"mouse", {Qmouse, 0, 0}, 0, 0444},
};

static int
keybdirgen(int qidpath, Dir* dirp, void* v)
{
	if(qidpath >= Qmax)
		return -1;
	memset(dirp, 0, sizeof dirp[0]);
	dirp->qid = dirtab[qidpath].qid;
	dirp->name = estrdup9p(dirtab[qidpath].name);
	dirp->mode = dirtab[qidpath].mode;
	dirp->atime = time0;
	dirp->mtime = time0;
	dirp->uid = estrdup9p("");
	dirp->gid = estrdup9p("");
	dirp->muid = estrdup9p("");
	return 0;
}

static void
keybattach(Req* r)
{
	char* spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]) {
		respond(r, "invalid attach specifier");
		return;
	}
	r->ofcall.qid = dirtab[Qdir].qid;
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

static char*
keybwalk1(Fid* fid, char* name, Qid* qid)
{
	int i;
	if(fid->qid.path != Qdir)
		return "fswalk1: bad qid";
	for(i = 1; i < Qmax; i++) {
		if(!strcmp(dirtab[i].name, name)) {
			fid->qid = dirtab[i].qid;
			*qid = dirtab[i].qid;
			return nil;
		}
	}
	return "no such file or directory";
}

static void
keybstat(Req* r)
{
	if(keybdirgen((int)r->fid->qid.path, &r->d, nil) == -1)
		respond(r, "bad qid");
	respond(r, nil);
}

static void
keybread(Req* r)
{
	int qidpath;

	qidpath = r->fid->qid.path;
	switch(qidpath) {
	case Qdir:
		dirread9p(r, keybdirgen, nil);
		respond(r, nil);
		return;
	default:
		respond(r, "not implemented");
		return;
	}
}

static void
keybwrite(Req* r)
{
	respond(r, "prohibition ale");
}

static void
keybopen(Req* r)
{
	if(r->fid->qid.path >= Qmax) {
		respond(r, "bad qid");
		return;
	}
	if(r->ifcall.mode != OREAD)
		respond(r, "permission denied");

	respond(r, nil);
}

Srv keybsrv = {
    .attach = keybattach,
    .open = keybopen,
    .read = keybread,
    .write = keybwrite,
    .stat = keybstat,
    .walk1 = keybwalk1,
};

static void
usage(void)
{
	fprint(2, "usage: usb/keyb [-D] [-m mtpt] [-S srvname]\n");
	exits("usage");
}

static int
cmdreq(Dev* d, int type, int req, int value, int index, uint8_t* data,
       int count)
{
	int ndata, n;
	uint8_t* wp;
	uint8_t buf[8];
	char* hd, *rs;

	assert(d != nil);
	if(data == nil) {
		wp = buf;
		ndata = 0;
	} else {
		ndata = count;
		wp = emallocz(8 + ndata, 0);
	}
	wp[0] = type;
	wp[1] = req;
	PUT2(wp + 2, value);
	PUT2(wp + 4, index);
	PUT2(wp + 6, count);
	if(data != nil)
		memmove(wp + 8, data, ndata);
	if(usbdebug > 2) {
		hd = hexstr(wp, ndata + 8);
		rs = reqstr(type, req);
		fprint(2, "%s: %s val %d|%d idx %d cnt %d out[%d] %s\n", d->dir,
		       rs, value >> 8, value & 0xFF, index, count, ndata + 8,
		       hd);
		free(hd);
	}
	n = write(d->dfd, wp, 8 + ndata);
	if(wp != buf)
		free(wp);
	if(n < 0)
		return -1;
	if(n != 8 + ndata) {
		dprint(2, "%s: cmd: short write: %d\n", argv0, n);
		return -1;
	}
	return n;
}

static int
cmdrep(Dev* d, void* buf, int nb)
{
	char* hd;

	nb = read(d->dfd, buf, nb);
	if(nb > 0 && usbdebug > 2) {
		hd = hexstr(buf, nb);
		fprint(2, "%s: in[%d] %s\n", d->dir, nb, hd);
		free(hd);
	}
	return nb;
}

int
usbcmd(Dev* d, int type, int req, int value, int index, uint8_t* data,
       int count)
{
	int i, r, nerr;
	char err[64];

	/*
	 * Some devices do not respond to commands some times.
	 * Others even report errors but later work just fine. Retry.
	 */
	r = -1;
	*err = 0;
	for(i = nerr = 0; i < Uctries; i++) {
		if(type & Rd2h)
			r = cmdreq(d, type, req, value, index, nil, count);
		else
			r = cmdreq(d, type, req, value, index, data, count);
		if(r > 0) {
			if((type & Rd2h) == 0)
				break;
			r = cmdrep(d, data, count);
			if(r > 0)
				break;
			if(r == 0)
				werrstr("no data from device");
		}
		nerr++;
		if(*err == 0)
			rerrstr(err, sizeof(err));
		sleep(Ucdelay);
	}
	if(r > 0 && i >= 2)
		/* let the user know the device is not in good shape */
		fprint(2, "%s: usbcmd: %s: required %d attempts (%s)\n", argv0,
		       d->dir, i, err);
	return r;
}

int
loaddevdesc(Dev* d)
{
	uint8_t buf[Ddevlen + 255];
	int nr;
	int type;
	Ep* ep0;

	type = Rd2h | Rstd | Rdev;
	nr = sizeof(buf);
	memset(buf, 0, Ddevlen);
	if((nr = usbcmd(d, type, Rgetdesc, Ddev << 8 | 0, 0, buf, nr)) < 0)
		return -1;
	/*
	 * Several hubs are returning descriptors of 17 bytes, not 18.
	 * We accept them and leave number of configurations as zero.
	 * (a get configuration descriptor also fails for them!)
	 */
	if(nr < Ddevlen) {
		print("%s: %s: warning: device with short descriptor\n", argv0,
		      d->dir);
		if(nr < Ddevlen - 1) {
			werrstr("short device descriptor (%d bytes)", nr);
			return -1;
		}
	}
	d->usb = emallocz(sizeof(Usbdev), 1);
	ep0 = mkep(d->usb, 0);
	ep0->dir = Eboth;
	ep0->type = Econtrol;
	ep0->maxpkt = d->maxpkt = 8; /* a default */
	nr = parsedev(d, buf, nr);
	if(nr >= 0) {
		d->usb->vendor = loaddevstr(d, d->usb->vsid);
		if(strcmp(d->usb->vendor, "none") != 0) {
			d->usb->product = loaddevstr(d, d->usb->psid);
			d->usb->serial = loaddevstr(d, d->usb->ssid);
		}
	}
	return nr;
}

static void
keybscanproc(void)
{
	static char path[256];
	static char buf[256];
	Keyb* keyb, **kpp;
	Dir* dir, *dirs;
	int i, nrd, ndirs;
	int fd, waitfd;
	int pid;

	snprint(path, sizeof path, "/proc/%d/wait", getpid());
	waitfd = open(path, OREAD);
	if(waitfd == -1)
		sysfatal("open %s", path);

	for(;;) {
		fd = open("/dev/usb", OREAD);
		if(fd == -1)
			sysfatal("/dev/usb: %r");
		ndirs = dirreadall(fd, &dirs);
		close(fd);

		kpp = &keybs;
		for(i = 0; i < ndirs; i++) {
			dir = dirs + i;
			if(strcmp(dir->name, "ctl") == 0 ||
			   strstr(dir->name, ".0") == nil)
				continue;
			snprint(path, sizeof path, "/dev/usb/%s/ctl",
			        dir->name);
			fd = open(path, ORDWR);
			if(fd == -1)
				continue; /* went away */
			nrd = read(fd, buf, sizeof buf - 1);
			close(fd);
			if(nrd == -1)
				continue; /* went away */
			buf[nrd - 1] = '\0';
			if(strstr(buf, "enabled ") != nil &&
			   strstr(buf, " busy") == nil) {
				/* is it a keyboard? */
				if(strstr(buf, "csp 0x010103")) {

					for(keyb = keybs; keyb != nil;
					    keyb = keyb->next)
						if(!strcmp(keyb->name,
						           dir->name))
							break;
					if(keyb != nil)
						continue; /* already in use */

					keyb = mallocz(sizeof keyb[0], 1);
					snprint(path, sizeof path,
					        "/dev/usb/%s/ctl", dir->name);
					strncpy(keyb->name, dir->name);
					keyb->name[sizeof keyb->name - 1] =
					    '\0';
					keyb->dev = dev;

					keyb->ep = openep(keyb->dev, ep->id);
					if(keyb->ep == nil) {
						fprint(
						    2,
						    "keyb: %s: openep %d: %r\n",
						    dev->dir, ep->id);
						closedev(dev);
						free(keyb);
						continue;
					}

					switch(pid = rfork(RFPROC | RFMEM)) {
					case -1:
						fprint(2, "rfork failed. keep "
						          "moving...");
						free(keyb);
						continue;
					case 0:
						keybproc(keyb);
						exits(nil);
					default:
						keyb->pid = pid;
						break;
					}
					*kpp = keyb;
					kpp = &keyb->next;
				}
			}
		}
		free(dirs);

		/* collect dead children */
		for(;;) {
			dir = dirfstat(waitfd);
			if(dir == nil)
				sysfatal("dirfstat waitfd %s", path);
			if(dir->length > 0) {
				nrd = read(waitfd, buf, sizeof buf - 1);
				if(nrd == -1) {
					fprint(2, "read waitfd");
					break;
				}
				buf[nrd] = '\0';
				pid = atoi(fld[0]);
				for(kpp = &keybs; *kpp != nil;
				    kpp = &(*kpp)->next) {
					keyb = *kpp;
					if(keyb->pid == pid) {
						close(keyb->ctlfd);
						if(keyb->datafd != -1)
							close(keyb->datafd);
						if(keyb->epfd != -1)
							close(keyb->epfd);
						*kpp = keyb->next;
						free(keyb);
						break;
					}
				}
				free(dir);
				continue;
			}
			free(dir);
			break;
		}
		sleep(1000);
	}
}

void
main(int argc, char** argv)
{
	char* mtpt, *srvname;
	srvname = nil;
	mtpt = "/dev/usb";

	time0 = time(0);
	ARGBEGIN
	{
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 'S':
		srvname = EARGF(usage());
		break;
	case 'D':
		chatty9p++;
		break;
	default:
		usage();
	}
	ARGEND

	/* don't leave standard descriptors open to confuse mk */
	close(0);
	close(1);
	close(2);
	postmountsrv(&keybsrv, srvname, mtpt, MBEFORE);
	exits(nil);
}
