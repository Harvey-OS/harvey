#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>

#define TBLOCK	512
#define NBLOCK	40	/* maximum blocksize */
#define DBLOCK	20	/* default blocksize */
#define NAMSIZ	100
union	hblock
{
	char	dummy[TBLOCK];
	struct	header
	{
		char	name[NAMSIZ];
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	linkflag;
		char	linkname[NAMSIZ];
	} dbuf;
} dblock, tbuf[NBLOCK];

Dir stbuf;
Biobuf bout;

int	rflag, xflag, vflag, tflag, mt, cflag, fflag, Tflag;
int	uflag, gflag;
int	chksum, recno, first;
int	nblock = DBLOCK;

void	usage(void);
void	dorep(char **);
int	endtar(void);
void	getdir(void);
void	passtar(void);
void	putfile(char *, char *);
void	doxtract(char **);
void	dotable(void);
void	putempty(void);
void	longt(Dir *);
int	checkdir(char *);
void	tomodes(Dir *);
int	checksum(void);
int	checkupdate(char *);
int	prefix(char *, char *);
int	readtar(char *);
int	writetar(char *);
void	backtar(void);
void	flushtar(void);
void	affix(int, char *);
char	*findname(char *);
void	fixname(char *);
int	volprompt(void);
void
main(int argc, char **argv)
{
	char *usefile;
	char *cp, *ap;

	if (argc < 2)
		usage();

	Binit(&bout, 1, OWRITE);
	usefile =  0;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++) 
		switch(*cp) {
		case 'f':
			usefile = *argv++;
			if(!usefile)
				usage();
			fflag++;
			break;
		case 'u':
			ap = *argv++;
			if(!ap)
				usage();
			uflag = strtoul(ap, 0, 0);
			break;
		case 'g':
			ap = *argv++;
			if(!ap)
				usage();
			gflag = strtoul(ap, 0, 0);
			break;
		case 'c':
			cflag++;
			rflag++;
			break;
		case 'r':
			rflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		case 'T':
			Tflag++;
			break;
		case 't':
			tflag++;
			break;
		case '-':
			break;
		default:
			fprint(2, "tar: %c: unknown option\n", *cp);
			usage();
		}

	fmtinstall('M', dirmodeconv);

	if (rflag) {
		if (!usefile) {
			if (cflag == 0) {
				fprint(2, "Can only create standard output archives\n");
				exits("arg error");
			}
			mt = dup(1, -1);
			nblock = 1;
		}
		else if ((mt = open(usefile, ORDWR)) < 0) {
			if (cflag == 0 || (mt = create(usefile, OWRITE, 0666)) < 0) {
				fprint(2, "tar: cannot open %s\n", usefile);
				exits("open");
			}
		}
		dorep(argv);
	}
	else if (xflag)  {
		if (!usefile) {
			mt = dup(0, -1);
			nblock = 1;
		}
		else if ((mt = open(usefile, OREAD)) < 0) {
			fprint(2, "tar: cannot open %s\n", usefile);
			exits("open");
		}
		doxtract(argv);
	}
	else if (tflag) {
		if (!usefile) {
			mt = dup(0, -1);
			nblock = 1;
		}
		else if ((mt = open(usefile, OREAD)) < 0) {
			fprint(2, "tar: cannot open %s\n", usefile);
			exits("open");
		}
		dotable();
	}
	else
		usage();
	exits(0);
}

void
usage(void)
{
	fprint(2, "tar: usage  tar {txrc}[vf] [tarfile] file1 file2...\n");
	exits("usage");
}

void
dorep(char **argv)
{
	char *cp, *cp2;

	if (!cflag) {
		getdir();
		do {
			passtar();
			getdir();
		} while (!endtar());
	}

	while (*argv) {
		cp2 = *argv;
		if (!strcmp(cp2, "-C") && argv[1]) {
			argv++;
			if (chdir(*argv) < 0)
				perror(*argv);
			argv++;
			continue;
		}
		for (cp = *argv; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != *argv) {
			*cp2 = '\0';
			chdir(*argv);
			*cp2 = '/';
			cp2++;
		}
		putfile(*argv++, cp2);
	}
	putempty();
	putempty();
	flushtar();
}

