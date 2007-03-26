#include "all.h"

struct {
	char*	name;
	Userid	uid;
	Userid	lead;
} minusers[] = {
	"adm",		-1,	-1,
	"none",		0,	-1,
	"tor",		1,	1,
	"sys",		10000,	0,
	"map",		10001,	10001,
	"doc",		10002,	0,
	"upas",		10003,	10003,
	"font",		10004,	0,
	"bootes",	10005,	10005,
	0
};

static char buf[4096];
static Rune ichar[] = L"?=+-/:";

Uid*	chkuid(char *name, int chk);
void	do_newuser(int, char*[]);
char*	getword(char*, Rune, char*, int);
void	pentry(char*, Uid*);
int	readln(char*, int);
void	setminusers(void);
Uid*	uidtop(int);

void
cmd_users(int argc, char *argv[])
{
	Uid *ui;
	int u, g, o, line;
	char *file, *p, *uname, *ulead, *unext;

	file = "/adm/users";
	if(argc > 1)
		file = argv[1];

	if(strcmp(file, "default") == 0) {
		setminusers();
		return;
	}

	uidgc.uidbuf = getbuf(devnone, Cuidbuf, 0);
	if(walkto(file) || con_open(FID2, 0)) {
		print("cmd_users: cannot access %s\n", file);
		putbuf(uidgc.uidbuf);
		return;
	}

	uidgc.flen = 0;
	uidgc.find = 0;
	cons.offset = 0;
	cons.nuid = 0;

	u = 0;
	line = 0;
	while(readln(buf, sizeof buf) != 0) {
		line++;
		p = getword(buf, L':', "no : after number", line);
		if(p == nil)
			continue;
		ulead = getword(p, L':', "no : after name", line);
		if(ulead == nil)
			continue;

		if(strlen(p) > NAMELEN-1) {
			print("%s: name too long\n", p);
			continue;
		}
		strcpy(uid[u].name, p);
		uid[u].uid = number(buf, 0, 10);
		uid[u].lead = 0;
		uid[u].ngrp = 0;
		u++;
		if(u >= conf.nuid) {
			print("conf.nuid too small (%ld)\n", conf.nuid);
			break;
		}
	}

	/* Sorted by uid for use in uidtostr */
	wlock(&uidgc.uidlock);
	qsort(uid, u, sizeof(uid[0]), byuid);
	cons.nuid = u;
	wunlock(&uidgc.uidlock);

	/* Parse group table */
	uidgc.flen = 0;
	uidgc.find = 0;
	cons.offset = 0;
	cons.ngid = 0;

	g = 0;
	line = 0;
	while(readln(buf, sizeof buf) != 0) {
		line++;
		uname = getword(buf, L':', 0, 0);	/* skip number */
		if(uname == nil)
			continue;

		ulead = getword(uname, L':', 0, 0);	/* skip name */
		if(ulead == nil)
			continue;

		p = getword(ulead, L':', "no : after leader", line);
		if(p == nil)
			continue;

		ui = uidpstr(uname);
		if(ui == nil)
			continue;

		/* set to owner if name not known */
		ui->lead = 0;
		if(ulead[0]) {
			o = strtouid(ulead);
			if(o >= 0)
				ui->lead = o;
			else
				ui->lead = ui->uid;
		}
		ui->gtab = &gidspace[g];
		ui->ngrp = 0;
		while (p != nil) {
			unext = getword(p, L',', 0, 0);
			o = strtouid(p);
			if(o >= 0) {
				gidspace[g++] = o;
				ui->ngrp++;
			}
			p = unext;
		}
	}

	cons.ngid = g;

	putbuf(uidgc.uidbuf);
	print("%d uids read, %d groups used\n", cons.nuid, cons.ngid);
}

void
cmd_newuser(int argc, char *argv[])
{
	if(argc <= 1) {
		print("usage: newuser args\n");
		print("\tname -- create a new user\n");
		print("\tname : -- create a new group\n");
		print("\tname ? -- show entry for user\n");
		print("\tname name -- rename\n");
		print("\tname =[name] -- add/alter/remove leader\n");
		print("\tname +name -- add member\n");
		print("\tname -name -- delete member\n");
		return;
	}
	do_newuser(argc, argv);
}

