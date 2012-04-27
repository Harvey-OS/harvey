#include "all.h"

/*
 * Locking is coarse, only functions used from outside
 * care to lock the user information.
 *
 * Access checks are like those described in Plan 9's stat(5), but for:
 * 
 * - to change gid, the group leader is not required to be a leader
 *   of the new group; it suffices if he's a member.
 * - attempts to change muid are honored if the user is "allowed",
 *   and ignored otherwise.
 * - "allowed" users can also set atime.
 * - attributes other than length, name, uid, gid, muid, mode, atime, and mtime
 *   are ignored (no error raised if they are not void)
 * 
 *
 * The user file has the format:
 *	uid:name:leader:members
 * uid is a number.
 *
 * This program insists on preserving uids already seen.
 * That is, after editing /active/adm/users, the server program will notice
 * and re-read the file, then clean it up, and upate its contents.
 *
 * Cleaning ensures that uids for known users are kept as they were, and
 * that users not yet seen get unique uids. Numeric uids are only an internal
 * concept, the protocol uses names.
 */

/*
 * The uid numbers are irrelevant, they are rewritten.
 */
static char *defaultusers = 
	"1:none::\n"
	"2:adm:adm:sys, elf \n"
	"3:sys::glenda,elf\n"
	"4:glenda:glenda:\n"
	"5:elf:elf:sys\n";

static RWLock ulk;
static Usr *uids[Uhashsz];
static Usr *unames[Uhashsz];
static Usr *uwrite;
static int uidgen;

static uint
usrhash(char* s)
{
	uchar *p;
	uint hash;

	hash = 0;
	for(p = (uchar*)s; *p != '\0'; p++)
		hash = hash*7 + *p;

	return hash % Uhashsz;
}

static int
findmember(Usr *u, int member)
{
	Member *m;

	for(m = u->members; m != nil; m = m->next)
		if(member == m->u->id)
			return 1;
	return 0;
}

static Usr*
finduid(int uid)
{
	Usr *u;

	for(u = uids[uid%Uhashsz]; u != nil; u = u->inext)
		if(u->id == uid)
			return u;
	return nil;
}

static Usr*
finduname(char *name, int mkit)
{
	Usr *u;
	uint h;

	h = usrhash(name);
	for(u = unames[h]; u != nil; u = u->nnext)
		if(strcmp(u->name, name) == 0)
			return u;
	if(mkit){
		/* might be leaked. see freeusr() */
		u = mallocz(sizeof *u, 1);
		strecpy(u->name, u->name+sizeof u->name, name);
		u->nnext = unames[h];
		unames[h] = u;
	}
	return u;
}

char*
usrname(int uid)
{
	Usr *u;

	xrwlock(&ulk, Rd);
	u = finduid(uid);
	if(u == nil){
		xrwunlock(&ulk, Rd);	/* zero patatero: */
		return "ZP";		/*    disgusting, isn't it? */
	}
	xrwunlock(&ulk, Rd);
	return u->name;
}

int
usrid(char *n)
{
	Usr *u;

	xrwlock(&ulk, Rd);
	u = finduname(n, Dontmk);
	if(u == nil || !u->enabled){
		xrwunlock(&ulk, Rd);
		return -1;
	}
	xrwunlock(&ulk, Rd);
	return u->id;
}

int
member(int uid, int member)
{
	Usr *u;
	int r;

	if(uid == member)
		return 1;
	xrwlock(&ulk, Rd);
	u = finduid(uid);
	r = u != nil && u->lead != nil && u->lead->id == member;
	r |= u != nil && findmember(u, member);
	xrwunlock(&ulk, Rd);
	return r;
}

int
leader(int gid, int lead)
{
	Usr *u;
	int r;

	xrwlock(&ulk, Rd);
	u = finduid(gid);
	r = 0;
	if(u != nil)
		if(u->lead != nil)
			r = u->lead->id == lead;
		else
			r = findmember(u, lead);
	xrwunlock(&ulk, Rd);
	return r;
}

