#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"

Biobuf	*mf;
Font	*tinyfont;		/* to overwrite unknowns */
Font	*medifont;		/* for icon captions */
char	machine[512];		/* machine in latest ikon */
char	*label;			/* machine label for this ikon, if any */
char	token[64];
int	First=1, Same=1;
char	toklen;

void
start_trail(char *Log)
{
	int f, n;

	if(user[0] == '\0'){
		n = 0;
		if((f = open("/dev/user", OREAD)) < 0
		||  (n = read(f, user, sizeof(user)-1)) <= 0)
			error("can't read /dev/user");
		user[n] = '\0';
		close(f);
	}
	tinyfont = rdfontfile("/lib/font/bit/misc/ascii.5x7.font", 0);
	if(tinyfont == 0)
		tinyfont = font;
	medifont = rdfontfile("/lib/font/bit/pelm/latin1.8.font", 0);
	if(medifont == 0)
		medifont = font;

	toklen = sprint(token, "delivered %s From ", user);
	restart(Log, 1, aflag);
}

void
restart(char *log, int fatal, int showall)
{
	if(mf != 0)
		Bclose(mf);
	mf = Bopen(log, OREAD);
	if(mf == 0){
		if(fatal)
			error("can't open log");
		return;
	}
	if(!showall)
		Bseek(mf, 0, 2);
}

void
trail(char *log)
{
	char *match;
	char *line;
	char buf[ERRLEN];
	int i;
	Dir d;

	if(mf == 0){
		restart(log, 0, 1);
		if(mf == 0)
			return;
	}
	for(i=0; i<15; i++){
		errstr(buf);		/* clear error */
		if((line=Brdline(mf, '\n')) == 0){
			errstr(buf);
			if(strcmp(buf, "no error") != 0){
				restart(log, 0, aflag);
				if(mf == 0)
					return;
			}
			if(dirfstat(Bfildes(mf), &d) < 0)
				return;
			if(d.length < Boffset(mf))
				restart(log, 0, 1);
			return;
		}
		line[Blinelen(mf)-1] = 0;
		match = strstr(line, token);
		if(match)
			incoming(match);
	}
}

void
overwrite(Bitmap *face)
{
	Point p, q;
	char *s;

	s = label;
	if(s==0 || *s==0)
		return;
	q = strsize(tinyfont, s);
	p = Pt(face->r.min.x, face->r.max.y - 2 - q.y);
	if(q.x < face->r.max.x - face->r.min.x)
		p.x += (face->r.max.x - face->r.min.x-q.x)/2;
	bitblt(face, sub(p, Pt(1, 1)), face,
		Rpt(sub(p, Pt(1, 1)), add(add(p, q), Pt(1, 1))), 0);
	string(face, p, tinyfont, s, S|D);
}

void
incoming(char *line)
{
	char *p, *q, *r, *newmachine;

	newmachine = "";
	if((p=strchr(&line[toklen], ' ')) == 0)
		return;
	if((q=strchr(p, ':')) == 0)
		q = "whois";
	else{
		q[3] = '\0';
		q -= 2;
	}
	*p = 0;	/* end of user-name */
	p = strrchr(&line[toklen], '!');
	if(p == 0){
		p = &line[toklen];
		if((r = strchr(&line[toklen], '@')) != 0){
			*r = '\0';
			newmachine = r+1;
		}
	}else{
		*p = '\0';
		for(r=p-1; r>line; r--)
			if(*r == ' ' || *r == '!'){
				newmachine = r+1;
				break;
			}
		p++;	/* start of user-name */
	}
	sayit(p);
	puticon(newmachine, p, q);
}

char lastmachine[512];
char lastuser[512];
char lastlabel[512];

void
puticon(char *mach, char *user, char *what)
{
	char buf[512];

	memmove(&old, &new, sizeof(SRC));
	geticon(&new, mach, user);
	if(label == 0)
		label = "";
	if(First)
		First=0;
	else if(sflag &&
            (strcmp(realmachine, lastmachine) == 0) &&
	    (strcmp(user, lastuser) == 0) &&
	    (strcmp(label, lastlabel) == 0)){
		Same++;
		twirl(&old, &new);
		sprint(buf, "%d*%s", Same, user);
		user = buf;
		nomessage();
	}else{
		wipe(&old, &new);
		Same = 1;
	}
	if(Same < 2){
		strncpy(lastmachine, realmachine, sizeof(lastmachine));
		strncpy(lastuser, user, sizeof(lastuser));
		strncpy(lastlabel, label, sizeof(lastlabel));
	}
	showimage(&new, 0);
	message(user, what);
	bflush();
}

void
sayit(char *user)
{
	static int vfd, inited;
	if (!inited) {
		inited = 1;
		vfd = open("/voice/ascii", OWRITE);
	}
	if (vfd < 0)
		return;
	fprint(vfd, "mail from %s\n", user);
}
