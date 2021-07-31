#include <alef.h>
#include <bio.h>
#include "httpd.h"

intern	int		eof;
intern	byte*		localsys;			/* ip addr of local system */

intern	byte*		abspath(byte*, byte*);
intern	byte		*csquery(byte*, byte*, byte*);
intern	int		getc(void);
intern	byte*		getip(byte*, byte*);
intern	byte*		getword();
intern	void		senddir(byte*, byte*, int, Dir*);
intern	(byte*, byte*)	stripmagic(byte*);
intern	byte*		stripprefix(byte*, byte*);
intern	(byte*, byte*)	stripsearch(byte*);
intern	byte*		sysdom(void);
intern	byte*		sysname(void);

void
usage()
{
	fprint(2, "usage: httpd [-n namespace]\n");
	exits("usage");
}

void
main(int argc, byte **argv)
{
	Arg *arg;
	int c;

	ALEFcheck = checkfail;

	bout.init(1, OWRITE);
	namespace = nil;
	arg = arginit(argc, argv);
	fmtinstall('D', dateconv);
	fmtinstall('H', httpconv);
	while(c = argopt(arg)){
		switch(c){
		case 'n':
			namespace = argf(arg);
			break;
		default:
			usage();
		}
	}

	if(namespace == nil)
		namespace = "/lib/namespace.http";

	/*
	 * open all files we might need before castrating namespace
	 */
	if(arg->ac){
		localsys = getip(arg->av[arg->ac-1], "local");
		remotesys = getip(arg->av[arg->ac-1], "remote");
	}else{
		localsys = "";
		remotesys = "unknown";
	}
	time();
	mydomain = sysdom();
	syslog(0, HTTPLOG, nil);

	contentinit();
	parsereq();
	exits(nil);
}

void
parsereq()
{
	byte *meth, *v, *magic, *search, *uri, *origuri;

	/*
	 * 15 minutes to get request line
	 */
	alarm(15*60*1000);
	meth = getword();
	if(meth == nil){
		logit("no method");
		fail(Syntax);
	}
	uri = getword();
	if(uri == nil || strlen(uri) == 0){
		logit("no uri: %s", meth);
		fail(Syntax);
	}
	v = getword();
	if(v == nil){
		if(strcmp(meth, "GET") != 0){
			logit("unimplemented method %s", meth);
			fail(Unimp, meth);
		}
		v = "";
	}else{
		if(strcmp(v, "HTTP/V1.0") != 0
		&& strcmp(v, "HTTP/1.0") != 0){
			logit("method %s uri %s", meth, uri);
			fail(UnkVers, v);
		}
		if(getword() != nil)
			fail(Syntax);
		if(strcmp(meth, "GET") != 0 && strcmp(meth, "HEAD") != 0){
			logit("unimplemented method %s", meth);
			fail(Unimp, meth);
		}
	}
	alarm(0);

	/*
	 * the fragment is not supposed to be sent
	 * strip it cause some clients send it
	 */
	origuri = uri;
	uri = strchr(origuri, '#');
	if(uri != nil){
		logit("fragment %s", origuri);
		*uri = 0;
	}

	/*
	 * munge uri for search, protection, and magic
	 */
	(origuri, search) = stripsearch(origuri);
	uri = urlunesc(origuri);
	uri = abspath(uri, "/");
	if(uri == nil || uri[0] == 0)
		fail(NotFound, "no object specified");
	(uri, magic) = stripmagic(uri);

	/*
	 * normal case is just file transfer
	 */
	if(magic == nil || strcmp(magic, "httpd") == 0){
		httpheaders(v);
		send(meth, v, uri, search);
		exits(nil);
	}

	/*
	 * for magic we exec a new program
	 */
	snprint(xferbuf, 3*NAMELEN, "/bin/http/%s", magic);
	execl(xferbuf, magic, "-n", namespace, "-d", mydomain, "-r", remotesys, meth, v, uri, search, nil);
	logit("no magic %s uri %s", magic, uri);
	fail(NotFound, origuri);
}

