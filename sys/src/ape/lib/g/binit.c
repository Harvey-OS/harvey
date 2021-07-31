#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <libg.h>

	int		bitbltfd;
	Bitmap		screen;
	Font		*font;
static	unsigned char	bbuf[8192];
static	unsigned char	*bbufp = bbuf;
static	void		(*onerr)(char*);
static	char		oldlabel[128+1];
static	char		*label;

unsigned char*
bneed(int n)	/* n can be negative */
{
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
	unsigned char *buf, *p;
	char *t;
	char fontname[32];
	int fd, n, j;
	Bitmap *b;
	Fontchar *info, *i;

	onerr = f;
	bitbltfd = open("/dev/bitblt", O_RDWR);
	if(bitbltfd < 0)
		berror("open /dev/bitblt");
	if(write(bitbltfd, "i", 1) != 1)
		berror("binit write /dev/bitblt");
	buf = malloc(18+3*12+300*6);
	if(buf == 0){
		free(buf);
		berror("binit alloc");
	}
	n = read(bitbltfd, (char *)buf, 18+3*12+300*6);
	if(n<18+3*12 || buf[0]!='I'){
		free(buf);
		berror("binit read");
	}
	screen.ldepth = buf[1];
	screen.r.min.x = BGLONG(buf+2);
	screen.r.min.y = BGLONG(buf+6);
	screen.r.max.x = BGLONG(buf+10);
	screen.r.max.y = BGLONG(buf+14);
	screen.cache = 0;

	if(s){
		free(buf);
		t = s+strlen(s)-1;
		if(t>=s && '0'<=*t && *t<='9'){
			j = *t;
			*t = '0'+screen.ldepth;	/* try to use right ldepth */
			if(access(s, R_OK) != 0)
				*t = j;
		}
		fd = open(s, O_RDONLY);
		if(fd < 0)
			berror("binit font open");
		b = rdbitmapfile(fd);
		if(b == 0)
			berror("binit font bitmap read");
		font = rdfontfile(fd, b);
		if(font == 0)
			berror("binit font info read");
		close(fd);
	}else{
		n = atoi((char*)buf+18);
		info = malloc(sizeof(Fontchar)*(n+1));
		if(info == 0){
			free(buf);
			berror("binit info alloc");
		}
		p = buf+18+3*12;
		for(i=info,j=0; j<=n; j++,i++,p+=6){
			i->x = BGSHORT(p);
			i->top = p[2];
			i->bottom = p[3];
			i->left = p[4];
			i->width = p[5];
		}
		font = malloc(sizeof(Font));
		if(font == 0){
			free(buf);
			free(info);
			berror("binit font alloc");
		}
		font->n = n;
		font->height = atoi((char*)buf+18+12);
		font->ascent = atoi((char*)buf+18+24);
		font->info = info;
		font->id = 0;
		free(buf);
	}
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
}

void
bexit(void)
{
	int fd;

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
