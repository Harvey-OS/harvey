#include <alef.h>
#include <libg.h>

	int	bitbltfd;
	Bitmap	screen;
	Font	*font;
intern	byte	bbuf[8000];
	byte	_btmp[8192];
intern	byte	*bbufp = bbuf;
intern	void	(*onerr)(byte*);
intern	byte	oldlabel[128+1];
intern	byte	*label;

byte*
bneed(int n)
{
	if(n < 0)
		berror("negative count in bneed");

	if(n==0 || bbufp-bbuf>sizeof bbuf-n){
		if(!bwrite())
			berror("write to /dev/bitblt");
		if(n > sizeof bbuf)
			berror("write to /dev/bitblt too big");
	}
	bbufp += n;
	return bbufp-n;
}

void
bflush(void)
{
	bneed(0);
}

int
bwrite(void)
{
	int r;

	if(bbufp == bbuf)
		return 1;

	r = (write(bitbltfd, bbuf, bbufp-bbuf) == bbufp-bbuf);
	bbufp = bbuf;
	return r;
}

int
Rconv(Printspec *ps)
{
	Rectangle *r;
	byte buf[128];

	r = ps->o;
	sprint(buf, "%P %P", r->min, r->max);
	strconv(ps, buf);
	return sizeof(Rectangle);
}

int
Pconv(Printspec *ps)
{
	Point *p;
	byte buf[64];

	p = ps->o;
	sprint(buf, "[%ld %ld]", p->x, p->y);
	strconv(ps, buf);
	return sizeof(Point);
}

void
binit(void (*f)(byte *), byte *s, byte *nlabel)
{
	byte *buf, *p;
	byte fontname[128];
	int fd, n, m, j;
	Fontchar *info;
	Subfont	*subfont;

	if(s == nil){
		fd = open("/env/font", OREAD);
		if(fd >= 0){
			n = read(fd, fontname, sizeof(fontname));
			if(n > 0){
				fontname[n] = 0;
				s = fontname;
			}
			close(fd);
		}
	}
	onerr = f;
	if(access("/dev/bitblt", 0) < 0)
		bind("#b", "/dev", MAFTER);
	bitbltfd = open("/dev/bitblt", 2);
	if(bitbltfd < 0)
		berror("open /dev/bitblt");
	if(write(bitbltfd, "i", 1) != 1)
		berror("binit write /dev/bitblt");

	n = 18+16;
	m = 18+16;
	if(s == nil){
		m += 3*12;
		n += 3*12+1300*6;	/* 1300 charinfos: big enough */
	}
	buf = malloc(n);
	if(buf == nil){
		free(buf);
		berror("binit alloc");
	}
	j = read(bitbltfd, buf, n);
	if(j<m || buf[0]!='I'){
		free(buf);
		berror("binit read");
	}
	screen.ldepth = buf[1];
	screen.r.min.x = BGLONG(buf+2);
	screen.r.min.y = BGLONG(buf+6);
	screen.r.max.x = BGLONG(buf+10);
	screen.r.max.y = BGLONG(buf+14);
	screen.clipr.min.x = BGLONG(buf+18);
	screen.clipr.min.y = BGLONG(buf+22);
	screen.clipr.max.x = BGLONG(buf+26);
	screen.clipr.max.y = BGLONG(buf+30);
	screen.cache = nil;
	fmtinstall('P', Pconv);
	fmtinstall('R', Rconv);

	if(s){
		/*
		 * Load specified font
		 */
		font = rdfontfile(s, screen.ldepth);
		if(font == nil)
			berror("binit font load");
	}else{
		/*
		 * Cobble fake font using existing subfont
		 */
		n = atoi((byte*)buf+18+16);
		info = malloc(sizeof(Fontchar)*(n+1));
		if(info == nil){
			free(buf);
			berror("binit info alloc");
		}
		p = buf+18+16+3*12;
		_unpackinfo(info, p, n);
		subfont = malloc(sizeof(Subfont));
		if(subfont == nil){
	Err:
			free(buf);
			free(info);
			berror("binit font alloc");
		}
		subfont->n = n;
		subfont->height = atoi((byte*)buf+18+16+12);
		subfont->ascent = atoi((byte*)buf+18+16+24);
		subfont->info = info;
		subfont->id = 0;
		font = mkfont(subfont, 0);
		if(font == nil)
			goto Err;
	}
	free(buf);
	
	label = nlabel;
	if(label){
		fd = open("/dev/label", OREAD);
		if(fd >= 0){
			read(fd, oldlabel, sizeof oldlabel-1);
			close(fd);
			fd = create("/dev/label", OWRITE, 0666);
			if(fd >= 0){
				n = strlen(label);
				if(n > sizeof(oldlabel)-1)
					n = sizeof(oldlabel)-1;
				write(fd, label, n);
				close(fd);
			}
		}
	}
	atexit(bexit);
}

void
bclose(void)
{
/*	bwrite(); */
	close(bitbltfd);
	atexitdont(bexit);
}

void
bexit(void)
{
	int fd;

	bflush();
	if(label){
		fd = open("/dev/label", OWRITE);
		if(fd >= 0){
			write(fd, oldlabel, strlen(oldlabel));
			close(fd);
		}
	}
}

void
berror(byte *s)
{
	byte err[ERRLEN];

	if(onerr)
		(*onerr)(s);
	else{
		errstr(err);
		fprint(2, "bitblt: %s: %s\n", s, err);
		exits(s);
	}
}