static void
clearmembers(Usr *u)
{
	Member *m;

	while(u->members != nil){
		m = u->members;
		u->members = m->next;
		free(m);
	}
}

static void
addmember(Usr *u, char *n)
{
	Member *m, **ml;

	for(ml = &u->members; (m = *ml) != nil; ml = &m->next)
		if(strcmp(m->u->name, n) == 0){
			xrwunlock(&ulk, Wr);
			fprint(2, "%s: '%s' is already a member of '%s'",
				argv0, n, u->name);
			return;
		}
	m = mallocz(sizeof *m, 1);
	m->u = finduname(n, Mkit);
	*ml = m;
}

static void
checkmembers(Usr *u)
{
	Member *m, **ml;

	for(ml = &u->members; (m = *ml) != nil; )
		if(m->u->id == 0){
			fprint(2, "no user '%s' (member of '%s')\n",
				m->u->name, u->name);
			*ml = m->next;
			free(m);
		}else
			ml = &m->next;
}

int
usrfmt(Fmt *fmt)
{
	Usr *usr;
	Member *m;

	usr = va_arg(fmt->args, Usr*);

	if(usr == nil)
		return fmtprint(fmt, "#no user");
	fmtprint(fmt, "%s%d:%s:", usr->enabled?"":"!",
		usr->id, usr->name);
	fmtprint(fmt, "%s:", usr->lead?usr->lead->name:"");
	for(m = usr->members; m != nil; m = m->next){
		fmtprint(fmt, "%s", m->u->name);
		if(m->next != nil)
			fmtprint(fmt, ",");
	}
	return 0;
}

static void
dumpusers(void)
{
	int i;
	Usr *usr;

	for(i = 0; i < nelem(uids); i++)
		for(usr = uids[i]; usr != nil; usr = usr->inext){
			fprint(2, "%A\n", usr);
		}
}

/*
 * Add a user.
 * A partial user entry might already exists, as a placeholder
 * for the user name (if seen before in the file).
 * If the user was known, it's uid is preserved.
 * If not, a new unique uid is assigned.
 */
static Usr*
mkusr(char *name)
{
	Usr *u;
	uint h;

	u = finduname(name, Mkit);
	if(u->id == 0){
		/* first seen! */
		u->id = ++uidgen;
		h = u->id%Uhashsz;
		u->inext = uids[h];
		uids[h] = u;
	}
	if(strcmp(name, "write") == 0)
		uwrite = u;
	return u;
}

static void
addusr(char *p)
{
	char *c, *nc, *s, *args[5];
	int nargs, on;
	Usr *usr;

	on = 1;
	if(*p == '!'){
		on = 0;
		p++;
	}
	nargs = getfields(p, args, nelem(args), 0, ":");
	if(nargs != 4)
		error("wrong number of fields %s", args[0]);
	if(*args[1] == 0)
		error("null name");
	usr = mkusr(args[1]);
	usr->enabled = on;
	usr->lead = finduname(args[2], Mkit);
	clearmembers(usr);
	for(c = args[3]; c != nil; c = nc){
		while(*c == ' ' || *c == '\t')
			c++;
		if(*c == 0)
			break;
		nc = utfrune(c, ',');
		if(nc != nil)
			*nc++ = 0;
		s = utfrune(c, ' ');
		if(s != nil)
			*s = 0;
		s = utfrune(c, '\t');
		if(s != nil)
			*s = 0;
		if(*c != 0)
			addmember(usr, c);
	}
}

/*
 * Absorb the new user information as read from u.
 * Old users are not removed, but renamed to be disabled.
 */
static void
rwdefaultusers(void)
{
	char *u, *c, *p, *np;
	static int once;

	if(once++ > 0)
		return;

	u = strdup(defaultusers);
	if(catcherror()){
		free(u);
		error(nil);
	}
	p = u;
	do{
		np = utfrune(p, '\n');
		if(np != nil)
			*np++ = 0;
		c = utfrune(p, '#');
		if(c != nil)
			*c = 0;
		if(*p == 0)
			continue;
		if(catcherror()){
			fprint(2, "users: %r\n");
			consprint("users: %r\n");
			continue;
		}
		addusr(p);
		noerror();
	}while((p = np) != nil);

	if(dbg['d']){
		dprint("users:\n");
		dumpusers();
		dprint("\n");
	}
	noerror();
	free(u);

}

