/*
 * Trivial web browser
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <panel.h>
#include "mothra.h"
Panel *root;	/* the whole display */
Panel *cmd;	/* command entry */
Panel *curttl;	/* label giving the title of the visible text */
Panel *cururl;	/* label giving the url of the visible text */
Panel *list;	/* list of previously acquired www pages */
Panel *msg;	/* message display */
Mouse mouse;	/* current mouse data */
Www www[NWWW];
Url defurl={
	"http://research.att.com/",
	0,
	"research.att.com",
	"/",
	"",
	80,
	HTTP,
	HTML
};
Url badurl={
	"No file loaded",
	0,
	"",
	"/dev/null",
	"",
	0,
	FILE,
	HTML
};
Cursor patientcurs={
	0, 0,
	0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x07, 0xe0,
	0x07, 0xe0, 0x07, 0xe0, 0x03, 0xc0, 0x0F, 0xF0,
	0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8,
	0x0F, 0xF0, 0x1F, 0xF8, 0x3F, 0xFC, 0x3F, 0xFC,

	0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x04, 0x20,
	0x04, 0x20, 0x06, 0x60, 0x02, 0x40, 0x0C, 0x30,
	0x10, 0x08, 0x14, 0x08, 0x14, 0x28, 0x12, 0x28,
	0x0A, 0x50, 0x16, 0x68, 0x20, 0x04, 0x3F, 0xFC,
};
Cursor confirmcurs={
	0, 0,
	0x0F, 0xBF, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE,
	0xFF, 0xFE, 0xFF, 0xFF, 0x00, 0x03, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFC,

	0x00, 0x0E, 0x07, 0x1F, 0x03, 0x17, 0x73, 0x6F,
	0xFB, 0xCE, 0xDB, 0x8C, 0xDB, 0xC0, 0xFB, 0x6C,
	0x77, 0xFC, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
	0x94, 0xA6, 0x63, 0x3C, 0x63, 0x18, 0x94, 0x90
};
Cursor readingcurs={
	{-7, -7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x0F, 0xC0, 0x0F, 0xC0,
	 0x03, 0xC0, 0x07, 0xC0, 0x0E, 0xC0, 0x1C, 0xC0,
	 0x38, 0x00, 0x70, 0x00, 0xE0, 0xDB, 0xC0, 0xDB,}
};
int nwww=1;
Www *current=0;
Url *selection=0;
int logfile;
void docmd(Panel *, char *);
void doprev(Panel *, int, int);
void selurl(char *);
void setcurrent(int, char *);
char *genwww(Panel *, int);
void updtext(Www *);
void dolink(Panel *, int, Rtext *);
void hit3(int, int);
char *buttons[]={
	"save back",
	"get back",
	"fix cmap",
	"exit",
	0
};
void err(char *msg){
	fprint(2, "err: %s (%r)\n", msg);
	abort();
}
void mkpanels(void){
	Panel *p, *pp, *bar, *menu3;
	menu3=plmenu(0, 0, buttons, PACKN|FILLX, hit3);
	root=plgroup(0, EXPAND);
		p=plpopup(root, PACKN|FILLX, 0, 0, menu3);
			msg=pllabel(p, PACKN|FILLX, "mothra");
			plplacelabel(msg, PLACEW);
			pllabel(p, PACKW, "command:");
			cmd=plentry(p, PACKE|EXPAND, 0, "", docmd);
		p=plgroup(root, PACKN|FILLX);
			bar=plscrollbar(p, PACKW);
			pp=plpopup(p, PACKN|FILLX, 0, 0, menu3);
				list=pllist(pp, PACKN|FILLX, genwww, 8, doprev);
			plscroll(list, 0, bar);
		p=plpopup(root, PACKN|FILLX, 0, 0, menu3);
			pllabel(p, PACKW, "Title:");
			curttl=pllabel(p, PACKE|EXPAND, "Initializing");
			plplacelabel(curttl, PLACEW);
		p=plpopup(root, PACKN|FILLX, 0, 0, menu3);
			pllabel(p, PACKW, "Url:");
			cururl=pllabel(p, PACKE|EXPAND, "---");
			plplacelabel(cururl, PLACEW);
		p=plgroup(root, PACKN|EXPAND);
			bar=plscrollbar(p, PACKW);
			pp=plpopup(p, PACKE|EXPAND, 0, 0, menu3);
				text=pltextview(pp, PACKE|EXPAND, Pt(0, 0), 0, dolink);
			plscroll(text, 0, bar);
	plgrabkb(cmd);
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
int getmap(char *cmapname){
	uchar cmap[256*3];
	int nbit, npix;
	RGB map[256];
	int i, j;
	if(!getcmap(cmapname, cmap)) return 0;
	nbit=1<<screen.ldepth;
	npix=1<<nbit;
	for(j=0;j!=npix;j++){
		i=3*255*j/(npix-1);
		map[npix-1-j].red=rep(cmap[i], 8);
		map[npix-1-j].green=rep(cmap[i+1], 8);
		map[npix-1-j].blue=rep(cmap[i+2], 8);
	}
	wrcolmap(&screen, map);
	return 1;
}
void killcohort(void){
	int i;
	for(i=0;i!=3;i++){	/* It's a long way to the kitchen */
		postnote(PNGROUP, getpid(), "kill\n");
		sleep(1);
	}
}
void dienow(void*, char*){
	noted(NDFLT);
}
void main(int argc, char *argv[]){
	Event e;
	char *url;
	char *henv;
	char filename[512];
	int errfile;
	/*
	 * so that we can stop all subprocesses with a note,
	 * and to isolate rendezvous from other processes
	 */
	rfork(RFNOTEG|RFNAMEG);
	atexit(killcohort);
	switch(argc){
	default:
		fprint(2, "Usage: %s [url]\n", argv[0]);
		exits("usage");
	case 1:
		url=getenv("url");
		if(url==0 || url[0]=='\0')
			url="file:/sys/lib/mothra/start.html";
		break;
	case 2: url=argv[1]; break;
	}
	henv=getenv("home");
	if(henv)
		sprint(home, "%s/lib/mothra", henv);
	else
		strcpy(home, "/tmp");
	sprint(filename, "%s/mothra.err", home);
	errfile=create(filename, OWRITE, 0666);
	if(errfile==-1)
		errfile=create("mothra.err", OWRITE, 0666);
	if(errfile!=-1){
		dup(errfile, 2);
		close(errfile);
	}
	sprint(filename, "%s/mothra.log", home);
	logfile=open(filename, OWRITE);
	if(logfile==-1)
		logfile=create(filename, OWRITE, 0666);
	binit(err,0,"mothra");
	chrwidth=strwidth(font, "0");
	getmap("9");
	einit(Emouse|Ekeyboard);
	etimer(0, 1000);
	plinit(screen.ldepth);
	getfonts();
	hrule=balloc(Rect(0,0,1280, 5), screen.ldepth);
	if(hrule==0){
		fprint(2, "%s: can't balloc!\n", argv[0]);
		exits("no mem");
	}
	bitblt(hrule, hrule->r.min, hrule, Rect(0,1,1280,3), F);
	bullet=balloc(Rect(0,0,25, 8), screen.ldepth);
	disc(bullet, Pt(4,4), 3, ~0, S);
	www[0].text=0;
	www[0].url=&badurl;
	strcpy(www[0].title, "See error message above");
	plrtstr(&www[0].text, 0, 0, font, "See error message above", 0, 0);
	www[0].alldone=1;
	mkpanels();
	ereshaped(screen.r);
	geturl(url, GET, 0, 1);
	if(logfile==-1) message("Can't open log file");
	mouse.buttons=0;
	for(;;){
		if(mouse.buttons==0 && current){
			if(current->finished){
				updtext(current);
				current->finished=0;
				current->changed=0;
				current->alldone=1;
				cursorswitch(0);
			}
			else if(current->changed){
				updtext(current);
				current->changed=0;
			}
		}
		switch(event(&e)){
		case Ekeyboard:
			plkeyboard(e.kbdc);
			break;
		case Emouse:
			mouse=e.mouse;
			if(mouse.buttons) plgrabkb(cmd);
			plmouse(root, e.mouse);
			break;
		}
	}
}
void message(char *s, ...){
	static char buf[1024];
	char *out;
	out = doprint(buf, buf+sizeof(buf), s, (&s+1));
	*out='\0';
	plinitlabel(msg, PACKN|FILLX, buf);
	pldraw(msg, &screen);
}
void htmlerror(char *name, int line, char *m, ...){
	static char buf[1024];
	char *out;
	out=buf+sprint(buf, "%s: line %d: ", name, line);
	out=doprint(out, buf+sizeof(buf)-1, m, (&m+1));
	*out='\0';
	fprint(2, "%s\n", buf);
}
void ereshaped(Rectangle r){
	screen.r=r;
	r=inset(r, 4);
	message("");
	plinitlabel(curttl, PACKE|EXPAND, "---");
	plinitlabel(cururl, PACKE|EXPAND, "---");
	plpack(root, r);
	if(current){
		plinitlabel(curttl, PACKE|EXPAND, current->title);
		plinitlabel(cururl, PACKE|EXPAND, current->url->fullname);
	}
	bitblt(&screen, r.min, &screen, r, Zero);
	pldraw(root, &screen);
}
void *emalloc(int n){
	void *v;
	v=malloc(n);
	if(v==0){
		fprint(2, "out of space\n");
		exits("no mem");
	}
	return v;
}
char *genwww(Panel *, int index){
	int i;
	for(i=1;i!=nwww;i++) if(index==www[i].index) return www[i].title;
	return 0;
}
void donecurs(void){
	cursorswitch(current && current->alldone?0:&readingcurs);
}
/*
 * selected text should be a url.
 * get the document
 */
void setcurrent(int index, char *tag){
	static char buf[1024];
	Www *new;
	Rtext *tp;
	Action *ap;
	int i;
	new=&www[index];
	if(new==current && (tag==0 || tag[0]==0)) return;
	if(current)
		current->top=plgetpostextview(text);
	current=new;
	plinitlabel(curttl, PACKE|EXPAND, current->title);
	pldraw(curttl, &screen);
	plinitlabel(cururl, PACKE|EXPAND, current->url->fullname);
	pldraw(cururl, &screen);
	plinittextview(text, PACKE|EXPAND, Pt(0, 0), current->text, dolink);
	if(tag && tag[0]){
		for(tp=current->text;tp;tp=tp->next){
			ap=tp->user;
			if(ap && ap->name && strcmp(ap->name, tag)==0){
				current->top=tp;
				break;
			}
		}
	}
	else if(!current->top)
		current->top=current->text;
	plsetpostextview(text, current->top);
	if(index!=0){
		for(i=1;i!=nwww;i++) if(www[i].index<current->index) www[i].index++;
		current->index=0;
		plinitlist(list, PACKN|FILLX, genwww, 8, doprev);
		pldraw(list, &screen);
	}
	donecurs();
	bflush();
}
char *arg(char *s){
	do ++s; while(*s==' ' || *s=='\t');
	return s;
}
void save(Url *url, char *name){
	int ofd, ifd, n;
	char buf[4096];
	ofd=create(name, OWRITE, 0444);
	if(ofd==-1){
		message("%s: %r", name);
		return;
	}
	cursorswitch(&patientcurs);
	ifd=urlopen(url, GET, 0);
	donecurs();
	if(ifd==-1){
		message("%s: %r", selection->fullname);
		close(ofd);
	}
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		message("Can't fork -- please wait");
		cursorswitch(&patientcurs);
		while((n=read(ifd, buf, 4096))>=0)
			write(ofd, buf, n);
		donecurs();
		break;
	case 0:
		while((n=read(ifd, buf, 4096))>=0)
			write(ofd, buf, n);
		if(n==-1) fprint(2, "%s: %r\n", url->fullname);
		_exits(0);
	}
	close(ifd);
	close(ofd);
}
/*
 * user typed a command.
 */
