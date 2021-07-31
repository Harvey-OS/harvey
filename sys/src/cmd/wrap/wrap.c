#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

#define H ((void*)~0)

static int
mountarch(char *file, char *mtpt)
{
	Waitmsg w;

	switch(fork()) {
	case -1:
		sysfatal("mountarch fork: %r");
	case 0:
		close(1);
		execl("/bin/archfs", "archfs", "-m", mtpt, file, nil);
		sysfatal("exec archfs: %r");
	default:
		wait(&w);
		if(w.msg[0]) {
			errstr(w.msg);
			return -1;	
		}
		return 0;
	}
}

static char*
getfirstdir(char *root, Dir *d)
{
	int fd;

	if((fd = open(root, OREAD)) < 0)
		return nil;

	while(dirread(fd, d, sizeof(*d)) == sizeof(*d)) {
		if(d->mode & CHDIR) {
			close(fd);
			return d->name;
		}
	}
	close(fd);
	return nil;
}

static Update*
readupdate(Update *u, char *base, char *elem)
{
	char *p, *q;

	u->dir = mkpath(base, elem);

	p = mkpath(u->dir, "desc");
	u->desc = readfile(p);
	free(p);
	if(u->desc && (p = strchr(u->desc, '\n')))
		*p = '\0';

	u->time = strtoll(elem, 0, 10);

	p = mkpath(u->dir, "md5sum");
	u->bmd5 = Bopen(p, OREAD);
	free(p);

	q = mkpath(u->dir, "update");
	if(p = readfile(q)) {
		u->utime = strtoll(p, 0, 10);
		free(p);
	} else
		u->utime = 0LL;
	free(q);

	if(u->bmd5 == nil) {
		free(u->dir);
		free(u->desc);
		return nil;
	}
	return u;
}

enum {
	NONE = 0,
	PKG = 1,
	UPD = 2
};

static int
sniffdir(char *base, char *elem, vlong *pt)
{
	vlong t;
	char buf[NAMELEN];
	char *p;
	int rv;

	t = strtoll(elem, 0, 10);
	if(t == 0)
		return NONE;

	sprint(buf, "%lld", t);
	if(strcmp(buf, elem) != 0)
		return NONE;

	rv = NONE;
	p = mkpath3(base, elem, "package");
	if(access(p, 0) >= 0)
		rv = PKG;
	free(p);

	p = mkpath3(base, elem, "update");
	if(access(p, 0) >= 0) {
		if(rv == PKG)
			rv = NONE;
		else
			rv = UPD;
	}
	free(p);

	if(rv != NONE)
		*pt = t;
	return rv;
}

static int
openupdate(Update **up, char *dir)
{
	Dir d;
	int fd;
	vlong t, tmax, tmaxupd;
	Update *u;
	int nu;

	if((fd = open(dir, OREAD)) < 0)
		return -1;

	tmax = 0;
	tmaxupd = 0;
	while(dirread(fd, &d, sizeof(d)) == sizeof(d)) {
		switch(sniffdir(dir, d.name, &t)) {
		case PKG:
			if(t > tmax)
				tmax = t;
			break;
		case UPD:
			if(t > tmaxupd)
				tmaxupd = t;
			break;
		}
	}
	close(fd);

	if(tmax == 0) {
		tmax = tmaxupd;
		if(tmax == 0)
			return -1;
	}

	u = nil;
	nu = 0;
	if((fd = open(dir, OREAD)) < 0)
		return -1;
	while(dirread(fd, &d, sizeof(d)) == sizeof(d)) {
		if(sniffdir(dir, d.name, &t) != NONE) {
			if(nu%8 == 0)
				u = erealloc(u, (nu+8)*sizeof(u[0]));
			if(readupdate(&u[nu], dir, d.name) != nil)
				nu++;
		}
	}
	close(fd);

	if(nu == 0)
		return -1;

	*up = u;
	return nu;
}

