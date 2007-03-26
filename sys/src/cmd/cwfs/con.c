#include "all.h"

static	Command	command[100];
static	Flag	flag[35];
static	char	statsdef[20];	/* default stats list */
static	int	whoflag;

static	void	consserve1(void *);
static	void	installcmds(void);

void
consserve(void)
{
	int i;

	strncpy(cons.chan->whochan, "console", sizeof(cons.chan->whochan));
	installcmds();
	con_session();
	cmd_exec("cfs");
	cmd_exec("users");
	cmd_exec("version");

	for(i = 0; command[i].arg0; i++)
		if(strcmp("cwcmd", command[i].arg0) == 0){
			cmd_exec("cwcmd touchsb");
			break;
		}

	newproc(consserve1, 0, "con");
}

/* console commands process */
static void
consserve1(void *)
{
	char *conline;

	for (;;) {
		/* conslock(); */
		do {
			print("%s: ", service);
			if ((conline = Brdline(&bin, '\n')) == nil)
				print("\n");
			else {
				conline[Blinelen(&bin)-1] = '\0';
				cmd_exec(conline);
			}
		} while (conline != nil);
	}
}

static int
cmdcmp(void *va, void *vb)
{
	Command *a, *b;

	a = va;
	b = vb;
	return strcmp(a->arg0, b->arg0);
}

void
cmd_install(char *arg0, char *help, void (*func)(int, char*[]))
{
	int i;

	qlock(&cons);
	for(i=0; command[i].arg0; i++)
		;
	if(i >= nelem(command)-2) {
		qunlock(&cons);
		print("cmd_install: too many commands\n");
		return;
	}
	command[i+1].arg0 = 0;
	command[i].help = help;
	command[i].func = func;
	command[i].arg0 = arg0;
	qsort(command, i+1, sizeof(Command), cmdcmp);
	qunlock(&cons);
}

void
cmd_exec(char *arg)
{
	char line[2*Maxword], *s;
	char *argv[10];
	int argc, i, c;

	if(strlen(arg) >= nelem(line)-2) {
		print("cmd_exec: line too long\n");
		return;
	}
	strcpy(line, arg);

	argc = 0;
	s = line;
	c = *s++;
	for(;;) {
		while(isascii(c) && isspace(c))
			c = *s++;
		if(c == 0)
			break;
		if(argc >= nelem(argv)-2) {
			print("cmd_exec: too many args\n");
			return;
		}
		argv[argc++] = s-1;
		while((!isascii(c) || !isspace(c)) && c != '\0')
			c = *s++;
		s[-1] = 0;
	}
	if(argc <= 0)
		return;
	for(i=0; s=command[i].arg0; i++)
		if(strcmp(argv[0], s) == 0) {
			(*command[i].func)(argc, argv);
			prflush();
			return;
		}
	print("cmd_exec: unknown command: %s\n", argv[0]);
}

static void
cmd_halt(int, char *[])
{
	wlock(&mainlock);	/* halt */
	sync("halt");
	exit();
}

static void
cmd_duallow(int argc, char *argv[])
{
	int uid;

	if(argc <= 1) {
		duallow = 0;
		return;
	}

	uid = strtouid(argv[1]);
	if(uid < 0)
		uid = number(argv[1], -2, 10);
	if(uid < 0) {
		print("bad uid %s\n", argv[1]);
		return;
	}
	duallow = uid;
}

static void
cmd_stats(int argc, char *argv[])
{
	int i, c;
	char buf[30], *s, *p, *q;

	if(argc <= 1) {
		if(statsdef[0] == 0)
			strcpy(statsdef, "a");
		sprint(buf, "stats s%s", statsdef);
		cmd_exec(buf);
		return;
	}

	strcpy(buf, "stat");
	p = strchr(buf, 0);
	p[1] = 0;

	q = 0;
	for(i = 1; i < argc; i++)
		for(s = argv[i]; c = *s; s++) {
			if(c == 's')
				continue;
			if(c == '-') {
				q = statsdef;
				continue;
			}
			if(q) {
				*q++ = c;
				*q = 0;
			}
			*p = c;
			cmd_exec(buf);
		}
}

