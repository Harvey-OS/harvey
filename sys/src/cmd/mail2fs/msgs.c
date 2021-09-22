#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include "util.h"

/*
 * Generate a list of msgs from a plan b mail box.
 * This can be used either to read mail by running the
 * program in the mail box directory or to generate
 * indexes for msgs
 */

#define dprint	if(debug)fprint

int showarch;
int showspam;
char** months;	// show only these dirs
char* showrunes;
int nmonths;
Biobuf	bout;
Biobuf	blist;
int debug;


char*
readf(char*f)
{
	static char	buf[1024];
	char	err[ERRMAX];
	Dir*	d;
	int	fd;
	long	n;

	fd = open(f, OREAD);
	if(fd < 0)
		return nil;
	d = dirfstat(fd);
	if(d == nil)
		goto fail;
	n = d->length;
	if(n == 0 || n > sizeof buf - 1)
		n = sizeof buf - 1;
	if(n == 0)
		n = read(fd, buf, n);
	else
		n = readn(fd, buf, n);
	if(n < 0)
		goto fail;
	buf[n] = 0;
	free(d);
	close(fd);
	return buf;
fail:
	rerrstr(err, sizeof(err));
	free(d);
	close(fd);
	werrstr(err);
	return nil;
}

char*
gethdr(char** hdrs, char* h)
{
	int	l;

	l = strlen(h);
	while(*hdrs){
		if(cistrncmp(*hdrs, h, l) == 0)
			return *hdrs + l + 1;
		hdrs++;
	}
	return "<none>";
}


char*
cleanfrom(char* f)
{
	char *c, *e;
	c = strdup(f);
	e = strchr(c, '@');
	if(!e)
		return nil;
	else
		*e = '\0';
	return c;
}

void
hdrline(char*fn, char* buf, char* flags)
{
	char*	hdrs[20+1];
	int	nhdrs;
	char*	f;
	char*	cf;
	char*	s;
	Dir*	de;
	int	n;
	int	fd;
	int	i;
	int	l;

	s = buf;
	for(i = 0; s && i < 20; i++){
		f = utfrune(s, '\n');
		if(f)
			*f++ = 0;
		hdrs[i] = s;
		s = f;
	}
	nhdrs = i;
	hdrs[nhdrs] = nil;

	f = gethdr(hdrs, "from");
	s = gethdr(hdrs, "subject");
	cf = cleanfrom(f);
	l = strlen(s);
	if(l > 48)
		l = 48;
	Bprint(&bout, "%s%-19.19s %-12.12s  %-*.*s\n", flags, fn, cf?cf:f, l, l, s);
	free(cf);
	fd = open(fn, OREAD);
	n = dirreadall(fd, &de);
	close(fd);
	if(n > 1)
		Bprint(&bout, "\t");
	for(i = 0; i < n; i++)
		if(strcmp(de[i].name, "text") != 0)
			Bprint(&bout, "\t%s/%s\n", fn, de[i].name);
}

int
mustshow(char* name)
{
	Rune r;
	int nc;

	if(isdigit(name[0]))
		return 1;
	if(showarch && name[0] == 'a' && name[1] == '.')
		return 1;
	if(showrunes != nil){
		nc = chartorune(&r, name);
		if(utfrune(showrunes, r) != nil && name[nc] == '.')
			return 1;
	}
	return 0;
}

static int
cmpent(void* a1, void* a2)
{
	Dir*	d1 = a1;
	Dir*	d2 = a2;
	int	n1, n2;

	n1 = atoi(mailnamenb(d1->name));
	n2 = atoi(mailnamenb(d2->name));
	return n2 - n1;
}

static void
attachline(char* msg, char* rel)
{
	Dir*	d;
	int	nd;
	int	fd;
	int	i;

	fd = open(msg, OREAD);
	if(fd < 0)
		return;
	nd = dirreadall(fd, &d);
	close(fd);
	for(i = 0; i < nd; i++)
		if(strcmp(d[i].name, "text") && strcmp(d[i].name, "raw") && strcmp(d[i].name, "L.mbox"))
		if(d[i].qid.type&QTDIR)
			Bprint(&bout, "\t%s/%s/text\n", rel, d[i].name);
		else
			Bprint(&bout, "\t%s/%s\n", rel, d[i].name);
	if(nd > 0)
		free(d);
}

static int
listf(char* file, int mayarchive)
{
	Biobuf*	bin;
	char*	ln;
	char*	s;
	char*	e;
	int	l;
	int	show;
	int	some;
	int	allarchived;
	Dir*	d;

	bin = Bopen(file, OREAD);
	if(bin == nil)
		return 0;
	show = some = 0;
	dprint(2, "%s: listing [file] %s\n", argv0, file);
	allarchived = 1;
	while((ln = Brdline(bin, '\n')) != nil){
		l = Blinelen(bin);
		if(l < 2)
			continue;
		ln[l-1] = 0;
		if(ln[0] >= '0' && ln[0] <= '9'){
			s = strchr(ln, '/');
			if(s != nil){
				s++;
				if(s[0] >= '0' && s[0] <= '9')
					allarchived = 0;
				show = mustshow(s);
				if(show){
					e = utfrune(s, ' ');
					if(e != nil)
						*e = 0;
					Bprint(&blist, "%s\n", s);
					if(e != nil)
						*e = ' ';
				}
			}
		}
		if(show != 0){
			some++;
			Bprint(&bout, "%s\n", ln);
		}
	}
	Bterm(bin);
	if(mayarchive && allarchived){
		/* move archive from 200901.l to 200901.la; skip it next time */
		dprint(2, "%s: %s -> %sa\n", argv0, file, file);
		d = dirstat(file);
		if(d != nil){
			s = smprint("%sa", d->name);
			if(s != nil){
				nulldir(d);
				d->name = s;
				dirwstat(file, d);
				free(s);
			}
			free(d);
		}
	}
	return some;
}

