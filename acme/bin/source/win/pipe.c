/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"

typedef struct Wpid Wpid;
struct Wpid {
	int pid;
	Window *w;
	Wpid *next;
};

void pipectl(void *);

int pipefd;
Wpid *wpid;
int snarffd;
Channel *newpipechan;

int
newpipewin(int pid, char *p)
{
	int id;
	Window *w;
	Wpid *wp;

	w = newwindow();
	winname(w, p);
	wintagwrite(w, "Send ", 5);
	wp = emalloc(sizeof(Wpid));
	wp->pid = pid;
	wp->w = w;
	wp->next = wpid; /* BUG: this happens in fsread proc (we don't use wpid, so it's okay) */
	wpid = wp;
	id = w->id;
	sendp(newpipechan, w);
	return id;
}

int
pipecommand(Window *w, char *s)
{
	uint32_t q0, q1;
	char tmp[32], *t;
	int n, k;

	while(*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if(strcmp(s, "Delete") == 0) {
		windel(w, 1);
		threadexits(nil);
		return 1;
	}
	if(strcmp(s, "Del") == 0) {
		if(windel(w, 0))
			threadexits(nil);
		return 1;
	}
	if(strcmp(s, "Send") == 0) {
		if(w->addr < 0)
			w->addr = winopenfile(w, "addr");
		ctlprint(w->ctl, "addr=dot\n");
		seek(w->addr, 0UL, 0);
		if(read(w->addr, tmp, 2 * 12) == 2 * 12) {
			q0 = atol(tmp + 0 * 12);
			q1 = atol(tmp + 1 * 12);
			if(q0 == q1) {
				t = nil;
				k = 0;
				if(snarffd > 0) {
					seek(0, snarffd, 0);
					for(;;) {
						t = realloc(t, k + 8192 + 2);
						if(t == nil)
							error("alloc failed: %r\n");
						n = read(snarffd, t + k, 8192);
						if(n <= 0)
							break;
						k += n;
					}
					t[k] = 0;
				}
			} else {
				t = emalloc((q1 - q0) * UTFmax + 2);
				winread(w, q0, q1, t);
				k = strlen(t);
			}
			if(t != nil && t[0] != '\0') {
				if(t[k - 1] != '\n' && t[k - 1] != '\004') {
					t[k++] = '\n';
					t[k] = '\0';
				}
				sendit(t);
			}
			free(t);
		}
		return 1;
	}
	return 0;
}

void
pipectl(void *v)
{
	Window *w;
	Event *e;

	w = v;
	proccreate(wineventproc, w, STACK);

	windormant(w);
	winsetaddr(w, "0", 0);
	for(;;) {
		e = recvp(w->cevent);
		switch(e->c1) {
		default:
		Unknown:
			fprint(2, "unknown message %c%c\n", e->c1, e->c2);
			break;

		case 'E': /* write to body; can't affect us */
			break;

		case 'F': /* generated by our actions; ignore */
			break;

		case 'K': /* ignore */
			break;

		case 'M':
			switch(e->c2) {
			case 'x':
			case 'X':
				execevent(w, e, pipecommand);
				break;

			case 'l': /* reflect all searches back to acme */
			case 'L':
				if(e->flag & 2)
					recvp(w->cevent);
				winwriteevent(w, e);
				break;

			case 'I': /* modify away; we don't care */
			case 'i':
			case 'D':
			case 'd':
				break;

			default:
				goto Unknown;
			}
		}
	}
}

void
newpipethread(void *)
{
	Window *w;

	while(w = recvp(newpipechan))
		threadcreate(pipectl, w, STACK);
}

void
startpipe(void)
{
	newpipechan = chancreate(sizeof(Window *), 0);
	threadcreate(newpipethread, nil, STACK);
	snarffd = open("/dev/snarf", OREAD | OCEXEC);
}