static void
cmd_stata(int, char *[])
{
	int i;

	print("cons stats\n");
//	print("\twork =%7W%7W%7W rps\n", cons.work+0, cons.work+1, cons.work+2);
//	print("\trate =%7W%7W%7W tBps\n", cons.rate+0, cons.rate+1, cons.rate+2);
//	print("\thits =%7W%7W%7W iops\n", cons.bhit+0, cons.bhit+1, cons.bhit+2);
//	print("\tread =%7W%7W%7W iops\n", cons.bread+0, cons.bread+1, cons.bread+2);
//	print("\trah  =%7W%7W%7W iops\n", cons.brahead+0, cons.brahead+1, cons.brahead+2);
//	print("\tinit =%7W%7W%7W iops\n", cons.binit+0, cons.binit+1, cons.binit+2);
	print("\tbufs =    %3ld sm %3ld lg %ld res\n",
		cons.nsmall, cons.nlarge, cons.nreseq);

	for(i=0; i<nelem(mballocs); i++)
		if(mballocs[i])
			print("\t[%d]=%d\n", i, mballocs[i]);

	print("\tioerr=    %3ld wr %3ld ww %3ld dr %3ld dw\n",
		cons.nwormre, cons.nwormwe, cons.nwrenre, cons.nwrenwe);
	print("\tcache=     %9ld hit %9ld miss\n",
		cons.nwormhit, cons.nwormmiss);
}

static int
flagcmp(void *va, void *vb)
{
	Flag *a, *b;

	a = va;
	b = vb;
	return strcmp(a->arg0, b->arg0);
}

ulong
flag_install(char *arg, char *help)
{
	int i;

	qlock(&cons);
	for(i=0; flag[i].arg0; i++)
		;
	if(i >= 32) {
		qunlock(&cons);
		print("flag_install: too many flags\n");
		return 0;
	}
	flag[i+1].arg0 = 0;
	flag[i].arg0 = arg;
	flag[i].help = help;
	flag[i].flag = 1<<i;
	qsort(flag, i+1, sizeof(Flag), flagcmp);
	qunlock(&cons);
	return 1<<i;
}

void
cmd_flag(int argc, char *argv[])
{
	int f, n, i, j;
	char *s;
	Chan *cp;

	if(argc <= 1) {
		for(i=0; flag[i].arg0; i++)
			print("%.4lux %s %s\n",
				flag[i].flag, flag[i].arg0, flag[i].help);
		if(cons.flags)
			print("flag[*]   = %.4lux\n", cons.flags);
		for(cp = chans; cp; cp = cp->next)
			if(cp->flags)
				print("flag[%3d] = %.4lux\n", cp->chan, cp->flags);
		return;
	}

	f = 0;
	n = -1;
	for(i=1; i<argc; i++) {
		for(j=0; s=flag[j].arg0; j++)
			if(strcmp(s, argv[i]) == 0)
				goto found;
		j = number(argv[i], -1, 10);
		if(j < 0) {
			print("bad flag argument: %s\n", argv[i]);
			continue;
		}
		n = j;
		continue;
	found:
		f |= flag[j].flag;
	}

	if(n < 0) {
		cons.flags ^= f;
		if(f == 0)
			cons.flags = 0;
		print("flag      = %.8lux\n", cons.flags);
		return;
	}
	for(cp = chans; cp; cp = cp->next)
		if(cp->chan == n) {
			cp->flags ^= f;
			if(f == 0)
				cp->flags = 0;
			print("flag[%3d] = %.8lux\n", cp->chan, cp->flags);
			return;
		}
	print("no such channel\n");
}

static void
cmd_who(int argc, char *argv[])
{
	Chan *cp;
	int i, c;

	c = 0;
	for(cp = chans; cp; cp = cp->next) {
		if(cp->whotime == 0 && !(cons.flags & whoflag)) {
			c++;
			continue;
		}
		if(argc > 1) {
			for(i=1; i<argc; i++)
				if(strcmp(argv[i], cp->whoname) == 0)
					break;
			if(i >= argc) {
				c++;
				continue;
			}
		}
		print("%3d: %10s %24s", cp->chan,
			cp->whoname? cp->whoname: "<nowhoname>", cp->whochan);
		if(cp->whoprint)
			cp->whoprint(cp);
		print("\n");
		prflush();
	}
	if(c > 0)
		print("%d chans not listed\n", c);
}