int
endtar(void)
{
	if (dblock.dbuf.name[0] == '\0') {
		backtar();
		return(1);
	}
	else
		return(0);
}

void
getdir(void)
{
	Dir *sp;

	readtar((char*)&dblock);
	if (dblock.dbuf.name[0] == '\0')
		return;
	sp = &stbuf;
	sp->mode = strtol(dblock.dbuf.mode, 0, 8);
	strncpy(sp->uid, "adm", sizeof(sp->uid));
	strncpy(sp->gid, "adm", sizeof(sp->gid));
	sp->length = strtol(dblock.dbuf.size, 0, 8);
	sp->mtime = strtol(dblock.dbuf.mtime, 0, 8);
	chksum = strtol(dblock.dbuf.chksum, 0, 8);
	if (chksum != checksum()) {
		fprint(2, "directory checksum error\n");
		exits("checksum error");
	}
	if(xflag){
		fixname(dblock.dbuf.name);
		fixname(dblock.dbuf.linkname);
	}
}

void
passtar(void)
{
	long blocks;
	char buf[TBLOCK];

	if (dblock.dbuf.linkflag == '1' || dblock.dbuf.linkflag == 's')
		return;
	blocks = stbuf.length;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0)
		readtar(buf);
}

void
putfile(char *longname, char *sname)
{
	int infile;
	long blocks;
	char buf[TBLOCK], shortname[NAMELEN+4];
	char *cp, *cp2;
	Dir db[50];
	int i, n;

	sprint(shortname, "./%s", sname);
	infile = open(shortname, OREAD);
	if (infile < 0) {
		fprint(2, "tar: %s: cannot open file\n", longname);
		return;
	}

	dirfstat(infile, &stbuf);

	if (stbuf.qid.path & CHDIR) {
		for (i = 0, cp = buf; *cp++ = longname[i++];);
		*--cp = '/';
		*++cp = 0;
		if( (cp - buf) >= NAMSIZ) {
			fprint(2, "%s: file name too long\n", longname);
			close(infile);
			return;
		}
		stbuf.length = 0;
		tomodes(&stbuf);
		strcpy(dblock.dbuf.name,buf);
		sprint(dblock.dbuf.chksum, "%6o", checksum());
		writetar( (char *) &dblock);
		if (chdir(shortname) < 0) {
			fprint(2, "tar: can't cd to %s\n", shortname);
			exits("cd");
		}
		while ((n = dirread(infile, db, sizeof(db))) > 0) {
			n /= sizeof(Dir);
			for(i = 0; i < n; i++) {
				strncpy(cp, db[i].name, NAMELEN);
				putfile(buf, cp);
			}
		}
		close(infile);
		if (chdir("..") < 0) {
			fprint(2, "tar: can't cd to ..\n");
			exits("cd");
		}
		return;
	}

	tomodes(&stbuf);

	cp2 = longname;
	for (cp = dblock.dbuf.name, i=0; (*cp++ = *cp2++) && i < NAMSIZ; i++);
	if (i >= NAMSIZ) {
		fprint(2, "%s: file name too long\n", longname);
		close(infile);
		return;
	}

	blocks = (stbuf.length + (TBLOCK-1)) / TBLOCK;
	if (vflag) {
		fprint(2, "a %s ", longname);
		fprint(2, "%ld blocks\n", blocks);
	}
	sprint(dblock.dbuf.chksum, "%6o", checksum());
	writetar( (char *) &dblock);

	while ((i = read(infile, buf, TBLOCK)) > 0 && blocks > 0) {
		writetar(buf);
		blocks--;
	}
	close(infile);
	if (blocks != 0 || i != 0)
		fprint(2, "%s: file changed size\n", longname);
	while (blocks-- >  0)
		putempty();
}