void
send(byte *name, byte *vers, byte *uri, byte *search)
{
	Content *type, *enc;
	Dir dir;
	byte *w;
	int fd, fd1, n, bad;

	if(search)
		fail(NoSearch, uri);

	/*
	 * whooa baby; castrate the namespace before letting the heathens at it.
	 * figure out the type of file and send headers
	 */
	anonymous(namespace);

	fd = open(uri, OREAD);
	if(fd < 0)
		notfound(0, uri);
	if(dirfstat(fd, &dir) < 0)
		fail(Internal);
	if(dir.mode & CHDIR){
		n = strlen(uri) + strlen("/index.html") + 1;
		w = malloc(n);
		snprint(w, n, "%s/index.html", uri);
		fd1 = open(w, OREAD);
		if(fd1 < 0){
			logit("%s directory %s", name, uri);
			if(modtime >= dir.mtime)
				notmodified();
			senddir(vers, uri, fd, &dir);
		}
		close(fd);
		fd = fd1;
		uri = w;
		if(dirfstat(fd, &dir) < 0)
			fail(Internal);
	}
	logit("%s %s %d", name, uri, dir.length);
	if(modtime >= dir.mtime)
		notmodified();
	if(vers[0] != 0){
		(type, enc) = classify(uri);
		bad = 0;
		if(!checkcontent(type, oktype, "Content-Type", dir.length)){
			bad = 1;
			bout.print("%s 406 None Acceptable\r\n", version);
			logit("no content-type ok");
		}else if(!checkcontent(enc, okencode, "Content-Encoding", 0)){
			bad = 1;
			bout.print("%s 406 None Acceptable\r\n", version);
			logit("no content-encoding ok");
		}else
			bout.print("%s 200 OK\r\n", version);
		bout.print("Server: Plan9\r\n");
		bout.print("Last-Modified: %D\r\n", dir.mtime);
		bout.print("Version: %luxv%lux\r\n", dir.qid.path, dir.qid.vers);
		bout.print("Message-Id: <%luxv%lux@%s>\r\n",
			dir.qid.path, dir.qid.vers, mydomain);
		bout.print("Content-Type: %s/%s\r\n", type->generic, type->specific);
		if(enc != nil){
			bout.print("Content-Encoding: %s", enc->generic);
			bout.print("\r\n");
		}
		bout.print("Content-Length: %d\r\n", dir.length);
		bout.print("Date: %D\r\n", time());
		bout.print("MIME-version: 1.0\r\n");
		bout.print("\r\n");
		if(bad)
			exits(nil);
	}
	if(strcmp(name, "HEAD") == 0){
		exits(nil);
	}

	/*
	 * send the file if it's a normal file
	 */
	bout.flush();
	bout.term();
	while((n = read(fd, xferbuf, BufSize)) > 0)
		write(1, xferbuf, n);
}

intern int
dircomp(void *va, void *vb)
{
	Dir *a, *b;

	a = va;
	b = vb;
	return strcmp(a->name, b->name);
}

/*
 * read in a directory, format it in html, and send it back
 */
intern void
senddir(byte *vers, byte *uri, int fd, Dir *mydir)
{
	Dir *d;
	byte myname[NAMELEN+2], *p;
	int i, n, alloced;

	myname[0] = 0;
	p = strrchr(uri, '/');
	if(p != nil && p[1] != 0)
		snprint(myname, NAMELEN+2, "%s/", p+1);

	alloced = 32;
	d = malloc(alloced*sizeof(Dir));
	n = 0;
	for(;;){
		i = dirread(fd, &d[n], sizeof(Dir)*(alloced-n));
		if(i <= 0)
			break;
		n += i/sizeof(Dir);
		if(n == alloced){
			alloced += 32;
			d = realloc(d, alloced*sizeof(Dir));
		}
	}
	close(fd);

	qsort(d, n, sizeof(Dir), dircomp);
	if(vers[0] != 0){
		okheaders();
		bout.print("Content-Type: text/html\r\n");
		bout.print("Date: %D\r\n", time());
		bout.print("Last-Modified: %D\r\n", mydir->mtime);
		bout.print("Message-Id: <%luxv%lux@%s>\r\n",
			mydir->qid.path, mydir->qid.vers, mydomain);
		bout.print("Version: %lux\r\n", mydir->qid.vers);
		bout.print("\r\n");
	}

	bout.print("<head><title>Contents of directory %H.</title></head>\n",
		uri);
	bout.print("<body><h1>Contents of directory %H.</h1><dl compact>\n",
		uri);
	for(i = 0; i < n; i++){
		bout.print("<dt><a href=\"%H%H\">%H</A><dd>%H\n",
			myname, d[i].name, d[i].name, "some kind of file");
	}
	if(n == 0)
		bout.print("<dd>This directory is empty\n");
	bout.print("</dl></body>\n");
	exits(nil);
}

intern (byte*, byte*)
stripmagic(byte *uri)
{
	byte *newuri, *prog;

	prog = stripprefix("/magic/", uri);
	if(prog == nil)
		return(uri, nil);

	newuri = strchr(prog, '/');
	if(newuri == nil)
		newuri = "";
	else
		*newuri++ = 0;
	return (newuri, prog);
}

intern byte*
stripprefix(byte *pre, byte *str)
{
	while(*pre)
		if(*str++ != *pre++)
			return nil;
	return str;
}

intern (byte*, byte*)
stripsearch(byte *uri)
{
	byte *search;

	search = strchr(uri, '?');
	if(search != nil){
		*search++ = 0;
	}
	return (uri, search);
}

/*
 *  to circumscribe the accessible files we have to eliminate ..'s
 *  and resolve all names from the root.
 */
