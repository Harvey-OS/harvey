/*
 * Flags:
 *	-m		don't load color map
 *	-M		only load color map
 *	-q		don't accept input -- any event exits
 *	-w x0 y0 x1 y1	specify window position and size
 *	-c x y		specify window center
 * Bugs:
 *	can't do full-color pictures on color screens
 * Should have:
 *	interactive scroll & zoom
 *	cross-sections
 *	histograms
 *	etc.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	BORDER	4
#define	TOPAIR	1
#define	BOTAIR	1
#define	LEFTAIR	1
#define	FONTHGT	16	/* should be font->height, but font is unknown when we need this */
uchar *image;		/* image bytes */
int nchan, wid, hgt;	/* image size */
Bitmap *b;		/* the image in question */
Rectangle msgr;		/* messages drawn here */
Rectangle leadr;	/* lead between message and image */
Rectangle imager;	/* image drawn here */
char *name;		/* image file name */
Point origin;		/* ul corner of image is here */
Point offs;		/* offset in image file */
uchar *cmap;		/* color map pointer */
char *items2[]={
	"button",
	"two",
	"does",
	"nothing",
	0
};
Menu menu2={items2};
char *items3[]={
	"fix cmap",
	"exit",
	0
};
Menu menu3={items3};
Cursor confirmcursor={
	0, 0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0x00, 0x0E, 0x07, 0x1F, 0x03, 0x17, 0x73, 0x6F,
	0xFB, 0xCE, 0xDB, 0x8C, 0xDB, 0xC0, 0xFB, 0x6C,
	0x77, 0xFC, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
	0x94, 0xA6, 0x63, 0x3C, 0x63, 0x18, 0x94, 0x90,
};
int confirm(int b){
	Mouse down, up;
	cursorswitch(&confirmcursor);
	do down=emouse(); while(!down.buttons);
	do up=emouse(); while(up.buttons);
	cursorswitch(0);
	return down.buttons==(1<<(b-1));
}
Rectangle getscr(void){
	Rectangle r;
	int fd;
	char buf[12*5];
	fd=open("/dev/screen", OREAD);
	if(fd==-1) fd=open("/mnt/term/dev/screen", OREAD);
	if(fd==-1) return Rect(0,0,1024,1024);
	if(read(fd, buf, sizeof buf)!=sizeof buf){
		fprint(2, "Can't read /dev/screen: %r\n");
		exits("screen read");
	}
	r.min.x=atoi(buf+12);
	r.min.y=atoi(buf+24);
	r.max.x=atoi(buf+36);
	r.max.y=atoi(buf+48);
	return r;
}
char *rdenv(char *name){
	char *v;
	int fd, size;
	fd=open(name, OREAD);
	if(fd<0) return 0;
	size=seek(fd, 0, 2);
	v=malloc(size+1);
	if(v==0){
		fprint(2, "Can't malloc: %r\n");
		exits("no mem");
	}
	seek(fd, 0, 0);
	read(fd, v, size);
	v[size]=0;
	close(fd);
	return v;
}
void winit(void (*errfun)(char *), char *font, char *label, Rectangle r){
	char *srv, *mntsrv;
	char spec[100];
	int srvfd, cons, pid;
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFENVG|RFNOTEG|RFNOWAIT)){
	case -1:
		fprint(2, "Can't fork: %r\n");
		exits("no fork");
	case 0:
		break;
	default:
		exits(0);
	}
	srv=rdenv("/env/8½srv");
	if(srv==0){
		free(srv);
		mntsrv=rdenv("/mnt/term/env/8½srv");
		srv=malloc(strlen(mntsrv)+10);
		sprint(srv, "/mnt/term%s", mntsrv);
		free(mntsrv);
		pid=0;			/* 8½srv can't send notes to remote processes! */
	}
	else pid=getpid();
	srvfd=open(srv, ORDWR);
	free(srv);
	if(srvfd==-1){
		fprint(2, "Can't open %s: %r\n", srv);
		exits("no srv");
	}
	sprint(spec, "N%d,%d,%d,%d,%d\n", pid, r.min.x, r.min.y, r.max.x, r.max.y);
	if(mount(srvfd, "/mnt/8½", 0, spec)==-1){
		fprint(2, "Can't mount: %r\n");
		exits("no mount");
	}
	close(srvfd);
	bind("/mnt/8½", "/dev", MBEFORE);
	cons=open("/dev/cons", OREAD);
	if(cons==-1){
	NoCons:
		fprint(2, "Can't open /dev/cons: %r");
		exits("no cons");
	}
	dup(cons, 0);
	close(cons);
	cons=open("/dev/cons", OWRITE);
	if(cons==-1) goto NoCons;
	dup(cons, 1);
	dup(cons, 2);
	close(cons);
	binit(errfun, font, label);
}
/* replicate (from top) value in v (n bits) until it fills a ulong */
ulong rep(ulong v, int n){
	int o;
	ulong rv;
	rv=0;
	for(o=32-n;o>=0;o-=n) rv|=v<<o;
	if(o!=-n) rv|=v>>-o;
	return rv;
}
uchar *rcmap;
void putcmap(uchar cmap[256*3]){
	int i, j;
	int nbit, npix;
	RGB map[256];
	nbit=1<<screen.ldepth;
	npix=1<<nbit;
	for(j=0;j!=npix;j++){
		i=3*255*j/(npix-1);
		map[npix-1-j].red=rep(cmap[i], 8);
		map[npix-1-j].green=rep(cmap[i+1], 8);
		map[npix-1-j].blue=rep(cmap[i+2], 8);
	}
	wrcolmap(&screen, map);
	rcmap=cmap;
}
void msg(char *m){
	Rectangle r;
	Point p;
	r=screen.clipr;
	clipr(&screen, msgr);
	p=string(&screen, add(msgr.min, Pt(LEFTAIR, TOPAIR)), font, m, S);
	bitblt(&screen, p, &screen, Rpt(p, msgr.max), Zero);
	clipr(&screen, r);
}
void ereshaped(Rectangle r){
	screen.r=r;
	r=inset(r, BORDER);
	clipr(&screen, r);
	bitblt(&screen, r.min, &screen, r, Zero);
	msgr=r;
	msgr.max.y=msgr.min.y+font->height+TOPAIR+BOTAIR;
	leadr=msgr;
	leadr.min.y=msgr.max.y;
	leadr.max.y=leadr.max.y+BORDER;
	imager=r;
	imager.min.y=leadr.max.y;
	bitblt(&screen, leadr.min, &screen, leadr, F);
	clipr(&screen, imager);
	origin=div(sub(add(imager.min, imager.max), b->r.max), 2);
	bitblt(&screen, origin, b, b->r, S);
	msg(name);
	bflush();
}
/*
 * This routine is distressingly close to a verbatim copy of /sys/src/libfb/rdpicfile.c
 */
