#include	"all.h"
#include	"mem.h"

static	char	conline[100];
static	Command	command[100];
static	Flag	flag[35];
static	void	installcmds(void);
static	void	consserve1(void);
static	char	statsdef[20];	/* default stats list */

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

	for(i = 0; command[i].arg0; i++){
		if(strcmp("cwcmd", command[i].arg0) == 0){
			cmd_exec("cwcmd touchsb");
			break;
		}
	}

	userinit(consserve1, 0, "con");
}

static
void
consserve1(void)
{
	int i, ch;

	/*conslock();*/

loop:

	print("%s: ", service);
	for(i=0;;) {
		ch = getc();
		switch(ch) {
		default:
			if(i < nelem(conline)-2)
				conline[i++] = ch;
			break;
		case '\b':
			if(i > 0)
				i--;
			break;
		case 'U' - '@':
			i = 0;
			break;
		case '\n':
			conline[i] = 0;
			cmd_exec(conline);
		case -1:
			goto loop;
		case 'D' - '@':
			print("\n");
			/*conslock();*/
			goto loop;
		}
	}
	goto loop;
}

static
int
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
	char line[100], *s;
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
		while(c == ' ' || c == '\t')
			c = *s++;
		if(c == 0)
			break;
		if(argc >= nelem(argv)-2) {
			print("cmd_exec: too many args\n");
			return;
		}
		argv[argc++] = s-1;
		while(c != ' ' && c != '\t' && c != 0)
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

static
void
cmd_halt(int, char *[])
{

	wlock(&mainlock);	/* halt */
	sync("halt");
	exit();
}

static
void
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

static
void
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
	for(i=1; i<argc; i++) {
		for(s=argv[i]; c=*s; s++) {
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
}

static
void
cmd_stata(int, char *[])
{
	int i;

	print("cons stats\n");
	print("	work =%7F%7F%7F rps\n", cons.work+0, cons.work+1, cons.work+2);
	print("	rate =%7F%7F%7F tBps\n", cons.rate+0, cons.rate+1, cons.rate+2);
	print("	hits =%7F%7F%7F iops\n", cons.bhit+0, cons.bhit+1, cons.bhit+2);
	print("	read =%7F%7F%7F iops\n", cons.bread+0, cons.bread+1, cons.bread+2);
	print("	rah  =%7F%7F%7F iops\n", cons.brahead+0, cons.brahead+1, cons.brahead+2);
	print("	init =%7F%7F%7F iops\n", cons.binit+0, cons.binit+1, cons.binit+2);
	print("	bufs =    %3ld sm %3ld lg %ld res\n",
		cons.nsmall, cons.nlarge, cons.nreseq);

	for(i=0; i<nelem(mballocs); i++)
		if(mballocs[i])
			print("	[%d]=%d\n", i, mballocs[i]);

	print("	ioerr=    %3ld wr %3ld ww %3ld dr %3ld dw\n",
		cons.nwormre, cons.nwormwe, cons.nwrenre, cons.nwrenwe);
	print("	cache=     %9ld hit %9ld miss\n",
		cons.nwormhit, cons.nwormmiss);
}

static
int
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
	for(cp = chans; cp; cp = cp->next) {
		if(cp->chan == n) {
			cp->flags ^= f;
			if(f == 0)
				cp->flags = 0;
			print("flag[%3d] = %.8lux\n", cp->chan, cp->flags);
			return;
		}
	}
	print("no such channel\n");
}

static
void
cmd_who(int argc, char *argv[])
{
	Chan *cp;
	ulong t;
	int i, c;

	c = 0;
	for(cp = chans; cp; cp = cp->next) {
		if(cp->whotime == 0) {
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
		print("%3d: %10s %24s%7F%7F",
			cp->chan,
			cp->whoname,
			cp->whochan,
			&cp->work,
			&cp->rate);
		switch(cp->type) {
		case Devil:
			t = MACHP(0)->ticks * (1000/HZ);
			print(" (%d,%d)", cp->ilp.alloc, cp->ilp.state);
			print(" (%ld,%ld,%ld)",
				cp->ilp.timeout-t, cp->ilp.querytime-t,
				cp->ilp.lastrecv-t);
			print(" (%ld,%ld,%ld,%ld)", cp->ilp.rate, cp->ilp.delay,
				cp->ilp.mdev, cp->ilp.unackedbytes);
			break;
		}
		print("\n");
		prflush();
	}
	if(c > 0)
		print("%d chans not listed\n", c);
}

static
void
cmd_hangup(int argc, char *argv[])
{
	Chan *cp;
	int n;

	if(argc < 2) {
		print("usage: hangup chan number\n");
		return;
	}
	n = number(argv[1], -1, 10);
	for(cp = chans; cp; cp = cp->next) {
		if(cp->whotime == 0) {
			if(cp->chan == n)
				print("that chan is hung up\n");
			continue;
		}
		if(cp->chan == n)
			fileinit(cp);
	}
}

static
void
cmd_sync(int, char *[])
{

	wlock(&mainlock);	/* sync */
	sync("command");
	wunlock(&mainlock);
	print("\n");
}

static
void
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
		print("	%s %s\n", arg, command[i].help);
		prflush();
	}
}

