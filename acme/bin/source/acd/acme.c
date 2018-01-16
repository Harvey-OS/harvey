/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "acd.h"

static int
iscmd(char *s, char *cmd)
{
	int len;

	len = strlen(cmd);
	return strncmp(s, cmd, len)==0 && (s[len]=='\0' || s[len]==' ' || s[len]=='\t' || s[len]=='\n');
}

static char*
skip(char *s, char *cmd)
{
	s += strlen(cmd);
	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	return s;
}

//#define PLAYSTRING "/^[0-9:]+>"
//#define PLAYSTRINGSPACE "/^[0-9:]+> ?"
//#define INITSTRING "0:00> "

#define INITSTRING "> "
#define PLAYSTRING "/^>"
#define PLAYSTRINGSPACE "/^> ?"

/*
 * find the playing string, leave in addr
 * if q0, q1 are non-nil, set them to the addr of the string.
 */
int
findplay(Window *w, char *s, uint32_t *q0, uint32_t *q1)
{
	char xbuf[25];
	if(w->data < 0)
		w->data = winopenfile(w, "data");

	if(!winsetaddr(w, "#0", 1) || !winsetaddr(w, s, 1))
		return 0;

	seek(w->addr, 0, 0);
	if(read(w->addr, xbuf, 24) != 24)
		return 0;
	
	xbuf[24] = 0;
	if(q0)
		*q0 = atoi(xbuf);
	if(q1)
		*q1 = atoi(xbuf+12);

	return 1;
}

/*
 * find the playing string and replace the time
 */
int
setplaytime(Window *w, char *new)
{
	char buf[40];
	uint32_t q0, q1;

return 1;
	if(!findplay(w, PLAYSTRING, &q0, &q1))
		return 0;

	q1--;	/* > */
	sprint(buf, "#%lu,#%lu", q0, q1);
	DPRINT(2, "setaddr %s\n", buf);
	if(!winsetaddr(w, buf, 1))
		return 0;
	
	if(write(w->data, new, strlen(new)) != strlen(new))
		return 0;

	return 1;
}

/*
 * find the playing string, and remove it.
 * return the string at the beginning of hte next line in buf
 * (presumably a track number).
 */
static int
unmarkplay(Window *w, char *buf, int n, uint32_t *q0, uint32_t *q1,
	   uint32_t *qbegin)
{
	char xbuf[24];

	if(!findplay(w, PLAYSTRINGSPACE, q0, q1))
		return 0;

	if(write(w->data, "", 0) < 0 || !winsetaddr(w, "+1+#0", 1))
		return 0;

	if(qbegin) {
		seek(w->addr, 0, 0);
		if(read(w->addr, xbuf, 24) != 24)
			return 0;
		*qbegin = atoi(xbuf);
	}

	if(buf) {
		if((n = read(w->data, buf, n-1)) < 0)
			return 0;
	
		buf[n] = '\0';
	}

	return 1;
}

int
markplay(Window *w, uint32_t q0)
{
	char buf[20];

	if(w->data < 0)
		w->data = winopenfile(w, "data");

	sprint(buf, "#%lu", q0);
	DPRINT(2, "addr %s\n", buf);
	if(!winsetaddr(w, buf, 1) || !winsetaddr(w, "-0", 1))
		return 0;
	if(write(w->data, INITSTRING, strlen(INITSTRING)) != strlen(INITSTRING))
		return 0;
	return 1;
}

/* return 1 if handled, 0 otherwise */
int
cdcommand(Window *w, Drive *d, char *s)
{
	s = skip(s, "");

	if(iscmd(s, "Del")){
		if(windel(w, 0))
			threadexitsall(nil);
		return 1;
	}
	if(iscmd(s, "Stop")){
		unmarkplay(w, nil, 0, nil, nil, nil);
		stop(d);
		return 1;
	}
	if(iscmd(s, "Eject")){
		unmarkplay(w, nil, 0, nil, nil, nil);
		eject(d);
		return 1;
	}
	if(iscmd(s, "Ingest")){
		unmarkplay(w, nil, 0, nil, nil, nil);
		ingest(d);
		return 1;
	}
	if(iscmd(s, "Pause")){
		pause(d);
		return 1;
	}
	if(iscmd(s, "Resume")){
		resume(d);
		return 1;
	}
	return 0;
}

void
drawtoc(Window *w, Drive *d, Toc *t)
{
	int i, playing;

	if(w->data < 0)
		w->data = winopenfile(w, "data");
	if(!winsetaddr(w, ",", 1))
		return;

	fprint(w->data, "Title\n\n");
	playing = -1;
	if(d->status.state == Splaying || d->status.state == Spaused)
		playing = d->status.track-t->track0;

	for(i=0; i<t->ntrack; i++)
		fprint(w->data, "%s%d/ Track %d\n", i==playing ? "> " : "", i+1, i+1);
	fprint(w->data, "");
}

