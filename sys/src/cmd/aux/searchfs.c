#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>

/*
 * caveat:
 * this stuff is only meant to work for ascii databases
 */

typedef struct Fid Fid;
typedef struct Fs Fs;
typedef struct Quick Quick;
typedef struct Match Match;
typedef struct Search Search;

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Nfidhash	= 32,

	/*
	 * qids
	 */
	Qroot	= 1,
	Qsearch	= 2,
	Qstats	= 3,
};

/*
 * boyer-moore quick string matching
 */
struct Quick
{
	char	*pat;
	char	*up;		/* match string for upper case of pat */
	int	len;		/* of pat (and up) -1; used for fast search */
	uchar 	jump[256];	/* jump index table */
	int	miss;		/* amount to jump if we falsely match the last char */
};
extern void	quickmk(Quick*, char*, int);
extern void	quickfree(Quick*);
extern char*	quicksearch(Quick*, char*, char*);

/*
 * exact matching of a search string
 */
struct Match
{
	Match	*next;
	char	*pat;				/* null-terminated search string */
	char	*up;				/* upper case of pat */
	int	len;				/* length of both pat and up */
	int	(*op)(Match*, char*, char*);		/* method for this partiticular search */
};

struct Search
{
	Quick		quick;		/* quick match */
	Match		*match;		/* exact matches */
	int		skip;		/* number of matches to skip */
};

extern char*	searchsearch(Search*, char*, char*, int*);
extern Search*	searchparse(char*, char*);
extern void	searchfree(Search*);

struct Fid
{
	Lock;
	Fid	*next;
	Fid	**last;
	uint	fid;
	int	ref;		/* number of fcalls using the fid */
	int	attached;		/* fid has beed attached or cloned and not clunked */

	int	open;
	Qid	qid;
	Search	*search;		/* search patterns */
	char	*where;		/* current location in the database */
	int	n;		/* number of bytes left in found item */
};

int			dostat(int, uchar*, int);
void*			emalloc(uint);
void			fatal(char*, ...);
Match*			mkmatch(Match*, int(*)(Match*, char*, char*), char*);
Match*			mkstrmatch(Match*, char*);
char*		nextsearch(char*, char*, char**, char**);
int			strlook(Match*, char*, char*);
char*			strndup(char*, int);
int			tolower(int);
int			toupper(int);
char*			urlunesc(char*, char*);
void			usage(void);

struct Fs
{
	Lock;			/* for fids */

	Fid	*hash[Nfidhash];
	uchar	statbuf[1024];	/* plenty big enough */
};
extern	void	fsrun(Fs*, int);
extern	Fid*	getfid(Fs*, uint);
extern	Fid*	mkfid(Fs*, uint);
extern	void	putfid(Fs*, Fid*);
extern	char*	fsversion(Fs*, Fcall*);
extern	char*	fsauth(Fs*, Fcall*);
extern	char*	fsattach(Fs*, Fcall*);
extern	char*	fswalk(Fs*, Fcall*);
extern	char*	fsopen(Fs*, Fcall*);
extern	char*	fscreate(Fs*, Fcall*);
extern	char*	fsread(Fs*, Fcall*);
extern	char*	fswrite(Fs*, Fcall*);
extern	char*	fsclunk(Fs*, Fcall*);
extern	char*	fsremove(Fs*, Fcall*);
extern	char*	fsstat(Fs*, Fcall*);
extern	char*	fswstat(Fs*, Fcall*);

char	*(*fcalls[])(Fs*, Fcall*) =
{
	[Tversion]		fsversion,
	[Tattach]	fsattach,
	[Tauth]	fsauth,
	[Twalk]		fswalk,
	[Topen]		fsopen,
	[Tcreate]	fscreate,
	[Tread]		fsread,
	[Twrite]	fswrite,
	[Tclunk]	fsclunk,
	[Tremove]	fsremove,
	[Tstat]		fsstat,
	[Twstat]	fswstat
};

