#include <u.h>
#include <libc.h>
#include <libg.h>
#include "term.h"
#include "menu.h"
#include "proto.h"

extern char message[];
extern Font * defont;

Font *
gfont(char * file)
{
	Font *f;
	f = rdfontfile(file, 0);
	if(f == 0){
		perror("rdf");
		exits("rdf");
	}
	return(f);
}

int remotefd0, remotefd1;
extern int errfd;
char mydirbuf[100];
char * mydir;
int
alnum(char c)
{
	return ('0'<=c && c<='9') || (c=='_') ||
	       ('a'<=c && c<='z') || ('A'<=c && c<='Z');
}


void
addachar(char * s, int c)
{
	char *p;
	for (p = s; *p; p++);
	if (c >= ' ')			/* append c */
		*p++ = c;
	else if (c == '\b' && p > s)	/* backspace */
		p--;
	else if (c == 027) {		/* control-W */
		while (p > s && !alnum(*(p-1)))
			p--;
		while (p > s && alnum(*(p-1)))
			p--;
	}
	*p = 0;
}

char *
getline(char *msg, char *buf)
{
	Point p;
	int c;

	if(message[0]) 
		string(&screen, scrn.bname.min, defont, message, S^D);
	else if(caption)
		string(&screen, scrn.bname.min, defont, caption, S^D);
	p = scrn.bname.min;
	twostring(p, msg, buf);
	while ((c = ekbd()) != '\n') {
		twostring(p,msg,buf);
		addachar(buf,c);
		twostring(p,msg,buf);
	}
	twostring(p,msg,buf);
	if(message[0]) 
		string(&screen, scrn.bname.min, defont, message, S^D);
	else if(caption)
		string(&screen, scrn.bname.min, defont, caption, S^D);
	return buf;
}


void
getmydir(void)
{
	char buf[100];
	if(!mydir | (*mydir == 0)) {
		mydir = mydirbuf;
		if(getwd(mydir, 100) == (char *) 0) {
			hosterr(buf, 0);
			return;
		}
	}
	fprint(errfd, "getmydir: %s\n", mydir);
}

void
call_host(char *machine)
{
	char buf[100];
	fprint(errfd, "call_host %s.exec\n", machine);
	if(Connect(machine, "/bin/cda/aux/hplace")) {
		dup(remotefd0, 0);
		dup(remotefd1, 1);
		close(remotefd0);
		close(remotefd1);
		put1(MYDIR);
		puts(mydir);
		while(rcv());
		return;
	}
	fprint(errfd,"can't open %s on %s", "/bin/cda/aux/hplace", machine);
	hosterr(buf, 0);
	close(remotefd0);
	close(remotefd1);
	remotefd0 = remotefd1 = 0;
}

int
Connect(char *machine, char *cmd)
{
	int p1[2], p2[2];

	if(pipe(p1)<0 || pipe(p2)<0){
		fprint(errfd,"can't pipe /bin/rx\n");
		exits("pipe");
	}
	remotefd0 = p1[0];
	remotefd1 = p2[1];
	switch(fork()){
	case 0:
		dup(p2[0], 0);
		dup(p1[1], 1);
		close(p1[0]);
		close(p1[1]);
		close(p2[0]);
		close(p2[1]);
		if (machine[0])
			execl("/bin/rx", "rx", machine, cmd, 0);
		else
			execl("/bin/rc", "rc", "-c", cmd, 0);
		fprint(errfd,"can't exec /bin/rx\n");
		exits("exec");

	case -1:
		fprint(errfd,"can't fork\n");
		exits("fork");
	}
	close(p1[1]);
	close(p2[0]);
	return(1);
}

void
sendnchars(int n, char *s)
{
	write(1, s, n);
}

int
rcvchar(void)
{
	static char buf[1000];
	static char *bp;
	static int n = 0;
	if(n == 0) {
		n = read(0, buf, 1000);
		bp = buf;
	}
	n--;
	return(0xff & *bp++);
}

void
twostring(Point p,char * a,char * b)
{
	string(&screen,string(&screen,p, defont,a,S^D), defont,b,S^D);	/* show off */
}

int
getc(void)
{
	return(rcvchar());
}

int
getn(void)
{
	short k, n;

	n = rcvchar();
	k = rcvchar();
	k |= (n<<8);
	return(k);
}


void poot(void)
{
}
