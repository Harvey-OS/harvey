#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

/*
 * Generate a list of msgs from files named in the input.
 */
Biobuf	bout;
Biobuf	blist;

char*
readf(char*f)
{
	static char buf[1024];
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
		if(strcmp(d[i].name, "text") && strcmp(d[i].name, "raw") &&
		   strcmp(d[i].name, "L.mbox"))
			if(d[i].qid.type&QTDIR)
				Bprint(&bout, "\t%s/%s/text\n", rel, d[i].name);
			else
				Bprint(&bout, "\t%s/%s\n", rel, d[i].name);
	if(nd > 0)
		free(d);
}

void
hdrline(char*fn, char* buf, char* flags)
{
	char*	hdrs[10+1];
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
	for(i = 0; s && i < 10; i++){
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

static char*
cleanpath(char* file, char* dir)
{
	char*	s;
	char*	t;
	char	cwd[512];

	assert(file && file[0]);
	if(file[1])
		file = strdup(file);
	else {
		s = file;
		file = malloc(3);
		file[0] = s[0];
		file[1] = 0;
	}
	s = cleanname(file);
	if(s[0] != '/' && access(s, AEXIST) == 0){
		getwd(cwd, sizeof(cwd));
		t = smprint("%s/%s", cwd, s);
		free(s);
		return t;
	}
	if(s[0] != '/' && dir != nil && dir[0] != 0){
		t = smprint("%s/%s", dir, s);
		free(s);
		s = cleanname(t);
	}
	return s;
}

void
usage(void)
{
	fprint(2, "usage: %s \n", argv0);
	exits("usage");
}

void
main(int argc, char*argv[])
{
	char*	top;
	char*	f;
	char*	mf;
	Biobuf	bin;
	char*	buf;
	char*	amf;
	char*	p;

	ARGBEGIN{
	default:
		usage();
	}ARGEND;
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	top = smprint("/mail/box/%s/msgs", getuser());
	while((f = Brdstr(&bin, '\n', 1)) != nil){
		mf = strchr(f, ' ');
		if(mf != nil)
			*mf = 0;
		mf = cleanpath(f, top);
		buf = readf(mf);
		if(buf == nil)
			continue;
		amf = mf;
		if(strncmp(mf, top, strlen(top)) == 0)
			mf += strlen(top)+1;
		hdrline(mf, buf, "");
		p = strstr(mf, "/text");
		if(p != nil)
			*p = 0;
		attachline(amf,mf);
	}
	Bterm(&bout);
	Bterm(&bin);
	exits(nil);
}
