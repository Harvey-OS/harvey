/*
 * bzip2-based file system.
 * the file system itself is just a bzipped2 xzipped mkfs archive
 * prefixed with "bzfilesystem\n" and suffixed with
 * a kilobyte of zeros.
 *
 * changes to the file system are only kept in 
 * memory, not written back to the disk.
 *
 * this is intended for use on a floppy boot disk.
 * we assume the file is in the dos file system and
 * contiguous on the disk: finding it amounts to
 * looking at the beginning of each sector for 
 * "bzfilesystem\n".  then we pipe it through 
 * bunzip2 and store the files in a file tree in memory.
 * things are slightly complicated by the fact that
 * devfloppy requires reads to be on a 512-byte
 * boundary and be a multiple of 512 bytes; we
 * fork a process to relieve bunzip2 of this restriction.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include "bzfs.h"

enum{
	LEN	= 8*1024,
	NFLDS	= 6,		/* filename, modes, uid, gid, mtime, bytes */
};

void	mkdirs(char*, char*);
void	mkdir(char*, ulong, ulong, char*, char*);
void	extract(char*, ulong, ulong, char*, char*, ulong);
void	seekpast(ulong);
void	error(char*, ...);
void	warn(char*, ...);
void	usage(void);
char *mtpt;
Biobufhdr bin;
uchar	binbuf[2*LEN];

void
usage(void)
{
	fprint(2, "usage: bzfs [-m mtpt] [-s] [-f file] [-h]\n");
	exits("usage");
}

/*
 * floppy disks can only be read on 512-byte 
 * boundaries and in 512 byte multiples.
 * feed one over a pipe to allow arbitrary reading.
 */
char zero[512];
int
blockread(int in, char *first, int nfirst)
{
	int p[2], out, n, rv;
	char blk[512];

	if(pipe(p) < 0)
		sysfatal("pipe: %r");
	rv = p[0];
	out = p[1];
	switch(rfork(RFPROC|RFNOTEG|RFFDG)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		close(rv);
		break;
	default:
		close(in);
		close(out);
		return rv;
	}

	write(out, first, nfirst);
	
	while((n=read(in, blk, sizeof blk)) > 0){
		if(write(out, blk, n) != n)
			break;
		if(n == sizeof(blk) && memcmp(zero, blk, n) == n)
			break;
	}
	_exits(0);
	return -1;
}

enum { NAMELEN = 28 };

void
main(int argc, char **argv)
{
	char *rargv[10];
	int rargc;
	char *fields[NFLDS], name[2*LEN], *p, *namep;
	char uid[NAMELEN], gid[NAMELEN];
	ulong mode, bytes, mtime;
	char *file;
	int i, n, stdin, fd, chatty;
	char blk[512];

	if(argc>1 && strcmp(argv[1], "RAMFS") == 0){
		argv[1] = argv[0];
		ramfsmain(argc-1, argv+1);
		exits(nil);
	}
	if(argc>1 && strcmp(argv[1], "BUNZIP") == 0){
		_unbzip(0, 1);
		exits(nil);
	}

	rfork(RFNOTEG);
	stdin = 0;
	file = nil;
	namep = name;
	mtpt = "/root";
	chatty = 0;
	ARGBEGIN{
	case 'd':
		chatty = !chatty;
		break;
	case 'f':
		file = ARGF();
		break;
	case 's':
		stdin++;
		break;
	case 'm':
		mtpt = ARGF();
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	if(file == nil) {
		fprint(2, "must specify -f file\n");
		usage();
	}

	if((fd = open(file, OREAD)) < 0) {
		fprint(2, "cannot open \"%s\": %r\n", file);
		exits("open");
	}

	rargv[0] = "ramfs";
	rargc = 1;
	if(stdin)
		rargv[rargc++] = "-i";
	rargv[rargc++] = "-m";
	rargv[rargc++] = mtpt;
	rargv[rargc] = nil;
	ramfsmain(rargc, rargv);

	if(1 || strstr(file, "disk")) {	/* search for archive on block boundary */
if(chatty) fprint(2, "searching for bz\n");
		for(i=0;; i++){
			if((n = readn(fd, blk, sizeof blk)) != sizeof blk)
				sysfatal("read %d gets %d: %r\n", i, n);
			if(strncmp(blk, "bzfilesystem\n", 13) == 0)
				break;
		}
if(chatty) fprint(2, "found at %d\n", i);
	}

	if(chdir(mtpt) < 0)
		error("chdir %s: %r", mtpt);

	fd = unbflz(unbzip(blockread(fd, blk+13, sizeof(blk)-13)));

	Binits(&bin, fd, OREAD, binbuf, sizeof binbuf);
	while(p = Brdline(&bin, '\n')){
		p[Blinelen(&bin)-1] = '\0';
if(chatty) fprint(2, "%s\n", p);
		if(strcmp(p, "end of archive") == 0){
			_exits(0);
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
		if(mode & DMDIR)
			mkdir(name, mode, mtime, uid, gid);
		else
			extract(name, mode, mtime, uid, gid, bytes);
	}
	fprint(2, "premature end of archive\n");
	exits("premature end of archive");
}

char buf[8192];

int
ffcreate(char *name, ulong mode, char *uid, char *gid, ulong mtime, int length)
{
	int fd, om;
	Dir nd;

	sprint(buf, "%s/%s", mtpt, name);
	om = ORDWR;
	if(mode&DMDIR)
		om = OREAD;
	if((fd = create(buf, om, (mode&DMDIR)|0666)) < 0)
		error("create %s: %r", buf);

	nulldir(&nd);
	nd.mode = mode;
	nd.uid = uid;
	nd.gid = gid;
	nd.mtime = mtime;
	if(length)
		nd.length = length;
	if(dirfwstat(fd, &nd) < 0)	
		error("fwstat %s: %r", buf);

	return fd;
}

void
mkdir(char *name, ulong mode, ulong mtime, char *uid, char *gid)
{
	close(ffcreate(name, mode, uid, gid, mtime, 0));
}

void
extract(char *name, ulong mode, ulong mtime, char *uid, char *gid, ulong bytes)
{
	int fd, tot, n;

	fd = ffcreate(name, mode, uid, gid, mtime, bytes);

	for(tot = 0; tot < bytes; tot += n){
		n = sizeof buf;
		if(tot + n > bytes)
			n = bytes - tot;
		n = Bread(&bin, buf, n);
		if(n <= 0)
			error("premature eof reading %s", name);
		if(write(fd, buf, n) != n)
			error("short write writing %s", name);
	}
	close(fd);
}

void
error(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	sprint(buf, "%s: ", argv0);
	va_start(arg, fmt);
	vseprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg);
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
	vseprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s\n", buf);
}

int
_efgfmt(Fmt*)
{
	return -1;
}
