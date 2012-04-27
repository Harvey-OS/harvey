#include "all.h"

static Entry *fe;

static Entry*
allocentry(void)
{
	int i;
	Entry *e;

	if(fe == nil){
		fe = emalloc(128*sizeof(Entry));
		for(i=0; i<128-1; i++)
			fe[i].name = (char*)&fe[i+1];
		fe[i].name = nil;
	}

	e = fe;
	fe = (Entry*)e->name;
	memset(e, 0, sizeof *e);
	return e;
}

static void
freeentry(Entry *e)
{
	e->name = (char*)fe;
	fe = e;
}

static void
_removedb(Db *db, char *name)
{
	Entry *e, k;

	memset(&k, 0, sizeof k);
	k.name = name;
	e = nil;
	deleteavl(db->avl, (Avl*)&k, (Avl**)&e);
	if(e)
		freeentry(e);
}

static void
_insertdb(Db *db, Entry *e)
{
	Entry *o, *ne;

	ne = allocentry();
	*ne = *e;
	o = nil;
	insertavl(db->avl, (Avl*)ne, (Avl**)&o);
	if(o)
		freeentry(o);
}

static int
entrycmp(Avl *a, Avl *b)
{
	Entry *ea, *eb;

	ea = (Entry*)a;
	eb = (Entry*)b;
	return strcmp(ea->name, eb->name);
}

Db*
opendb(char *file)
{
	char *f[10], *s, *t;
	int i, fd, nf;
	Biobuf b;
	Db *db;
	Entry e;

	if(file == nil)
		fd = -1;
	else if((fd = open(file, ORDWR)) < 0)
		sysfatal("opendb %s: %r", file);
	db = emalloc(sizeof(Db));
	db->avl = mkavltree(entrycmp);
	db->fd = fd;
	if(fd < 0)
		return db;
	Binit(&b, fd, OREAD);
	i = 0;
	for(; s=Brdstr(&b, '\n', 1); free(s)){
		t = estrdup(s);
		nf = tokenize(s, f, nelem(f));
		if(nf != 7)
			sysfatal("bad database entry '%s'", t);
		free(t);
		if(strcmp(f[2], "REMOVED") == 0)
			_removedb(db, f[0]);
		else{
			memset(&e, 0, sizeof e);
			e.name = atom(f[0]);
			e.d.name = atom(f[1]);
			if(strcmp(e.d.name, "-")==0)
				e.d.name = e.name;
			e.d.mode = strtoul(f[2], 0, 8);
			e.d.uid = atom(f[3]);
			e.d.gid = atom(f[4]);
			e.d.mtime = strtoul(f[5], 0, 10);
			e.d.length = strtoll(f[6], 0, 10);
			_insertdb(db, &e);
			i++;
		}
	}
	return db;
}

static int
_finddb(Db *db, char *name, Dir *d, int domark)
{
	Entry *e, k;

	memset(&k, 0, sizeof k);
	k.name = name;

	e = (Entry*)lookupavl(db->avl, (Avl*)&k);
	if(e == nil)
		return -1;
	memset(d, 0, sizeof *d);
	d->name = e->d.name;
	d->uid = e->d.uid;
	d->gid = e->d.gid;
	d->mtime = e->d.mtime;
	d->mode = e->d.mode;
	d->length = e->d.length;
	if(domark)
		e->d.mark = 1;
	return 0;
}

int
finddb(Db *db, char *name, Dir *d)
{
	return _finddb(db, name, d, 0);
}

int
markdb(Db *db, char *name, Dir *d)
{
	return _finddb(db, name, d, 1);
}

void
removedb(Db *db, char *name)
{
	if(db->fd>=0 && fprint(db->fd, "%q xxx REMOVED xxx xxx 0 0\n", name) < 0)
		sysfatal("appending to db: %r");
	_removedb(db, name);
}

void
insertdb(Db *db, char *name, Dir *d)
{
	char *dname;
	Entry e;

	memset(&e, 0, sizeof e);
	e.name = atom(name);
	e.d.name = atom(d->name);
	e.d.uid = atom(d->uid);
	e.d.gid = atom(d->gid);
	e.d.mtime = d->mtime;
	e.d.mode = d->mode;
	e.d.length = d->length;
	e.d.mark = d->muid!=0;

	dname = d->name;
	if(strcmp(name, dname) == 0)
		dname = "-";
	if(db->fd>=0 && fprint(db->fd, "%q %q %luo %q %q %lud %lld\n", name, dname, d->mode, d->uid, d->gid, d->mtime, d->length) < 0)
		sysfatal("appending to db: %r");
	_insertdb(db, &e);
}

