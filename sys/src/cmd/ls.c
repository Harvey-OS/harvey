#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>

typedef struct NDir NDir;
struct NDir
{
	Dir;
	char	*prefix;
};

int	errs = 0;
int	dflag;
int	lflag;
int	nflag;
int	pflag;
int	qflag;
int	rflag;
int	sflag;
int	tflag;
int	uflag;
int	ndirbuf;
int	ndir;
NDir*	dirbuf;
int	ls(char*, int);
int	compar(NDir*, NDir*);
char*	asciitime(long);
char*	darwx(long);
void	rwx(long, char*);
void	growto(long);
void	dowidths(Dir*);
void	format(Dir*, char*);
void	output(void);
ulong	clk;
int	swidth;
int	qwidth;
int	vwidth;
int	uwidth;
int	glwidth;
Biobuf	bin;

#define		HUNK	50

void
main(int argc, char *argv[])
{
	int i, fd;
	char buf[64];

	Binit(&bin, 1, OWRITE);
	ARGBEGIN{
	case 'd':	dflag++; break;
	case 'l':	lflag++; break;
	case 'n':	nflag++; break;
	case 'p':	pflag++; break;
	case 'q':	qflag++; break;
	case 'r':	rflag++; break;
	case 's':	sflag++; break;
	case 't':	tflag++; break;
	case 'u':	uflag++; break;
	default:	fprint(2, "usage: ls [-dlnpqrstu] files\n");
			exits("usage");
	}ARGEND

	fmtinstall('M', dirmodeconv);

	if(lflag){
		fd = open("/dev/time", OREAD);
		if(fd<0 || read(fd, buf, sizeof buf-1)<=0)
			fprint(2, "ls: can't open /dev/time\n");
		close(fd);
		clk = strtoul(buf, 0, 0);
	}
	if(argc == 0)
		errs = ls(".", 0);
	else for(i=0; i<argc; i++)
		errs |= ls(argv[i], 1);
	output();
	exits(errs? "errors" : 0);
}

int
ls(char *s, int multi)
{
	int fd;
	long i, n;
	char *p;
	Dir db[HUNK];

	for(;;) {
		p = utfrrune(s, '/');
		if(p == 0 || p[1] != 0 || p == s)
			break;
		*p = 0;
	}
	if(dirstat(s, db)==-1){
    error:
		fprint(2, "ls: %s: ", s);
		perror(0);
		return 1;
	}
	if(db[0].qid.path&CHDIR && dflag==0){
		output();
		fd = open(s, OREAD);
		if(fd == -1)
			goto error;
		while((n=dirread(fd, db, sizeof db)) > 0){
			n /= sizeof(Dir);
			growto(ndir+n);
			for(i=0; i<n; i++){
				memmove(dirbuf+ndir+i, db+i, sizeof(Dir));
				dirbuf[ndir+i].prefix = multi? s : 0;
			}
			ndir += n;
		}
		close(fd);
		output();
	}else{
		growto(ndir+1);
		memmove(dirbuf+ndir, db, sizeof(Dir));
		dirbuf[ndir].prefix = 0;
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
		qsort(dirbuf, ndir, sizeof dirbuf[0], (int (*)(void*, void*))compar);
	swidth = 0;	/* max width of -s size */
	qwidth = 0;	/* max width of -q version */
	vwidth = 0;	/* max width of dev */
	uwidth = 0;	/* max width of userid */
	glwidth = 0;	/* max width of groupid and length */
	for(i=0; i<ndir; i++)
		dowidths(&dirbuf[i]);
	for(i=0; i<ndir; i++) {
		if(!pflag && (s = dirbuf[i].prefix)) {
			if(strcmp(s, "/") ==0)	/* / is a special case */
				s = "";
			sprint(buf, "%s/%s", s, dirbuf[i].name);
			format(&dirbuf[i], buf);
		} else
			format(&dirbuf[i], dirbuf[i].name);
	}
	ndir = 0;
	Bflush(&bin);
}

void
dowidths(Dir *db)
{
	char buf[100];
	int n;

	if(sflag) {
		sprint(buf, "%ld", (db->length+1023)/1024);
		n = strlen(buf);
		if(n > swidth)
			swidth = n;
	}
	if(qflag) {
		sprint(buf, "%ld", db->qid.vers);
		n = strlen(buf);
		if(n > qwidth)
			qwidth = n;
	}
	if(lflag) {
		sprint(buf, "%d", db->dev);
		n = strlen(buf);
		if(n > vwidth)
			vwidth = n;
		n = strlen(db->uid);
		if(n > uwidth)
			uwidth = n;
		sprint(buf, "%ld", db->length);
		n = strlen(buf) + strlen(db->gid);
		if(n > glwidth)
			glwidth = n;
	}
}

void
format(Dir *db, char *name)
{
	if(sflag)
		Bprint(&bin, "%*ld ",
			swidth, (db->length+1023)/1024);
	if(qflag)
		Bprint(&bin, "%.8lux %*lud ",
			db->qid.path,
			qwidth, db->qid.vers);
	if(lflag)
		Bprint(&bin, "%M %c %*d %*s %s %*ld %s %s\n",
			db->mode, db->type,
			vwidth, db->dev,
			-uwidth, db->uid,
			db->gid,
			glwidth-strlen(db->gid), db->length,
			asciitime(uflag? db->atime : db->mtime), name);
	else
		Bprint(&bin, "%s\n", name);
}

void
growto(long n)
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
compar(NDir *a, NDir *b)
{
	long i;

	if(tflag){
		if(uflag)
			i = b->atime-a->atime;
		else
			i = b->mtime-a->mtime;
	}else{
		if(a->prefix && b->prefix){
			i = strcmp(a->prefix, b->prefix);
			if(i == 0)
				i = strcmp(a->name, b->name);
		}else if(a->prefix){
			i = strcmp(a->prefix, b->name);
			if(i == 0)
				i = 1;	/* a is longer than b */
		}else if(b->prefix){
			i = strcmp(a->name, b->prefix);
			if(i == 0)
				i = -1;	/* b is longer than a */
		}else
			i = strcmp(a->name, b->name);
	}
	if(i == 0)
		i = (a<b? -1 : 1);
	if(rflag)
		i = -1 * i;
	return i;
}

char*
asciitime(long l)
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
