#include "common.h"
#include <plumb.h>
#include <ctype.h>
#include "util.h"

/*
 * Generate plumber messages as a plan b mail box
 * changes. We run upas/fs and mail2fs in a cpu server
 * and not at the user terminal. Users get their msgs
 * already processed for reading.
 */

enum {
	// for plumbmsg
	Old,
	New
};

typedef struct Msg Msg;
struct Msg{
	char*	path;
	char*	from;
	char*	date;
	Msg*	next;
	int	visited;
};

int	hopt;	// send plumb msgs for old ones
int	debug;
char*	mboxdir;	// only used by plumbmsg()
int	octopus;	// plumb using /mnt/ports/post

void
printmsgs(Msg* m)
{
	if(m == nil)
		fprint(2, "no msgs\n");
	for(; m != nil; m = m->next)
		fprint(2, "%s\tfrom %s\n", m->path, m->from);
}

static int
cmpent(void* a1, void* a2)
{
	Dir*	d1 = a1;
	Dir*	d2 = a2;
	int	n1, n2;

	n1 = atoi(mailnamenb(d1->name));
	n2 = atoi(mailnamenb(d2->name));
	return n2 - n1;
}

Msg*
newmsg(char* dir)
{
	static char buf[512];
	Msg*	m;
	char*	fname;
	int	fd;
	char*	s;
	int	nr;

	fname = smprint("%s/raw", dir);
	m = mallocz(sizeof(Msg), 1);
	if(m == nil || fname == nil)
		sysfatal("not enough memory");
	fd = open(fname, OREAD);
	if(fd < 0){
		// maybe just a new one, give it some time.
		sleep(1000);
		fd = open(fname, OREAD);
	}
	free(fname);
	if(fd < 0)
		goto fail;
	nr = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(nr <= 5 || strncmp(buf, "From ", 5))
		goto fail;
	buf[nr] = 0;
	s = strchr(buf+5, ' ');
	if(s == nil)
		goto fail;
	*s = 0;
	m->from = strdup(buf+5);
	m->path = strdup(dir);
	if(m->from == nil || m->path == nil)
		goto fail;
	return m;
fail:
	/* Message may have been archived in the
	 * mean while. In any case, this is not to
	 * be considered an error.
	 */
	if(debug)
		fprint(2, "%s: %r\n", dir);
	free(m->path);
	free(m->from);
	free(m);
	return nil;
}

void
freemsg(Msg* m)
{
	if(m != nil){
		free(m->path);
		free(m->from);
		free(m->date);
	}
	free(m);
}

int
isnew(Msg* m, Msg* msgs)
{
	for(; msgs != nil; msgs = msgs->next)
		if(strcmp(msgs->path, m->path) == 0){
			msgs->visited = 1;
			return 0;
		}
	return 1;
}
 
void
oplumbmsg(Msg* m, int what)
{
	char*	str[2] = {"gone", "new"};
	char	msg[80];
	char*	e;
	int	fd;

	e = msg+sizeof(msg);
	e = seprint(msg, e, "/msgs: %s %s %s\n", m->path, m->from, str[what]);
	fd = open("/mnt/ports/post", OWRITE);
	if(fd < 0)
		sysfatal("ports open:  %r");
	if(write(fd, msg, e-msg) != e - msg)
		sysfatal("pors write: %r");
	close(fd);
}

void
plumbmsg(Msg* m, int what)
{
	static int fd = -1;
	char*	str[2] = { "delete", "new" };
	Plumbmsg p;
	Plumbattr a[10];
	int ai;

	assert(what == 0 || what == 1);
	if(debug)
		fprint(2, "%s msg %s from %s\n", str[what], m->path, m->from);
	if(octopus){
		oplumbmsg(m, what);
		return;
	}
	memset(&p, 0, sizeof(p));
	p.src = "mailplumb";
	p.dst = "seemail";
	p.wdir = mboxdir;
	p.type = "text";

	ai = 0;
	a[ai].name = "filetype";
	a[ai].value = "mail";

	a[++ai].name = "sender";
	a[ai].value = m->from;
	a[ai-1].next = &a[ai];

	a[++ai].name = "length";
	a[ai].value = "42";
	a[ai-1].next = &a[ai];

	a[++ai].name = "mailtype";
	a[ai].value = str[what];
	a[ai-1].next = &a[ai];

	a[++ai].name = "digest";
	a[ai].value = m->path;
	a[ai-1].next = &a[ai];

	a[ai].next = nil;
	p.attr = a;
	p.ndata = strlen(m->path);
	p.data = m->path;

	if(fd < 0)
		fd = plumbopen("send", OWRITE);
	if(fd < 0)
		sysfatal("plumb: %r");
	if (plumbsend(fd, &p) < 0){
		fprint(2, "plumbsend: %r\n");
		close(fd);
		fd = -1;
	}
}

