/*
 * File system for tar archives (read-only)
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "tapefs.h"

/* fundamental constants */
enum {
	Tblock = 512,
	Namsiz = 100,
	Maxpfx = 155,		/* from POSIX */
	Maxname = Namsiz + 1 + Maxpfx,
	Binsize = 0x80,		/* flag in size[0], from gnu: positive binary size */
	Binnegsz = 0xff,	/* flag in size[0]: negative binary size */
};

/* POSIX link flags */
enum {
	LF_PLAIN1 =	'\0',
	LF_PLAIN2 =	'0',
	LF_LINK =	'1',
	LF_SYMLINK1 =	'2',
	LF_SYMLINK2 =	's',		/* 4BSD used this */
	LF_CHR =	'3',
	LF_BLK =	'4',
	LF_DIR =	'5',
	LF_FIFO =	'6',
	LF_CONTIG =	'7',
	/* 'A' - 'Z' are reserved for custom implementations */
};

typedef union {
	char	dummy[Tblock];
	char	tbuf[Maxbuf];
	struct Header {
		char	name[Namsiz];
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	linkflag;
		char	linkname[Namsiz];

		/* rest are defined by POSIX's ustar format; see p1003.2b */
		char	magic[6];	/* "ustar" */
		char	version[2];
		char	uname[32];
		char	gname[32];
		char	devmajor[8];
		char	devminor[8];
		char	prefix[Maxpfx]; /* if non-null, path= prefix "/" name */
	};
} Hdr;

Hdr dblock;
int tapefile;

int	checksum(void);

static int
isustar(Hdr *hp)
{
	return strcmp(hp->magic, "ustar") == 0;
}

/*
 * s is at most n bytes long, but need not be NUL-terminated.
 * if shorter than n bytes, all bytes after the first NUL must also
 * be NUL.
 */
static int
strnlen(char *s, int n)
{
	return s[n - 1] != '\0'? n: strlen(s);
}

/* set fullname from header */
static char *
tarname(Hdr *hp)
{
	int pfxlen, namlen;
	static char fullname[Maxname+1];

	namlen = strnlen(hp->name, sizeof hp->name);
	if (hp->prefix[0] == '\0' || !isustar(hp)) {	/* old-style name? */
		memmove(fullname, hp->name, namlen);
		fullname[namlen] = '\0';
		return fullname;
	}

	/* posix name: name is in two pieces */
	pfxlen = strnlen(hp->prefix, sizeof hp->prefix);
	memmove(fullname, hp->prefix, pfxlen);
	fullname[pfxlen] = '/';
	memmove(fullname + pfxlen + 1, hp->name, namlen);
	fullname[pfxlen + 1 + namlen] = '\0';
	return fullname;
}

void
populate(char *name)
{
	long chksum, linkflg;
	vlong blkno;
	char *fname;
	Fileinf f;
	Hdr *hp;

	tapefile = open(name, OREAD);
	if (tapefile < 0)
		error("Can't open argument file");
	replete = 1;
	hp = &dblock;
	for (blkno = 0; ; blkno++) {
		seek(tapefile, Tblock*blkno, 0);
		if (readn(tapefile, hp->dummy, sizeof hp->dummy) < sizeof hp->dummy)
			break;
		fname = tarname(hp);
		if (fname[0] == '\0')
			break;

		/* crack header */
		f.addr = blkno + 1;
		f.mode = strtoul(hp->mode, 0, 8);
		f.uid  = strtoul(hp->uid, 0, 8);
		f.gid  = strtoul(hp->gid, 0, 8);
		if((uchar)hp->size[0] == 0x80)
			f.size = b8byte(hp->size+3);
		else
			f.size = strtoull(hp->size, 0, 8);
		f.mdate = strtoul(hp->mtime, 0, 8);
		chksum  = strtoul(hp->chksum, 0, 8);
		/* the mode test is ugly but sometimes necessary */
		if (hp->linkflag == LF_DIR || (f.mode&0170000) == 040000 ||
		    strrchr(fname, '\0')[-1] == '/'){
			f.mode |= DMDIR;
			f.size = 0;
		}
		f.mode &= DMDIR | 0777;

		/* make file name safe and canonical */
		while (fname[0] == '/')		/* don't allow absolute paths */
			++fname;
		cleanname(fname);

		/* reject links */
		linkflg = hp->linkflag == LF_SYMLINK1 ||
			hp->linkflag == LF_SYMLINK2 || hp->linkflag == LF_LINK;
		if (chksum != checksum()){
			fprint(2, "%s: bad checksum on %.28s at offset %lld\n",
				argv0, fname, Tblock*blkno);
			exits("checksum");
		}
		if (linkflg) {
			/*fprint(2, "link %s->%s skipped\n", fname, hp->linkname);*/
			f.size = 0;
		} else {
			/* accept this file */
			f.name = fname;
			if (f.name[0] == '\0')
				fprint(2, "%s: null name skipped\n", argv0);
			else
				poppath(f, 1);
			blkno += (f.size + Tblock - 1)/Tblock;
		}
	}
}

void
dotrunc(Ram *r)
{
	USED(r);
}

void
docreate(Ram *r)
{
	USED(r);
}

char *
doread(Ram *r, vlong off, long cnt)
{
	int n;

	seek(tapefile, Tblock*r->addr + off, 0);
	if (cnt > sizeof dblock.tbuf)
		error("read too big");
	n = readn(tapefile, dblock.tbuf, cnt);
	if (n != cnt)
		memset(dblock.tbuf + n, 0, cnt - n);
	return dblock.tbuf;
}

void
popdir(Ram *r)
{
	USED(r);
}

void
dowrite(Ram *r, char *buf, long off, long cnt)
{
	USED(r); USED(buf); USED(off); USED(cnt);
}

int
dopermw(Ram *r)
{
	USED(r);
	return 0;
}

int
checksum(void)
{
	int i, n;
	uchar *cp;

	memset(dblock.chksum, ' ', sizeof dblock.chksum);
	cp = (uchar *)dblock.dummy;
	i = 0;
	for (n = Tblock; n-- > 0; )
		i += *cp++;
	return i;
}
