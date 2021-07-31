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

/*
 * Wrap/inst might try to overwrite itself.
 * To avoid problems with this, we copy ourselves
 * into /tmp and then re-exec.
 */
char *rmargv0;

static void
rmself(void)
{
	remove(rmargv0);
}

static void
membogus(char **argv)
{
	int n, fd, wfd;
	char template[50], buf[1024];

	if(strprefix(argv[0], "/tmp/_inst_")==0) {
		rmargv0 = argv[0];
		atexit(rmself);
		return;
	}

	if((fd = open(argv[0], OREAD)) < 0)
		return;

	strcpy(template, "/tmp/_inst_XXXXXX");
	if((wfd = genopentemp(template, OWRITE, 0700)) < 0)
		return;

	while((n = read(fd, buf, sizeof buf)) > 0)
		if(write(wfd, buf, n) != n)
			goto Error;

	if(n != 0)
		goto Error;

	close(fd);
	close(wfd);

	argv[0] = template;
	exec(template, argv);
	fprint(2, "exec error %r\n");

Error:
	close(fd);
	close(wfd);
	remove(template);
	return;
}

void
usage(void)
{
	fprint(2, "usage: wrap/inst [-ovxF] [-r root] package [prefix...]\n");
	exits("usage");
}

static int skipfd = -1;
static int skiprmfd = -1;
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
skiprmfile(char *file, char *why, int log)
{
	char buf[2*NAMELEN];

	problems = 1;

	fprint(2, "not removing %s: %s\n", file, why);
	if(log) {
		strcpy(buf, "/tmp/wrap.skip.XXXXXX");
		if(skiprmfd == -1)
			skiprmfd = opentemp(buf);

		fprint(skiprmfd, "%s\n", file);
	}
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
	char *p, *q, *tm, *why;
	int docopy;
	vlong t;
	Biobuf bin, *b;

	membogus(argv);

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
	if(w->nu != 1)
		sysfatal("strange package: more than one piece");

	oldw = openwrap(w->name, root);

	if(w->u->utime && (oldw == nil || oldw->tfull < w->u->utime)) {
		tm = asctime(localtime(w->u->utime));
		if(q = strchr(tm, '\n'))
			*q = '\0';
		sysfatal("need %s version of %s already installed", tm, w->name);
	}

	why = nil;
	while(a = gethdr(arch)) {
		if(match(a->name, argv+1, argc-1) == 0)
			continue;

		p = mkpath(root, a->name);

		/*
		 * Decide whether to install the archived copy of the file.
		 */

		/*
		 * If the force flag is set, always copy it.
		 */
		if(force)
			docopy = 1;

		/* 
		 * If it's a directory, copying it can do no harm.
		 */
		else if(a->mode & CHDIR)
			docopy = 1;

		/*
		 * If the file does not currently exist, we will install our copy.
		 */
		else if(md5file(p, digest) < 0)
			docopy = 1;

		/*
		 * The file exists.  If it matches one from another package,
		 * that date must be older than our installing package's date.
		 */
		else if(getfileinfo(oldw, a->name, &t, digest, nil) >= 0) {
			if(t > w->u->time) {
				/*
				 * The installed file comes from a newer package
				 * than the one being installed.  Leave it be.
				 */
				docopy = 0;
				why = "version from newer package exists";
			} else
				docopy = 1;
		}

		/*
		 * The file exists.  It does not match a previously installed one.
		 * If we expected it to previously exist, then it has been 
		 * locally modified.  Otherwise, it has been locally created.
		 */
		else if(getfileinfo(oldw, a->name, nil, nil, nil) >= 0) {
			docopy = 0;
			why = "locally modified";
		} else {
			docopy = 0;
			why = "locally created";
		}


		if(docopy) {
			extract(arch, a, p);
		} else {
			/*
			 * Don't whine about not writing a file if the one that
			 * is already there is what we would have written.
			 */
			Bmd5sum(arch->b, digest0, a->length);
			if(memcmp(digest, digest0, MD5dlen) == 0)
				/* don't whine */;
			else
				skipfile(a->name, why, 1);
		}
	}

	p = mkpath(w->u->dir, "remove");
	if(b = Bopen(p, OREAD)) {
		while(p = Bgetline(b)) {
			/*
			 * The file has to be the one we expected to remove.
			 * Don't want to remove local changes.
			 */
			q = mkpath(root, p);
			if(md5file(q, digest) < 0)
				/* already gone */;
			else if(getfileinfo(oldw, p, nil, digest, nil) < 0)
				skiprmfile(q, "locally modified", 1);
			else {
				if(verbose || donothing)
					print("rm %s\n", q);
				if(!donothing)
					if(remove(q) < 0)
						fprint(2, "rm %s: %r\n", q);
			}
			free(q);
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
	}

	if(skiprmfd != -1) {
		fprint(2, "# rm \\\n");
		seek(skiprmfd, 0, 0);	/* Binit assumes this */
		Binit(&bin, skiprmfd, OREAD);
		while(p = Bgetline(&bin)) {
			fprint(2, "\t%s \\\n", p);
			free(p);
		}
	}

	exits(problems ? "problems" : nil);
}