#define	NBIT	8		/* bits per uchar, a source of non-portability */
int rdpicarray(PICFILE *f, uchar *cmap){
	int w=1<<screen.ldepth;
	int nval=1<<w;
	int i;
	uchar lummap[1<<NBIT];
	static uchar mono[(1<<NBIT)*3];
	uchar *data;
	int *error;
	int *ep, *eerror, cerror, y, lum, value, shift, pv;
	uchar *dp;
	uchar *p;
	int wid, hgt, nchan;
	wid=PIC_WIDTH(f);
	hgt=PIC_HEIGHT(f);
	nchan=PIC_NCHAN(f);
	error=malloc((wid+2)*sizeof(int));
	data=malloc((wid*w+NBIT-1)/NBIT);
	image=malloc(wid*hgt*nchan);
	b=balloc(Rect(0, 0, wid, hgt), screen.ldepth);
	if(image==0 || error==0 || data==0 || b==0){
		if(image) free(image);
		if(error) free(error);
		if(data) free(data);
		if(b) bfree(b);
		return 0;
	}
	eerror=error+wid+1;
	switch(nchan){
	case 3:
	case 4:
	case 7:
	case 8:
		if(!flag['m'] && screen.ldepth==3){
			for(i=0;i!=1<<NBIT;i++) mono[3*i]=mono[3*i+1]=mono[3*i+2]=i;
			putcmap(mono);
		}
		lum=1;
		break;
	default:
		lum=0;
	}
	if(cmap){
		for(i=0,p=cmap;i!=1<<NBIT;i++,p+=3)
			lummap[i]=(p[0]*299+p[1]*587+p[2]*114)/1000;
	}
	else{
		for(i=0;i!=1<<NBIT;i++) lummap[i]=i;
	}
	for(ep=error;ep<=eerror;ep++) *ep=0;
	ereshaped(screen.r);
	for(y=0;y!=hgt;y++){
		cerror=0;
		p=image+nchan*wid*y;
		if(!picread(f, p)) break;
		dp=data-1;
		shift=0;
		for(ep=error+1;ep!=eerror;ep++){
			shift-=w;
			if(shift<0){
				shift+=NBIT;
				*++dp=0;
			}
			if(lum)
				value=(p[0]*299+p[1]*587+p[2]*114)/1000;
			else
				value=lummap[p[0]];
			value+=ep[0]/16;
			pv=value<=0?0:value<255?value*nval/256:nval-1;
			p+=nchan;
			*dp|=(nval-1-pv)<<shift;
			value-=pv*255/(nval-1);
			ep[1]+=value*7;
			ep[-1]+=value*3;
			ep[0]=cerror+value*5;
			cerror=value;
		}
		wrbitmap(b, y, y+1, data);
		bitblt(&screen, add(origin, Pt(0,y)), b, Rect(0, y, wid, y+1), S);
	}
	free(error);
	free(data);
	bflush();
	return 1;
}
void showpixel(int x, int y, char *chan){
	char buf[1024];
	char b2[20];
	uchar *p;
	if(x<0) x=0;
	if(x>=wid) x=wid-1;
	if(y<0) y=0;
	if(y>=hgt) y=hgt-1;
	sprint(buf, "x %d y %d", x+offs.x, y+offs.y);
	for(p=image+nchan*(x+wid*y);*chan;chan++,p++){
		sprint(b2, " %c %d", *chan, *p);
		strcat(buf, b2);
		if(cmap) switch(*chan){
		case 'r':
			sprint(b2, " (%d)", cmap[*p*3]);
			strcat(buf, b2);
			break;
		case 'g':
			sprint(b2, " (%d)", cmap[*p*3+1]);
			strcat(buf, b2);
			break;
		case 'b':
			sprint(b2, " (%d)", cmap[*p*3+2]);
			strcat(buf, b2);
			break;
		case 'm':
			sprint(b2, " (%d %d %d)", cmap[*p*3], cmap[*p*3+1], cmap[*p*3+2]);
			strcat(buf, b2);
			break;
		}
	}
	msg(buf);
}
int max(int a, int b){
	return a>b?a:b;
}
typedef struct Cvt Cvt;
struct Cvt{
	char *suffix;
	char *cmd;
};
Cvt cvt[]={
	".gif",	 "/bin/fb/gif2pic -m %s",
	".jpg",  "/bin/fb/jpg2pic %s|/bin/fb/3to1 9",
	".jpeg", "/bin/fb/jpg2pic %s|/bin/fb/3to1 9",
	".ega",  "/bin/fb/ega2pic %s",
	".face", "/bin/fb/face2pic %s",
	".pcx",  "/bin/fb/pcx2pic %s",
	".sgi",  "/bin/fb/sgi2pic %s|/bin/fb/3to1 9",
	".tga",  "/bin/fb/targa2pic %s|/bin/fb/3to1 9",
	".tif",  "/bin/fb/tiff2pic %s|/bin/fb/3to1 9",
	".rle",  "/bin/fb/utah2pic %s|/bin/fb/3to1 9",
	".xbm",  "/bin/fb/xbm2pic %s",
	".tiff", "/bin/fb/tiff2pic %s|/bin/fb/3to1 9",
	0
};
/*
 * Comme picopen_r, mais on fait quelque intelligence
 * artificielle pour voir les images étrangères.
 */
