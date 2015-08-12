/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum { False,
       True,
       EVENTSIZE = 256,
};

typedef struct Event Event;
struct Event {
	int c1;
	int c2;
	int q0;
	int q1;
	int flag;
	int nb;
	int nr;
	char b[EVENTSIZE * UTFmax + 1];
	Rune r[EVENTSIZE + 1];
};

typedef struct Win Win;
struct Win {
	int winid;
	int addr;
	Biobuf* body;
	int ctl;
	int data;
	int event;
	char buf[512];
	char* bufp;
	int nbuf;
};

int dead(Win*);
void wnew(Win*);
void wwritebody(Win*, char* s, int n);
void wread(Win*, uint, uint, char*);
void wclean(Win*);
void wname(Win*, char*);
void wdormant(Win*);
void wevent(Win*, Event*);
void wtagwrite(Win*, char*, int);
void wwriteevent(Win*, Event*);
void wslave(Win*, Channel*); /* chan(Event) */
void wreplace(Win*, char*, char*, int);
void wselect(Win*, char*);
int wdel(Win*);
int wreadall(Win*, char**);

void ctlwrite(Win*, char*);
int getec(Win*);
int geten(Win*);
int geter(Win*, char*, int*);
int openfile(Win*, char*);
void openbody(Win*, int);