void
do_newuser(int argc, char *argv[])
{
	int i, l, n, nuid;
	char *p, *md, *q;
	Rune *r;
	Userid *s;
	Uid *ui, *u2;

	nuid = 10000;
	md = 0;
	if(argc == 2) {
		nuid = 1;
		argv[2] = ":";
	}

	for(r = ichar; *r; r++)
		if(utfrune(argv[1], *r)) {
			print("illegal character in name\n");
			return;
		}
	if(strlen(argv[1]) > NAMELEN-1) {
		print("name %s too long\n", argv[1]);
		return;
	}

	p = argv[2];
	switch(*p) {
	case '?':
		ui = chkuid(argv[1], 1);
		if(ui == 0)
			return;
		pentry(buf, ui);
		n = strlen(buf);
		p = buf;
		while(n > PRINTSIZE-5) {
			q = p;
			p += PRINTSIZE-5;
			n -= PRINTSIZE-5;
			i = *p;
			*p = 0;
			print("%s", q);
			*p = i;
		}
		print("%s\n", p);
		return;

	case ':':
		if(chkuid(argv[1], 0))
			return;
		while(uidtop(nuid) != 0)
			nuid++;
		if(cons.nuid >= conf.nuid) {
			print("conf.nuid too small (%ld)\n", conf.nuid);
			return;
		}

		wlock(&uidgc.uidlock);
		ui = &uid[cons.nuid++];
		ui->uid = nuid;
		ui->lead = 0;
		if(nuid < 10000) {
			ui->lead = ui->uid;
			md = argv[1];
		}
		strcpy(ui->name, argv[1]);
		ui->ngrp = 0;
		qsort(uid, cons.nuid, sizeof(uid[0]), byuid);
		wunlock(&uidgc.uidlock);
		break;

	case '=':
		ui = chkuid(argv[1], 1);
		if(ui == 0)
			return;
		p++;
		if(*p == '\0') {
			ui->lead = 0;
			break;
		}
		u2 = chkuid(p, 1);
		if(u2 == 0)
			return;
		ui->lead = u2->uid;
		break;

	case '+':
		ui = chkuid(argv[1], 1);
		if(ui == 0)
			return;
		p++;
		u2 = chkuid(p, 1);
		if(u2 == 0)
			return;
		if(u2->uid == ui->uid)
			return;
		if(cons.ngid+ui->ngrp+1 >= conf.gidspace) {
			print("conf.gidspace too small (%ld)\n", conf.gidspace);
			return;
		}
		for(i = 0; i < ui->ngrp; i++) {
			if(ui->gtab[i] == u2->uid) {
				print("member already in group\n");
				return;
			}
		}

		wlock(&uidgc.uidlock);
		s = gidspace+cons.ngid;
		memmove(s, ui->gtab, ui->ngrp*sizeof(*s));
		ui->gtab = s;
		s[ui->ngrp++] = u2->uid;
		cons.ngid += ui->ngrp+1;
		wunlock(&uidgc.uidlock);
		break;

	case '-':
		ui = chkuid(argv[1], 1);
		if(ui == 0)
			return;
		p++;
		u2 = chkuid(p, 1);
		if(u2 == 0)
			return;
		for(i = 0; i < ui->ngrp; i++)
			if(ui->gtab[i] == u2->uid)
				break;

		if(i == ui->ngrp) {
			print("%s not in group\n", p);
			return;
		}

		wlock(&uidgc.uidlock);
		s = ui->gtab+i;
		ui->ngrp--;
		memmove(s, s+1, (ui->ngrp-i)*sizeof(*s));
		wunlock(&uidgc.uidlock);
		break;

	default:
		if(chkuid(argv[2], 0))
			return;

		for(r = ichar; *r; r++)
			if(utfrune(argv[2], *r)) {
				print("illegal character in name\n");
				return;
			}

		ui = chkuid(argv[1], 1);
		if(ui == 0)
			return;

		if(strlen(argv[2]) > NAMELEN-1) {
			print("name %s too long\n", argv[2]);
			return;
		}

		wlock(&uidgc.uidlock);
		strcpy(ui->name, argv[2]);
		wunlock(&uidgc.uidlock);
		break;
	}


	if(walkto("/adm/users") || con_open(FID2, OWRITE|OTRUNC)) {
		print("can't open /adm/users for write\n");
		return;
	}

	cons.offset = 0;
	for(i = 0; i < cons.nuid; i++) {
		pentry(buf, &uid[i]);
		l = strlen(buf);
		n = con_write(FID2, buf, cons.offset, l);
		if(l != n)
			print("short write on /adm/users\n");
		cons.offset += n;
	}

	if(md != 0) {
		sprint(buf, "create /usr/%s %s %s 755 d", md, md, md);
		print("%s\n", buf);
		cmd_exec(buf);
	}
}