void
redrawtoc(Window *w, Toc *t)
{
	int i;
	char old[50];

	if(w->data < 0)
		w->data = winopenfile(w, "data");
	if(t->title) {
		if(winsetaddr(w, "/Title", 1))
			write(w->data, t->title, strlen(t->title));
	}
	for(i=0; i<t->ntrack; i++) {
		if(t->track[i].title) {
			sprint(old, "/Track %d", i+1);
			if(winsetaddr(w, old, 1))
				write(w->data, t->track[i].title, strlen(t->track[i].title));
		}
	}
}

void
advancetrack(Drive *d, Window *w)
{
	int n;
	uint32_t q0, q1, qnext;
	char buf[20];

	q0 = q1 = 0;
	if(!unmarkplay(w, buf, sizeof(buf), &q0, &q1, &qnext)) {
		DPRINT(2, "unmark: %r\n");
		return;
	}

	DPRINT(2, "buf: %s\n", buf);
	if(strncmp(buf, "repeat", 6) == 0) {
		if(!winsetaddr(w, "#0", 1) || !findplay(w, "/^[0-9]+\\/", &qnext, nil)) {
			DPRINT(2, "set/find: %r\n");
			return;
		}
		if(w->data < 0)
			w->data = winopenfile(w, "data");
		if((n = read(w->data, buf, sizeof(buf)-1)) <= 0) {
			DPRINT(2, "read %d: %r\n", n);
			return;
		}
		buf[n] = 0;
		DPRINT(2, "buf: %s\n", buf);
	}

	if((n = atoi(buf)) == 0)
		return;

	if(!markplay(w, qnext))
		DPRINT(2, "err: %r");

	playtrack(d, n-1, n-1);
}

void
acmeevent(Drive *d, Window *w, Event *e)
{
	Event *ea, *e2, *eq;
	char *s, *t, *buf;
	int n, na;
	uint32_t q0, q1;

	switch(e->c1){	/* origin of action */
	default:
	Unknown:
		fprint(2, "unknown message %c%c\n", e->c1, e->c2);
		break;

	case 'E':	/* write to body or tag; can't affect us */
		break;

	case 'F':	/* generated by our actions; ignore */
		break;

	case 'K':	/* type away; we don't care */
		break;

	case 'M':	/* mouse event */
		switch(e->c2){		/* type of action */
		case 'x':	/* mouse: button 2 in tag */
		case 'X':	/* mouse: button 2 in body */
			ea = nil;
		//	e2 = nil;
			s = e->b;
			if(e->flag & 2){	/* null string with non-null expansion */
				e2 = recvp(w->cevent);
				if(e->nb==0)
					s = e2->b;
			}
			if(e->flag & 8){	/* chorded argument */
				ea = recvp(w->cevent);	/* argument */
				na = ea->nb;
				recvp(w->cevent);		/* ignore origin */
			}else
				na = 0;
			
			/* append chorded arguments */
			if(na){
				t = emalloc(strlen(s)+1+na+1);
				sprint(t, "%s %s", s, ea->b);
				s = t;
			}
			/* if it's a known command, do it */
			/* if it's a long message, it can't be for us anyway */
			DPRINT(2, "exec: %s\n", s);
			if(!cdcommand(w, d, s))	/* send it back */
				winwriteevent(w, e);
			if(na)
				free(s);
			break;

		case 'l':	/* mouse: button 3 in tag */
		case 'L':	/* mouse: button 3 in body */
		//	buf = nil;
			eq = e;
			if(e->flag & 2){
				e2 = recvp(w->cevent);
				eq = e2;
			}
			s = eq->b;
			if(eq->q1>eq->q0 && eq->nb==0){
				buf = emalloc((eq->q1-eq->q0)*UTFmax+1);
				winread(w, eq->q0, eq->q1, buf);
				s = buf;
			}
			DPRINT(2, "load %s\n", s);
			if((n = atoi(s)) != 0) {
				DPRINT(2, "mark %d\n", n);
				q0 = q1 = 0;
				unmarkplay(w, nil, 0, &q0, &q1, nil);

				/* adjust eq->q* for deletion */
				if(eq->q0 > q1) {
					eq->q0 -= (q1-q0);
					eq->q1 -= (q1-q0);
				}
				if(!markplay(w, eq->q0))
					DPRINT(2, "err: %r\n");

				playtrack(d, n-1, n-1);
			} else
				winwriteevent(w, e);
			break;

		case 'i':	/* mouse: text inserted in tag */
		case 'I':	/* mouse: text inserted in body */
		case 'd':	/* mouse: text deleted from tag */
		case 'D':	/* mouse: text deleted from body */
			break;

		default:
			goto Unknown;
		}
	}
}
