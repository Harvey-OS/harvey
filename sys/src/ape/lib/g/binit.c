#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <libg.h>

typedef unsigned char uchar;

	int	dontbexit;
	int	bitbltfd;
	Bitmap	screen;
	Font	*font;
static	uchar	bbuf[8000];
	uchar	_btmp[8000];
static	uchar	*bbufp = bbuf;
static	void	(*onerr)(char*);
static	char	oldlabel[128+1];
static	char	*label;

uchar*
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
	r = (write(bitbltfd, (char *)bbuf, bbufp-bbuf) == bbufp-bbuf);
	bbufp = bbuf;
	return r;
}

void
binit(void (*f)(char *), char *s, char *nlabel)
{
	uchar *buf, *p;
	char fontname[128];
	int fd, n, m, j;
	Fontchar *info;
	Subfont	*subfont;

	if(s == 0){
		fd = open("/env/font", O_RDONLY);
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
	bitbltfd = open("/dev/bitblt", O_RDWR);
	if(bitbltfd < 0)
		berror("open /dev/bitblt");
	if(write(bitbltfd, "i", 1) != 1)
		berror("binit write /dev/bitblt");

	n = 18+16;
	m = 18+16;
	if(s == 0){
		m += 3*12;
		n += 3*12+1300*6;	/* 1300 charinfos: big enough? */
	}
	buf = malloc(n);
	if(buf == 0){
		free(buf);
		berror("binit alloc");
	}
	j = read(bitbltfd, (char *)buf, n);
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
	screen.cache = 0;

	if(s){
		/*
		 * Load specified font
		 */
		font = rdfontfile(s, screen.ldepth);
		if(font == 0)
			berror("binit font load");
	}else{
		/*
		 * Cobble fake font using existing subfont
		 */
		n = atoi((char*)buf+18+16);
		info = malloc(sizeof(Fontchar)*(n+1));
		if(info == 0){
			free(buf);
			berror("binit info alloc");
		}
		p = buf+18+16+3*12;
		_unpackinfo(info, p, n);
		subfont = malloc(sizeof(Subfont));
		if(subfont == 0){
	Err:
			free(buf);
			free(info);
			berror("binit font alloc");
		}
		subfont->n = n;
		subfont->height = atoi((char*)buf+18+16+12);
		subfont->ascent = atoi((char*)buf+18+16+24);
		subfont->info = info;
		subfont->id = 0;
		font = mkfont(subfont, 0);
		if(font == 0)
			goto Err;
	}
	free(buf);
	
	label = nlabel;
	if(label){
		fd = open("/dev/label", O_RDONLY);
		if(fd >= 0){
			read(fd, oldlabel, sizeof oldlabel-1);
			close(fd);
			fd = creat("/dev/label", 0666);
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
	close(bitbltfd);
	dontbexit = 1;
}

void
bexit(void)
{
	int fd;

	if(dontbexit)
		return;
	bflush();
	if(label){
		fd = creat("/dev/label", 0666);
		if(fd >= 0){
			write(fd, oldlabel, strlen(oldlabel));
			close(fd);
		}
	}
}

void
berror(char *s)
{
	char errbuf[150];

	if(onerr)
		(*onerr)(s);
	else{
		sprintf(errbuf, "bitblt: %s", s);
		perror(errbuf);
		exit(1);
	}
}
