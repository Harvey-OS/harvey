#include "all.h"

static void	uxfree(Unixid*);

static Unixid *	xfree;

Unixidmap *idhead, *idtail;

Unixscmap *scmap;

#define	UNUSED	0x7FFFFFFF

/*
 * Sadly we have to use the IP address, since some systems (FreeBSD in particular)
 * do not believe it to be safe to depend on the hostname and so refuse to send it.
 * I dislike making this IP-centric, but so be it.
 * We keep a cache of host names in getdom.
 */
Unixidmap *
pair2idmap(char *server, ulong clientip)
{
	Resub match;
	Unixscmap *m, *mp;
	Unixidmap *r;
	char dom[256];

	for(mp=0,m=scmap; m; mp=m,m=m->next){
		if(m->server[0] != server[0])
			continue;
		if(strcmp(m->server, server))
			continue;
		if(m->clientip != clientip)
			continue;
		if(mp){
			mp->next = m->next;
			m->next = scmap;
			scmap = m;
		}
		r = m->map;
		if(r->u.timestamp != 0 && r->g.timestamp != 0)
			return r;
		scmap = m->next;
		free(m);
		break;
	}
	if(rpcdebug)
		fprint(2, "looking for %lux\n", clientip);
	if(getdom(clientip, dom, sizeof dom)<0){
		clog("auth: unknown ip address");
		return nil;
	}
	if(rpcdebug)
		fprint(2, "dom is %s\n", dom);
	for(r=idhead; r; r=r->next){
		if(r->u.timestamp == 0 || r->g.timestamp == 0)
			continue;
		match.sp = match.ep = 0;
		if(regexec(r->sexp, server, &match, 1) == 0)
			continue;
		if(match.sp != server || match.ep <= match.sp || *match.ep)
			continue;
		match.sp = match.ep = 0;
		if(regexec(r->cexp, dom, &match, 1) == 0)
			continue;
		if(match.sp != dom || match.ep <= match.sp || *match.ep)
			continue;
		m = malloc(sizeof(Unixscmap));
		m->next = scmap;
		scmap = m;
		m->server = strstore(server);
		m->clientip = clientip;
		m->map = r;
		break;
	}
	return r;
}

int
readunixidmaps(char *file)
{
	Waitmsg *w;
	Biobuf *in;
	Unixidmap *m;
	int i, arc; char *arv[16], buf[256];
	char *l;
// 	long savalarm;

// 	savalarm = alarm(0);
	in = Bopen(file, OREAD);
	if(in == 0){
		clog("readunixidmaps can't open %s: %r\n", file);
//		alarm(savalarm);
		return -1;
	}
	for(m=idhead; m; m=m->next)
		m->flag = 0;
	while(l = Brdline(in, '\n')){	/* assign = */
		l[Blinelen(in)-1] = 0;
		arc = strparse(l, nelem(arv), arv);
		if(arc > 0 && arv[0][0] == '!'){
			++arv[0];
			snprint(buf, sizeof buf, "/bin/%s", arv[0]);
			if(chatty){
				chat("!");
				for(i=0; i<arc; i++)
					chat(" %s", arv[i]);
				chat("...");
			}
			w = system(buf, arv);
			if(w == nil)
				chat("err: %r\n");
			else if(w->msg && w->msg[0])
				chat("status: %s\n", w->msg);
			else
				chat("OK\n");
			free(w);
			continue;
		}
		if(arc != 4)
			continue;
		for(m=idhead; m; m=m->next)
			if(strcmp(arv[0], m->server) == 0 &&
			   strcmp(arv[1], m->client) == 0)
				break;
		if(m == 0){
			m = malloc(sizeof(Unixidmap));
			if(idtail)
				idtail->next = m;
			else
				idhead = m;
			idtail = m;
			m->next = 0;
			m->server = strstore(arv[0]);
			m->client = strstore(arv[1]);
			m->sexp = regcomp(m->server);
			m->cexp = regcomp(m->client);
			m->u.file = strstore(arv[2]);
			m->u.style = 'u';
			m->u.timestamp = 0;
			m->u.ids = 0;
			m->g.file = strstore(arv[3]);
			m->g.style = 'u';
			m->g.timestamp = 0;
			m->g.ids = 0;
		}else{
			if(!m->u.file || strcmp(m->u.file, arv[2]) != 0){
				m->u.file = strstore(arv[2]);
				m->u.timestamp = 0;
			}
			if(!m->g.file || strcmp(m->g.file, arv[3]) != 0){
				m->g.file = strstore(arv[3]);
				m->g.timestamp = 0;
			}
		}
		m->flag = 1;
		checkunixmap(&m->u);
		checkunixmap(&m->g);
	}
	Bterm(in);
	for(m=idhead; m; m=m->next)
		if(m->flag == 0){
			m->u.file = 0;
			m->u.timestamp = 0;
			uxfree(m->u.ids);
			m->u.ids = 0;
			m->g.file = 0;
			m->g.timestamp = 0;
			uxfree(m->g.ids);
			m->g.ids = 0;
		}
// 	alarm(savalarm);
	return 0;
}