/*
 * This should be called at start time and whenever
 * the user updates /active/adm/users, to rewrite it according to our
 * in memory data base.
 */
void
rwusers(Memblk *uf)
{
	static char ubuf[512];
	char *p, *nl, *c;
	uvlong off;
	u64int z;
	long tot, nr, nw;
	int i;
	Usr *usr;

	xrwlock(&ulk, Wr);
	if(catcherror()){
		fprint(2, "%s: users: %r\n", argv0);
		goto update;
	}
	if(uf == nil){
		rwdefaultusers();
		xrwunlock(&ulk, Wr);
		return;
	}
	tot = 0;
	p = nil;
	for(off = 0; off < uf->d.length; off += nr){
		nr = dfpread(uf, ubuf + tot, sizeof ubuf - tot - 1, off);
		tot += nr;
		ubuf[tot] = 0;
		for(p = ubuf; p != nil && p - ubuf < tot; p = nl){
			nl = utfrune(p, '\n');
			if(nl == nil){
				tot = strlen(p);
				memmove(ubuf, p, tot+1);
				break;
			}
			*nl++ = 0;
			c = utfrune(p, '#');
			if(c != nil)
				*c = 0;
			if(*p != 0)
				addusr(p);
		}
	}
	if(p != nil)
		fprint(2, "%s: last line in users is not a full line\n", argv0);

	noerror();
	if(uf->frozen){	/* loaded at boot time */
		xrwunlock(&ulk, Wr);
		return;
	}

update:
	if(catcherror()){
		xrwunlock(&ulk, Wr);
		fprint(2, "%s: users: %r\n", argv0);
		return;	/* what could we do? */
	}
	ismelted(uf);
	isrwlocked(uf, Wr);
	z = 0;
	dfwattr(uf, "length", &z, sizeof z);
	off = 0;
	dprint("users updated:\n");
	for(i = 0; i < uidgen; i++)
		if((usr=finduid(i)) != nil){
			dprint("%A\n", usr);
			p = seprint(ubuf, ubuf+sizeof ubuf, "%A\n", usr);
			nw = dfpwrite(uf, ubuf, p - ubuf, &off);
			off += nw;
		}
	noerror();
	xrwunlock(&ulk, Wr);
}

int
writedenied(int uid)
{
	int r;

	if(uwrite == nil)
		return 0;
	xrwlock(&ulk, Rd);
	r = findmember(uwrite, uid) == 0;
	xrwunlock(&ulk, Rd);
	return r;
}

int
allowed(int uid)
{
	Usr *u;
	int r;

	xrwlock(&ulk, Rd);
	u = finduid(uid);
	r = 0;
	if(u)
		r = u->allow;
	xrwunlock(&ulk, Rd);
	return r;
}

/*
 * TODO: register multiple fids for the cons file by keeping a list
 * of console channels.
 * consread will have to read from its per-fid channel.
 * conprint will have to bcast to all channels.
 *
 * With that, multiple users can share the same console.
 * Although perhaps it would be easier to use C in that case.
 */

void
consprint(char *fmt, ...)
{
	va_list	arg;
	char *s, *x;

	va_start(arg, fmt);
	s = vsmprint(fmt, arg);
	va_end(arg);
	/* consume some message if the channel is full */
	while(nbsendp(fs->consc, s) == 0)
		if((x = nbrecvp(fs->consc)) != nil)
			free(x);
}

long
consread(char *buf, long count)
{
	char *s;
	int tot, nr;

	if(count <= 0)		/* shouldn't happen */
		return 0;
	quiescent(Yes);
	s = recvp(fs->consc);
	quiescent(No);
	tot = 0;
	do{
		nr = strlen(s);
		if(tot + nr > count)
			nr = count - tot;
		memmove(buf+tot, s, nr);
		tot += nr;
		free(s);
	}while((s = nbrecvp(fs->consc)) != nil && tot + 80 < count);
	/*
	 * +80 to try to guarantee that we have enough room in the user
	 * buffer for the next received string, or we'd drop part of it.
	 * Most of the times each string is a rune typed by the user.
	 * Other times, it's the result of a consprint() call.
	 */
	return tot;
}