char	Eperm[] =	"permission denied";
char	Enotdir[] =	"not a directory";
char	Enotexist[] =	"file does not exist";
char	Eisopen[] = 	"file already open for I/O";
char	Einuse[] =	"fid is already in use";
char	Enofid[] = 	"no such fid";
char	Enotopen[] =	"file is not open";
char	Ebadsearch[] =	"bad search string";

Fs	fs;
char	*database;
char	*edatabase;
int	messagesize = 8192+IOHDRSZ;
void
main(int argc, char **argv)
{
	Dir *d;
	char buf[12], *mnt, *srv;
	int fd, p[2], n;

	mnt = "/tmp";
	srv = nil;
	ARGBEGIN{
		case 's':
			srv = ARGF();
			mnt = nil;
			break;
		case 'm':
			mnt = ARGF();
			break;
	}ARGEND

	fmtinstall('F', fcallfmt);

	if(argc != 1)
		usage();
	d = nil;
	fd = open(argv[0], OREAD);
	if(fd < 0 || (d=dirfstat(fd)) == nil)
		fatal("can't open %s: %r", argv[0]);
	n = d->length;
	free(d);
	if(n == 0)
		fatal("zero length database %s", argv[0]);
	database = emalloc(n);
	if(read(fd, database, n) != n)
		fatal("can't read %s: %r", argv[0]);
	close(fd);
	edatabase = database + n;

	if(pipe(p) < 0)
		fatal("pipe failed");

	switch(rfork(RFPROC|RFMEM|RFNOTEG|RFNAMEG)){
	case 0:
		fsrun(&fs, p[0]);
		exits(nil);
	case -1:	
		fatal("fork failed");
	}

	if(mnt == nil){
		if(srv == nil)
			usage();
		fd = create(srv, OWRITE, 0666);
		if(fd < 0){
			remove(srv);
			fd = create(srv, OWRITE, 0666);
			if(fd < 0){
				close(p[1]);
				fatal("create of %s failed", srv);
			}
		}
		sprint(buf, "%d", p[1]);
		if(write(fd, buf, strlen(buf)) < 0){
			close(p[1]);
			fatal("writing %s", srv);
		}
		close(p[1]);
		exits(nil);
	}

	if(mount(p[1], -1, mnt, MREPL, "") < 0){
		close(p[1]);
		fatal("mount failed");
	}
	close(p[1]);
	exits(nil);
}

/*
 * execute the search
 * do a quick match,
 * isolate the line in which the occured,
 * and try all of the exact matches
 */
char*
searchsearch(Search *search, char *where, char *end, int *np)
{
	Match *m;
	char *s, *e;

	*np = 0;
	if(search == nil || where == nil)
		return nil;
	for(;;){
		s = quicksearch(&search->quick, where, end);
		if(s == nil)
			return nil;
		e = memchr(s, '\n', end - s);
		if(e == nil)
			e = end;
		else
			e++;
		while(s > where && s[-1] != '\n')
			s--;
		for(m = search->match; m != nil; m = m->next){
			if((*m->op)(m, s, e) == 0)
				break;
		}

		if(m == nil){
			if(search->skip > 0)
				search->skip--;
			else{
				*np = e - s;
				return s;
			}
		}

		where = e;
	}
}

/*
 * parse a search string of the form
 * tag=val&tag1=val1...
 */