PICFILE *picopen_ai(char *name){
	Cvt *c;
	int len, offs, pfd[2];
	char cmd[1024], file[20];
	len=strlen(name);
	for(c=cvt;c->suffix;c++){
		offs=len-strlen(c->suffix);
		if(offs>=0 && strcmp(name+offs, c->suffix)==0){
			pipe(pfd);
			switch(fork()){
			case -1:
				close(pfd[0]);
				close(pfd[1]);
				return 0;
			case 0:
				dup(pfd[1], 1);
				close(pfd[0]);
				close(pfd[1]);
				sprint(cmd, c->cmd, name);
				execl("/bin/rc", "rc", "-c", cmd, 0);
				exits("no exec!");
			default:
				sprint(file, "/fd/%d", pfd[0]);
				close(pfd[1]);
				return picopen_r(file);
			}
		}
	}
	return picopen_r(name);
}
void main(int argc, char *argv[]){
	PICFILE *f;
	Rectangle scr;
	Point size;
	Event e;
	Mouse m;
	char *pname;
	argc=getflags(argc, argv, "mMqw:4[x0 y0 x1 y1]c:2[cenx ceny]");
	switch(argc){
	default: usage("[picfile]");
	case 1: name="IN"; break;
	case 2: name=argv[1]; break;
	}
	f=picopen_ai(name);
	if(f==0){
		perror(name);
		exits("can't open picfile");
	}
	pname=picgetprop(f, "NAME");
	if(pname) name=pname;
	nchan=PIC_NCHAN(f);
	wid=PIC_WIDTH(f);
	hgt=PIC_HEIGHT(f);
	offs=Pt(PIC_XOFFS(f), PIC_YOFFS(f));
	cmap=(uchar *)f->cmap;
	if(flag['w']){
		scr.min.x=atoi(flag['w'][0]);
		scr.min.y=atoi(flag['w'][1]);
		scr.max.x=atoi(flag['w'][2]);
		scr.max.y=atoi(flag['w'][3]);
	}
	else{
		if(flag['c']){
			scr.min.x=atoi(flag['c'][0]);
			scr.min.y=atoi(flag['c'][1]);
		}
		else{
			scr=getscr();
			scr.min=div(add(scr.min, scr.max), 2);
		}
		/*
		 * 8½ wants windows to be at least 100x50.
		 * Rio wants them to be 100x(3*font->height).
		 * Unfortunately, we don't know font->height
		 * at this point, so we'll guess that 50 is
		 * enough, even for rio.
		 */
		size=Pt(max(wid+2*BORDER, 100),
			max(hgt+3*BORDER+FONTHGT+TOPAIR+BOTAIR, 50));
		scr.min=sub(scr.min, div(size, 2));
		scr.max=add(scr.min, size);
	}
	winit(0, 0, argv[0], scr);
	if(!flag['m'] && cmap && screen.ldepth==3){
		putcmap(cmap);
		f->cmap=0;
	}
	if(flag['M']) exits(0);
	if(flag['q']){
		b=rdpicfile(f, screen.ldepth);
		if(b==0){
			fprint(2, "%s: no space for bitmap\n", argv[0]);
			exits("no space");
		}
		einit(Emouse|Ekeyboard);
		ereshaped(screen.r);
		bfree(b);
		event(&e);
		exits(0);
	}
	if(!rdpicarray(f, (uchar *)f->cmap)){
		fprint(2, "%s: can't read image\n", argv[0]);
		exits("no rdpicarray");
	}
	einit(Emouse|Ekeyboard);
	for(;;){
		m=emouse();
		switch(m.buttons){
		case 1:
			showpixel(m.xy.x-origin.x, m.xy.y-origin.y, f->chan);
			break;
		case 2:
			menuhit(2, &m, &menu2);
			break;
		case 4:
			switch(menuhit(3, &m, &menu3)){
			case 0: if(rcmap && screen.ldepth==3) putcmap(rcmap); break;
			case 1: if(confirm(3)) exits(0);
			}
			break;
		}
	}
}