/*
 * list msgs in dir. If a file named `dir.l' or `dir.la' exists and is up to date
 * it is considered a listing of dir and used as a cache.
 */
int
list(char* dir)
{
	Dir*	d;
	Dir*	dd;
	int	n;
	int	fd;
	int	i; 
	char*	tf;
	char*	sf;
	int	some;
	char*	suf;
	char*	buf;
	char*	flags;
	char*	list;
	int	archivedlist;

	fd = open(dir, OREAD);
	if(fd < 0){
		fprint(2, "%s: %r\n", dir);
		return 0;
	}
	list = smprint("%s.l", dir);
	d = dirstat(list);
	dd = dirfstat(fd);
	archivedlist = 0;
	if(d == nil){
		free(list);
		list = smprint("%s.la", dir);
		d = dirstat(list);
		archivedlist = 1;
	}
	if(d != nil && dd != nil && d->length > 0 && dd->mtime < d->mtime){
		free(d);
		free(dd);
		close(fd);
		if(archivedlist && showarch == 0  && showrunes == nil)
			some = 0;
		else
			some = listf(list, archivedlist == 0);
		free(list);
		return some;
	}
	free(d);
	free(dd);
	dprint(2, "%s: listing %s\n", argv0, dir);
	n = dirreadall(fd, &d);
	close(fd);
	if(n <= 0)
		return 0;
	some = 0;
	qsort(d, n, sizeof(Dir), cmpent);
	for(i = 0; i < n; i++)
		if(mustshow(d[i].name)){
			dprint(2, "%s: listing %s/%s\n", argv0, dir, d[i].name);
			flags = "";
			some++;
			tf = smprint("%s/%s/text", dir, d[i].name);
			buf = readf(tf);
			sf = tf + strlen(dir) - 1;
			for(suf = sf; *suf != '/' && suf > tf; suf--)
				;
			suf++;
			hdrline(suf, buf, flags);
			free(tf);
			Bprint(&blist, "%s/%s\n", dir, d[i].name);
			tf = smprint("%s/%s", dir, d[i].name);
			sf = tf + strlen(dir) - 1;
			for(suf = sf; *suf != '/' && suf > tf; suf--)
				;
			suf++;
			attachline(tf, suf);
			free(tf);
		}
	free(d);
	return some;
}

int
member(char* s, char** list, int nlist)
{
	int	i;

	for(i = 0; i < nlist; i++)
		if(strcmp(s, list[i]) == 0)
			return 1;
	return 0;
}

int
listmbox(char* mbox)
{
	Dir*	d;
	int	fd;
	int	i;
	int	n;
	int	some;
	char*	dir;

	dprint(2, "%s: reading mbox...", argv0);
	fd = open(mbox, OREAD);
	if(fd < 0){
		fprint(2, "%s: %r\n", mbox);
		return -1;
	}
	n = dirreadall(fd, &d);
	close(fd);
	dprint(2, "done\n");
	if(n <= 0)
		return 0;
	some = 0;
	qsort(d, n, sizeof(Dir), cmpent);
	for(i = 0; i < n; i++)
		if(d[i].qid.type&QTDIR){
			if(nmonths > 0)
				if(!member(d[i].name, months, nmonths))
					continue;
			dir=smprint("%s/%s", mbox, d[i].name);
			some |= list(dir);
			free(dir);
		}
	free(d);
	return some;
}

void
usage(void)
{
	fprint(2, "usage: %s [-aD] [-s runes] [dir] [month...]\n", argv0);
	exits("usage");
}

void
main(int argc, char*argv[])
{
	char*	dir;
	char*	top;
	int	fd;
	char*	list;

	top = smprint("/mail/box/%s", getuser());
	ARGBEGIN{
	case 'D':
		debug++;
		break;
	case 'a':
		showarch++;
		break;
	case 's':
		showrunes = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;
	if(argc == 1)
		dir = cleanpath(argv[0], top);
	else{
		months = argv;
		nmonths= argc;
		dir = smprint("%s/msgs", top);
	}
	list = smprint("/tmp/msgs.%s", getuser());
	fd = create(list, OWRITE, 0640);
	if(fd < 0)
		sysfatal("%s: %r", list);
	free(list);
	Binit(&blist, fd, OWRITE);
	Binit(&bout, 1, OWRITE);
	if(!listmbox(dir))
		Bprint(&bout, "No mail\n");
	Bterm(&bout);
	Bterm(&blist);
	exits(nil);
}