Uid*
chkuid(char *name, int chk)
{
	Uid *u;

	u = uidpstr(name);
	if(chk == 1) {
		if(u == 0)
			print("%s does not exist\n", name);
	}
	else {
		if(u != 0)
			print("%s already exists\n", name);
	}
	return u;
}

void
pentry(char *buf, Uid *u)
{
	int i, posn;
	Uid *p;

	posn = sprint(buf, "%d:%s:", u->uid, u->name);
	p = uidtop(u->lead);
	if(p && u->lead != 0)
		posn += sprint(buf+posn, "%s", p->name);

	posn += sprint(buf+posn, ":");
	for(i = 0; i < u->ngrp; i++) {
		p = uidtop(u->gtab[i]);
		if(i != 0)
			posn += sprint(buf+posn, ",");
		if(p != 0)
			posn += sprint(buf+posn, "%s", p->name);
		else
			posn += sprint(buf+posn, "%d", u->gtab[i]);
	}
	sprint(buf+posn, "\n");
}

void
setminusers(void)
{
	int u;

	for(u = 0; minusers[u].name; u++) {
		strcpy(uid[u].name, minusers[u].name);
		uid[u].uid = minusers[u].uid;
		uid[u].lead = minusers[u].lead;
	}
	cons.nuid = u;
	qsort(uid, u, sizeof(uid[0]), byuid);
}

Uid*
uidpstr(char *name)
{
	Uid *s, *e;

	s = uid;
	for(e = s+cons.nuid; s < e; s++) {
		if(strcmp(name, s->name) == 0)
			return s;
	}
	return 0;
}

char*
getword(char *buf, Rune delim, char *error, int line)
{
	char *p;

	p = utfrune(buf, delim);
	if(p == 0) {
		if(error)
			print("cmd_users: %s line %d\n", error, line);
		return 0;
	}
	*p = '\0';
	return p+1;
}

int
strtouid(char *name)
{
	Uid *u;
	int id;

	rlock(&uidgc.uidlock);

	u = uidpstr(name);
	id = -2;
	if(u != 0)
		id = u->uid;

	runlock(&uidgc.uidlock);

	return id;
}

Uid*
uidtop(int id)
{
	Uid *bot, *top, *new;

	bot = uid;
	top = bot + cons.nuid-1;

	while(bot <= top){
		new = bot + (top - bot)/2;
		if(new->uid == id)
			return new;
		if(new->uid < id)
			bot = new + 1;
		else
			top = new - 1;
	}
	return 0;
}

void
uidtostr(char *name, int id, int dolock)
{
	Uid *p;

	if(dolock)
		rlock(&uidgc.uidlock);

	p = uidtop(id);
	if(p == 0)
		strcpy(name, "none");
	else
		strcpy(name, p->name);

	if(dolock)
		runlock(&uidgc.uidlock);
}

int
ingroup(int u, int g)
{
	Uid *p;
	Userid *s, *e;

	if(u == g)
		return 1;

	rlock(&uidgc.uidlock);
	p = uidtop(g);
	if(p != 0) {
		s = p->gtab;
		for(e = s + p->ngrp; s < e; s++) {
			if(*s == u) {
				runlock(&uidgc.uidlock);
				return 1;
			}
		}
	}
	runlock(&uidgc.uidlock);
	return 0;
}

int
leadgroup(int ui, int gi)
{
	int i;
	Uid *u;

	/* user 'none' cannot be a group leader */
	if(ui == 0)
		return 0;

	rlock(&uidgc.uidlock);
	u = uidtop(gi);
	if(u == 0) {
		runlock(&uidgc.uidlock);
		return 0;
	}
	i = u->lead;
	runlock(&uidgc.uidlock);
	if(i == ui)
		return 1;
	if(i == 0)
		return ingroup(ui, gi);

	return 0;
}

int
byuid(void *a1, void *a2)
{
	Uid *u1, *u2;

	u1 = a1;
	u2 = a2;
	return u1->uid - u2->uid;
}

int
fchar(void)
{
	int n;

	n = BUFSIZE;
	if(n > MAXDAT)
		n = MAXDAT;
	if(uidgc.find >= uidgc.flen) {
		uidgc.find = 0;
		uidgc.flen = con_read(FID2, uidgc.uidbuf->iobuf, cons.offset, n);
		if(uidgc.flen <= 0)
			return -1;
		cons.offset += uidgc.flen;
	}
	return (uchar)uidgc.uidbuf->iobuf[uidgc.find++];
}

int
readln(char *p, int len)
{
	int n, c;

	n = 0;
	while(len--) {
		c = fchar();
		if(c == -1 || c == '\n')
			break;
		n++;
		*p++ = c;
	}
	*p = '\0';
	return n;
}