void docmd(Panel *p, char *s){
	USED(p);
	while(*s==' ' || *s=='\t') s++;
	switch(s[0]){
	default:
		message("Unknown command %s, type h for help", s);
		break;
	case '?':
	case 'h':
		geturl("file:/sys/lib/mothra/help.html", GET, 0, 1);
		break;
	case 'g':
		s=arg(s);
		if(*s=='\0') message("Usage: g url");
		else geturl(s, GET, 0, 1);
		break;
	case 's':
		s=arg(s);
		if(*s=='\0'){
			message("Usage: s file");
			break;
		}
		save(selection, s);
		break;
	case 'q':
		bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
		exits(0);
	}
	plinitentry(cmd, EXPAND, 0, "", docmd);
	pldraw(cmd, &screen);
}
void hiturl(int buttons, char *url){
	message("");
	switch(buttons){
	case 1: geturl(url, GET, 0, 1); break;
	case 2: selurl(url); break;
	case 4: message("Button 3 hit on url can't happen!"); break;
	}
}
/*
 * user selected from the list of available pages
 */
void doprev(Panel *p, int buttons, int index){
	int i;
	USED(p);
	message("");
	for(i=1;i!=nwww;i++) if(www[i].index==index){
		hiturl(buttons, www[i].url->fullname);
		break;
	}
}
/*
 * Follow an html link
 * According to Mosaic source, to support images with ISMAP:
 *	sprint(url, "%s?%d,%d", a->link, sub(mouse.xy, word->r.min));
 */