Search*
searchparse(char *search, char *esearch)
{
	Search *s;
	Match *m, *next, **last;
	char *tag, *val, *p;
	int ok;

	s = emalloc(sizeof *s);
	s->match = nil;

	/*
	 * acording to the http spec,
	 * repeated search queries are ingored.
	 * the last search given is performed on the original object
	 */
	while((p = memchr(s, '?', esearch - search)) != nil){
		search = p + 1;
	}
	while(search < esearch){
		search = nextsearch(search, esearch, &tag, &val);
		if(tag == nil)
			continue;

		ok = 0;
		if(strcmp(tag, "skip") == 0){
			s->skip = strtoul(val, &p, 10);
			if(*p == 0)
				ok = 1;
		}else if(strcmp(tag, "search") == 0){
			s->match = mkstrmatch(s->match, val);
			ok = 1;
		}
		free(tag);
		free(val);
		if(!ok){
			searchfree(s);
			return nil;
		}
	}

	if(s->match == nil){
		free(s);
		return nil;
	}

	/*
	 * order the matches by probability of occurance
	 * first cut is just by length
	 */
	for(ok = 0; !ok; ){
		ok = 1;
		last = &s->match;
		for(m = *last; m && m->next; m = *last){
			if(m->next->len > m->len){
				next = m->next;
				m->next = next->next;
				next->next = m;
				*last = next;
				ok = 0;
			}
			last = &m->next;
		}
	}

	/*
	 * convert the best search into a fast lookup
	 */
	m = s->match;
	s->match = m->next;
	quickmk(&s->quick, m->pat, 1);
	free(m->pat);
	free(m->up);
	free(m);
	return s;
}

void
searchfree(Search *s)
{
	Match *m, *next;

	if(s == nil)
		return;
	for(m = s->match; m; m = next){
		next = m->next;
		free(m->pat);
		free(m->up);
		free(m);
	}
	quickfree(&s->quick);
	free(s);
}

char*
nextsearch(char *search, char *esearch, char **tagp, char **valp)
{
	char *tag, *val;

	*tagp = nil;
	*valp = nil;
	for(tag = search; search < esearch && *search != '='; search++)
		;
	if(search == esearch)
		return search;
	tag = urlunesc(tag, search);
	search++;
	for(val = search; search < esearch && *search != '&'; search++)
		;
	val = urlunesc(val, search);
	if(search != esearch)
		search++;
	*tagp = tag;
	*valp = val;
	return search;
}

Match*
mkstrmatch(Match *m, char *pat)
{
	char *s;

	for(s = pat; *s; s++){
		if(*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'){
			*s = 0;
			m = mkmatch(m, strlook, pat);
			pat = s + 1;
		}else
			*s = tolower(*s);
	}
	return mkmatch(m, strlook, pat);
}

Match*
mkmatch(Match *next, int (*op)(Match*, char*, char*), char *pat)
{
	Match *m;
	char *p;
	int n;

	n = strlen(pat);
	if(n == 0)
		return next;
	m = emalloc(sizeof *m);
	m->op = op;
	m->len = n;
	m->pat = strdup(pat);
	m->up = strdup(pat);
	for(p = m->up; *p; p++)
		*p = toupper(*p);
	for(p = m->pat; *p; p++)
		*p = tolower(*p);
	m->next = next;
	return m;
}

int
strlook(Match *m, char *str, char *e)
{
	char *pat, *up, *s;
	int c, pc, fc, fuc, n;

	n = m->len;
	fc = m->pat[0];
	fuc = m->up[0];
	for(; str + n <= e; str++){
		c = *str;
		if(c != fc && c != fuc)
			continue;
		s = str + 1;
		up = m->up + 1;
		for(pat = m->pat + 1; pc = *pat; pat++){
			c = *s;
			if(c != pc && c != *up)
				break;
			up++;
			s++;
		}
		if(pc == 0)
			return 1;
	}
	return 0;
}

/*
 * boyer-moore style pattern matching
 * implements an exact match for ascii
 * however, if mulitbyte upper-case and lower-case
 * characters differ in length or in more than one byte,
 * it only implements an approximate match
 */
