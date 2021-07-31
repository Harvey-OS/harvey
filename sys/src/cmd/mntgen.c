#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Name	Name;

enum {
	Qroot = ~0,
};

struct Name {
	Ref;		// one per fid in use.
	char*	s;
};

static	void	aopen(Req*);
static	void	aread(Req*);
static	void	astat(Req*);
static	void	awstat(Req*);
static	char*	awalk1(Fid* fid, char *name, Qid *qid);
static	char*	aclone(Fid*, Fid*);
static	void	aattach(Req*);
static	void	aclunk(Fid*);

Name**	names;
int	nnames;


Srv asrv = {
	.tree	= nil,
	.attach	= aattach,
	.auth	= nil,
	.open	= aopen,
	.create	= nil,
	.read	= aread,
	.write	= nil,
	.remove = nil,
	.flush	= nil,
	.stat	= astat,
	.wstat	= awstat,
	.walk	= nil,
	.walk1	= awalk1,
	.clone	= aclone,
	.destroyfid	= aclunk,
	.destroyreq	= nil,
	.end	= nil,
	.aux	= nil,

	.infd	= -1,
	.outfd	= -1,
	.nopipe	= 0,
	.srvfd	= -1,
};

	
static void
usage(void)
{
	fprint(2, "usage: mntgen [-s srv] [mnt]\n");
	exits("usage");
}


static Name*
newname(char* name)
{
	Name*	n;

	if ((nnames % 100) == 0)
		names = realloc(names, (nnames+100)*sizeof(Name*));
	n = names[nnames++] = malloc(sizeof(Name));
	n->s = strdup(name);
	n->ref = 1;
	return n;
}

static void
closename(int i)
{
	assert (i >= 0 && i < nnames && names[i] != nil);
	if (decref(names[i]) <= 0){
		free(names[i]->s);
		free(names[i]);
		names[i] = nil;	// never reused; qids are unique
	}
}

static int
n2i(int n)
{
	int	i;

	for(i=0; i<nnames; i++)
		if(names[i] != nil && n-- == 0)
			return i;
	return -1;
}

static int
agen(int n, Dir *dir, void* a)
{
	int	i;

	i = n2i(n);
	if (a == nil || i < 0)
		return -1;
	dir->qid.type = QTDIR;
	dir->qid.path = i;
	dir->qid.vers = 0;
	dir->name = estrdup9p(names[i]->s);
	dir->uid = estrdup9p("sys");
	dir->gid = estrdup9p("sys");
	dir->mode= 0555|DMDIR;
	dir->length= 0;
	return 0;
}

static void
aattach(Req* r)
{
	Qid q;

	q.type = QTDIR;
	q.path = Qroot;
	q.vers = 0;
	r->fid->qid = q;
	r->ofcall.qid = q;
	respond(r, nil);
}

static void
aopen(Req* r)
{
	respond(r, nil);
}

static void
aread(Req* r)
{
	Qid	q;

	q = r->fid->qid;
	if (q.path < 0 || q.path >= nnames && q.path != Qroot)
		respond(r, "bug: bad qid");
	if (q.path == Qroot)
		dirread9p(r, agen, names);
	else
		dirread9p(r, agen, nil);
	respond(r, nil);
}

static void
astat(Req* r)
{
	Qid	q;

	q = r->fid->qid;
	if (q.path < 0 || q.path >= nnames && q.path != Qroot)
		respond(r, "bug: bad qid");
	r->d.qid = q;
	if (q.path == Qroot)
		r->d.name = estrdup9p("/");
	else
		r->d.name = estrdup9p(names[q.path]->s);
	r->d.uid = estrdup9p("sys");
	r->d.gid = estrdup9p("sys");
	r->d.length= 0;
	r->d.mode= 0555|DMDIR;
	respond(r, nil);
}

static void
awstat(Req* r)
{
	respond(r, "wstat not allowed");
}

static char*
awalk1(Fid* fid, char *name, Qid *qid)
{
	int	i, oldi;

	oldi = fid->qid.path;
	if (strcmp(name, "..") == 0){
		i = Qroot;
		goto done;
	}
	if (fid->qid.path != Qroot)
		return "no such name";
	for (i = 0; i < nnames; i++)
		if (names[i] != nil && strcmp(name, names[i]->s) == 0){
			incref(names[i]);
			break;
		}
	if (i == nnames)
		newname(name);
done:
	if (oldi >=0 && oldi < nnames)
		closename(oldi);
	qid->path = i;
	qid->type = QTDIR;
	qid->vers = 0;
	fid->qid = *qid;
	return nil;
}

static char*
aclone(Fid* old, Fid*)
{
	int	i;

	i = old->qid.path;
	if (i >= 0 && i < nnames)
		incref(names[i]);
	return nil;
}

static void
aclunk(Fid* fid)
{
	int	i;

	i = fid->qid.path;
	if (i >= 0 && i < nnames)
		closename(i);
}
	
void
main(int argc, char* argv[])
{
	char*	mnt;
	char*	srvname;

	srvname = nil;
	ARGBEGIN{
	case 's':
		srvname = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if (argc > 1)
		usage();
	if (argc == 0)
		mnt = "/n";
	else
		mnt = *argv;
	postmountsrv(&asrv, srvname, mnt, MAFTER);
	exits(nil);	
}