void dolink(Panel *p, int buttons, Rtext *word){
	char mapurl[512];
	Action *a;
	Point coord;
	Rtext *top;
	USED(p);
	a=word->user;
	if(a->link){
		if(a->ismap){
			top=plgetpostextview(p);
			coord=sub(sub(mouse.xy, word->r.min), p->r.min);
			sprint(mapurl, "%s?%d,%d", a->link, coord.x, coord.y+top->topy);
			hiturl(buttons, mapurl);
		}
		else
			hiturl(buttons, a->link);
	}
}
void filter(char *cmd, int fd, int teefd){
	char cmdbuf[512];
	if(teefd!=-1) sprint(cmdbuf, "tee -a /fd/%d | %s", teefd, cmd);
	else strcpy(cmdbuf, cmd);
	bflush();
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		message("Can't fork!");
		break;
	case 0:
		close(0);
		bclose();
		dup(fd, 0);
		close(fd);
		execl("/bin/rc", "rc", "-c", cmdbuf, 0);
		message("Can't exec /bin/rc!");
		_exits(0);
	default:
		break;
	}
	close(teefd);
	close(fd);
}
void gettext(Www *w, int fd, int type){
	bflush();
	switch(rfork(RFFDG|RFPROC|RFNOWAIT|RFMEM)){
	case -1:
		message("Can't fork, please wait");
		if(type==HTML)
			plrdhtml(w->url->fullname, fd, w);
		else
			plrdplain(w->url->fullname, fd, w);
		break;
	case 0:
		if(type==HTML)
			plrdhtml(w->url->fullname, fd, w);
		else
			plrdplain(w->url->fullname, fd, w);
		_exits(0);
	}
	close(fd);
}
void popwin(char *cmd){
	bflush();
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		message("sorry, can't fork to %s", cmd);
		break;
	case 0:
		bclose();
		execl("/bin/window", "window", "100 100 800 800", "rc", "-c", cmd, 0);
		_exits(0);
	}
}
/*
 * Caching bugs:
 *	There's no way to tell if the cached copy is complete.
 *	One way to fix this is to change the name in the create to
 *	write.%.8lux and move the file to cache.%.8lux when reading succeeds.
 *	The cache should record the modification date of the file.
 *	Cached copies should be removed if reading fails.
 */
