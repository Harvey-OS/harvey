/*
 * wrap/install - install software package
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

Wrap *oldw;
Wrap *w;

char *root = "/";
char **prefix;
int nprefix;

int debug;
int setowner;
int force;
int verbose;
int donothing;
int problems;

void
usage(void)
{
	fprint(2, "usage: wrap/inst [-ovxF] [-r root] package [prefix...]\n");
	exits("usage");
}

static int skipfd = -1;
void
skipfile(char *file, char *why, int log)
{
	char buf[2*NAMELEN];

	problems = 1;

	fprint(2, "skipping %s: %s\n", file, why);
	if(log) {
		strcpy(buf, "/tmp/wrap.skip.XXXXXX");
		if(skipfd == -1)
			skipfd = opentemp(buf);

		fprint(skipfd, "%s\n", file);
	}
}

void
doremove(char *fn)
{
	char *p;

	p = mkpath(root, fn);
	if(access(p, 0) >= 0) {
		if(verbose || donothing)
			print("rm %s\n", p);
		if(!donothing)
			if(remove(p) < 0)
				fprint(2, "rm %s: %r\n", p);
	}
	free(p);
}

void
extract(Arch *arch, Ahdr *a, char *to)
{
	Dir d;
	int fd;
	Biobuf b;

	if(a->mode & CHDIR) {
		if(dirstat(to, &d) < 0) {
			if(verbose || donothing)
				print("mkdir %s\n", to);
			if(donothing)
				return;
			if((fd = create(to, OREAD, a->mode)) < 0) {
				fprint(2, "mkdir %s: %r\n", to);
				problems = 1;
				return;
			}
			close(fd);
		} else if((d.mode & CHDIR) == 0) {
			if(verbose || donothing)
				print("mkdir %s\n", to);
			if(donothing)
				return;
			fprint(2, "mkdir %s: is a file\n", to);
			problems = 1;
			return;
		} else {
			/*
			 * the directory already existed, don't wstat it.
			 */
			return;
		}
	} else {
		if(verbose || donothing)
			print("write %s\n", to);
		if(donothing)
			return;
		if((fd = create(to, OWRITE, a->mode)) < 0) {
			fprint(2, "write %s: %r\n", to);
			problems = 1;
			return;
		}

		if(dirstat(to, &d) < 0) {	/* should not happen */
			fprint(2, "stat created file %s: %r\n", to);
			problems = 1;
			return;
		}

		Binit(&b, fd, OWRITE);
		if(Bcopy(&b, arch->b, a->length) < 0)
			fprint(2, "write %s: copy error; %r\n", to);
		Bterm(&b);
		close(fd);
	}

	if(verbose)
		print("wstat %s\n", to);

	if(dirstat(to, &d) < 0) {
		fprint(2, "stat created file failed! %r\n");
		return;
	}

	d.mode = a->mode;
	if(setowner) {
		strcpy(d.uid, a->uid);
		strcpy(d.gid, a->gid);
	}
	d.mtime = a->mtime;
	if(dirwstat(to, &d) < 0)
		fprint(2, "wstat %s: %r\n", to);
}

void
main(int argc, char **argv)
{
	Arch *arch;
	Ahdr *a;
	uchar digest[MD5dlen], digest0[MD5dlen];
	char *p, *q, *tm;
	vlong t;
	Biobuf bin, *b;

	ARGBEGIN{
	case 'D':
		debug = 1;
		break;
	case 'r':
		root = ARGF();
		break;
	case 'o':
		setowner = 1;
		break;
	case 'F':
		force = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	case 'x':
		donothing = 1;
		break;
	default:
		usage();
	}ARGEND

	rfork(RFNAMEG);
	fmtinstall('M', md5conv);

	if(argc < 1)
		usage();

	if((arch = openarch(argv[0])) == nil)
		sysfatal("cannot open archive '%s': %r", argv[0]);

	w = openwraphdr(argv[0], root, nil, -1);
	if(w == nil)
		sysfatal("no such package found");

	oldw = openwrap(w->name, root);

	if(w->u->utime && (oldw == nil || oldw->time < w->u->utime)) {
		tm = asctime(localtime(w->u->utime));
		if(q = strchr(tm, '\n'))
			*q = '\0';
		sysfatal("need %s version of %s already installed", tm, w->name);
	}
	while(a = gethdr(arch)) {
		if(match(a->name, argv+1, argc-1) == 0)
			continue;

		p = mkpath(root, a->name);
		if(force == 0 && (a->mode & CHDIR) == 0) {
			if(getfileinfo(oldw, a->name, &t, digest) >= 0) {
				if(md5file(p, digest0) >= 0) {
					if(memcmp(digest, digest0, MD5dlen) != 0) {
						if(debug)
							fprint(2, "file %s expect %M got %M\n",
								a->name, digest, digest0);
						/*
						 * Don't complain if it's the file we were
						 * going to install.
						 */
						Bmd5sum(arch->b, digest, a->length);
						if(memcmp(digest, digest0, MD5dlen) != 0)
							skipfile(a->name, "locally updated", 1);
						free(p);
						continue;
					}
					if(t >= w->time) {
						if(t > w->time)
							skipfile(a->name, "newer than archive", 1);
						/* don't warn about t == w->t; probably same archive */
						free(p);
						continue;
					}
				}
			} else {
				if(access(p, 0) >= 0) {
					/*
					 * Don't complain if it's the file we were
					 * going to install.
					 */
					Bmd5sum(arch->b, digest, a->length);
					if(md5file(p, digest0) >= 0 && memcmp(digest, digest0, MD5dlen) != 0)
						skipfile(a->name, "locally created", 1);
					free(p);
					continue;
				}
			}
		}
		extract(arch, a, p);
		free(p);
	}

	p = mkpath(w->u->dir, "remove");
	if(b = Bopen(p, OREAD)) {
		while(p = Bgetline(b)) {
			doremove(p);
			free(p);
		}
	}

	if(skipfd != -1) {
		fprint(2, "wrap/wdiff -r %s %s \\\n", root, argv[0]);
		seek(skipfd, 0, 0);	/* Binit assumes this */
		Binit(&bin, skipfd, OREAD);
		while(p = Bgetline(&bin)) {
			fprint(2, "\t%s \\\n", p);
			free(p);
		}
		exits("some skips");
	}
	exits(0);
}
