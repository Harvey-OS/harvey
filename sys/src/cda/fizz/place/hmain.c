#include <u.h>
#include	<libc.h>
#include	<stdio.h>
#include	<cda/fizz.h>
#include	"host.h"
#include	"proto.h"
void sendpin(int );


Board b;
FILE *dbfp = 0;
char mydir[200];
int getn(void);
void getstr(char *);
int ns[1000];
Point pp[1000];
extern int quiet;

int
death(void *a, char *s)	/* don't leave corpses around */
{
	if(strncmp(s,"sys:", 4) == 0){
		print("place: note: %s\n");
		_exits(s);
		return 0;
	}
	return 0;
}

void
main(int argc, char **argv)
{
	int c, *ppp;
	char buf[256];
	Chip *cp;
	Point dif;
	int n, errs, times;
	int ip, side;
	quiet = 1;
/*	atnotify(death, 1); */
	for(;;){
		c = fgetc(stdin);
if(dbfp) fprintf(dbfp, "new rcv cmd %c\n", c);
		errs = 0;
		switch(c)
		{
		case EOF:
		case EXIT:
			quit();
			break;
		case MYDIR:
			getstr(mydir);
			sprint(buf, "%s/fizz.debug", mydir);
			if((dbfp = fopen(buf, "w")) == 0) {
				put1(ERROR);
				putstr("can't open debug file");
			}
			put1(ENDOFILE);
			flush();
			break;
		case READFILE:
			getstr(buf);
			rfile(buf);
			put1(ENDOFILE);
			flush();
			break;
		case WRITEFILE:
			getstr(buf);
			wfile(buf);
			put1(ENDOFILE);
			flush();
			break;
		case CHUNPLACE:
			while(n = getn()){
				cp = cofn(n);
				unplace(cp);
				sendchip(cp, n);
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHSOFT:
			while(n = getn()){
				cp = cofn(n);
				cp->flags = (cp->flags&~PLACE)|SOFT;
				sendchip(cp, n);
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHHARD:
			while(n = getn()){
				cp = cofn(n);
				cp->flags = (cp->flags&~PLACE)|HARD;
				sendchip(cp, n);
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHMANUAL:
			while(n = getn()){
				cp = cofn(n);
				cp->flags = (cp->flags&~PLACE)|NOMOVE;
				sendchip(cp, n);
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHOVER:
			while(n = getn()){
				cp = cofn(n);
				cp->flags ^= IGRECT;
				sendchip(cp, n);
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHNAME:
			while(n = getn()){
				cp = cofn(n);
				cp->flags ^= IGNAME;
				sendchip(cp, n);
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHIGPINS:
			while(n = getn()){
				cp = cofn(n);
				cp->flags ^= IGPINS;
				sendchip(cp, n);
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHROT:
			for(ip = 0; n = getn(); ip++)
				ns[ip] = n;
			for(n = 0; n < ip; n++)
				if(rotchip(ns[n]))
					errs = 1;
			if(errs){
				put1(ERROR);
				putstr("rotate errors");
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHMOVE:
			for(ip = 0; n = getn(); ip++)
				ns[ip] = n;
			dif = getp();
			if(move(ns, pp, ip, dif)){
				put1(ERROR);
				putstr("move errors");
			}
			put1(ENDOFILE);
			flush();
			break;
		case CHINSERT:
			n = getn();
			side = getn();
			insert(n, side, getp());
			put1(ENDOFILE);
			flush();
			break;
		case SENDSIG:
			sendsig(getn());
			put1(ENDOFILE);
			flush();
			break;
		case SENDPIN:
			sendpin(getn());
			put1(ENDOFILE);
			flush();
			break;
		case SENDSIGS:
			sendchsig(getn());
			put1(ENDOFILE);
			flush();
			break;
		case IMPROVEF:
		case IMPROVE1:
		case IMPROVEN:
			if(c==IMPROVEN) times = getn();
			else if(c==IMPROVE1) times = 1;
			else times = 999;
			for(ip = 0; n = getn(); ip++)
				ns[ip] = n;
			improve(ns, ip, times);
			put1(ENDOFILE);
			flush();
			break;
		default:
			sprint(buf, "bad char %c = 0%o", c, 0xFF&c);
			put1(ERROR);
			putstr(buf);
		}
	}
}
void
quit(void)
{
	exits(0);
}

void
putn(int n)
{
	char buf[2];

	buf[0] = n>>8;
	buf[1] = n;
	fwrite(buf, 1, 2, stdout);
}

void
putl(long n)
{
	char buf[4];

	buf[0] = n>>24;
	buf[1] = n>>16;
	buf[2] = n>>8;
	buf[3] = n;
	fwrite(buf, 1, 4, stdout);
}


void
putp(Point p)
{
	putn(p.x);
	putn(p.y);
}

void
putr(Rectangle r)
{
	putp(r.min);
	putp(r.max);
}

void
putstr(char *s)
{
	fwrite(s, 1, strlen(s)+1, stdout);
}

int
getn(void)
{
	unsigned char buf[2];
	register short n;

	fread(buf, 1, 2, stdin);
	n = buf[0]<<8;
	n |= buf[1];
	return(n);
}

Point
getp(void)
{
	Point p;

	p.x = getn();
	p.y = getn();
	return(p);
}

Rectangle
getr(void)
{
	Rectangle r;

	r.min = getp();
	r.max = getp();
	return(r);
}

void
getstr(char *s)
{
	while(*s = getchar())
		s++;
}