int cacheopen(Url *url){
#ifndef broken
	url->cachefd=-1;
	return -1;
#else
	ulong hash;
	int fd, n;
	char hashname[513], *s, *l;
	/* the hash should include the modification date of the file */
	hash=0;
	for(s=url->fullname,n=0;*s;s++,n++) hash=hash*n+(*s&255);
	sprint(hashname, "%s/cache.%.8lux", home, hash);
	fd=open(hashname, OREAD);
	if(fd<0){
		url->cachefd=create(hashname, OWRITE, 0444);
		fprint(url->cachefd, "%s\n", url->fullname);
		return -1;
	}
	url->cachefd=-1;
	for(l=hashname;l!=&hashname[512];l+=n){
		n=&hashname[512]-l;
		n=read(fd, l, n);
		if(n<=0) break;
	}
	*l='\0';
	s=strchr(hashname, '\n');
	if(s==0){
		close(fd);
		return -1;
	}
	*s='\0';
	if(strcmp(url->fullname, hashname)!=0){
		close(fd);
		return -1;
	}
	seek(fd, s-hashname+1, 0);
	return fd;
#endif
}
int urlopen(Url *url, int method, char *body){
#ifdef securityHole
	int pfd[2];
#endif
	int fd;
	Url prev;
	int nredir;
	nredir=0;
Again:
	if(++nredir==NREDIR){
		werrstr("redir loop");
		return -1;
	}
	seek(logfile, 0, 2);
	fd=cacheopen(url);
	if(fd>=0) return fd;
	fprint(logfile, "%s\n", url->fullname);
	switch(url->access){
	default:
		werrstr("unknown access type");
		return -1;
	case FTP:
		werrstr("sorry, no ftp!");
		return -1;
	case HTTP:
		fd=http(url, method, body);
		if(url->type==FORWARD){
			prev=*url;
			crackurl(url, prev.redirname, &prev);
			goto Again;
		}
		return fd;	
	case FILE:
		url->type=suffix2type(url->reltext);
		return open(url->reltext, OREAD);
	case GOPHER:
		return gopher(url);
#ifdef securityHole
	case EXEC:
		/*
		 * Can you say `security hole'?  I knew you could.
		 */
		if(pipe(pfd)==-1) return -1;
		switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
		case -1:
			close(pfd[0]);
			close(pfd[1]);
			return -1;
		case 0:
			dup(pfd[1], 1);
			close(pfd[0]);
			close(pfd[1]);
			execl("/bin/rc", "rc", "-c", url->reltext, 0);
			_exits("no exec!");
		default:
			close(pfd[1]);
			return pfd[0];
		}
#endif
	}
}
int uncompress(char *cmd, int fd){
	int pfd[2];
	if(pipe(pfd)==-1){
		close(fd);
		return -1;
	}
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		close(pfd[0]);
		close(pfd[1]);
		close(fd);
		return -1;
	case 0:
		dup(fd, 0);
		dup(pfd[1], 1);
		close(pfd[0]);
		close(pfd[1]);
		close(fd);
		execl(cmd, cmd, 0);
		_exits("no exec!");
	default:
		close(pfd[1]);
		close(fd);
		return pfd[0];
	}
}
/*
 * select the file at the given url
 */