static void
cdump(int argc, char *argv[])
{
	switch(argc){
	case 1:
		fsdump(0, strcmp(argv[0], "dumpall") == 0);
		break;
	case 2:
		if(strcmp(argv[1], "-l") == 0){
			fsdump(1, strcmp(argv[0], "dumpall") == 0);
			break;
		}
		/*fall*/
	default:
		consprint("usage: %s [-l]\n", argv[0]);
		return;
	}
}

static void
csync(int, char**)
{
	fssync();
	consprint("synced\n");
}

static void
chalt(int, char**)
{
	fssync();
	threadexitsall(nil);
}

static void
cusers(int, char *[])
{
	int i;
	Usr *usr;

	xrwlock(&ulk, Rd);
	if(catcherror()){
		xrwunlock(&ulk, Rd);
		error(nil);
	}
	for(i = 0; i < uidgen; i++)
		if((usr=finduid(i)) != nil)
			consprint("%A\n", usr);
	noerror();
	xrwunlock(&ulk, Rd);
}

static void
cstats(int argc, char *argv[])
{
	int clr;

	clr  =0;
	if(argc == 2 && strcmp(argv[1], "-c") == 0){
		clr = 1;
		argc--;
	}
	if(argc != 1){
		consprint("usage: %s [-c]\n", argv[0]);
		return;
	}
	consprint("%s\n", updatestats(clr));
}

static void
cdebug(int, char *argv[])
{
	char *f;
	char flags[50];
	int i;

	f = argv[1];
	if(strcmp(f, "on") == 0){
		dbg['D'] = 1;
		return;
	}
	if(strcmp(f, "off") == 0){
		memset(dbg, 0, sizeof dbg);
		return;
	}
	if(*f != '+' && *f != '-')
		memset(dbg, 0, sizeof dbg);
	else
		f++;
	for(; *f != 0; f++){
		dbg[*f] = 1;
		if(*argv[1] == '-')
			dbg[*f] = 0;
	}
	f = flags;
	for(i = 0; i < nelem(dbg) && f < flags+nelem(flags)-1; i++)
		if(dbg[i])
			*f++ = i;
	*f = 0;
	consprint("debug = '%s'\n", flags);
		
}

static void
clocks(int, char *argv[])
{
	if(strcmp(argv[1], "on") == 0)
		lockstats(1);
	else if(strcmp(argv[1], "off") == 0)
		lockstats(0);
	else if(strcmp(argv[1], "dump") == 0)
		dumplockstats();
	else
		consprint("usage: %s [on|off|dump]\n", argv[0]);
}

static void
cfids(int, char**)
{
	dumpfids();
}

static void
crwerr(int, char *argv[])
{
	if(*argv[0] == 'r'){
		swreaderr = atoi(argv[1]);
		fprint(2, "%s: sw read err count = %d\n", argv0, swreaderr);
	}else{
		swwriteerr = atoi(argv[1]);
		fprint(2, "%s: sw write err count = %d\n", argv0, swwriteerr);
	}
}

static void
ccheck(int argc, char *argv[])
{
	switch(argc){
	case 1:
		fscheck();
		break;
	case 2:
		if(strcmp(argv[1], "-v") == 0){
			if(fscheck() > 0)
				fsdump(1, 0);
		}else
			consprint("usage: %s [-v]\n", argv[0]);
		break;
	default:
		consprint("usage: %s [-v]\n", argv[0]);
	}
}

static void
clru(int, char**)
{
	fslru();
}


static void
creclaim(int, char**)
{
	fsreclaim();
}

