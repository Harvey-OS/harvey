#include <u.h>
#include <libc.h>

extern	ulong	du(char*, Dir*, int);
extern	ulong	k(ulong);
extern	void	err(char*);
extern	int	warn(char*);
extern	int	seen(Dir*);

int	aflag;
int	fflag;
int	nflag;
int	sflag;
int	tflag;
int	uflag;
int	qflag;
char	fmt[] = "%lud\t%s\n";
ulong	blocksize = 1024;

void
main(int argc, char *argv[])
{
	int i;
	char *s, *ss;

	ARGBEGIN {
	case 'a':	/* all files */
		aflag = 1;
		break;
	case 's':	/* only top level */
		sflag = 1;
		break;
	case 'f':	/* ignore errors */
		fflag = 1;
		break;
	case 'n':	/* all files, number of bytes */
		aflag = 1;
		nflag = 1;
		break;
	case 't':	/* return modified/accessed time */
		tflag = 1;
		break;
	case 'u':	/* accessed time */
		uflag = 1;
		break;
	case 'q':	/* qid */
		qflag = 1;
		break;
	case 'b':	/* block size */
		s = ARGF();
		if(s) {
			blocksize = strtoul(s, &ss, 0);
			if(s == ss)
				blocksize = 1;
			if(*ss == 'k')
				blocksize *= 1024;
		}
		break;
	} ARGEND
	if(argc==0)
		print(fmt, du(".", nil, 1), ".");
	else
	for(i=0; i<argc; i++)
		print(fmt, du(argv[i], nil, 1), argv[i]);
	exits(0);
}

ulong
du(char *name, Dir *dir, int top)
{
	int fd, i, n;
	Dir buf[25];
	char file[256];
	ulong nk, t, mtime;

	mtime = 0;
	if(dir == 0) {
		dir = buf;
		if(dirstat(name, dir) < 0)
			return warn(name);
		if((dir->mode&CHDIR) == 0) {
			if(tflag) {
				if(uflag)
					return dir->atime;
				return dir->mtime;
			}
			if(qflag)
				return dir->qid.path;
			return k(dir->length);
		}
		mtime = dir->mtime;
	}
	fd = open(name, OREAD);
	if(fd < 0)
		return warn(name);
	nk = 0;
	while((n=dirread(fd, buf, sizeof buf)) > 0) {
		n /= sizeof(Dir);
		dir = buf;
		for(i=0; i<n; i++, dir++) {
			if((dir->mode&CHDIR) == 0) {
				t = k(dir->length);
				nk += t;
				if(aflag) {
					sprint(file, "%s/%s", name, dir->name);
					if(tflag) {
						t = dir->mtime;
						if(uflag)
							t = dir->atime;
					}
					if(qflag)
						t = dir->qid.path;
					print(fmt, t, file);
				}
				continue;
			}
			if(strcmp(dir->name, ".") == 0 ||
			   strcmp(dir->name, "..") == 0 ||
			   seen(dir))
				continue;
			sprint(file, "%s/%s", name, dir->name);
			t = du(file, dir, 0);
			nk += t;
			if(tflag) {
				t = dir->mtime;
				if(uflag)
					t = dir->atime;
			}
			if(qflag)
				t = dir->qid.path;
			if(!sflag || top)
				print(fmt, t, file);
		}
	}
	if(n < 0)
		warn(name);
	close(fd);
	if(tflag) {
		if(uflag)
			return dir->atime;
		return mtime;
	}
	if(qflag)
		return dir->qid.path;
	return nk;
}

#define	NCACHE	128	/* must be power of two */
typedef	struct	Cache	Cache;
struct	Cache
{
	Dir*	cache;
	int	n;
	int	max;
} cache[NCACHE];

int
seen(Dir *dir)
{
	Dir *dp;
	int i;
	Cache *c;

	c = &cache[dir->qid.path&(NCACHE-1)];
	dp = c->cache;
	for(i=0; i<c->n; i++, dp++)
		if(dir->qid.path == dp->qid.path &&
		   dir->type == dp->type &&
		   dir->dev == dp->dev)
			return 1;
	if(c->n == c->max){
		c->cache = realloc(c->cache, (c->max+=20)*sizeof(Dir));
		if(cache == 0)
			err("malloc failure");
	}
	c->cache[c->n++] = *dir;
	return 0;
}

void
err(char *s)
{
	fprint(2, "du: ");
	perror(s);
	exits(s);
}

int
warn(char *s)
{
	if(fflag == 0) {
		fprint(2, "du: ");
		perror(s);
	}
	return 0;
}

ulong
k(ulong n)
{
	if(nflag)
		return n;
	n = (n+blocksize-1)/blocksize;
	return n*blocksize/1024;
}