static void
cmd_hangup(int argc, char *argv[])
{
	Chan *cp;
	int n;

	if(argc < 2) {
		print("usage: hangup chan-number\n");
		return;
	}
	n = number(argv[1], -1, 10);
	for(cp = chans; cp; cp = cp->next) {
		if(cp->whotime == 0) {
			if(cp->chan == n)
				print("that chan is hung up\n");
			continue;
		}
		if(cp->chan == n) {
			/* need more than just fileinit with tcp */
			chanhangup(cp, "console command", 1);
			fileinit(cp);
		}
	}
}

static void
cmd_sync(int, char *[])
{
	wlock(&mainlock);	/* sync */
	sync("command");
	wunlock(&mainlock);
	print("\n");
}

static void
cmd_help(int argc, char *argv[])
{
	char *arg;
	int i, j;

	for(i=0; arg=command[i].arg0; i++) {
		if(argc > 1) {
			for(j=1; j<argc; j++)
				if(strcmp(argv[j], arg) == 0)
					goto found;
			continue;
		}
	found:
		print("\t%s %s\n", arg, command[i].help);
		prflush();
	}
}

void
cmd_fstat(int argc, char *argv[])
{
	int i;

	for(i=1; i<argc; i++) {
		if(walkto(argv[i])) {
			print("cant stat %s\n", argv[i]);
			continue;
		}
		con_fstat(FID2);
	}
}

void
cmd_create(int argc, char *argv[])
{
	int uid, gid;
	long perm;
	char elem[NAMELEN], *p;

	if(argc < 5) {
		print("usage: create path uid gid mode [lad]\n");
		return;
	}

	p = utfrrune(argv[1], '/');
	if(p) {
		*p++ = 0;
		if(walkto(argv[1])) {
			print("create failed in walkto: %s\n", p);
			return;
		}
	} else {
		if(walkto("/"))
			return;
		p = argv[1];
	}
	if(strlen(p) >= NAMELEN) {
		print("name too long %s\n", p);
		return;
	}

	memset(elem, 0, sizeof(elem));
	strcpy(elem, p);

	uid = strtouid(argv[2]);
	if(uid < -1)
		uid = number(argv[2], -2, 10);
	if(uid < -1) {
		print("bad uid %s\n", argv[2]);
		return;
	}

	gid = strtouid(argv[3]);
	if(gid < -1)
		gid = number(argv[3], -2, 10);
	if(gid < -1) {
		print("bad gid %s\n", argv[3]);
		return;
	}

	perm = number(argv[4], 0777, 8) & 0777;

	if(argc > 5) {
		if(strchr(argv[5], 'l'))
			perm |= PLOCK;
		if(strchr(argv[5], 'a'))
			perm |= PAPND;
		if(strchr(argv[5], 'd'))
			perm |= PDIR;
	}

	if(con_create(FID2, elem, uid, gid, perm, 0))
		print("create failed: %s/%s\n", argv[1], p);
}

static void
cmd_clri(int argc, char *argv[])
{
	int i;

	for(i=1; i<argc; i++) {
		if(walkto(argv[i])) {
			print("cant remove %s\n", argv[i]);
			continue;
		}
		con_clri(FID2);
	}
}

static void
cmd_allow(int, char**)
{
	wstatallow = writeallow = 1;
}

static void
cmd_disallow(int, char**)
{
	wstatallow = writeallow = 0;
}

void
ckblock(Device *d, Off a, int typ, Off qpath)
{
	Iobuf *p;

	if(a) {
		p = getbuf(d, a, Brd);
		if(p) {
			checktag(p, typ, qpath);
			putbuf(p);
		}
	}
}

void
doclean(Iobuf *p, Dentry *d, int n, Off a)
{
	int i, mod, typ;
	Off qpath;

	mod = 0;
	qpath = d->qid.path;
	typ = Tfile;
	if(d->mode & DDIR)
		typ = Tdir;
	for(i=0; i<NDBLOCK; i++) {
		print("dblock[%d] = %lld\n", i, (Wideoff)d->dblock[i]);
		ckblock(p->dev, d->dblock[i], typ, qpath);
		if(i == n) {
			d->dblock[i] = a;
			mod = 1;
			print("dblock[%d] modified %lld\n", i, (Wideoff)a);
		}
	}

	/* add NDBLOCK so user can cite block address by index */
	for (i = 0; i < NIBLOCK; i++) {
		print("iblocks[%d] = %lld\n", NDBLOCK+i, (Wideoff)d->iblocks[i]);
		ckblock(p->dev, d->iblocks[i], Tind1+i, qpath);
		if(NDBLOCK+i == n) {
			d->iblocks[i] = a;
			mod = 1;
			print("iblocks[%d] modified %lld\n", NDBLOCK+i, (Wideoff)a);
		}
	}

	if(mod)
		p->flags |= Bmod|Bimm;
}

