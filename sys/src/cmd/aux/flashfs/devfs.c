#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

static	char*	file;
static	int	fd;
static	uchar	*ones;

static	int	isdev;

struct {
	int	dfd;	/* data */
	int	cfd;	/* control */
} flash;

void
initdata(char *f, int)
{
	char err[ERRMAX];
	char buf[1024], *fld[8];
	int n;
	Dir *d;

	isdev = 1;
	flash.dfd = open(f, ORDWR);
	if(flash.dfd < 0){
		errstr(err, sizeof err);
		if((flash.dfd = create(f, ORDWR, 0666)) >= 0){
			fprint(2, "warning: created plain file %s\n", buf);
			goto Plain;
		}
		errstr(err, sizeof err);	/* restore open error */
		sysfatal("opening %s: %r", f);
	}
	if(snprint(buf, sizeof buf, "%sctl", f) != strlen(f)+3)
		sysfatal("path too long: %s", f);
	flash.cfd = open(buf, ORDWR);
	if(flash.cfd < 0){
		fprint(2, "warning: cannot open %s (%r); assuming plain file\n", buf);
	Plain:
		isdev = 0;
		if(sectsize == 0)
			sectsize = 512;
		if(nsects == 0){
			if((d = dirstat(f)) == nil)
				sysfatal("stat %s: %r", f);
			nsects = d->length / sectsize;
			free(d);
		}
		ones = emalloc9p(sectsize);
		memset(ones, ~0, sectsize);
	}else{
		n = read(flash.cfd, buf, sizeof(buf)-1);
		if(n <= 0)
			sysfatal("reading %sctl: %r", f);
		buf[n] = 0;
		n = tokenize(buf, fld, nelem(fld));
		if(n < 7)
			sysfatal("bad flash geometry");
		nsects = atoi(fld[5]);
		sectsize = atoi(fld[6]);
		if(nsects < 8)
			sysfatal("unreasonable value for nsects: %lud", nsects);
		if(sectsize < 512)
			sysfatal("unreasonable value for sectsize: %lud", sectsize);
	}
}

void
clearsect(int sect)
{
	if(isdev==0){
		if(pwrite(flash.dfd, ones, sectsize, sect*sectsize) != sectsize)
			sysfatal("couldn't erase sector %d: %r", sect);
	}else{
		if(fprint(flash.cfd, "erase %lud", sect * sectsize) < 0)
			sysfatal("couldn't erase sector %d: %r", sect);
	}
}

void
readdata(int sect, void *buff, ulong count, ulong off)
{
	long n;
	ulong m;

	m = sect * sectsize + off;
	n = pread(flash.dfd, buff, count, m);
	if(n < 0)
		sysfatal("error reading at %lux: %r", m);
	if(n != count)
		sysfatal("short read at %lux, %ld instead of %lud", m, n, count);
}

int
writedata(int err, int sect, void *buff, ulong count, ulong off)
{
	long n;
	ulong m;

	m = sect*sectsize + off;
	n = pwrite(flash.dfd, buff, count, m);
	if(n < 0){
		if(err)
			return 0;
		sysfatal("error writing at %lux: %r", m);
	}
	if(n != count){
		if(err)
			return 0;
		sysfatal("short write at %lud, %ld instead of %lud", m, n, count);
	}
	return 1;
}
