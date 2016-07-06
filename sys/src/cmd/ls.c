/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>

typedef struct NDir NDir;
struct NDir
{
	Dir *d;
	char	*prefix;
};

int	errs = 0;
int	dflag;
int	lflag;
int	mflag;
int	nflag;
int	pflag;
int	qflag;
int	Qflag;
int	rflag;
int	sflag;
int	tflag;
int	Tflag;
int	uflag;
int	Fflag;
int	ndirbuf;
int	ndir;
NDir*	dirbuf;
int	ls(char*, int);
int	compar(const NDir*, const NDir*);
char*	asciitime(int32_t);
char*	darwx(int32_t);
void	rwx(int32_t, char*);
void	growto(int32_t);
void	dowidths(Dir*);
void	format(Dir*, char*);
void	output(void);
char*	xcleanname(char*);
uint32_t	clk;
int	swidth;			/* max width of -s size */
int	qwidth;			/* max width of -q version */
int	vwidth;			/* max width of dev */
int	uwidth;			/* max width of userid */
int	mwidth;			/* max width of muid */
int	lwidth;			/* max width of length */
int	gwidth;			/* max width of groupid */
Biobuf	bin;

void
main(int argc, char *argv[])
{
	int i;
	char dot[] = {'.', 0};


	Binit(&bin, 1, OWRITE);
	ARGBEGIN{
	case 'F':	Fflag++; break;
	case 'd':	dflag++; break;
	case 'l':	lflag++; break;
	case 'm':	mflag++; break;
	case 'n':	nflag++; break;
	case 'p':	pflag++; break;
	case 'q':	qflag++; break;
	case 'Q':	Qflag++; break;
	case 'r':	rflag++; break;
	case 's':	sflag++; break;
	case 't':	tflag++; break;
	case 'T':	Tflag++; break;
	case 'u':	uflag++; break;
	default:	fprint(2, "usage: ls [-dlmnpqrstuFQT] [file ...]\n");
			exits("usage");
	}ARGEND

	doquote = needsrcquote;
	quotefmtinstall();
	fmtinstall('M', dirmodefmt);

	if(lflag)
		clk = time(0);
	if(argc == 0)
		errs = ls(dot, 0);
	else for(i=0; i<argc; i++)
		errs |= ls(argv[i], 1);
	output();
	exits(errs? "errors" : 0);
}

int
ls(char *s, int multi)
{
	int fd;
	int32_t i, n;
	char *p;
	Dir *db;

	db = dirstat(s);
	if(db == nil){
    error:
		fprint(2, "ls: %s: %r\n", s);
		return 1;
	}
	if((db->qid.type&QTDIR) && dflag==0){
		free(db);
		output();
		fd = open(s, OREAD);
		if(fd == -1)
			goto error;
		n = dirreadall(fd, &db);
		if(n < 0)
			goto error;
		xcleanname(s);
		growto(ndir+n);
		for(i=0; i<n; i++){
			dirbuf[ndir+i].d = db+i;
			dirbuf[ndir+i].prefix = multi? s : 0;
		}
		ndir += n;
		close(fd);
		output();
	}else{
		growto(ndir+1);
		dirbuf[ndir].d = db;
		dirbuf[ndir].prefix = 0;
		xcleanname(s);
		p = utfrrune(s, '/');
		if(p){
			dirbuf[ndir].prefix = s;
			*p = 0;
		}
		ndir++;
	}
	return 0;
}

void
output(void)
{
	int i;
	char buf[4096];
	char *s;

	if(!nflag)
		qsort(dirbuf, ndir, sizeof dirbuf[0], (int (*)(const void *, const void*))compar);
	for(i=0; i<ndir; i++)
		dowidths(dirbuf[i].d);
	for(i=0; i<ndir; i++) {
		if(!pflag && (s = dirbuf[i].prefix)) {
			if(strcmp(s, "/") ==0)	/* / is a special case */
				s = "";
			sprint(buf, "%s/%s", s, dirbuf[i].d->name);
			format(dirbuf[i].d, buf);
		} else
			format(dirbuf[i].d, dirbuf[i].d->name);
	}
	ndir = 0;
	Bflush(&bin);
}