static void
cmd_clean(int argc, char *argv[])
{
	int n;
	Off a;
	Iobuf *p;
	Dentry *d;
	File *f;

	p = 0;
	f = 0;
	while(argc > 1) {
		n = -1;
		if(argc > 2)
			n = number(argv[2], -1, 10);
		a = 0;
		if(argc > 3)
			a = number(argv[3], 0, 10);
		if(walkto(argv[1])) {
			print("cant remove %s\n", argv[1]);
			break;
		}
		f = filep(cons.chan, FID2, 0);
		if(!f)
			break;
		if(n >= 0 && f->fs->dev->type == Devro) {
			print("readonly %s\n", argv[1]);
			break;
		}
		p = getbuf(f->fs->dev, f->addr, Brd);
		d = getdir(p, f->slot);
		if(!d || !(d->mode & DALLOC)) {
			print("not alloc %s\n", argv[1]);
			break;
		}
		doclean(p, d, n, a);
		break;
	}
	if(f)
		qunlock(f);
	if(p)
		putbuf(p);
}

static void
cmd_remove(int argc, char *argv[])
{
	int i;

	for(i=1; i<argc; i++) {
		if(walkto(argv[i])) {
			print("cant remove %s\n", argv[i]);
			continue;
		}
		con_remove(FID2);
	}
}

static void
cmd_version(int, char *[])
{
	print("%d-bit %s as of %T\n", sizeof(Off)*8 - 1, service, fs_mktime);
	print("\tlast boot %T\n", boottime);
}

static void
cmd_cfs(int argc, char *argv[])
{
	Filsys *fs;
	char *name;

	name = "main";
	if(argc > 1)
		name = argv[1];
	fs = fsstr(name);
	if(fs == 0) {
		print("%s: unknown file system\n", name);
		if(cons.curfs)
			return;
		fs = &filsys[0];
	}
	if(con_attach(FID1, "adm", fs->name))
		panic("FID1 attach to root");
	cons.curfs = fs;
	print("current fs is \"%s\"\n", cons.curfs->name);
}

static void
cmd_prof(int argc, char *argv[])
{
	int n;
	long m, o;
	char *p;

	if(cons.profbuf == 0) {
		print("no buffer\n");
		return;
	}
	n = !cons.profile;
	if(argc > 1)
		n = number(argv[1], n, 10);
	if(n && !cons.profile) {
		print("clr and start\n");
		memset(cons.profbuf, 0, cons.nprofbuf*sizeof(cons.profbuf[0]));
		cons.profile = 1;
		return;
	}
	if(!n && cons.profile) {
		cons.profile = 0;
		print("stop and write\n");
		if(walkto("/adm/kprofdata"))
			goto bad;
		if(con_open(FID2, OWRITE|OTRUNC)) {
		bad:
			print("cant open /adm/kprofdata\n");
			return;
		}
		p = (char*)cons.profbuf;
		for(m=0; m<cons.nprofbuf; m++) {
			n = cons.profbuf[m];
			p[0] = n>>24;
			p[1] = n>>16;
			p[2] = n>>8;
			p[3] = n>>0;
			p += 4;
		}

		m = cons.nprofbuf*sizeof(cons.profbuf[0]);
		o = 0;
		while(m > 0) {
			n = 8192;
			if(n > m)
				n = m;
			con_write(FID2, (char*)cons.profbuf+o, o, n);
			m -= n;
			o += n;
		}
		return;
	}
}

static void
cmd_time(int argc, char *argv[])
{
	int i, len;
	char *cmd;
	Timet t1, t2;

	t1 = time(nil);
	len = 0;
	for(i=1; i<argc; i++)
		len += 1 + strlen(argv[i]);
	cmd = malloc(len + 1);
	cmd[0] = 0;
	for(i=1; i<argc; i++) {
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}
	cmd_exec(cmd);
	t2 = time(nil);
	free(cmd);
	print("time = %ld ms\n", TK2MS(t2-t1));
}

void
cmd_noattach(int, char *[])
{
	noattach = !noattach;
	if(noattach)
		print("attaches are DISABLED\n");
}

