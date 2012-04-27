/*
 * push changes from client to server.
 */
#include "all.h"

int douid;
Db *db;
char **x;
int nx;
int justshow;
int verbose;
int conflicts;
char newpath[10000];
char oldpath[10000];
char *clientroot;
char *serverroot;
int copyfile(char*, char*, Dir*, int);
int metafile(char*, Dir*);
char **match;
int nmatch;

int
ismatch(char *s)
{
	int i, len;

	if(nmatch == 0)
		return 1;
	for(i=0; i<nmatch; i++){
		if(strcmp(s, match[i]) == 0)
			return 1;
		len = strlen(match[i]);
		if(strncmp(s, match[i], len) == 0 && s[len]=='/')
			return 1;
	}
	return 0;
}

void
xlog(char c, char *path, Dir *d)
{
	if(!verbose)
		return;
	print("%c %s %luo %s %s %lud\n", c, path, d->mode, d->uid, d->gid, d->mtime);
}

void
walk(char *new, char *old, Dir *pd, void*)
{
	int i, len;
	Dir od, d;
	static Dir *xd;

	new = unroot(new, "/");
	old = unroot(old, serverroot);

	if(!ismatch(new))
		return;
	if(xd != nil){
		free(xd);
		xd = nil;
	}

	for(i=0; i<nx; i++){
		if(strcmp(new, x[i]) == 0)
			return;
		len = strlen(x[i]);
		if(strncmp(new, x[i], len)==0 && new[len]=='/')
			return;
	}

	d = *pd;
	d.name = old;
	memset(&od, 0, sizeof od);
	snprint(newpath, sizeof newpath, "%s/%s", clientroot, new);
	snprint(oldpath, sizeof oldpath, "%s/%s", serverroot, old);

	xd = dirstat(oldpath);
	if(markdb(db, new, &od) < 0){
		if(xd != nil){
			print("x %s create/create conflict\n", new);
			conflicts = 1;
			return;
		}
		xlog('a', new, &d);
		d.muid = "mark";	/* mark bit */
		if(!justshow){
			if(copyfile(newpath, oldpath, &d, 1) == 0)
				insertdb(db, new, &d);
		}
	}else{
		if((d.mode&DMDIR)==0 && od.mtime!=d.mtime){
			if(xd==nil){
				print("%s update/remove conflict\n", new);
				conflicts = 1;
				return;
			}
			if(xd->mtime != od.mtime){
				print("%s update/update conflict\n", new);
				conflicts = 1;
				return;
			}
			od.mtime = d.mtime;
			od.muid = "mark";
			xlog('c', new, &od);
			if(!justshow){
				if(copyfile(newpath, oldpath, &od, 0) == 0)
					insertdb(db, new, &od);
			}
		}
		if((douid&&strcmp(od.uid,d.uid)!=0)
		|| strcmp(od.gid,d.gid)!=0 
		|| od.mode!=d.mode){
			if(xd==nil){
				print("%s metaupdate/remove conflict\n", new);
				conflicts = 1;
				return;
			}
			if((douid&&strcmp(od.uid,xd->uid)!=0)
			|| strcmp(od.uid,xd->gid)!=0
			|| od.mode!=xd->mode){
				print("%s metaupdate/metaupdate conflict\n", new);
				conflicts = 1;
				return;
			}
			if(douid)
				od.uid = d.uid;
			od.gid = d.gid;
			od.mode = d.mode;
			od.muid = "mark";
			xlog('m', new, &od);
			if(!justshow){
				if(metafile(oldpath, &od) == 0)
					insertdb(db, new, &od);
			}
		}
	}
}

