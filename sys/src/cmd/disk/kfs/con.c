#include	"all.h"

static	char	elem[NAMELEN];
static	Filsys*	cur_fs;
static	char	conline[100];

void
consserve(void)
{
	strncpy(cons.chan->whoname, "console", sizeof(cons.chan->whoname));
	con_session();
	cmd_exec("cfs");
	cmd_exec("user");
	cmd_exec("auth");
}

int
cmd_exec(char *arg)
{
	char *s, *c;
	int i;

	for(i=0; s = command[i].string; i++) {
		for(c=arg; *s; c++)
			if(*c != *s++)
				goto brk;
		if(*c == '\0' || *c == ' ' || *c == '\t'){
			cons.arg = c;
			(*command[i].func)();
			return 1;
		}
	brk:;
	}
	return 0;
}

void
cmd_check(void)
{
	char *s;
	int flags;

	flags = 0;
	for(s = cons.arg; *s; s++){
		while(*s == ' ' || *s == '\t')
			s++;
		if(*s == '\0')
			break;
		switch(*s){
		/* rebuild the free list */
		case 'f':	flags |= Cfree;			break;
		/* fix bad tags */
		case 't':	flags |= Ctag;			break;
		/* fix bad tags and clear the contents of the block */
		case 'c':	flags |= Cream;			break;
		/* delete all redundant references to a block */
		case 'd':	flags |= Cbad;			break;
		/* read and check tags on all blocks */
		case 'r':	flags |= Crdall;		break;
		/* write all of the blocks you touch */
		case 'w':	flags |= Ctouch;		break;
		/* print all directories as they are read */
		case 'p':	flags |= Cpdir;			break;
		/* print all files as they are read */
		case 'P':	flags |= Cpfile;		break;
		/* quiet, just report really nasty stuff */
		case 'q':	flags |= Cquiet;		break;
		}
	}
	check(cur_fs, flags);
}

enum
{
	Sset	= (1<<0),
	Setc	= (1<<1),
};
void
cmd_stats(void)
{
	cprint("work stats\n");
	cprint("	work = %F rps\n", (Filta){&cons.work, 1});
	cprint("	rate = %F tBps\n", (Filta){&cons.rate, 1000});
	cprint("	hits = %F iops\n", (Filta){&cons.bhit, 1});
	cprint("	read = %F iops\n", (Filta){&cons.bread, 1});
	cprint("	init = %F iops\n", (Filta){&cons.binit, 1});
/*	for(i = 0; i < MAXTAG; i++)
		cprint("	tag %G = %F iops\n", i, (Filta){&cons.tags[i], 1});
*/
}

void
cmd_sync(void)
{
	rlock(&mainlock);
	syncall();
	runlock(&mainlock);
}

void
cmd_halt(void)
{
	wlock(&mainlock);
	syncall();
	superok(cur_fs->dev, superaddr(cur_fs->dev), 1);
	print("kfs: file system halted\n");
}

void
cmd_help(void)
{
	int i;

	for(i=0; command[i].string; i++)
		cprint("	%s %s\n", command[i].string, command[i].args);
	cprint("check options:\n"
		" r	read all blocks\n"
		" f	rebuild the free list\n"
		" t	fix all bad tags\n"
		" c	fix bad tags and zero the blocks\n"
		" d	delete all redundant references to blocks\n"
		" p	print directories as they are checked\n"
		" P	print all files as they are checked\n"
		" w	write all blocks that are read\n");
}

void
cmd_create(void)
{
	int uid, gid, err;
	long perm;
	char oelem[NAMELEN];
	char name[NAMELEN];

	if(con_clone(FID1, FID2))
		return;
	if(skipbl(1))
		return;
	oelem[0] = 0;
	while(nextelem()) {
		if(oelem[0])
			if(con_walk(FID2, oelem))
				return;
		memmove(oelem, elem, NAMELEN);
	}
	if(skipbl(1))
		return;
	uid = strtouid(cname(name));
	if(uid == 0){
		cprint("unknown user %s\n", name);
		return;
	}
	gid = strtouid(cname(name));
	if(gid == 0){
		cprint("unknown group %s\n", name);
		return;
	}
	perm = number(0777, 8);
	skipbl(0);
	for(; *cons.arg; cons.arg++){
		if(*cons.arg == 'l')
			perm |= PLOCK;
		else
		if(*cons.arg == 'a')
			perm |= PAPND;
		else
		if(*cons.arg == 'd')
			perm |= PDIR;
		else
			break;
	}
	err = con_create(FID2, elem, uid, gid, perm, 0);
	if(err)
		cprint("can't create %s: %s\n", elem, errstr[err]);
}

