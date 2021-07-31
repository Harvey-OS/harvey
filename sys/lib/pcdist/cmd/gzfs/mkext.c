/*
 * gzip-based file system.
 * the file system itself is just a gzipped mkfs archive
 * prefixed with "gzfilesystem\n" and suffixed with
 * a kilobyte of zeros.
 *
 * changes to the file system are only kept in 
 * memory, not written back to the disk.
 *
 * this is intended for use on a floppy boot disk.
 * we assume the file is in the dos file system and
 * contiguous on the disk: finding it amounts to
 * looking at the beginning of each sector for 
 * "gzfilesystem\n".  then we pipe it through 
 * gunzip and store the files in a file tree in memory.
 * things are slightly complicated by the fact that
 * devfloppy requires reads to be on a 512-byte
 * boundary and be a multiple of 512 bytes; we
 * fork a process to relieve gunzip of this restriction.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "gzfs.h"

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

Tree *tree;
Biobufhdr bin;
uchar	binbuf[2*LEN];

void
usage(void)
{
	fprint(2, "usage: gzfs [-m mtpt] [-s] [-f file] [-h]\n");
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

void
main(int argc, char **argv)
{
	char *fields[NFLDS], name[2*LEN], *p, *namep;
	char uid[NAMELEN], gid[NAMELEN];
	ulong mode, bytes, mtime;
	char *file;
	int i, n, stdin, fd, chatty, isramfs;
	char *mtpt;
	char blk[512];

	rfork(RFNOTEG);
	stdin = 0;
	isramfs = 0;
	file = nil;
	namep = name;
	mtpt = "/tmp";
	chatty = 0;
	lib9p_chatty = 0;
	ARGBEGIN{
	case 'd':
		chatty = !chatty;
		break;
	case 'v':
		lib9p_chatty = !lib9p_chatty;
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
	case 'r':
		isramfs = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	if(isramfs) {
		ramfs(mktree("sys", "sys", CHDIR|0777), mtpt, 0);
		_exits(0);
	}

	if(file == nil) {
		fprint(2, "must specify -f file\n");
		usage();
	}

	if((fd = open(file, OREAD)) < 0) {
		fprint(2, "cannot open \"%s\": %r\n", file);
		exits("open");
	}

	if(strstr(file, "disk")) {	/* search for archive on block boundary */
if(chatty) fprint(2, "searching for gz\n");
		for(i=0;; i++){
			if((n = read(fd, blk, sizeof blk)) != sizeof blk)
				sysfatal("read %d gets %d: %r\n", i, n);
			if(strncmp(blk, "gzfilesystem\n", 13) == 0)
				break;
		}
if(chatty) fprint(2, "found at %d\n", i);
	}

	fd = gzexpand(blockread(fd, blk+13, sizeof(blk)-13));

	tree = mktree("sys", "sys", CHDIR|0777);

	Binits(&bin, fd, OREAD, binbuf, sizeof binbuf);
	while(p = Brdline(&bin, '\n')){
		p[Blinelen(&bin)-1] = '\0';
if(chatty) fprint(2, "%s\n", p);
		if(strcmp(p, "end of archive") == 0){
			ramfs(tree, mtpt, stdin);
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
		if(mode & CHDIR)
			mkdir(name, mode, mtime, uid, gid);
		else
			extract(name, mode, mtime, uid, gid, bytes);
	}
	fprint(2, "premature end of archive\n");
	exits("premature end of archive");
}

File*
ffcreate(char *name, char *uid, char *gid, ulong mode)
{
	char *p, *q;
	File *f, *nf;

	f = tree->root;
	incref(&f->ref);
	assert(name[0] == '/');
	for(p=name+1; p; p=q+1) {
		q = utfrune(p, L'/');
		if(q == nil)
			break;
		*q = '\0';
		nf = fwalk(f, p);
		if(nf == nil) {
			fprint(2, "cannot walk to %s: %r\n", p);
			exits("walk");
		}
		assert(nf != nil);
		*q = '/';
		fclose(f);
		f = nf;
	}
	nf = fcreate(f, p, uid, gid, mode);
	fclose(f);
	if(nf == nil) {
		fprint(2, "cannot create %s: %r\n", p);
		exits("create");
	}

	return nf;
}

void
mkdir(char *name, ulong mode, ulong mtime, char *uid, char *gid)
{
	File *f;

	f = ffcreate(name, uid, gid, mode);
	f->mtime = mtime;
	fclose(f);
}

void
extract(char *name, ulong mode, ulong mtime, char *uid, char *gid, ulong bytes)
{
	ulong n, tot;
	File *f;
	Ramfile *rf;

	f = ffcreate(name, uid, gid, mode);
	f->mtime = mtime;

	rf = emalloc(sizeof(*rf));
	rf->data = emalloc(bytes);
	rf->ndata = bytes;
	f->aux = rf;
	f->length = bytes;

	for(tot = 0; tot < bytes; tot += n){
		n = MAXFDATA;
		if(tot + n > bytes)
			n = bytes - tot;
		n = Bread(&bin, rf->data+tot, n);
		if(n <= 0)
			error("premature eof reading %s", name);
	}
	fclose(f);
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
