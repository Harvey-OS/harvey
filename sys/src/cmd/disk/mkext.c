#include <u.h>
#include <libc.h>
#include <bio.h>

enum{
	LEN	= 8*1024,
	NFLDS	= 6,		/* filename, modes, uid, gid, mtime, bytes */
};

int	selected(char*, int, char*[]);
void	mkdirs(char*, char*);
void	mkdir(char*, ulong, ulong, char*, char*);
void	extract(char*, ulong, ulong, char*, char*, ulong);
void	seekpast(ulong);
void	error(char*, ...);
void	warn(char*, ...);
void	usage(void);

Biobufhdr bin;
uchar	binbuf[2*LEN];
int	uflag;
int	hflag;
int	vflag;

void
main(int argc, char **argv)
{
	Biobuf bout;
	char *fields[NFLDS], name[2*LEN], *p, *namep;
	char uid[NAMELEN], gid[NAMELEN];
	ulong mode, bytes, mtime;

	namep = name;
	ARGBEGIN{
	case 'd':
		p = ARGF();
		if(strlen(p) >= LEN)
			error("destination fs name too long\n");
		strcpy(name, p);
		namep = name + strlen(name);
		break;
	case 'h':
		hflag = 1;
		Binit(&bout, 1, OWRITE);
		break;
	case 'u':
		uflag = 1;
		break;
	case 'v':
		vflag = 1;
		break;
	default:
		usage();
	}ARGEND
	
	Binits(&bin, 0, OREAD, binbuf, sizeof binbuf);
	while(p = Brdline(&bin, '\n')){
		p[Blinelen(&bin)-1] = '\0';
		if(strcmp(p, "end of archive") == 0){
			Bterm(&bout);
			fprint(2, "done\n");
			exits(0);
		}
		if(getfields(p, fields, NFLDS, 0, " \t") != NFLDS){
			warn("too few fields in file header");
			continue;
		}
		strcpy(namep, fields[0]);
		mode = strtoul(fields[1], 0, 8);
		mtime = strtoul(fields[4], 0, 10);
		bytes = strtoul(fields[5], 0, 10);
		strncpy(uid, fields[2], NAMELEN);
		strncpy(gid, fields[3], NAMELEN);
		if(argc){
			if(!selected(namep, argc, argv)){
				if(bytes)
					seekpast(bytes);
				continue;
			}
			mkdirs(name, namep);
		}
		if(hflag){
			Bprint(&bout, "%s %luo %s %s %lud %lud\n",
				name, mode, uid, gid, mtime, bytes);
			if(bytes)
				seekpast(bytes);
			continue;
		}
		if(mode & CHDIR)
			mkdir(name, mode, mtime, uid, gid);
		else
			extract(name, mode, mtime, uid, gid, bytes);
	}
	fprint(2, "premature end of archive\n");
	exits("premature end of archive");
}

int
fileprefix(char *prefix, char *s)
{
	while(*prefix)
		if(*prefix++ != *s++)
			return 0;
	if(*s && *s != '/')
		return 0;
	return 1;
}

int
selected(char *s, int argc, char *argv[])
{
	int i;

	for(i=0; i<argc; i++)
		if(fileprefix(argv[i], s))
			return 1;
	return 0;
}

void
mkdirs(char *name, char *namep)
{
	char buf[2*LEN], *p;
	int fd;

	strcpy(buf, name);
	for(p = &buf[namep - name]; p = utfrune(p, '/'); p++){
		if(p[1] == '\0')
			return;
		*p = 0;
		fd = create(buf, OREAD, 0775|CHDIR);
		close(fd);
		*p = '/';
	}
}

void
mkdir(char *name, ulong mode, ulong mtime, char *uid, char *gid)
{
	Dir d;
	int fd;
	char *p;

	fd = create(name, OREAD, mode);
	if(fd < 0){
		if(dirstat(name, &d) < 0 || !(d.mode & CHDIR)){
			warn("can't make directory %s: %r", name);
			return;
		}
	}else if(dirfstat(fd, &d) < 0)
		warn("can't stat %s: %r", name);
	close(fd);

	p = utfrrune(name, L'/');
	if(p)
		p++;
	else
		p = name;
	strncpy(d.name, p, NAMELEN);
	if(uflag){
		strncpy(d.uid, uid, NAMELEN);
		strncpy(d.gid, gid, NAMELEN);
		d.mtime = mtime;
	}
	d.mode = mode;
	if(dirwstat(name, &d) < 0)
		warn("can't set modes for %s: %r", name);
	if(uflag){
		if(dirstat(name, &d) < 0)
			warn("can't reread modes for %s: %r", name);
		if(d.mtime != mtime)
			warn("%s: time mismatch %lud %lud\n", name, mtime, d.mtime);
		if(strcmp(uid, d.uid))
			warn("%s: uid mismatch %s %s", name, uid, d.uid);
		if(strcmp(gid, d.gid))
			warn("%s: gid mismatch %s %s", name, gid, d.gid);
	}
	close(fd);
}

void
extract(char *name, ulong mode, ulong mtime, char *uid, char *gid, ulong bytes)
{
	Dir d;
	Biobuf *b;
	char buf[LEN];
	char *p;
	ulong n, tot;

	if(vflag)
		print("x %s %lud bytes\n", name, bytes);

	b = Bopen(name, OWRITE);
	if(!b){
		warn("can't make file %s: %r", name);
		seekpast(bytes);
		return;
	}
	for(tot = 0; tot < bytes; tot += n){
		n = sizeof buf;
		if(tot + n > bytes)
			n = bytes - tot;
		n = Bread(&bin, buf, n);
		if(n <= 0)
			error("premature eof reading %s", name);
		if(Bwrite(b, buf, n) != n)
			warn("error writing %s: %r", name);
	}

	if(dirfstat(Bfildes(b), &d) < 0)
		warn("can't stat %s: %r", name);
	p = utfrrune(name, '/');
	if(p)
		p++;
	else
		p = name;
	strncpy(d.name, p, NAMELEN);
	if(uflag){
		strncpy(d.uid, uid, NAMELEN);
		strncpy(d.gid, gid, NAMELEN);
		d.mtime = mtime;
	}
	d.mode = mode;
	Bflush(b);
	if(dirfwstat(Bfildes(b), &d) < 0)
		warn("can't set modes for %s: %r", name);
	if(uflag){
		if(dirfstat(Bfildes(b), &d) < 0)
			warn("can't reread modes for %s: %r", name);
		if(d.mtime != mtime)
			warn("%s: time mismatch %lud %lud\n", name, mtime, d.mtime);
		if(strcmp(uid, d.uid))
			warn("%s: uid mismatch %s %s", name, uid, d.uid);
		if(strcmp(gid, d.gid))
			warn("%s: gid mismatch %s %s", name, gid, d.gid);
	}
	Bterm(b);
}

void
seekpast(ulong bytes)
{
	char buf[LEN];
	ulong tot, n;

	for(tot = 0; tot < bytes; tot += n){
		n = sizeof buf;
		if(tot + n > bytes)
			n = bytes - tot;
		n = Bread(&bin, buf, n);
		if(n < 0)
			error("premature eof");
	}
}

void
error(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	sprint(buf, "%s: ", argv0);
	va_start(arg, fmt);
	doprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s\n", buf);
	exits(0);
}

void
warn(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	sprint(buf, "%s: ", argv0);
	va_start(arg, fmt);
	doprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s\n", buf);
}

void
usage(void)
{
	fprint(2, "usage: mkext [-h] [-u] [-v] [-d dest-fs] [file ...]\n");
	exits("usage");
}