void
cmd_clri(void)
{
	if(con_clone(FID1, FID2))
		return;
	if(skipbl(1))
		return;
	while(nextelem())
		if(con_walk(FID2, elem)){
			cprint("can't walk %s\n", elem);
			return;
		}
	con_clri(FID2);
}

void
cmd_rename(void)
{
	Dentry d;
	char stat[DIRREC];
	char oelem[NAMELEN], nelem[NAMELEN];
	int err;

	if(con_clone(FID1, FID2))
		return;
	if(skipbl(1))
		return;
	oelem[0] = 0;
	while(nextelem()) {
		if(oelem[0])
			if(con_walk(FID2, oelem)){
				cprint("file does not exits");
				return;
			}
		memmove(oelem, elem, NAMELEN);
	}
	cname(nelem);
	if(!con_walk(FID2, nelem))
		cprint("file %s already exists\n", nelem);
	else if(con_walk(FID2, oelem))
		cprint("file does not already exist\n");
	else if(err = con_stat(FID2, stat))
		cprint("can't stat file: %s\n", errstr[err]);
	else{
		convM2D(stat, &d);
		strncpy(d.name, nelem, NAMELEN);
		convD2M(&d, stat);
		if(err = con_wstat(FID2, stat))
			cprint("can't move file: %s\n", errstr[err]);
	}
}

void
cmd_remove(void)
{
	if(con_clone(FID1, FID2))
		return;
	if(skipbl(1))
		return;
	while(nextelem())
		if(con_walk(FID2, elem)){
			cprint("can't walk %s\n", elem);
			return;
		}
	con_remove(FID2);
}

void
cmd_cfs(void)
{
	Filsys *fs;

	if(*cons.arg != ' ') {
		fs = &filsys[0];		/* default */
	} else {
		if(skipbl(1))
			return;
		if(!nextelem())
			fs = &filsys[0];	/* default */
		else
			fs = fsstr(elem);
	}
	if(fs == 0) {
		cprint("unknown file system %s\n", elem);
		return;
	}
	if(con_attach(FID1, "adm", fs->name))
		panic("FID1 attach to root");
	cur_fs = fs;
}

int
adduser(char *user)
{
	char stat[DIRREC];
	char msg[100];
	Uid *u;
	int i, c, nu;

	/*
	 * check uniq of name
	 * and get next uid
	 */
	cmd_exec("cfs");
	cmd_exec("user");
	nu = 0;
	for(i=0, u=uid; i<conf.nuid; i++,u++) {
		c = u->uid;
		if(c == 0)
			break;
		if(strcmp(uidspace+u->offset, user) == 0)
			return 1;
		if(c >= 10000)
			continue;
		if(c > nu)
			nu = c;
	}
	nu++;

	/*
	 * write onto adm/users
	 */
	if(con_clone(FID1, FID2)
	|| con_path(FID2, "/adm/users")
	|| con_open(FID2, 1)) {
		cprint("can't open /adm/users\n");
		return 0;
	}

	sprint(msg, "%d:%s:%s:\n", nu, user, user);
	cprint("add user '%s'", msg);
	c = strlen(msg);
	i = con_stat(FID2, stat);
	if(i){
		cprint("can't stat /adm/users: %s\n", errstr[i]);
		return 0;
	}
	i = con_write(FID2, msg, statlen(stat), c);
	if(i != c){
		cprint("short write on /adm/users: %d %d\n", c, i);
		return 0;
	}
	return 1;
}