void selurl(char *urlname){
	Url *cur;
	static Url url;
	if(current) cur=current->url;
	else cur=&defurl;
	crackurl(&url, urlname, cur);
	selection=&url;
	message("select %s", selection->fullname);
}
Url *copyurl(Url *u){
	Url *v;
	v=emalloc(sizeof(Url));
	*v=*u;
	return v;
}
/*
 * get the file at the given url
 */
void geturl(char *urlname, int method, char *body, int cache){
	int i, fd;
	char cmd[512];
	selurl(urlname);
	if(cache){
		for(i=1;i!=nwww;i++){
			if(strcmp(www[i].url->fullname, selection->fullname)==0){
				setcurrent(i, selection->tag);
				return;
			}
		}
	}
	message("getting %s", selection->fullname);
	cursorswitch(&patientcurs);
	switch(selection->access){
	default:
		message("unknown access %d", selection->access);
		break;
	case TELNET:
		sprint(cmd, "telnet %s", selection->reltext);
		popwin(cmd);
		break;
	case MAILTO:
		sprint(cmd, "mail %s", selection->reltext);
		popwin(cmd);
		break;
	case FTP:
	case HTTP:
	case FILE:
	case EXEC:
	case GOPHER:
		fd=urlopen(selection, method, body);
		if(fd!=-1){
			if(selection->type&COMPRESS)
				fd=uncompress("/bin/uncompress", fd);
			else if(selection->type&GUNZIP)
				fd=uncompress("/bin/pub/gunzip", fd);
		}
		if(fd==-1){
			message("%r");
			setcurrent(0, 0);
			break;
		}
		switch(selection->type&~COMPRESS){
		default:
			message("Bad type %x in geturl", selection->type);
			break;
		case PLAIN:
		case HTML:
			www[nwww].url=copyurl(selection);
			www[nwww].index=nwww;
			gettext(&www[nwww], fd, selection->type&~COMPRESS);
			nwww++;
			plinitlist(list, PACKN|FILLX, genwww, 8, doprev);
			pldraw(list, &screen);
			setcurrent(nwww-1, selection->tag);
			break;
		case POSTSCRIPT:
			filter("window page '0 0 850 1100' /fd/0", fd, selection->cachefd);
			break;
		case GIF:
			filter("fb/gif2pic -m|fb/9v", fd, selection->cachefd);
			break;
		case JPEG:
			if(screen.ldepth==3)
				filter("fb/jpg2pic|fb/3to1 9|fb/9v", fd, selection->cachefd);
			else
				filter("fb/jpg2pic|fb/9v", fd, selection->cachefd);
			break;
		case XBM:
			filter("fb/xbm2pic|fb/9v", fd, selection->cachefd);
			break;
		}
	}
	donecurs();
}
void updtext(Www *w){
	Rtext *t;
	Action *a;
	for(t=w->text;t;t=t->next){
		a=t->user;
		if(a){
			if(a->imagebits)
				setbitmap(t);
			if(a->field)
				mkfieldpanel(t);
			a->field=0;
		}
	}
	if(w->top==0)
		w->top=w->text;
	else
		w->top=plgetpostextview(text);
	plinittextview(text, PACKE|EXPAND, Pt(0, 0), w->text, dolink);
	plsetpostextview(text, w->top);
	pldraw(root, &screen);
}
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
	donecurs();
	return down.buttons==(1<<(b-1));
}
void hit3(int button, int item){
	char name[100], *home;
	int fd;
	USED(button);
	message("");
	switch(item){
	case 0:
		home=getenv("home");
		if(home==0){
			message("no $home");
			return;
		}
		sprint(name, "%s/lib/mothra/back.html", home);
		fd=open(name, OWRITE);
		if(fd==-1){
			fd=create(name, OWRITE, 0666);
			if(fd==-1){
				message("can't open %s", name);
				return;
			}
			fprint(fd, "<head><title>Back List</title></head>\n");
			fprint(fd, "<body><h1>Back list</h1>\n");
		}
		seek(fd, 0, 2);
		fprint(fd, "<p><a href=\"%s\">%s</a>\n",
			selection->fullname, selection->fullname);
		close(fd);
		break;
	case 1:
		home=getenv("home");
		if(home==0){
			message("no $home");
			return;
		}
		sprint(name, "file:%s/lib/mothra/back.html", home);
		geturl(name, GET, 0, 1);
		break;
	case 2:
		getmap("9");
		break;
	case 3:
		if(confirm(3)){
			bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
			exits(0);
		}
		break;
	}
}