static
void
cmd_date(int argc, char *argv[])
{
	ulong ct;
	long t;
	char *arg;

	if(argc <= 1)
		goto out;

	ct = time();
	arg = argv[1];
	switch(*arg) {
	default:
		t = number(arg, -1, 10);
		if(t <= 0)
			goto out;
		ct = t;
		break;
	case '+':
		t = number(arg+1, 0, 10);
		ct += t;
		break;
	case '-':
		t = number(arg+1, 0, 10);
		ct -= t;
	}
	settime(ct);
	setrtc(ct);

out:
	prdate();
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

static
void
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

void
ckblock(Device *d, long a, int typ, long qpath)
{
	Iobuf *p;

	if(a) {
		p = getbuf(d, a, Bread);
		checktag(p, typ, qpath);
		if(p)
			putbuf(p);
	}
}

void
doclean(Iobuf *p, Dentry *d, int n, long a)
{
	int i, mod, typ;
	long qpath;

	mod = 0;
	qpath = d->qid.path;
	typ = Tfile;
	if(d->mode & DDIR)
		typ = Tdir;
	for(i=0; i<NDBLOCK; i++) {
		print("dblock[%d] = %ld\n", i, d->dblock[i]);
		ckblock(p->dev, d->dblock[i], typ, qpath);
		if(i == n) {
			d->dblock[i] = a;
			mod = 1;
			print("dblock[%d] modified %ld\n", i, a);
		}
	}

	print("iblock[%d] = %ld\n", NDBLOCK, d->iblock);
	ckblock(p->dev, d->iblock, Tind1, qpath);
	if(NDBLOCK == n) {
		d->iblock = a;
		mod = 1;
		print("iblock[%d] modified %ld\n", NDBLOCK, a);
	}

	print("diblock[%d] = %ld\n", NDBLOCK+1, d->diblock);
	ckblock(p->dev, d->diblock, Tind2, qpath);
	if(NDBLOCK+1 == n) {
		d->diblock = a;
		mod = 1;
		print("diblock[%d] modified %ld\n", NDBLOCK+1, a);
	}

	if(mod)
		p->flags |= Bmod|Bimm;
}

static
void
cmd_clean(int argc, char *argv[])
{
	int n;
	long a;
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
		p = getbuf(f->fs->dev, f->addr, Bread);
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

static
void
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

static
void
cmd_version(int, char *[])
{

	print("%s as of %T\n", service, mktime);
	print("	last boot %T\n", boottime);
}

static
void
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

static
void
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
		if(con_open(FID2, MWRITE|MTRUNC)) {
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

static
void
cmd_time(int argc, char *argv[])
{
	long t1, t2;
	int i;

	t1 = MACHP(0)->ticks;
	conline[0] = 0;
	for(i=1; i<argc; i++) {
		strcat(conline, " ");
		strcat(conline, argv[i]);
	}
	cmd_exec(conline);
	t2 = MACHP(0)->ticks;
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
	for(cp = chans; cp; cp = cp->next) {
		if(cp->nfile) {
			print("%3d: %5d\n",
				cp->chan,
				cp->nfile);
			prflush();
			n += cp->nfile;
		}
	}
	print("%ld out of %ld files used\n", n, conf.nfile);
}

static
void
installcmds(void)
{
	cmd_install("cfs", "[file] -- set current filesystem", cmd_cfs);
	cmd_install("clean", "file [bno [addr]] -- block print/fix", cmd_clean);
	cmd_install("check", "[options]", cmd_check);
	cmd_install("clri", "[file ...] -- purge files/dirs", cmd_clri);
	cmd_install("create", "path uid gid perm [lad] -- make a file/dir", cmd_create);
	cmd_install("date", "[[+-]seconds] -- print/set date", cmd_date);
	cmd_install("duallow", "uid -- duallow", cmd_duallow);
	cmd_install("flag", "-- print set flags", cmd_flag);
	cmd_install("fstat", "path -- print info on a file/dir", cmd_fstat);
	cmd_install("halt", "-- return to boot rom", cmd_halt);
	cmd_install("help", "", cmd_help);
	cmd_install("newuser", "username -- add user to /adm/users", cmd_newuser);
	cmd_install("profile", "[01] -- kernel profile", cmd_prof);
	cmd_install("remove", "[file ...] -- remove files/dirs", cmd_remove);
	cmd_install("stata", "-- overall stats", cmd_stata);
	cmd_install("stats", "[[-]flags ...] -- various stats", cmd_stats);
	cmd_install("sync", "", cmd_sync);
	cmd_install("time", "command -- time another command", cmd_time);
	cmd_install("users", "[file] -- read /adm/users", cmd_users);
	cmd_install("version", "-- print time of mk and boot", cmd_version);
	cmd_install("who", "[user ...] -- print attaches", cmd_who);
	cmd_install("hangup", "chan -- clunk files", cmd_hangup);
	cmd_install("passwd", "passwd -- set passkey, id, and domain", cmd_passwd);
	cmd_install("noattach", "toggle noattach flag", cmd_noattach);
	cmd_install("files", "report on files structure", cmd_files);

	attachflag = flag_install("attach", "-- attach calls");
	chatflag = flag_install("chat", "-- verbose");
	errorflag = flag_install("error", "-- on errors");
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
		if(p == 0)
			p = strchr(name, 0);
		if(p == name) {
			if(*name == 0)
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
	return 0;
}

long
number(char *arg, int def, int base)
{
	int c, sign, any;
	long n;

	if(arg == 0)
		return def;

	sign = 0;
	any = 0;
	n = 0;

	c = *arg;
	while(c == ' ') {
		arg++;
		c = *arg;
	}
	if(c == '-') {
		sign = 1;
		arg++;
		c = *arg;
	}
	while((c >= '0' && c <= '9') ||
	      (base == 16 && c >= 'a' && c <= 'f') ||
	      (base == 16 && c >= 'A' && c <= 'F')) {
		n *= base;
		if(c >= 'a' && c <= 'f')
			n += c - 'a' + 10;
		else
		if(c >= 'A' && c <= 'F')
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