void
quickmk(Quick *q, char *spat, int ignorecase)
{
	char *pat, *up;
	uchar *j;	
	int ep, ea, cp, ca, i, c, n;

	/*
	 * allocate the machine
	 */
	n = strlen(spat);
	if(n == 0){
		q->pat = nil;
		q->up = nil;
		q->len = -1;
		return;
	}
	pat = emalloc(2* n + 2);
	q->pat = pat;
	up = pat;
	if(ignorecase)
		up = pat + n + 1;
	q->up = up;
	while(c = *spat++){
		if(ignorecase){
			*up++ = toupper(c);
			c = tolower(c);
		}
		*pat++ = c;
	}
	pat = q->pat;
	up = q->up;
	pat[n] = up[n] = '\0';

	/*
	 * make the skipping table
	 */
	if(n > 255)
		n = 255;
	j = q->jump;
	memset(j, n, 256);
	n--;
	q->len = n;
	for(i = 0; i <= n; i++){
		j[(uchar)pat[i]] = n - i;
		j[(uchar)up[i]] = n - i;
	}
	
	/*
	 * find the minimum safe amount to skip
	 * if we match the last char but not the whole pat
	 */
	ep = pat[n];
	ea = up[n];
	for(i = n - 1; i >= 0; i--){
		cp = pat[i];
		ca = up[i];
		if(cp == ep || cp == ea || ca == ep || ca == ea)
			break;
	}
	q->miss = n - i;
}

void
quickfree(Quick *q)
{
	if(q->pat != nil)
		free(q->pat);
	q->pat = nil;
}

char *
quicksearch(Quick *q, char *s, char *e)
{
	char *pat, *up, *m, *ee;
	uchar *j;
	int len, n, c, mc;

	len = q->len;
	if(len < 0)
		return s;
	j = q->jump;
	pat = q->pat;
	up = q->up;
	s += len;
	ee = e - (len * 4 + 4);
	while(s < e){
		/*
		 * look for a match on the last char
		 */
		while(s < ee && (n = j[(uchar)*s])){
			s += n;
			s += j[(uchar)*s];
			s += j[(uchar)*s];
			s += j[(uchar)*s];
		}
		if(s >= e)
			return nil;
		while(n = j[(uchar)*s]){
			s += n;
			if(s >= e)
				return nil;
		}

		/*
		 * does the string match?
		 */
		m = s - len;
		for(n = 0; c = pat[n]; n++){
			mc = *m++;
			if(c != mc && mc != up[n])
				break;
		}
		if(!c)
			return s - len;
		s += q->miss;
	}
	return nil;
}

void
fsrun(Fs *fs, int fd)
{
	Fcall rpc;
	char *err;
	uchar *buf;
	int n;

	buf = emalloc(messagesize);
	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read9pmsg(fd, buf, messagesize);
		if(n == 0)
			continue;
		if(n < 0)
			fatal("mount read");

		rpc.data = (char*)buf + IOHDRSZ;
		if(convM2S(buf, n, &rpc) == 0)
			continue;
		// fprint(2, "recv: %F\n", &rpc);


		/*
		 * flushes are way too hard.
		 * a reply to the original message seems to work
		 */
		if(rpc.type == Tflush)
			continue;
		else if(rpc.type >= Tmax || !fcalls[rpc.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rpc.type])(fs, &rpc);
		if(err){
			rpc.type = Rerror;
			rpc.ename = err;
		}else
			rpc.type++;
		n = convS2M(&rpc, buf, messagesize);
		// fprint(2, "send: %F\n", &rpc);
		if(write(fd, buf, n) != n)
			fatal("mount write");
	}
}

Fid*
mkfid(Fs *fs, uint fid)
{
	Fid *f;
	int h;

	h = fid % Nfidhash;
	for(f = fs->hash[h]; f; f = f->next){
		if(f->fid == fid)
			return nil;
	}

	f = emalloc(sizeof *f);
	f->next = fs->hash[h];
	if(f->next != nil)
		f->next->last = &f->next;
	f->last = &fs->hash[h];
	fs->hash[h] = f;

	f->fid = fid;
	f->ref = 1;
	f->attached = 1;
	f->open = 0;
	return f;
}

