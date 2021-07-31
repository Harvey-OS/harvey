/*
 * ar - portable (ascii) format version
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ar.h>

Dir		stbuf;
struct	ar_hdr	arbuf;
struct	lar_hdr
{
	char	lar_name[16];
	long	lar_date;
	short	lar_uid;
	short	lar_gid;
	ushort	lar_mode;
	long	lar_size;
} larbuf;

#define	SKIP	1
#define	IODD	2
#define	OODD	4
#define	HEAD	8

char	*man =		{ "mrxtdpq" };
char	*opt =		{ "uvnbailo" };
char	manpmt[] =	{ "/tmp/vXXXXX" };
char	tmp1nam[] =	{ "/tmp/v1XXXXX" };
char	tmp2nam[] =	{ "/tmp/v2XXXXX" };
/* int	signum[] = 	{SIGHUP, SIGINT, SIGQUIT, 0}; */

char	flg[26];
char	**namv;
int	namc;
char	*arnam;
char	*ponam;
char	*tfnam;
char	*tf1nam;
char	*tf2nam;
char	*file;
char	name[16+1];
int	af;
int	tf;
int	tf1;
int	tf2;
int	qf;
int	bastate;
char	buf[Bsize];
Biobuf	bout;

void	setcom(void(*)(void));
void	rcmd(void);
void	dcmd(void);
void	xcmd(void);
void	tcmd(void);
void	pcmd(void);
void	mcmd(void);
void	qcmd(void);
void	(*comfun)(void);
void	init(void);
int	getaf(void);
void	getqf(void);
void	usage(void);
void	noar(void);
void	sigdone(int);
void	done(int);
int	notfound(void);
int	morefil(void);
void	cleanup(void);
void	install(void);
void	movefil(int);
int	stats(void);
void	copyfil(int, int, int);
int	getdir(void);
int	match(void);
void	bamatch(void);
void	phserr(void);
void	mesg(int);
char	*trim(char*);
void	longt(void);
void	pmode(void);
void	select(int*);
void	wrerr(void);

void
main(int argc, char *argv[])
{
	char *cp;

	Binit(&bout, 1, OWRITE);
	if(argc < 3)
		usage();
	for(cp = argv[1]; *cp; cp++)
	switch(*cp) {
	case 'l':
	case 'v':
	case 'u':
	case 'n':
	case 'a':
	case 'b':
	case 'c':
	case 'i':
	case 'o':
		flg[*cp - 'a']++;
		continue;

	case 'r':
		setcom(rcmd);
		continue;

	case 'd':
		setcom(dcmd);
		continue;

	case 'x':
		setcom(xcmd);
		continue;

	case 't':
		setcom(tcmd);
		continue;

	case 'p':
		setcom(pcmd);
		continue;

	case 'm':
		setcom(mcmd);
		continue;

	case 'q':
		setcom(qcmd);
		continue;

	default:
		fprint(2, "ar: bad option `%c'\n", *cp);
		done(1);
	}
	if(flg['l'-'a']) {
		strcpy(manpmt, "vXXXXX");
		strcpy(tmp1nam, "v1XXXXX");
		strcpy(tmp2nam, "v2XXXXX");
	}
	if(flg['i'-'a'])
		flg['b'-'a']++;
	if(flg['a'-'a'] || flg['b'-'a']) {
		bastate = 1;
		ponam = trim(argv[2]);
		argv++;
		argc--;
		if(argc < 3)
			usage();
	}
	arnam = argv[2];
	namv = argv+3;
	namc = argc-3;
	if(comfun == 0) {
		if(flg['u'-'a'] == 0) {
			fprint(2, "ar: one of [%s] must be specified\n", man);
			done(1);
		}
		setcom(rcmd);
	}
	(*comfun)();
	done(notfound());
}

void
setcom(void (*fun)(void))
{

	if(comfun != 0) {
		fprint(2, "ar: only one of [%s] allowed\n", man);
		done(1);
	}
	comfun = fun;
}

