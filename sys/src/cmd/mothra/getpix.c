#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include <fb.h>
#include "mothra.h"
#define	NPIX	500
typedef struct Pix Pix;
struct Pix{
	char name[NNAME];
	Bitmap *b;
};
Pix pix[NPIX];
Pix *epix=pix;
char *pixcmd[]={
[GIF]	"fb/gif2pic -m|fb/3to1 9",
[JPEG]	"fb/jpg2pic|fb/3to1 9",
[PIC]	"fb/3to1 9",
[XBM]	"fb/xbm2pic",
};
void getimage(Action *a, int fd, int type, int teefd){
	PICFILE *f;
	int pfd[2];
	char fdname[30], cmdbuf[512];
	char *bits;
	int nx, ny, y;
	if(type!=GIF && type!=JPEG && type!=PIC && type!=XBM) return;
	if(pipe(pfd)==-1) return;
	switch(fork()){
	case -1:
		close(pfd[0]);
		close(pfd[1]);
		break;
	case 0:
		dup(fd, 0);
		dup(pfd[0], 1);
		close(pfd[0]);
		close(pfd[1]);
		if(teefd!=-1)
			sprint(cmdbuf, "tee -a /fd/%d | %s", teefd, pixcmd[type]);
		else strcpy(cmdbuf, pixcmd[type]);
		execl("/bin/rc", "rc", "-c", cmdbuf, 0);
		_exits(0);
	default:
		close(pfd[0]);
		sprint(fdname, "/fd/%d", pfd[1]);
		f=picopen_r(fdname);
		if(f==0) return;
		close(pfd[1]);
		nx=PIC_WIDTH(f);
		ny=PIC_HEIGHT(f);
		bits=emalloc(nx*ny);
		for(y=0;y!=ny;y++)
			picread(f, bits+y*nx);
		picclose(f);
		a->r=Rect(0, 0, nx, ny);
		a->imagebits=bits;
		break;
	}
}
Bitmap *getcachepix(char *name){
	Pix *p;
	for(p=pix;p!=epix;p++) if(strcmp(name, p->name)==0) return p->b;
	return 0;
}
void storebitmap(Rtext *t, Bitmap *b){
	Action *a;
	a=t->user;
	t->b=b;
	free(t->text);
	t->text=0;
	free(a->image);
	if(a->link==0){
		free(a);
		t->user=0;
	}
}
void storetext(Rtext *t, char *text){
	Action *a;
	a=t->user;
	free(t->text);
	t->text=strdup(text);
	free(a->image);
	if(a->link==0){
		free(a);
		t->user=0;
	}
}
void getpix(Rtext *t, Www *w){
	Action *ap;
	Url url;
	int fd;
	Bitmap *b;
	char err[512];
	for(;t!=0;t=t->next){
		ap=t->user;
		if(ap && ap->image){
			crackurl(&url, ap->image, w->url);
			free(ap->image);
			ap->image=strdup(url.fullname);
			b=getcachepix(url.fullname);
			if(b==0){
				fd=urlopen(&url, GET, 0);
				if(fd==-1){
					sprint(err, "[[open %s: %r]]", url.fullname);
					storetext(t, err);
				}
				else{
					getimage(ap, fd, url.type, url.cachefd);
					close(url.cachefd);
					close(fd);
					if(ap->imagebits) w->changed=1;
					else{
						sprint(err, "[[get %s: %r]]", url.fullname);
						storetext(t, err);
					}
				}
			}
			else{
				storebitmap(t, b);
				w->changed=1;
			}
		}
	}
}
void setbitmap(Rtext *t){
	Action *a;
	Bitmap *b;
	a=t->user;
	b=balloc(a->r, 3);
	if(b==0) message("can't balloc!");
	else{
		wrbitmap(b, a->r.min.y, a->r.max.y, (uchar *)a->imagebits);
		bitblt(b, b->r.min, b, b->r, ~S);
		free(a->imagebits);
		a->imagebits=0;
		if(epix!=&pix[NPIX]){
			strcpy(epix->name, a->image);
			epix->b=b;
			epix++;
		}
		storebitmap(t, b);
	}
}