Fid*
getfid(Fs *fs, uint fid)
{
	Fid *f;
	int h;

	h = fid % Nfidhash;
	for(f = fs->hash[h]; f; f = f->next){
		if(f->fid == fid){
			if(f->attached == 0)
				break;
			f->ref++;
			return f;
		}
	}
	return nil;
}

void
putfid(Fs *, Fid *f)
{
	f->ref--;
	if(f->ref == 0 && f->attached == 0){
		*f->last = f->next;
		if(f->next != nil)
			f->next->last = f->last;
		if(f->search != nil)
			searchfree(f->search);
		free(f);
	}
}

char*
fsversion(Fs *, Fcall *rpc)
{
	if(rpc->msize < 256)
		return "version: message size too small";
	if(rpc->msize > messagesize)
		rpc->msize = messagesize;
	messagesize = rpc->msize;
	if(strncmp(rpc->version, "9P2000", 6) != 0)
		return "unrecognized 9P version";
	rpc->version = "9P2000";
	return nil;
}

char*
fsauth(Fs *, Fcall *)
{
	return "searchfs: authentication not required";
}

char*
fsattach(Fs *fs, Fcall *rpc)
{
	Fid *f;

	f = mkfid(fs, rpc->fid);
	if(f == nil)
		return Einuse;
	f->open = 0;
	f->qid.type = QTDIR;
	f->qid.path = Qroot;
	f->qid.vers = 0;
	rpc->qid = f->qid;
	putfid(fs, f);
	return nil;
}

char*
fswalk(Fs *fs, Fcall *rpc)
{
	Fid *f, *nf;
	int nqid, nwname, type;
	char *err, *name;
	ulong path;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	nf = nil;
	if(rpc->fid != rpc->newfid){
		nf = mkfid(fs, rpc->newfid);
		if(nf == nil){
			putfid(fs, f);
			return Einuse;
		}
		nf->qid = f->qid;
		putfid(fs, f);
		f = nf;	/* walk f */
	}

	err = nil;
	path = f->qid.path;
	nwname = rpc->nwname;
	for(nqid=0; nqid<nwname; nqid++){
		if(path != Qroot){
			err = Enotdir;
			break;
		}
		name = rpc->wname[nqid];
		if(strcmp(name, "search") == 0){
			type = QTFILE;
			path = Qsearch;
		}else if(strcmp(name, "stats") == 0){
			type = QTFILE;
			path = Qstats;
		}else if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0){
			type = QTDIR;
			path = path;
		}else{
			err = Enotexist;
			break;
		}
		rpc->wqid[nqid] = (Qid){path, 0, type};
	}

	if(nwname > 0){
		if(nf != nil && nqid < nwname)
			nf->attached = 0;
		if(nqid == nwname)
			f->qid = rpc->wqid[nqid-1];
	}

	putfid(fs, f);
	rpc->nwqid = nqid;
	f->open = 0;
	return err;
}

char *
fsopen(Fs *fs, Fcall *rpc)
{
	Fid *f;
	int mode;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(f->open){
		putfid(fs, f);
		return Eisopen;
	}
	mode = rpc->mode & OPERM;
	if(mode == OEXEC
	|| f->qid.path == Qroot && (mode == OWRITE || mode == ORDWR)){
		putfid(fs, f);
		return Eperm;
	}
	f->open = 1;
	f->where = nil;
	f->n = 0;
	f->search = nil;
	rpc->qid = f->qid;
	rpc->iounit = messagesize-IOHDRSZ;
	putfid(fs, f);
	return nil;
}

char *
fscreate(Fs *, Fcall *)
{
	return Eperm;
}