void
dowidths(Dir *db)
{
	char buf[256];
	int n;

	if(sflag) {
		n = sprint(buf, "%llud", (db->length+1023)/1024);
		if(n > swidth)
			swidth = n;
	}
	if(qflag) {
		n = sprint(buf, "%lu", db->qid.vers);
		if(n > qwidth)
			qwidth = n;
	}
	if(mflag) {
		n = snprint(buf, sizeof buf, "[%q]", db->muid);
		if(n > mwidth)
			mwidth = n;
	}
	if(lflag) {
		n = sprint(buf, "%u", db->dev);
		if(n > vwidth)
			vwidth = n;
		n = sprint(buf, "%q", db->uid);
		if(n > uwidth)
			uwidth = n;
		n = sprint(buf, "%q", db->gid);
		if(n > gwidth)
			gwidth = n;
		n = sprint(buf, "%llud", db->length);
		if(n > lwidth)
			lwidth = n;
	}
}

char*
fileflag(Dir *db)
{
	if(Fflag == 0)
		return "";
	if(QTDIR & db->qid.type)
		return "/";
	if(0111 & db->mode)
		return "*";
	return "";
}

void
format(Dir *db, char *name)
{
	int i;

	if(sflag)
		Bprint(&bin, "%*llud ",
			swidth, (db->length+1023)/1024);
	if(mflag){
		Bprint(&bin, "[%q] ", db->muid);
		for(i=2+strlen(db->muid); i<mwidth; i++)
			Bprint(&bin, " ");
	}
	if(qflag)
		Bprint(&bin, "(%.16llux %*lud %.2ux) ",
			db->qid.path,
			qwidth, db->qid.vers,
			db->qid.type);
	if(Tflag)
		Bprint(&bin, "%c ", (db->mode&DMTMP)? 't': '-');

	if(lflag)
		Bprint(&bin, "%M %C %*ud %*q %*q %*llud %s ",
			db->mode, db->type,
			vwidth, db->dev,
			-uwidth, db->uid,
			-gwidth, db->gid,
			lwidth, db->length,
			asciitime(uflag? db->atime: db->mtime));
	Bprint(&bin, Qflag? "%s%s\n": "%q%s\n", name, fileflag(db));
}

void
growto(int32_t n)
{
	if(n <= ndirbuf)
		return;
	ndirbuf = n;
	dirbuf=(NDir *)realloc(dirbuf, ndirbuf*sizeof(NDir));
	if(dirbuf == 0){
		fprint(2, "ls: malloc fail\n");
		exits("malloc fail");
	}
}

int
compar(const NDir *a, const NDir *b)
{
	int32_t i;
	Dir *ad, *bd;

	ad = a->d;
	bd = b->d;

	if(tflag){
		if(uflag)
			i = bd->atime-ad->atime;
		else
			i = bd->mtime-ad->mtime;
	}else{
		if(a->prefix && b->prefix){
			i = strcmp(a->prefix, b->prefix);
			if(i == 0)
				i = strcmp(ad->name, bd->name);
		}else if(a->prefix){
			i = strcmp(a->prefix, bd->name);
			if(i == 0)
				i = 1;	/* a is int32_ter than b */
		}else if(b->prefix){
			i = strcmp(ad->name, b->prefix);
			if(i == 0)
				i = -1;	/* b is int32_ter than a */
		}else
			i = strcmp(ad->name, bd->name);
	}
	if(i == 0)
		i = (a<b? -1 : 1);
	if(rflag)
		i = -i;
	return i;
}

char*
asciitime(int32_t l)
{
	static char buf[32];
	char *t;

	t = ctime(l);
	/* 6 months in the past or a day in the future */
	if(l<clk-180L*24*60*60 || clk+24L*60*60<l){
		memmove(buf, t+4, 7);		/* month and day */
		memmove(buf+7, t+23, 5);		/* year */
	}else
		memmove(buf, t+4, 12);		/* skip day of week */
	buf[12] = 0;
	return buf;
}

/*
 * Compress slashes, remove trailing slash.  Don't worry about . and ..
 */
char*
xcleanname(char *name)
{
	char *r, *w;
	for(r=w=name; *r; r++){
		if(*r=='/' && r>name && *(r-1)=='/')
			continue;
		if(w != r)
			*w = *r;
		w++;
	}
	*w = 0;
	while(w-1>name && *(w-1)=='/')
		*--w = 0;
	return name;
}
