#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

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
if(u->bmd5 == nil)
	fprint(2, "warning: cannot open %s: %r\n", p);
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

static int
sniffdir(char *base, char *elem, vlong *pt)
{
	vlong t;
	char buf[NAMELEN];
	char *p;
	int rv;

	t = strtoll(elem, 0, 10);
	if(t == 0)
		return 0;

	sprint(buf, "%lld", t);
	if(strcmp(buf, elem) != 0)
		return 0;

	rv = 0;
	p = mkpath3(base, elem, "package");
	if(access(p, 0) >= 0)
		rv |= FULL;
	free(p);

	p = mkpath3(base, elem, "update");
	if(access(p, 0) >= 0)
		rv |= UPD;
	free(p);

	if(rv != 0)
		*pt = t;
	return rv;
}

static int
updatecmp(void *va, void *vb)
{
	Update *a, *b;

	a = va;
	b = vb;
	if(a->time == b->time)
		return 0;
	if(a->time < b->time)
		return -1;
	return 1;
}

static int
openupdate(Update **up, char *dir, vlong *tfullp)
{
	Dir d;
	int i, type, fd;
	vlong t, tbase, tfull;
	Update *u;
	int nu;

	if((fd = open(dir, OREAD)) < 0)
		return -1;

	/*
	 * We are looking to find the most recent full
	 * package; anything before that is irrelevant.
	 * Also figure out the most recent package update.
	 * Non-package updates before that are irrelevant.
	 * If there are no packages installed, 
	 * grab all the updates we can find.
	 */
	tbase = -1;
	tfull = -1;
	nu = 0;
	while(dirread(fd, &d, sizeof(d)) == sizeof(d)) {
		switch(sniffdir(dir, d.name, &t)) {
		case FULL:
			nu++;
			if(tfull < t)
				tfull = t;
			if(tbase < t)
				tbase = t;
			break;
		case FULL|UPD:
			nu++;
			if(tfull < t)
				tfull = t;
			break;
		case UPD:
			nu++;
			break;
		}
	}
	close(fd);

	if(nu == 0)
		return -1;

	u = nil;
	nu = 0;
	if((fd = open(dir, OREAD)) < 0)
		return -1;
	while(dirread(fd, &d, sizeof(d)) == sizeof(d)) {
		if((type = sniffdir(dir, d.name, &t)) == 0)
			continue;
		if(t < tbase)
			continue;
		if(t < tfull && type == UPD)
			continue;
		if(nu%8 == 0)
			u = erealloc(u, (nu+8)*sizeof(u[0]));
		u[nu].type = type;
		if(readupdate(&u[nu], dir, d.name) != nil)
			nu++;
	}
	close(fd);
	if(nu == 0)
		return -1;

	qsort(u, nu, sizeof(u[0]), updatecmp);

	/*
	 * paranoia: i never get the bloody sort correct.
	 * check that times ascend, and that the order of
	 * types is FULL? FULL|UPD* UPD*
	 */
	type = FULL;
	for(i=0; i<nu; i++) {
		if(0<i)
			assert(u[i-1].time < u[i].time);
		if(u[i].type != type) {
			switch(u[i].type) {
			case FULL:
				assert(0);
			case FULL|UPD:
				assert(type == FULL);
				type = FULL|UPD;
				break;
			case UPD:
				assert(type == FULL || (type == FULL|UPD));
				type = UPD;
				break;
			default:
				assert(0);
			}
		}
	}
	if(nu > 1)
		assert(u[1].type != FULL);

	*up = u;
	*tfullp = tfull;	/* save time of last complete set of files */
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

	if((w->nu = openupdate(&w->u, p, &w->tfull)) < 0) {
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
	Dir d;
	char *p;
	Wrap *w;

	if(root == nil)
		root = "/";

	if(dirstat(name, &d) < 0)
		return openmount(name, root);

	/*
	 * Accept root/ or root/wrap/pkgname
	 */
	if(d.mode & CHDIR) {
		p = estrdup(name);
		root = p;
		if(name = strstr(p, "/wrap/")) {
			*name = '\0';
			name += 6;
		}
		w = openmount(name, root);
		free(p);
		return w;
	}
		
	if(dirstat(name, &d) < 0 || (d.mode & CHDIR))
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
	Dir d;

	if(dirstat(name, &d) < 0 || (d.mode & CHDIR))
		return openwrap(name, root);

	if(openarchgz(name, &nname, prefix, nprefix) == nil)
		return nil;

	return openwrap(nname, "/mnt/wrap");
}

/*
 * Find the newest package containing file (with MD5 sum rdigest
 * when specified).  Return 0 if found, -1 if not.
 * If found and t != nil, set t to the time of the package,
 * if found and wdigest != nil, set wdigest to MD5sum.
 */
static char *hex = "0123456789abcdef";
int
getfileinfo(Wrap *w, char *file, vlong *t, uchar *rdigest, uchar *wdigest)
{
	int j, i, a, b;
	char *p, *q;
	uchar digest[MD5dlen];

	if(w == nil)
		return -1;

	for(j=w->nu-1; j>=0; j--) {
		if((p=Bsearch(w->u[j].bmd5, file)) == nil)
			continue;

		q = strchr(p, ' ');
		if(q == nil)
			continue;
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
			continue;

		if(rdigest==nil || memcmp(rdigest, digest, MD5dlen)==0)
			break;
	}

	if(j < 0)
		return -1;

	if(wdigest)
		memmove(wdigest, digest, MD5dlen);

	if(t)
		*t = w->u[j].time;

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
Bputwrap(Biobuf *b, char *name, vlong time, char *desc, vlong utime, int pkg)
{
	Dir d;
	char *p;
	char buf[4*NAMELEN];

	assert(utime || pkg);

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
	}
	if(pkg) {
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