void
rcmd(void)
{
	int f;

	init();
	getaf();
	while(!getdir()) {
		bamatch();
		if(namc == 0 || match()) {
			f = stats();
			if(f < 0) {
				if(namc)
					fprint(2, "ar: cannot open %s\n", file);
				goto cp;
			}
			if(flg['u'-'a'])
				if(stbuf.mtime <= larbuf.lar_date) {
					close(f);
					goto cp;
				}
			mesg('r');
			copyfil(af, -1, IODD+SKIP);
			movefil(f);
			continue;
		}
	cp:
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	cleanup();
}

void
dcmd(void)
{

	init();
	if(getaf())
		noar();
	while(!getdir()) {
		if(match()) {
			mesg('d');
			copyfil(af, -1, IODD+SKIP);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	install();
}

void
xcmd(void)
{
	int f;

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			f = create(file, OWRITE, larbuf.lar_mode & 0777);
			if(f < 0) {
				fprint(2, "ar: %s cannot create\n", file);
				goto sk;
			}
			mesg('x');
			copyfil(af, f, IODD);
			close(f);
			if(flg['o'-'a']) {
				Dir st;
				if(dirstat(file, &st) < 0)
					perror(file);
				else {
					st.atime = larbuf.lar_date;
					st.mtime = larbuf.lar_date;
					if(dirwstat(file, &st) < 0)
						perror(file);
				}
			}
			continue;
		}
	sk:
		mesg('c');
		copyfil(af, -1, IODD+SKIP);
		if(namc > 0  &&  !morefil())
			done(0);
	}
}

void
pcmd(void)
{

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			if(flg['v'-'a']) {
				Bprint(&bout, "\n<%s>\n\n", file);
				Bflush(&bout);
			}
			copyfil(af, 1, IODD);
			continue;
		}
		copyfil(af, -1, IODD+SKIP);
	}
}

void
mcmd(void)
{

	init();
	if(getaf())
		noar();
	tf2nam = mktemp(tmp2nam);
	tf2 = create(tf2nam, ORDWR|ORCLOSE, 0600);
	if(tf2 < 0) {
		fprint(2, "ar: cannot create third temp\n");
		done(1);
	}
	while(!getdir()) {
		bamatch();
		if(match()) {
			mesg('m');
			copyfil(af, tf2, IODD+OODD+HEAD);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	install();
}

void
tcmd(void)
{

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			if(flg['v'-'a'])
				longt();
			Bprint(&bout, "%s\n", trim(file));
		}
		copyfil(af, -1, IODD+SKIP);
	}
}

void
qcmd(void)
{
	int i, f;

	if(flg['a'-'a'] || flg['b'-'a']) {
		fprint(2, "ar: abi not allowed with q\n");
		done(1);
	}
	getqf();
	/* leave note group behind when writing archive; i.e. sidestep interrupts */
	rfork(RFNOTEG);
	seek(qf, 0l, 2);
	for(i=0; i<namc; i++) {
		file = namv[i];
		if(file == 0)
			continue;
		namv[i] = 0;
		mesg('q');
		f = stats();
		if(f < 0) {
			fprint(2, "ar: %s cannot open\n", file);
			continue;
		}
		tf = qf;
		movefil(f);
		qf = tf;
	}
}

void
init(void)
{

	tfnam = mktemp(manpmt);
	tf = create(tfnam, ORDWR|ORCLOSE, 0600);
	if(tf < 0) {
		fprint(2, "ar: cannot create temp file\n");
		done(1);
	}
	if(write(tf, ARMAG, SARMAG) != SARMAG)
		wrerr();
}

int
getaf(void)
{
	char mbuf[SARMAG];

	af = open(arnam, OREAD);
	if(af < 0)
		return 1;
	if(read(af, mbuf, SARMAG) != SARMAG || strncmp(mbuf, ARMAG, SARMAG)) {
		fprint(2, "ar: %s not in archive format\n", arnam);
		done(1);
	}
	return 0;
}