void
doxtract(char **argv)
{
	Dir mydir;
	long blocks, bytes;
	char buf[TBLOCK], outname[NAMSIZ+4];
	char **cp;
	int ofile;

	for (;;) {
		getdir();
		if (endtar())
			break;

		if (*argv == 0)
			goto gotit;

		for (cp = argv; *cp; cp++)
			if (prefix(*cp, dblock.dbuf.name))
				goto gotit;
		passtar();
		continue;

gotit:
		if(checkdir(dblock.dbuf.name))
			continue;

		if (dblock.dbuf.linkflag == '1') {
			fprint(2, "tar: can't link %s %s\n",
				dblock.dbuf.linkname, dblock.dbuf.name);
			remove(dblock.dbuf.name);
			continue;
		}
		if (dblock.dbuf.linkflag == 's') {
			fprint(2, "%s: cannot symlink\n", dblock.dbuf.name);
			continue;
		}
		if(dblock.dbuf.name[0] != '/')
			sprint(outname, "./%s", dblock.dbuf.name);
		else
			strcpy(outname, dblock.dbuf.name);
		if ((ofile = create(outname, OWRITE, stbuf.mode & 0777)) < 0) {
			fprint(2, "tar: %s - cannot create\n", dblock.dbuf.name);
			passtar();
			continue;
		}

		blocks = ((bytes = stbuf.length) + TBLOCK-1)/TBLOCK;
		if (vflag)
			fprint(2, "x %s, %ld bytes\n",
				dblock.dbuf.name, bytes);
		while (blocks-- > 0) {
			readtar(buf);
			if (bytes > TBLOCK) {
				if (write(ofile, buf, TBLOCK) < 0) {
					fprint(2, "tar: %s: HELP - extract write error\n", dblock.dbuf.name);
					exits("extract write");
				}
			} else
				if (write(ofile, buf, bytes) < 0) {
					fprint(2, "tar: %s: HELP - extract write error\n", dblock.dbuf.name);
					exits("extract write");
				}
			bytes -= TBLOCK;
		}
		if(Tflag){
			dirfstat(ofile, &mydir);
			mydir.mtime = stbuf.mtime;
			dirfwstat(ofile, &mydir);
		}
		close(ofile);
	}
}

void
dotable(void)
{
	for (;;) {
		getdir();
		if (endtar())
			break;
		if (vflag)
			longt(&stbuf);
		Bprint(&bout, "%s", dblock.dbuf.name);
		if (dblock.dbuf.linkflag == '1')
			Bprint(&bout, " linked to %s", dblock.dbuf.linkname);
		if (dblock.dbuf.linkflag == 's')
			Bprint(&bout, " -> %s", dblock.dbuf.linkname);
		Bprint(&bout, "\n");
		passtar();
	}
}

void
putempty(void)
{
	char buf[TBLOCK];

	memset(buf, 0, TBLOCK);
	writetar(buf);
}

void
longt(Dir *st)
{
	char *cp;

	Bprint(&bout, "%M %4d/%1d", st->mode, 0, 0);	/* 0/0 uid/gid */
	Bprint(&bout, "%7d", st->length);
	cp = ctime(st->mtime);
	Bprint(&bout, " %-12.12s %-4.4s ", cp+4, cp+24);
}

int
checkdir(char *name)
{
	char *cp;
	int f;

	cp = name;
	if(*cp == '/')
		cp++;
	for (; *cp; cp++) {
		if (*cp == '/') {
			*cp = '\0';
			if (access(name, 0) < 0) {
				f = create(name, OREAD, CHDIR + 0777L);
				close(f);
				if(f < 0)
					fprint(2, "tar: mkdir %s failed\n", name);
			}
			*cp = '/';
		}
	}
	return(cp[-1]=='/');
}

void
tomodes(Dir *sp)
{
	char *cp;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	sprint(dblock.dbuf.mode, "%6o ", sp->mode & 0777);
	sprint(dblock.dbuf.uid, "%6o ", uflag);
	sprint(dblock.dbuf.gid, "%6o ", gflag);
	sprint(dblock.dbuf.size, "%11lo ", sp->length);
	sprint(dblock.dbuf.mtime, "%11lo ", sp->mtime);
}

int
checksum(void)
{
	int i;
	char *cp;

	for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return(i);
}

int
prefix(char *s1, char *s2)
{
	while (*s1)
		if (*s1++ != *s2++)
			return(0);
	if (*s2)
		return(*s2 == '/');
	return(1);
}