Msg**
readmdir(char* dir, Msg* msgs, Msg** mp, int initial)
{
	Msg*	m;
	Dir*	d;
	int	n;
	int	fd;
	int	i; 
	char*	tf;

	if(dir == nil)
		return mp;
	fd = open(dir, OREAD);
	if(fd < 0){
		fprint(2, "%s: %r\n", dir);
		return mp;
	}
	n = dirreadall(fd, &d);
	close(fd);
	if(n < 0)
		fprint(2, "%s: %r\n", dir);
	if(n <= 0)
		return mp;
	qsort(d, n, sizeof(Dir), cmpent);
	for(i = 0; i < n; i++)
		if(isdigit(d[i].name[0])){
			tf = smprint("%s/%s", dir, d[i].name);
			m = newmsg(tf);
			free(tf);
			if(m != nil){
				if(isnew(m, msgs)){
					*mp = m;
					mp = &m->next;
					m->visited = 1;
					m->date = strdup(ctime(d[i].mtime));
					m->date[strlen(m->date)-1] = 0;
					if(!initial || hopt)
						plumbmsg(m, New);
				}else
					freemsg(m);
			}
		}
	free(d);
	return mp;
}

Msg*
readmbox(char* mbox, Qid* q, Msg* old, int initial)
{
	Msg**	mp;
	Dir*	d;
	int	fd;
	int	i;
	int	n;
	char*	dir;
	Msg*	msgs;
	Msg*	m;

	msgs = old;
	for(mp = &msgs; *mp != nil; mp = &(*mp)->next)
		(*mp)->visited = 0;
	fd = open(mbox, OREAD);
	if(fd < 0)
		sysfatal("%s: %r", mbox);
	d = dirfstat(fd);
	if(d == nil)
		sysfatal("%s: %r", mbox);
	*q = d->qid;
	free(d);
	n = dirreadall(fd, &d);
	close(fd);
	if(n <= 0)
		return nil;
	qsort(d, n, sizeof(Dir), cmpent);
	for(i = 0; i < n; i++)
		if(d[i].qid.type&QTDIR){
			dir = smprint("%s/%s", mbox, d[i].name);
			mp =  readmdir(dir, msgs, mp, initial);
			free(dir);
		}
	for(mp = &msgs; (m = *mp) != nil; )
		if(!m->visited){
			*mp = m->next;
			plumbmsg(m, Old);
			freemsg(m);
		}else
			mp = &m->next;
	free(d);
	return msgs;
}

void
watchmbox(char* dir, Qid q, Msg* msgs)
{
	Dir*	d;
	Qid	nqid;

	for(;;){
		sleep(octopus ? 5000 : 20000);
		d = dirstat(dir);
		if(d == nil)
			sysfatal("%s: %r", dir);
		nqid = d->qid;
		free(d);
		if(octopus && nqid.path == q.path && nqid.vers == q.vers)
			continue;
		msgs = readmbox(dir, &q, msgs, 0);
	}
}

void
usage(void)
{
	fprint(2, "usage: %s [-dho] [dir]\n", argv0);
	exits("usage");
}

void
main(int argc, char*argv[])
{
	char*	dir;
	char*	top;
	Msg*	m;
	Qid	mqid;
	Msg*	msgs;


	top = smprint("/mail/box/%s", getuser());
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'h':
		hopt++;
		break;
	case 'o':
		octopus++;
		break;
	default:
		usage();
	}ARGEND;
	if(argc == 1)
		dir = cleanpath(argv[0], top);
	else {
		if(argc != 0)
			usage();
		dir = smprint("%s/msgs", top);
	}
	mboxdir = dir;
	msgs = readmbox(dir, &mqid, nil, 1);
	if(debug){
		fprint(2, "initial msgs:\n");
		printmsgs(msgs);
	}
	switch(rfork(RFPROC|RFNOTEG|RFFDG|RFNOWAIT)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		if(hopt)
			for(m = msgs; m != nil; m = m->next)
				plumbmsg(m, New);
		watchmbox(dir, mqid, msgs);
		exits(nil);
	}
	exits(nil);
}
