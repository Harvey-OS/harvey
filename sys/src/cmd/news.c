/*
 *	news foo	prints /lib/news/foo
 *	news -a		prints all news items, latest first
 *	news -n		lists names of new items
 *	news		prints items changed since last news
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

#define	NINC	50	/* Multiples of directory allocation */
char	NEWS[] = "/lib/news";
char	TFILE[] = "%s/lib/newstime";

/*
 *	The following items should not be printed.
 */
char*	ignore[] =
{
	"core",
	"dead.letter",
	0
};

typedef
struct
{
	ulong	time;
	char	*name;
	char	*uid;
	vlong	length;
} File;
File*	n_list;
int	n_count;
int	n_items;
Biobuf	bout;

int	fcmp(void *a, void *b);
void	read_dir(int update);
void	print_item(char *f, File *);
void	eachitem(void (*emit)(char*, File *), int all, int update);
void	note(char *s, File *);

void
main(int argc, char *argv[])
{
	int i;

	Binit(&bout, 1, OWRITE);
	if(argc == 1) {
		eachitem(print_item, 0, 1);
		exits(0);
	}
	ARGBEGIN{
	case 'a':	/* print all */
		eachitem(print_item, 1, 0);
		break;

	case 'n':	/* names only */
		eachitem(note, 0, 0);
		if(n_items)
			Bputc(&bout, '\n');
		break;

	default:
		fprint(2, "news: bad option %c\n", ARGC());
		exits("usage");
	}ARGEND
	for(i=0; i<argc; i++)
		print_item(argv[i], nil);
	exits(0);
}

int
fcmp(void *a, void *b)
{
	vlong x;

	x = (vlong)((File*)b)->time - (vlong)((File*)a)->time;
	if(x < 0)
		return -1;
	if(x > 0)
		return 1;
	return 0;
}

static void
addfile(Dir *dir, int *nap, char *name, ulong mtime, vlong len)
{
	int j;
	File *fp;

	for(j=0; ignore[j]; j++)
		if(strcmp(ignore[j], name) == 0)
			return;
	if(*nap <= n_count) {
		*nap += NINC;
		n_list = realloc(n_list, *nap*sizeof(File));
	}
	fp = &n_list[n_count++];
	fp->name = strdup(name);
	fp->time = mtime;
	fp->uid = strdup(dir->muid[0]? dir->muid: dir->uid);
	fp->length = len;
}

/*
 *	read_dir: get the file names and modification dates for the
 *	files in /usr/news into n_list; sort them in reverse by
 *	modification date.
 */
void
read_dir(int update)
{
	Dir *d;
	char newstime[100], *home;
	int i, n, na, fd;

	n_count = 0;
	n_list = malloc(NINC*sizeof(File));
	na = NINC;
	home = getenv("home");
	if(home) {
		snprint(newstime, sizeof newstime,  TFILE, home);
		d = dirstat(newstime);
		if(d != nil) {
			addfile(d, &na, "", d->mtime-1, 0);
			free(d);
		}
		if(update) {
			fd = create(newstime, OWRITE, 0644);
			if(fd >= 0)
				close(fd);
		}
	}
	fd = open(NEWS, OREAD);
	if(fd < 0)
		sysfatal("%s: %r", NEWS);

	n = dirreadall(fd, &d);
	for(i=0; i<n; i++)
		addfile(d, &na, d[i].name, d[i].mtime, d[i].length);
	free(d);

	close(fd);
	qsort(n_list, n_count, sizeof(File), fcmp);
}

void
print_item(char *file, File *fp)
{
	int f, c, bol, bop;
	char name[4096], *p, *ep;
	Dir *dbuf;

	snprint(name, sizeof name, "%s/%s", NEWS, file);
	f = open(name, OREAD);
	if(f < 0) {
		fprint(2, "news: ");
		perror(name);
		return;
	}
	strcpy(name, "...");
	if(fp == nil) {
		dbuf = dirfstat(f);
		if(dbuf == nil)
			return;
		Bprint(&bout, "\n%s (%s) %s\n", file,
			dbuf->muid[0]? dbuf->muid : dbuf->uid,
			asctime(localtime(dbuf->mtime)));
		free(dbuf);
	} else
		Bprint(&bout, "\n%s (%s) %s\n", fp->name, fp->uid,
			asctime(localtime(fp->time)));

	bol = 1;	/* beginning of line ...\n */
	bop = 1;	/* beginning of page ...\n\n */
	for(;;) {
		c = read(f, name, sizeof(name));
		if(c <= 0)
			break;
		p = name;
		ep = p+c;
		while(p < ep) {
			c = *p++;
			if(c == '\n') {
				if(!bop) {
					Bputc(&bout, c);
					if(bol)
						bop = 1;
					bol = 1;
				}
				continue;
			}
			if(bol) {
				Bputc(&bout, '\t');
				bol = 0;
				bop = 0;
			}
			Bputc(&bout, c);
		}
	}
	if(!bol)
		Bputc(&bout, '\n');
	close(f);
}

static int
seen(File *fp)
{
	int i;

	for (i = 0; i < fp - n_list; i++)
		if (strcmp(n_list[i].name, fp->name) == 0)
			return 1;	/* duplicate in union dir. */
	return 0;
}

void
eachitem(void (*emit)(char *, File *), int all, int update)
{
	int i;
	File *fp;

	read_dir(update);
	for(i=0; i<n_count; i++) {
		fp = &n_list[i];
		if(fp->name[0] == 0) {	/* newstime */
			if(all)
				continue;
			break;
		} else if(fp->length > 0 && !seen(fp)) /* not in progress */
			(*emit)(fp->name, fp);
	}
}

void
note(char *file, File *)
{

	if(!n_items)
		Bprint(&bout, "news:");
	Bprint(&bout, " %s", file);
	n_items++;
}