void
cmd_files(int, char *[])
{
	long i, n;
	Chan *cp;

	for(cp = chans; cp; cp = cp->next)
		cp->nfile = 0;

	lock(&flock);
	n = 0;
	for(i=0; i<conf.nfile; i++)
		if(files[i].cp) {
			n++;
			files[i].cp->nfile++;
		}
	print("%ld out of %ld files used\n", n, conf.nfile);
	unlock(&flock);

	n = 0;
	for(cp = chans; cp; cp = cp->next)
		if(cp->nfile) {
			print("%3d: %5d\n", cp->chan, cp->nfile);
			prflush();
			n += cp->nfile;
		}
	print("%ld out of %ld files used\n", n, conf.nfile);
}

static void
installcmds(void)
{
	cmd_install("allow", "-- disable permission checking", cmd_allow);
	cmd_install("cfs", "[file] -- set current filesystem", cmd_cfs);
	cmd_install("clean", "file [bno [addr]] -- block print/fix", cmd_clean);
	cmd_install("check", "[options]", cmd_check);
	cmd_install("clri", "[file ...] -- purge files/dirs", cmd_clri);
	cmd_install("create", "path uid gid perm [lad] -- make a file/dir", cmd_create);
	cmd_install("disallow", "-- enable permission checking", cmd_disallow);
	cmd_install("duallow", "uid -- duallow", cmd_duallow);
	cmd_install("flag", "-- print set flags", cmd_flag);
	cmd_install("fstat", "path -- print info on a file/dir", cmd_fstat);
	cmd_install("halt", "-- return to boot rom", cmd_halt);
	cmd_install("help", "", cmd_help);
	cmd_install("newuser", "username -- add user to /adm/users", cmd_newuser);
	cmd_install("profile", "[01] -- fs profile", cmd_prof);
	cmd_install("remove", "[file ...] -- remove files/dirs", cmd_remove);
	cmd_install("stata", "-- overall stats", cmd_stata);
	cmd_install("stats", "[[-]flags ...] -- various stats", cmd_stats);
	cmd_install("sync", "", cmd_sync);
	cmd_install("time", "command -- time another command", cmd_time);
	cmd_install("users", "[file] -- read /adm/users", cmd_users);
	cmd_install("version", "-- print time of mk and boot", cmd_version);
	cmd_install("who", "[user ...] -- print attaches", cmd_who);
	cmd_install("hangup", "chan -- clunk files", cmd_hangup);
	cmd_install("printconf", "-- print configuration", cmd_printconf);
	cmd_install("noattach", "toggle noattach flag", cmd_noattach);
	cmd_install("files", "report on files structure", cmd_files);

	attachflag = flag_install("attach", "-- attach calls");
	chatflag = flag_install("chat", "-- verbose");
	errorflag = flag_install("error", "-- on errors");
	whoflag = flag_install("allchans", "-- on who");
	authdebugflag = flag_install("authdebug", "-- report authentications");
	authdisableflag = flag_install("authdisable", "-- disable authentication");
}

int
walkto(char *name)
{
	char elem[NAMELEN], *p;
	int n;

	if(con_clone(FID1, FID2))
		return 1;

	for(;;) {
		p = utfrune(name, '/');
		if(p == nil)
			p = strchr(name, '\0');
		if(p == name) {
			if(*name == '\0')
				return 0;
			name = p+1;
			continue;
		}
		n = p-name;
		if(n > NAMELEN)
			return 1;
		memset(elem, 0, sizeof(elem));
		memmove(elem, name, n);
		if(con_walk(FID2, elem))
			return 1;
		name = p;
	}
}

/* needs to parse and return vlongs to cope with new larger block numbers */
vlong
number(char *arg, int def, int base)
{
	int c, sign, any;
	vlong n;

	if(arg == nil)
		return def;

	sign = 0;
	any = 0;
	n = 0;

	for (c = *arg; isascii(c) && isspace(c) && c != '\n'; c = *arg)
		arg++;
	if(c == '-') {
		sign = 1;
		arg++;
		c = *arg;
	}
	while (isascii(c) && (isdigit(c) || base == 16 && isxdigit(c))) {
		n *= base;
		if(c >= 'a' && c <= 'f')
			n += c - 'a' + 10;
		else if(c >= 'A' && c <= 'F')
			n += c - 'A' + 10;
		else
			n += c - '0';
		arg++;
		c = *arg;
		any = 1;
	}
	if(!any)
		return def;
	if(sign)
		n = -n;
	return n;
}