static void
callow(int argc, char *argv[])
{
	Usr *u, *usr;
	int i;

	usr = nil;
	switch(argc){
	case 1:
		if(*argv[0] == 'd')
			for(i = 0; i < nelem(uids); i++)
				for(u = uids[i]; u != nil; u = u->inext)
					u->allow = 0;
		break;
	case 2:
		xrwlock(&ulk, Wr);
		usr = finduname(argv[1], Dontmk);
		if(usr == nil){
			xrwunlock(&ulk, Wr);
			consprint("user not found\n");
			return;
		}
		usr->allow = (*argv[0] == 'a');
		xrwunlock(&ulk, Wr);
		break;
	default:
		consprint("usage: %s [uid]\n", argv[0]);
		return;
	}
	xrwlock(&ulk, Rd);
	for(i = 0; i < nelem(uids); i++)
		for(u = uids[i]; u != nil; u = u->inext)
			if(u->allow)
				consprint("user '%s' is allowed\n", u->name);
			else if(u == usr)
				consprint("user '%s' is not allowed\n", u->name);
	xrwunlock(&ulk, Rd);
}

static void
cwho(int, char**)
{
	consprintclients();
}

static void chelp(int, char**);

static Cmd cmds[] =
{
	{"dump",	cdump, 0, "dump [-l]"},
	{"dumpall",	cdump, 0, "dumpall [-l]"},
	{"stats",	cstats, 0, "stats [-c]"},
	{"sync",	csync, 1, "sync"},
	{"halt",	chalt, 1, "halt"},
	{"users",	cusers, 1, "users"},
	{"debug",	cdebug, 2, "cdebug [+-]FLAGS | on | off"},
	{"locks",	clocks, 2, "locks [on|off|dump]"},
	{"fids",		cfids, 1, "fids"},
	{"rerr",	crwerr, 2, "rerr n"},
	{"werr",	crwerr, 2, "werr n"},
	{"check",	ccheck, 0, "check"},
	{"lru",		clru, 1, "lru"},
	{"reclaim",	creclaim, 1, "reclaim"},
	{"allow",	callow, 0, "allow [uid]"},
	{"disallow",	callow, 0, "disallow [uid]"},
	{"who",		cwho, 1, "who"},
	{"?",		chelp, 1, "?"},
};

static void
chelp(int, char**)
{
	int i;

	consprint("commands:\n");
	for(i = 0; i < nelem(cmds); i++)
		if(strcmp(cmds[i].name, "?") != 0)
			consprint("> %s\n", cmds[i].usage);
}

void
consinit(void)
{
	consprint("creepy> ");
}

long
conswrite(char *ubuf, long count)
{
	char *c, *p, *np, *args[5];
	int nargs, i, nr;
	Rune r;
	static char buf[80];
	static char *s, *e;

	if(count <= 0)
		return 0;
	if(s == nil){
		s = buf;
		e = buf + sizeof buf;
	}
	for(i = 0; i < count && s < e-UTFmax-1; i += nr){
		nr = chartorune(&r, ubuf+i);
		memmove(s, ubuf+i, nr);
		s += nr;
		consprint("%C", r);
	}
	*s = 0;
	if(s == e-1){
		s = buf;
		*s = 0;
		error("command is too large");
	}
	if(utfrune(buf, '\n') == 0)
		return count;
	p = buf;
	do{
		np = utfrune(p, '\n');
		if(np != nil)
			*np++ = 0;
		c = utfrune(p, '#');
		if(c != nil)
			*c = 0;
		nargs = tokenize(p, args, nelem(args));
		if(nargs < 1)
			continue;
		for(i = 0; i < nelem(cmds); i++){
			if(strcmp(args[0], cmds[i].name) != 0)
				continue;
			quiescent(Yes);
			if(catcherror()){
				quiescent(No);
				error(nil);
			}
			if(cmds[i].nargs != 0 && cmds[i].nargs != nargs)
				consprint("usage: %s\n", cmds[i].usage);
			else
				cmds[i].f(nargs, args);
			noerror();
			quiescent(No);
			break;
		}
		if(i == nelem(cmds))
			consprint("'%s'?\n", args[0]);
	}while((p = np) != nil);
	s = buf;
	*s = 0;
	consprint("creepy> ");
	return count;
}