void
cmd_newuser(void)
{
	char user[NAMELEN], msg[100];
	int i, c;

	/*
	 * get uid
	 */
	cname(user);
	for(i=0; i<NAMELEN; i++) {
		c = user[i];
		if(c == 0)
			break;
		if(c >= '0' && c <= '9'
		|| c >= 'a' && c <= 'z'
		|| c >= 'A' && c <= 'Z')
			continue;
		cprint("bad character in name: 0x%x\n", c);
		return;
	}
	if(i < 2) {
		cprint("name too short: %s\n", user);
		return;
	}
	if(i >= NAMELEN) {
		cprint("name too long: %s\n", user);
		return;
	}

	/*
	 * install and create directory
	 */
	if(!adduser(user))
		return;

	cmd_exec("user");
	sprint(msg, "create /usr/%s %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/tmp %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/lib %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/bin %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/bin/rc %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/bin/mips %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/bin/386 %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/bin/68020 %s %s 775 d", user, user, user);
	cmd_exec(msg);
	sprint(msg, "create /usr/%s/bin/sparc %s %s 775 d", user, user, user);
	cmd_exec(msg);
}

void
cmd_checkuser(void)
{
	uchar buf[DIRREC], *p;
	static char utime[4];

	if(con_clone(FID1, FID2)
	|| con_path(FID2, "/adm/users")
	|| con_open(FID2, 0)
	|| con_stat(FID2, (char*)buf))
		return;
	p = buf + 3*NAMELEN + 4*4;
	if(memcmp(utime, p, 4) == 0)
		return;
	memmove(utime, p, 4);
	cmd_user();
}

void
cmd_allow(void)
{
	wstatallow = 1;
}

void
cmd_disallow(void)
{
	wstatallow = 0;
}

Command	command[] =
{
	"allow",	cmd_allow,	"",
	"allowoff",	cmd_disallow,	"",
	"cfs",		cmd_cfs,	"[filsys]",
	"check",	cmd_check,	"[rftRdPpw]",
	"clri",		cmd_clri,	"filename",
	"create",	cmd_create,	"filename user group perm [ald]",
	"disallow",	cmd_disallow,	"",
	"halt",		cmd_halt,	"",
	"help",		cmd_help,	"",
	"newuser",	cmd_newuser,	"username",
	"remove",	cmd_remove,	"filename",
	"rename",	cmd_rename,	"file newname",
	"stats",	cmd_stats,	"[fw]",
	"sync",		cmd_sync,	"",
	"user",		cmd_user,	"",
	0
};

int
skipbl(int err)
{
	if(*cons.arg != ' ') {
		if(err)
			cprint("syntax error\n");
		return 1;
	}
	while(*cons.arg == ' ')
		cons.arg++;
	return 0;
}

char*
cname(char *name)
{
	int i, c;

	skipbl(0);
	memset(name, 0, NAMELEN);
	for(i=0;; i++) {
		c = *cons.arg;
		switch(c) {
		case ' ':
		case '\t':
		case '\n':
		case '\0':
			return name;
		}
		if(i < NAMELEN-1)
			name[i] = c;
		cons.arg++;
	}
	return 0;
}

int
nextelem(void)
{
	char *e;
	int i, c;

	e = elem;
	while(*cons.arg == '/')
		cons.arg++;
	c = *cons.arg;
	if(c == 0 || c == ' ')
		return 0;
	for(i = 0; c = *cons.arg; i++) {
		if(c == ' ' || c == '/')
			break;
		if(i == NAMELEN) {
			cprint("path name component too long\n");
			return 0;
		}
		*e++ = c;
		cons.arg++;
	}
	*e = 0;
	return 1;
}

long
number(int d, int base)
{
	int c, sign, any;
	long n;

	sign = 0;
	any = 0;
	n = 0;

	c = *cons.arg;
	while(c == ' ') {
		cons.arg++;
		c = *cons.arg;
	}
	if(c == '-') {
		sign = 1;
		cons.arg++;
		c = *cons.arg;
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
		cons.arg++;
		c = *cons.arg;
		any = 1;
	}
	if(!any)
		return d;
	if(sign)
		n = -n;
	return n;
}