static Wrap*
openmount(char *name, char *root)
{
	Dir d;
	Wrap *w;
	char *p;

	if(name == nil) {
		p = mkpath(root, "wrap");
		name = getfirstdir(p, &d);
		free(p);
		if(name == nil)
			return nil;
	}

	w = emalloc(sizeof(*w));
	w->name = estrdup(name);
	w->root = estrdup(root);
	p = mkpath3(root, "wrap", name);

	if((w->nu = openupdate(&w->u, p)) < 0) {
		free(p);
		closewrap(w);
		return nil;
	}
	free(p);
	return w;
}	

Wrap*
openwrap(char *name, char *root)
{
	if(access(name, 0) < 0)
		return openmount(name, root);

	if(mountarch(name, root) < 0)
		return nil;

	return openmount(nil, root);
}

void
closewrap(Wrap *w)
{
	int i;

	free(w->name);
	free(w->root);
	for(i=0; i<w->nu; i++) {
		free(w->u[i].desc);
		free(w->u[i].dir);
		Bterm(w->u[i].bmd5);
	}
	free(w->u);
	free(w);
}

Wrap*
openwraphdr(char *name, char *root, char **prefix, int nprefix)
{
	char *nname;

	if(access(name, 0) < 0)
		return openwrap(name, root);

	if(openarchgz(name, &nname, prefix, nprefix) == nil)
		return nil;

	return openwrap(nname, "/mnt/wrap");
}

static char *hex = "0123456789abcdef";
int
getfileinfo(Wrap *w, char *file, vlong *t, uchar *digest)
{
	int i, a, b;
	char *p, *q;
	Update *u;

	if(w == nil)
		return -1;

	p = nil;
	for(i=w->nu-1, u=w->u+i; i>=0; i--, u--)
		if(p=Bsearch(w->u[i].bmd5, file))
			break;

	if(p == nil)
		return -1;

	if(t)
		*t = u->time;

	if(digest) {
		q = strchr(p, ' ');
		if(q == nil)
			return -1;
	
		q++;
		for(i=0; i<MD5dlen; i++) {
			if((p = strchr(hex, q[2*i])) == nil)
				break;
			a = p-hex;
			if((p = strchr(hex, q[2*i+1])) == nil)
				break;
			b = p-hex;
			digest[i] = (a<<4)|b;
		}
		if(i != MD5dlen)
			return -1;
	}

	return 0;
}

void
Bputwrapfile(Biobuf *b, char *name, vlong time, char *elem, char *file)
{
	Dir d;
	char buf[4*NAMELEN];

	sprint(buf, "/wrap/%s/%lld/%s", name, time, elem);
	memset(&d, 0, sizeof d);
	strcpy(d.uid, "sys");
	strcpy(d.gid, "sys");
	d.mode = 0444;
	d.mtime = time;
	d.length = filelength(file);
	if(d.length < 0)
		sysfatal("cannot stat %s: %r", file);
	Bputhdr(b, buf, &d);
	if(Bcopyfile(b, file, d.length) < 0)
		sysfatal("error copying %s file: %r", file);
}

void
Bputwrap(Biobuf *b, char *name, vlong time, char *desc, vlong utime)
{
	Dir d;
	char *p;
	char buf[4*NAMELEN];

	memset(&d, 0, sizeof d);
	strcpy(d.uid, "sys");
	strcpy(d.gid, "sys");
	d.mode = CHDIR|0775;
	d.mtime = time;
	Bputhdr(b, "/wrap", &d);

	sprint(buf, "/wrap/%s", name);
	Bputhdr(b, buf, &d);

	sprint(buf, "/wrap/%s/%lld", name, time);
	Bputhdr(b, buf, &d);

	d.mode = 0444;
	p = buf+strlen(buf);
	*p++ = '/';

	if(utime) {
		strcpy(p, "update");
		d.length = 23;
		Bputhdr(b, buf, &d);
		Bprint(b, "%22lld\n", utime);
	} else {
		strcpy(p, "package");
		d.length = 0;
		Bputhdr(b, buf, &d);
	}

	if(desc) {
		strcpy(p, "desc");
		d.length = strlen(desc)+1;
		d.mode = 0444;
		Bputhdr(b, buf, &d);
		Bwrite(b, desc, strlen(desc));
		Bprint(b, "\n");
	}
}