void
getqf(void)
{
	char mbuf[SARMAG];

	if((qf = open(arnam, ORDWR)) < 0) {
		if(!flg['c'-'a'])
			fprint(2, "ar: creating %s\n", arnam);
		if((qf = create(arnam, OWRITE, 0664)) < 0) {
			fprint(2, "ar: cannot create %s\n", arnam);
			done(1);
		}
		if(write(qf, ARMAG, SARMAG) != SARMAG)
			wrerr();
	} else if (read(qf, mbuf, SARMAG) != SARMAG
		|| strncmp(mbuf, ARMAG, SARMAG)) {
		fprint(2, "ar: %s not in archive format\n", arnam);
		done(1);
	}
}

void
usage(void)
{
	Bprint(&bout, "usage: ar [%s][%s] archive files ...\n", opt, man);
	done(1);
}

void
noar(void)
{

	fprint(2, "ar: %s does not exist\n", arnam);
	done(1);
}

void
sigdone(int sig)
{
	USED(sig);
	done(100);
}

void
done(int c)
{
	exits(c? "error" : 0);
}

int
notfound(void)
{
	int i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i]) {
			fprint(2, "ar: %s not found\n", namv[i]);
			n++;
		}
	return n;
}

int
morefil(void)
{
	int i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i])
			n++;
	return n;
}

void
cleanup(void)
{
	int i, f;

	for(i=0; i<namc; i++) {
		file = namv[i];
		if(file == 0)
			continue;
		namv[i] = 0;
		mesg('a');
		f = stats();
		if(f < 0) {
			fprint(2, "ar: %s cannot open\n", file);
			continue;
		}
		movefil(f);
	}
	install();
}

void
install(void)
{
	int i;

	/* leave note group behind when copying back; i.e. sidestep interrupts */
	rfork(RFNOTEG);
	if(af < 0)
		if(!flg['c'-'a'])
			fprint(2, "ar: creating %s\n", arnam);
	close(af);
	af = create(arnam, OWRITE, 0664);
	if(af < 0) {
		fprint(2, "ar: cannot create %s\n", arnam);
		done(1);
	}
	if(tfnam) {
		seek(tf, 0l, 0);
		while((i = read(tf, buf, Bsize)) > 0)
			if(write(af, buf, i) != i)
				wrerr();
	}
	if(tf2nam) {
		seek(tf2, 0l, 0);
		while((i = read(tf2, buf, Bsize)) > 0)
			if(write(af, buf, i) != i)
				wrerr();
	}
	if(tf1nam) {
		seek(tf1, 0l, 0);
		while((i = read(tf1, buf, Bsize)) > 0)
			if(write(af, buf, i) != i)
				wrerr();
	}
}

/*
 * insert the file 'file'
 * into the temporary file
 */
void
movefil(int f)
{
	char buf[SAR_HDR+1];

	sprint(buf, "%-16.16s%-12ld%-6d%-6d%-8lo%-10ld%-2s",
		trim(file),
		stbuf.mtime,
		0, /* uid */
		0, /* gid */
		stbuf.mode,
		stbuf.length,
		ARFMAG);
	strncpy((char*)&arbuf, buf, SAR_HDR);
	larbuf.lar_size = stbuf.length;
	copyfil(f, tf, OODD+HEAD);
	close(f);
}

int
stats(void)
{
	int f;

	f = open(file, OREAD);
	if(f < 0)
		return f;
	if(dirfstat(f, &stbuf) < 0) {
		close(f);
		return -1;
	}
	return f;
}

/*
 * copy next file
 * size given in arbuf
 */
void
copyfil(int fi, int fo, int flag)
{
	int i, o, pe;

	if(flag & HEAD) {
		for(i=sizeof(arbuf.name)-1; i>=0; i--) {
			if(arbuf.name[i] == ' ')
				continue;
			if(arbuf.name[i] != 0)
				break;
			arbuf.name[i] = ' ';
		}
		if(write(fo, &arbuf, SAR_HDR) != SAR_HDR)
			wrerr();
	}
	pe = 0;
	while(larbuf.lar_size > 0) {
		i = o = Bsize;
		if(larbuf.lar_size < i) {
			i = o = larbuf.lar_size;
			if(i & 1) {
				buf[i] = '\n';
				if(flag & IODD)
					i++;
				if(flag & OODD)
					o++;
			}
		}
		if(read(fi, buf, i) != i)
			pe++;
		if((flag & SKIP) == 0)
			if(write(fo, buf, o) != o)
				wrerr();
		larbuf.lar_size -= Bsize;
	}
	if(pe)
		phserr();
}