static void
uxfree(Unixid *x)
{
	Unixid *tail;
	int count=0;

	if(x){
		tail = x;
		if(tail->id < 0)
			abort();
		tail->id = UNUSED;
		while(tail->next){
			tail = tail->next;
			++count;
			if(tail->id == UNUSED)
				abort();
			tail->id = UNUSED;
		}
		tail->next = xfree;
		xfree = x;
	}
}

int
checkunixmap(Unixmap *u)
{
	Dir *dir;

	dir = dirstat(u->file);
	if(dir == nil){
		clog("checkunixmap can't stat %s: %r\n", u->file);
		return -1;
	}
	if(u->timestamp > dir->mtime){
		free(dir);
		return 0;
	}
	uxfree(u->ids);
	u->ids = readunixids(u->file, u->style);
	u->timestamp = time(0);
	free(dir);
	return 1;
}

int
name2id(Unixid **list, char *name)
{
	Unixid *x, *xp;

	for(xp=0,x=*list; x; xp=x,x=x->next){
		if(x->name[0] == name[0] && strcmp(x->name, name) == 0){
			if(xp){
				xp->next = x->next;
				x->next = *list;
				*list = x;
			}
			return x->id;
		}
	}
	return -1;
}

char *
id2name(Unixid **list, int id)
{
	Unixid *x, *xp;

	for(xp=0,x=*list; x; xp=x,x=x->next){
		if(x->id == id){
			if(xp){
				xp->next = x->next;
				x->next = *list;
				*list = x;
			}
			return x->name;
		}
	}
	return "none";
}

void
idprint(int fd, Unixid *xp)
{
	while(xp){
		fprint(fd, "%d\t%s\n", xp->id, xp->name);
		xp = xp->next;
	}
}

/*
 *	style '9': 3:tom:tom:
 *	style 'u': sysadm:*:0:0:System-Administrator:/usr/admin:/bin/sh
 */

Unixid *
readunixids(char *file, int style)
{
	Biobuf *in;
	char *l, *name = 0;
	Unixid *x, *xp = 0;
	int id = 0;

	in = Bopen(file, OREAD);
	if(in == 0){
		clog("readunixids can't open %s: %r\n", file);
		return 0;
	}
	while(l = Brdline(in, '\n')){	/* assign = */
		l[Blinelen(in)-1] = 0;
		switch(style){
		case '9':
			id = strtol(l, &l, 10);
			if(*l != ':')
				continue;
			name = ++l;
			l = strchr(l, ':');
			if(l == 0)
				continue;
			*l = 0;
			break;
		case 'u':
			name = l;
			l = strchr(l, ':');
			if(l == 0)
				continue;
			*l++ = 0;
			/* skip password */
			l = strchr(l, ':');
			if(l == 0)
				continue;
			id = strtol(l+1, 0, 10);
			break;
		default:
			panic("unknown unixid style %d\n", style);
		}
		if(id == UNUSED)
			id = -1;	/* any value will do */
		if(!(x = xfree))	/* assign = */
			x = listalloc(1024/sizeof(Unixid), sizeof(Unixid));
		xfree = x->next;
		x->id = id;
		x->name = strstore(name);
		x->next = xp;
		xp = x;
	}
	Bterm(in);
	return xp;
}