char*
fsread(Fs *fs, Fcall *rpc)
{
	Fid *f;
	int n, off, count, len;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(!f->open){
		putfid(fs, f);
		return Enotopen;
	}
	count = rpc->count;
	off = rpc->offset;
	rpc->count = 0;
	if(f->qid.path == Qroot){
		if(off > 0)
			rpc->count = 0;
		else
			rpc->count = dostat(Qsearch, (uchar*)rpc->data, count);
		putfid(fs, f);
		if(off == 0 && rpc->count <= BIT16SZ)
			return "directory read count too small";
		return nil;
	}
	if(f->qid.path == Qstats){
		len = 0;
	}else{
		for(len = 0; len < count; len += n){
			if(f->where == nil || f->search == nil)
				break;
			if(f->n == 0)
				f->where = searchsearch(f->search, f->where, edatabase, &f->n);
			n = f->n;
			if(n != 0){
				if(n > count-len)
					n = count-len;
				memmove(rpc->data+len, f->where, n);
				f->where += n;
				f->n -= n;
			}
		}
	}
	putfid(fs, f);
	rpc->count = len;
	return nil;
}

char*
fswrite(Fs *fs, Fcall *rpc)
{
	Fid *f;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(!f->open || f->qid.path != Qsearch){
		putfid(fs, f);
		return Enotopen;
	}

	if(f->search != nil)
		searchfree(f->search);
	f->search = searchparse(rpc->data, rpc->data + rpc->count);
	if(f->search == nil){
		putfid(fs, f);
		return Ebadsearch;
	}
	f->where = database;
	f->n = 0;
	putfid(fs, f);
	return nil;
}

char *
fsclunk(Fs *fs, Fcall *rpc)
{
	Fid *f;

	f = getfid(fs, rpc->fid);
	if(f != nil){
		f->attached = 0;
		putfid(fs, f);
	}
	return nil;
}

char *
fsremove(Fs *, Fcall *)
{
	return Eperm;
}

char *
fsstat(Fs *fs, Fcall *rpc)
{
	Fid *f;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	rpc->stat = fs->statbuf;
	rpc->nstat = dostat(f->qid.path, rpc->stat, sizeof fs->statbuf);
	putfid(fs, f);
	if(rpc->nstat <= BIT16SZ)
		return "stat count too small";
	return nil;
}

char *
fswstat(Fs *, Fcall *)
{
	return Eperm;
}

int
dostat(int path, uchar *buf, int nbuf)
{
	Dir d;

	switch(path){
	case Qroot:
		d.name = ".";
		d.mode = DMDIR|0555;
		d.qid.type = QTDIR;
		break;
	case Qsearch:
		d.name = "search";
		d.mode = 0666;
		d.qid.type = QTFILE;
		break;
	case Qstats:
		d.name = "stats";
		d.mode = 0666;
		d.qid.type = QTFILE;
		break;
	}
	d.qid.path = path;
	d.qid.vers = 0;
	d.length = 0;
	d.uid = d.gid = d.muid = "none";
	d.atime = d.mtime = time(nil);
	return convD2M(&d, buf, nbuf);
}

char *
urlunesc(char *s, char *e)
{
	char *t, *v;
	int c, n;

	v = emalloc((e - s) + 1);
	for(t = v; s < e; s++){
		c = *s;
		if(c == '%'){
			if(s + 2 >= e)
				break;
			n = s[1];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			c = n;
			n = s[2];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			s += 2;
			c = c * 16 + n;
		}
		*t++ = c;
	}
	*t = 0;
	return v;
}

int
toupper(int c)
{
	if(c >= 'a' && c <= 'z')
		c += 'A' - 'a';
	return c;
}

int
tolower(int c)
{
	if(c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	return c;
}

void
fatal(char *fmt, ...)
{
	va_list arg;
	char buf[1024];

	write(2, "searchfs: ", 8);
	va_start(arg, fmt);
	vseprint(buf, buf+1024, fmt, arg);
	va_end(arg);
	write(2, buf, strlen(buf));
	write(2, "\n", 1);
	exits(fmt);
}

void *
emalloc(uint n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		fatal("out of memory");
	memset(p, 0, n);
	return p;
}

void
usage(void)
{
	fprint(2, "usage: searchfs [-m mountpoint] [-s srvfile] database\n");
	exits("usage");
}