int
getdir(void)
{
	char *cp;
	int i;

	i = read(af, &arbuf, SAR_HDR);
	if(i != SAR_HDR) {
		if(tf1nam) {
			i = tf;
			tf = tf1;
			tf1 = i;
		}
		return 1;
	}
	if(strncmp(arbuf.fmag, ARFMAG, sizeof(arbuf.fmag))) {
		fprint(2, "ar: malformed archive (at %ld)\n", seek(af, 0L, 1));
		done(1);
	}
	strncpy(name, arbuf.name, sizeof(arbuf.name));
	cp = name + sizeof(arbuf.name);
	while(*--cp==' ')
		;
	cp[1] = '\0';
	file = name;
	strncpy(larbuf.lar_name, name, sizeof(larbuf.lar_name));
	larbuf.lar_date = atol(arbuf.date);
	larbuf.lar_uid = atol(arbuf.uid);
	larbuf.lar_gid = atol(arbuf.gid);
	larbuf.lar_mode = strtoul(arbuf.mode, 0, 8);
	larbuf.lar_size = atol(arbuf.size);
	return 0;
}


int
match(void)
{
	int i;

	for(i=0; i<namc; i++) {
		if(namv[i] == 0)
			continue;
		if(strncmp(trim(namv[i]), file, sizeof(arbuf.name)) == 0) {
			file = namv[i];
			namv[i] = 0;
			return 1;
		}
	}
	return 0;
}

void
bamatch(void)
{
	int f;

	switch(bastate) {

	case 1:
		if(strncmp(file, ponam, sizeof(arbuf.name)) != 0)
			return;
		bastate = 2;
		if(flg['a'-'a'])
			return;

	case 2:
		bastate = 0;
		tf1nam = mktemp(tmp1nam);
		f = create(tf1nam, ORDWR|ORCLOSE, 0600);
		if(f < 0) {
			fprint(2, "ar: cannot create second temp\n");
			return;
		}
		tf1 = tf;
		tf = f;
	}
}

void
phserr(void)
{

	fprint(2, "ar: phase error on %s\n", file);
}

void
mesg(int c)
{

	if(flg['v'-'a'])
		if(c != 'c' || flg['v'-'a'] > 1)
			Bprint(&bout, "%c - %s\n", c, file);
}

char *
trim(char *s)
{
	char *p1, *p2;
	static char buf[sizeof(arbuf.name)+1];

	for(p1 = s; *p1; p1++)
		;
	while(p1 > s) {
		if(*--p1 != '/')
			break;
		*p1 = 0;
	}
	p2 = s;
	for(p1 = s; *p1; p1++)
		if(*p1 == '/')
			p2 = p1+1;
	strncpy(buf, p2, sizeof(arbuf.name));
	return buf;
}

#define	IFMT	060000
#define	ISARG	01000
#define	LARGE	010000
#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

void
longt(void)
{
	char *cp;

	pmode();
	Bprint(&bout, "%3d/%1d", larbuf.lar_uid, larbuf.lar_gid);
	Bprint(&bout, "%7ld", larbuf.lar_size);
	cp = ctime(larbuf.lar_date);
	Bprint(&bout, " %-12.12s %-4.4s ", cp+4, cp+24);
}

int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

void
pmode(void)
{
	int **mp;

	for(mp = &m[0]; mp < &m[9];)
		select(*mp++);
}

void
select(int *pairp)
{
	int n, *ap;

	ap = pairp;
	n = *ap++;
	while(--n>=0 && (larbuf.lar_mode&*ap++)==0)
		ap++;
	Bputc(&bout, *ap);
}

void
wrerr(void)
{
	perror("ar: write error");
	done(1);
}