intern byte*
abspath(byte *origpath, byte *curdir)
{
	byte *p, *sp, *path, *work, *rpath;
	int len, n, c;

	if(curdir == nil)
		curdir = "/";
	if(origpath == nil)
		origpath = "";
	work = strsave(origpath);
	path = work;

	/*
	 * remove any really special characters
	 */
	for(sp = "`;| "; *sp; sp++){
		p = strchr(path, *sp);
		if(p)
			*p = 0;
	}

	len = strlen(curdir) + strlen(path) + 2;
	if(len < 10)
		len = 10;
	rpath = malloc(len);
	if(*path == '/')
		rpath[0] = 0;
	else
		strcpy(rpath, curdir);
	n = strlen(rpath);

	while(path){
		p = strchr(path, '/');
		if(p)
			*p++ = 0;
		if(strcmp(path, "..") == 0){
			while(n > 1){
				n--;
				c = rpath[n];
				rpath[n] = 0;
				if(c == '/')
					break;
			}
		} else if(strcmp(path, ".") == 0)
			;
		else if(n == 1)
			n += snprint(rpath+n, len-n, "%s", path);
		else
			n += snprint(rpath+n, len-n, "/%s", path);
		path = p;
	}

	free(work);
	if(strncmp(rpath, "/bin/", 5) == 0)
		strcpy(rpath, "/");
	return rpath;
}

intern byte*
getword(void)
{
	byte buf[MaxWord];
	int c, n;

	while((c = getc()) == ' ' || c == '\t' || c == '\r')
		;
	buf[0] = 0;
	if(c == '\n')
		return nil;
	n = 0;
	for(;;){
		switch(c){
		case ' ': case '\t':
		case '\r':
			buf[n] = 0;
			return strsave(buf);
		case '\n':
			buf[n] = 0;
			return strsave(buf);
		}

		if(n < MaxWord-1)
			buf[n++] = c;
		c = getc();
	}
	return nil;
}

intern int
getc(void)
{
	byte buf[1];

	if(eof)
		return '\n';
	if(read(0, buf, 1) != 1){
		eof = 1;
		return '\n';
	}
	buf[0] &= 0x7f;
	if(buf[0] == '\n')
		eof = 1;
	return buf[0];
}

/*
 * couldn't open a file
 * figure out why and return and error message
 */
void
notfound(int new, byte *url)
{
	byte buf[ERRLEN];

	buf[0] = 0;
	errstr(buf);
	if(strstr(buf, "file does not exist") != nil)
		fail(NotFound, url);
	if(strstr(buf, "permission denied") != nil)
		fail(Unauth, url);
	if(new);
	fail(NotFound, url);
}

intern byte*
getip(byte *dir, byte *remloc)
{
	int fd, n;
	byte file[2*NAMELEN], *sys, *s;

	sys = malloc(2*NAMELEN);
	snprint(file, 2*NAMELEN-1, "%s/%s", dir, remloc);
	fd = open(file, OREAD);
	if(fd < 0)
		strcpy(sys, "unknown");
	n = read(fd, sys, 2*NAMELEN-1);
	if(n > 0)
		sys[n-1] = 0;
	else
		strcpy(sys, "unknown");
	s = strchr(sys, '!');
	if(s != nil)
		*s = 0;
	close(fd);
	return sys;
}

intern byte *
sysname()
{
	int n, fd;

	if(mysysname != nil)
		return mysysname;

	mysysname = malloc(NAMELEN);
	mysysname[0] = 0;
	fd = open("#c/sysname", OREAD);
	if(fd < 0)
		return nil;
	n = read(fd, mysysname, NAMELEN-1);
	close(fd);
	if(n <= 0)
		return nil;
	mysysname[n] = 0;
	return mysysname;
}

intern byte *
sysdom()
{
	byte *dn;

	dn = csquery("sys" , sysname(), "dom");
	if(dn == nil){
		dn = localsys;
		if(localsys[0] == 0)
			dn = "ip address";
	}
	return dn;
}

/*
 *  query the connection server
 */
intern byte*
csquery(byte *attr, byte *val, byte *rattr)
{
	byte token[64+4];
	byte buf[256], *p, *sp;
	int fd, n;

	if(val == nil || val[0] == 0)
		return nil;
	fd = open("/net/cs", ORDWR);
	if(fd < 0)
		return nil;
	fprint(fd, "!%s=%s", attr, val);
	seek(fd, 0, 0);
	snprint(token, sizeof(token), "%s=", rattr);
	for(;;){
		n = read(fd, buf, sizeof(buf)-1);
		if(n <= 0)
			break;
		buf[n] = 0;
		p = strstr(buf, token);
		if(p != nil && (p == buf || *(p-1) == 0)){
			close(fd);
			sp = strchr(p, ' ');
			if(sp)
				*sp = 0;
			p = strchr(p, '=');
			if(p == nil)
				return nil;
			return strsave(p+1);
		}
	}
	close(fd);
	return nil;
}