void
usage(void)
{
	fprint(2, "usage: replica/applychanges [-nuv] [-p proto] [-x path]... clientdb clientroot serverroot [path ...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *proto;
	Avlwalk *w;
	Dir *xd, d;
	Entry *e;

	quotefmtinstall();
	proto = "/sys/lib/sysconfig/proto/allproto";
	ARGBEGIN{
	case 'n':
		justshow = 1;
		verbose = 1;
		break;
	case 'p':
		proto = EARGF(usage());
		break;
	case 'u':
		douid = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	case 'x':
		if(nx%16 == 0)
			x = erealloc(x, (nx+16)*sizeof(x[0]));
		x[nx++] = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc < 3)
		usage();

	db = opendb(argv[0]);
	clientroot = argv[1];
	serverroot = argv[2];
	match = argv+3;
	nmatch = argc-3;


	if(revrdproto(proto, clientroot, serverroot, walk, nil, nil) < 0)
		sysfatal("rdproto: %r");

	w = avlwalk(db->avl);
	while(e = (Entry*)avlprev(w)){
		if(!ismatch(e->name))
			continue;
		if(!e->d.mark){		/* not visited during walk */
			snprint(newpath, sizeof newpath, "%s/%s", clientroot, e->name);
			snprint(oldpath, sizeof oldpath, "%s/%s", serverroot, e->d.name);
			xd = dirstat(oldpath);
			if(xd == nil){
				removedb(db, e->name);
				continue;
			}
			if(xd->mtime != e->d.mtime && (e->d.mode&xd->mode&DMDIR)==0){
				print("x %q remove/update conflict\n", e->name);
				free(xd);
				continue;
			}
			memset(&d, 0, sizeof d);
			d.name = e->d.name;
			d.uid = e->d.uid;
			d.gid = e->d.gid;
			d.mtime = e->d.mtime;
			d.mode = e->d.mode;
			xlog('d', e->name, &d);
			if(!justshow){
				if(remove(oldpath) == 0)
					removedb(db, e->name);
			}
			free(xd);
		}
	}

	if(conflicts)
		exits("conflicts");
	exits(nil);
}

enum { DEFB = 8192 };

static int
copy1(int fdf, int fdt, char *from, char *to)
{
	char buf[DEFB];
	long n, n1, rcount;
	int rv;
	char err[ERRMAX];

	/* clear any residual error */
	err[0] = '\0';
	errstr(err, ERRMAX);
	rv = 0;
	for(rcount=0;; rcount++) {
		n = read(fdf, buf, DEFB);
		if(n <= 0)
			break;
		n1 = write(fdt, buf, n);
		if(n1 != n) {
			fprint(2, "error writing %q: %r\n", to);
			rv = -1;
			break;
		}
	}
	if(n < 0) {
		fprint(2, "error reading %q: %r\n", from);
		rv = -1;
	}
	return rv;
}

int
copyfile(char *from, char *to, Dir *d, int dowstat)
{
	Dir nd;
	int rfd, wfd, didcreate;
	
	if((rfd = open(from, OREAD)) < 0)
		return -1;

	didcreate = 0;
	if(d->mode&DMDIR){
		if((wfd = create(to, OREAD, DMDIR)) < 0){
			fprint(2, "mkdir %q: %r\n", to);
			close(rfd);
			return -1;
		}
	}else{
		if((wfd = open(to, OTRUNC|OWRITE)) < 0){
			if((wfd = create(to, OWRITE, 0)) < 0){
				close(rfd);
				return -1;
			}
			didcreate = 1;
		}
		if(copy1(rfd, wfd, from, to) < 0){
			close(rfd);
			close(wfd);
			return -1;
		}
	}
	close(rfd);
	if(didcreate || dowstat){
		nulldir(&nd);
		nd.mode = d->mode;
		if(dirfwstat(wfd, &nd) < 0)
			fprint(2, "warning: cannot set mode on %q\n", to);
		nulldir(&nd);
		nd.gid = d->gid;
		if(dirfwstat(wfd, &nd) < 0)
			fprint(2, "warning: cannot set gid on %q\n", to);
		if(douid){
			nulldir(&nd);
			nd.uid = d->uid;
			if(dirfwstat(wfd, &nd) < 0)
				fprint(2, "warning: cannot set uid on %q\n", to);
		}
	}
	nulldir(&nd);
	nd.mtime = d->mtime;
	if(dirfwstat(wfd, &nd) < 0)
		fprint(2, "warning: cannot set mtime on %q\n", to);
	close(wfd);
	return 0;
}

int
metafile(char *path, Dir *d)
{
	Dir nd;

	nulldir(&nd);
	nd.gid = d->gid;
	nd.mode = d->mode;
	if(douid)
		nd.uid = d->uid;
	if(dirwstat(path, &nd) < 0){
		fprint(2, "dirwstat %q: %r\n", path);
		return -1;
	}
	return 0;
}
