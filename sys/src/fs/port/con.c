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
	strncpy(cons.chan->whochan, "console", sizeof(cons.chan->whochan));
	installcmds();
	con_session();
	cmd_exec("cfs");
	cmd_exec("users");
	cmd_exec("version");

	cmd_exec("cwcmd touchsb");
	userinit(consserve1, 0, "con");
}

static
void
consserve1(void)
{
	int i, ch;

	conslock();

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
			conslock();
			goto loop;
		}
	}
	goto loop;
}

static
int
cmdcmp(Command *a, Command *b)
{
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
cmd_halt(int argc, char *argv[])
{

	USED(argc);
	USED(argv);

	wlock(&mainlock);	/* halt */
	sync("halt");
	exit();
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
cmd_stata(int argc, char *argv[])
{
	int i;

	USED(argc);
	USED(argv);

	print("cons stats\n");
	print("	work = %F rps\n", (Filta){&cons.work, 1});
	print("	rate = %F tBps\n", (Filta){&cons.rate, 1000});
	print("	hits = %F iops\n", (Filta){&cons.bhit, 1});
	print("	read = %F iops\n", (Filta){&cons.bread, 1});
	print("	rah  = %F iops\n", (Filta){&cons.brahead, 1});
	print("	init = %F iops\n", (Filta){&cons.binit, 1});
	print("	bufs =    %3ld sm %3ld lg %d res\n",
		cons.nsmall, cons.nlarge, cons.nreseq);

	for(i=0; i<nelem(mballocs); i++)
		if(mballocs[i])
			print("	[%d]=%d\n", i, mballocs[i]);

	print("	ioerr=    %3ld wr %3ld ww %3ld dr %3ld dw\n",
		cons.nwormre, cons.nwormwe, cons.nwrenre, cons.nwrenwe);
}

static
int
flagcmp(Flag *a, Flag *b)
{
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
			print("flag[*]   = %.4ux\n", cons.flags);
		for(cp = chans; cp; cp = cp->next)
			if(cp->flags)
				print("flag[%3d] = %.4ux\n", cp->chan, cp->flags);
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
		print("flag      = %.8ux\n", cons.flags);
		return;
	}
	for(cp = chans; cp; cp = cp->next) {
		if(cp->chan == n) {
			cp->flags ^= f;
			if(f == 0)
				cp->flags = 0;
			print("flag[%3d] = %.8ux\n", cp->chan, cp->flags);
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
	int i;

	for(cp = chans; cp; cp = cp->next)
		if(cp->whotime != 0) {
			if(argc > 1) {
				for(i=1; i<argc; i++)
					if(strcmp(argv[i], cp->whoname) == 0)
						goto brk;
				continue;
			}
		brk:
			print("%3d: %10s %24s %T %.4ux\n",
				cp->chan,
				cp->whoname,
				cp->whochan,
				cp->whotime,
				cp->flags);
			prflush();
		}
}

static
void
cmd_sync(int argc, char *argv[])
{
	USED(argc);
	USED(argv);

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
	long t, ct;
	char *arg;

	USED(argc);
	USED(argv);

	if(argc <= 1)
		goto out;

	ct = time();
	arg = argv[1];
	switch(*argv) {
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
out:
	prdate();
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

	uid = strtouid(argv[2], 1);
	if(uid == 0)
		uid = number(argv[2], 0, 10);
	if(uid == 0) {
		print("bad uid %s\n", argv[2]);
		return;
	}

	gid = strtouid(argv[3], 1);
	if(gid == 0)
		gid = number(argv[3], 0, 10);
	if(gid == 0) {
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
cmd_version(int argc, char *argv[])
{

	USED(argc);
	USED(argv);

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
	print("time = %d ms\n", (t2-t1)*MS2HZ);
}

void
cmd_noattach(int argc, char *argv[])
{
	USED(argc, argv);

	noattach = !noattach;
	if(noattach)
		print("attaches are DISABLED\n");
}

static
void
installcmds(void)
{
	cmd_install("cfs", "[file] -- set current filesystem", cmd_cfs);
	cmd_install("check", "[options]", cmd_check);
	cmd_install("clri", "[file ...] -- purge files/dirs", cmd_clri);
	cmd_install("create", "path uid gid perm [lad] -- make a file/dir", cmd_create);
	cmd_install("date", "[[+-]seconds] -- print/set date", cmd_date);
	cmd_install("flag", "-- print set flags", cmd_flag);
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
	cmd_install("passwd", "passwd -- set passkey", cmd_passwd);
	cmd_install("auth", "[file] -- control authentication", cmd_auth);
	cmd_install("noattach", "toggle noattach flag", cmd_noattach);

	attachflag = flag_install("attach", "-- attach calls");
	chatflag = flag_install("chat", "-- verbose");
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