int
readtar(char *buffer)
{
	int i;

	if (recno >= nblock || first == 0) {
redo:
		if ((i = read(mt, tbuf, TBLOCK*nblock)) <= 0) {
			fprint(2, "Tar: archive read error\n");
			exits("archive read");
		}
		if (first == 0) {
			if ((i % TBLOCK) != 0) {
				fprint(2, "Tar: archive blocksize error\n");
				exits("blocksize");
			}
			i /= TBLOCK;
			if (i != nblock) {
				fprint(2, "Tar: blocksize = %d\n", i);
				nblock = i;
			}
		}
		recno = 0;
	}
	first = 1;
	memmove(buffer, &tbuf[recno++], TBLOCK);
	return(TBLOCK);
}

int
writetar(char *buffer)
{
	first = 1;
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) != TBLOCK*nblock) {
			fprint(2, "Tar: archive write error\n");
			exits("write");
		}
		recno = 0;
	}
	memmove(&tbuf[recno++], buffer, TBLOCK);
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) != TBLOCK*nblock) {
			fprint(2, "Tar: archive write error\n");
			exits("write");
		}
		recno = 0;
	}
	return(TBLOCK);
}

/*
 * backup over last tar block
 */
void
backtar(void)
{
	seek(mt, -TBLOCK*nblock, 1);
	recno--;
}

void
flushtar(void)
{
	write(mt, tbuf, TBLOCK*nblock);
}

#define MAXFILENAME (NAMELEN-1)
int longnamefd;

void
affix(int n, char *ptr)
{
	int i=0,m;
	char ext[5];

	while(1) {
		if((m=n%52)<26) ext[i++] = m + 'a';
		else ext[i++] = m + 'A' - 26;
		if(n < 52)break;
		n = n/52 - 1;  /* so we have Z,aa not Z,ba */
	}

	while(--i >= 0)*ptr++ = ext[i];
	*ptr = '\0';
}

#define MAXOVER	1000
struct {
	char *longname,
		*shortname;
	} pairs[MAXOVER];
int npairs = 0;	/* no. of tabulated pairs */

int ntoolong = 0;	/* no. of overlong pathnames */

char *
findname(char *key)
{
	int i, nprevious = 0, nprefix;
	char *longptr, *shortptr, *endbit;

	endbit = strrchr(key,'/');
	if(endbit == 0)endbit = key;
	else endbit++;
	nprefix = endbit - key;

	for(i=0;i<npairs;i++){
		if(strcmp(key,pairs[i].longname) == 0)
			return(pairs[i].shortname);
		if(strncmp(key,pairs[i].longname,nprefix+MAXFILENAME-4) == 0)
			nprevious++;
	}
	longptr = pairs[npairs].longname = malloc(strlen(key)+1);
	shortptr = pairs[npairs].shortname = malloc(MAXFILENAME+1);
	strcpy(longptr,key);
	strncpy(shortptr,endbit,MAXFILENAME-4);
	sprint(shortptr+MAXFILENAME-4,"..");
	affix(nprevious,shortptr+MAXFILENAME-2);
	npairs++;
	return(shortptr);
}

void
fixname(char *original)
{
  int length;
  char newname[100];
  char *inend, *outptr=newname, *instart=original,
		*outstart;
 int changed = 0;

  while(1){
	if(*instart == '\0')break;
	if(*instart == '/')*outptr++ = *instart++;
	outstart = outptr;
	for(inend=instart;*inend != '\0' && *inend != '/';)
		*outptr++ = *inend++;
	*outptr = '\0';
	length = strlen(outstart);
	if(length > MAXFILENAME){
		changed++;
		strcpy(outstart,findname(newname));
	}
	outptr = outstart + strlen(outstart);
	instart = inend;
	}

  if(changed){
	if(ntoolong == 0) {
		longnamefd = open("longnamelist", OWRITE);
		if(longnamefd < 0){
			if((longnamefd = create("longnamelist", OWRITE, 0666)) == -1){
				fprint(2, "can't create longnamelist file\n");
				exits("create");
			}
		}
		seek(longnamefd, 0, 2);
		fprint(2,"check out file 'longnamelist'\n");
	}
	fprint(2, "%s changed to %s\n",original,newname);
	fprint(longnamefd,"%s\t%s\n",original, newname);
	strcpy(original,newname);
	ntoolong++;
  }
}
