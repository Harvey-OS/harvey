/* Copyright © 2003 Russ Cox, MIT; see /sys/src/libsunrpc/COPYING */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <sunrpc.h>
#include <nfs3.h>

SunClient *nfscli;
SunClient *mntcli;
char *defaultpath = "/";
Channel *fschan;
char *sys;
int verbose;
int readplus = 0;


typedef struct Auth Auth;
struct Auth
{
	int ref;
	uchar *data;
	int ndata;
};

typedef struct FidAux FidAux;
struct FidAux
{
	Nfs3Handle handle;

	u64int cookie;	/* for continuing directory reads */
	char *name;	/* botch: for remove and rename */
	Nfs3Handle parent;	/* botch: for remove and rename */
	char err[ERRMAX];	/* for walk1 */
	Auth *auth;
};

/*
 * various RPCs.  here is where we'd insert support for NFS v2
 */

void
portCall(SunCall *c, PortCallType type)
{
	c->rpc.prog = PortProgram;
	c->rpc.vers = PortVersion;
	c->rpc.proc = type>>1;
	c->rpc.iscall = !(type&1);
	c->type = type;
}

int
getport(SunClient *client, uint prog, uint vers, uint prot, uint *port)
{
	PortTGetport tx;
	PortRGetport rx;

	memset(&tx, 0, sizeof tx);
	portCall(&tx.call, PortCallTGetport);
	tx.map.prog = prog;
	tx.map.vers = vers;
	tx.map.prot = prot;

	memset(&rx, 0, sizeof rx);
	portCall(&rx.call, PortCallRGetport);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		return -1;
	*port = rx.port;
	return 0;
}

void
mountCall(Auth *a, SunCall *c, NfsMount3CallType type)
{
	c->rpc.iscall = !(type&1);
	c->rpc.proc = type>>1;
	c->rpc.prog = NfsMount3Program;
	c->rpc.vers = NfsMount3Version;
	if(c->rpc.iscall && a){
		c->rpc.cred.flavor = SunAuthSys;
		c->rpc.cred.data = a->data;
		c->rpc.cred.ndata = a->ndata;
	}
	c->type = type;
}

int
mountNull(ulong tag)
{
	NfsMount3TNull tx;
	NfsMount3RNull rx;

	memset(&tx, 0, sizeof tx);
	mountCall(nil, &tx.call, NfsMount3CallTNull);

	memset(&rx, 0, sizeof rx);
	mountCall(nil, &rx.call, NfsMount3CallTNull);

	return sunClientRpc(mntcli, tag, &tx.call, &rx.call, nil);
}

int
mountMnt(Auth *a, ulong tag, char *path, Nfs3Handle *h)
{
	uchar *freeme;
	NfsMount3TMnt tx;
	NfsMount3RMnt rx;

	memset(&tx, 0, sizeof tx);
	mountCall(a, &tx.call, NfsMount3CallTMnt);
	tx.path = path;

	memset(&rx, 0, sizeof rx);
	mountCall(a, &rx.call, NfsMount3CallRMnt);
	if(sunClientRpc(mntcli, tag, &tx.call, &rx.call, &freeme) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
if(verbose)print("handle %.*H\n", rx.len, rx.handle);
	if(rx.len >= Nfs3MaxHandleSize){
		free(freeme);
		werrstr("server-returned handle too long");
		return -1;
	}
	memmove(h->h, rx.handle, rx.len);
	h->len = rx.len;
	free(freeme);
	return 0;
}

void
nfs3Call(Auth *a, SunCall *c, Nfs3CallType type)
{
	c->rpc.iscall = !(type&1);
	c->rpc.proc = type>>1;
	c->rpc.prog = Nfs3Program;
	c->rpc.vers = Nfs3Version;
	if(c->rpc.iscall && a){
		c->rpc.cred.flavor = SunAuthSys;
		c->rpc.cred.data = a->data;
		c->rpc.cred.ndata = a->ndata;
	}
	c->type = type;
}

int
nfsNull(ulong tag)
{
	Nfs3TNull tx;
	Nfs3RNull rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(nil, &tx.call, Nfs3CallTNull);

	memset(&rx, 0, sizeof rx);
	nfs3Call(nil, &rx.call, Nfs3CallTNull);

	return sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil);
}

int
nfsGetattr(Auth *a, ulong tag, Nfs3Handle *h, Nfs3Attr *attr)
{
	Nfs3TGetattr tx;
	Nfs3RGetattr rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTGetattr);
	tx.handle = *h;	

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRGetattr);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}

	*attr = rx.attr;
	return 0;
}

int
nfsAccess(Auth *a, ulong tag, Nfs3Handle *h, ulong want, ulong *got, u1int *have, Nfs3Attr *attr)
{
	Nfs3TAccess tx;
	Nfs3RAccess rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTAccess);
	tx.handle = *h;
	tx.access = want;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRAccess);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}

	*got = rx.access;

	*have = rx.haveAttr;
	if(rx.haveAttr)
		*attr = rx.attr;
	return 0;
}

int
nfsMkdir(Auth *a, ulong tag, Nfs3Handle *h, char *name, Nfs3Handle *nh, ulong mode, uint gid,
	u1int *have, Nfs3Attr *attr)
{
	Nfs3TMkdir tx;
	Nfs3RMkdir rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTMkdir);
	tx.handle = *h;
	tx.name = name;
	tx.attr.setMode = 1;
	tx.attr.mode = mode;
	tx.attr.setGid = 1;
	tx.attr.gid = gid;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRMkdir);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}

	if(!rx.haveHandle){
		werrstr("nfs mkdir did not return handle");
		return -1;
	}
	*nh = rx.handle;

	*have = rx.haveAttr;
	if(rx.haveAttr)
		*attr = rx.attr;
	return 0;
}

int
nfsCreate(Auth *a, ulong tag, Nfs3Handle *h, char *name, Nfs3Handle *nh, ulong mode, uint gid,
	u1int *have, Nfs3Attr *attr)
{
	Nfs3TCreate tx;
	Nfs3RCreate rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTCreate);
	tx.handle = *h;
	tx.name = name;
	tx.attr.setMode = 1;
	tx.attr.mode = mode;
	tx.attr.setGid = 1;
	tx.attr.gid = gid;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRCreate);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}

	if(!rx.haveHandle){
		werrstr("nfs create did not return handle");
		return -1;
	}
	*nh = rx.handle;

	*have = rx.haveAttr;
	if(rx.haveAttr)
		*attr = rx.attr;
	return 0;
}

int
nfsRead(Auth *a, ulong tag, Nfs3Handle *h, u32int count, u64int offset,
	uchar **pp, u32int *pcount, uchar **pfreeme)
{
	uchar *freeme;
	Nfs3TRead tx;
	Nfs3RRead rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTRead);
	tx.handle = *h;
	tx.count = count;
	tx.offset = offset;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRRead);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, &freeme) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
	if(rx.count != rx.ndata){
		werrstr("nfs read returned count=%ud ndata=%ud", (uint)rx.count, (uint)rx.ndata);
		free(freeme);
		return -1;
	}
	*pfreeme = freeme;
	*pcount = rx.count;
	*pp = rx.data;
	return 0;
}

int
nfsWrite(Auth *a, ulong tag, Nfs3Handle *h, uchar *data, u32int count, u64int offset, u32int *pcount)
{
	Nfs3TWrite tx;
	Nfs3RWrite rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTWrite);
	tx.handle = *h;
	tx.count = count;
	tx.offset = offset;
	tx.data = data;
	tx.ndata = count;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRWrite);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}

	*pcount = rx.count;
	return 0;
}

int
nfsRmdir(Auth *a, ulong tag, Nfs3Handle *h, char *name)
{
	Nfs3TRmdir tx;
	Nfs3RRmdir rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTRmdir);
	tx.handle = *h;
	tx.name = name;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRRmdir);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
	return 0;
}

int
nfsRemove(Auth *a, ulong tag, Nfs3Handle *h, char *name)
{
	Nfs3TRemove tx;
	Nfs3RRemove rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTRemove);
	tx.handle = *h;
	tx.name = name;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRRemove);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
	return 0;
}

int
nfsRename(Auth *a, ulong tag, Nfs3Handle *h, char *name, Nfs3Handle *th, char *tname)
{
	Nfs3TRename tx;
	Nfs3RRename rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTRename);
	tx.from.handle = *h;
	tx.from.name = name;
	tx.to.handle = *th;
	tx.to.name = tname;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRRename);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
	return 0;
}

int
nfsSetattr(Auth *a, ulong tag, Nfs3Handle *h, Nfs3SetAttr *attr)
{
	Nfs3TSetattr tx;
	Nfs3RSetattr rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTSetattr);
	tx.handle = *h;
	tx.attr = *attr;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRSetattr);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
	return 0;
}

int
nfsCommit(Auth *a, ulong tag, Nfs3Handle *h)
{
	Nfs3TCommit tx;
	Nfs3RCommit rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTCommit);
	tx.handle = *h;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRCommit);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;

	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
	return 0;
}

int
nfsLookup(Auth *a, ulong tag, Nfs3Handle *h, char *name, Nfs3Handle *nh, u1int *have, Nfs3Attr *attr)
{
	Nfs3TLookup tx;
	Nfs3RLookup rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTLookup);
	tx.handle = *h;
	tx.name = name;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRLookup);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, nil) < 0)
		return -1;

	if(rx.status != Nfs3Ok){
		nfs3Errstr(rx.status);
		return -1;
	}
	*nh = rx.handle;
	*have = rx.haveAttr;
	if(rx.haveAttr)
		*attr = rx.attr;
	return 0;
}

int
nfsReadDirPlus(Auth *a, ulong tag, Nfs3Handle *h, u32int count, u64int cookie, uchar **pp,
	u32int *pcount, int (**unpack)(uchar*, uchar*, uchar**, Nfs3Entry*), uchar **pfreeme)
{
	Nfs3TReadDirPlus tx;
	Nfs3RReadDirPlus rx;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTReadDirPlus);
	tx.handle = *h;
	tx.maxCount = count;
	tx.dirCount = 1000;
	tx.cookie = cookie;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRReadDirPlus);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, pfreeme) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		free(*pfreeme);
		*pfreeme = 0;
		nfs3Errstr(rx.status);
		return -1;
	}

	*unpack = nfs3EntryPlusUnpack;
	*pcount = rx.count;
	*pp = rx.data;
	return 0;
}

int
nfsReadDir(Auth *a, ulong tag, Nfs3Handle *h, u32int count, u64int cookie, uchar **pp,
	u32int *pcount, int (**unpack)(uchar*, uchar*, uchar**, Nfs3Entry*), uchar **pfreeme)
{
	/* BUG: try readdirplus */
	char e[ERRMAX];
	Nfs3TReadDir tx;
	Nfs3RReadDir rx;

	if(readplus!=-1){
		if(nfsReadDirPlus(a, tag, h, count, cookie, pp, pcount, unpack, pfreeme) == 0){
			readplus = 1;
			return 0;
		}
		if(readplus == 0){
			rerrstr(e, sizeof e);
			if(strstr(e, "procedure unavailable") || strstr(e, "not supported"))
				readplus = -1;
		}
		if(readplus == 0)
			fprint(2, "readdirplus: %r\n");
	}
	if(readplus == 1)
		return -1;

	memset(&tx, 0, sizeof tx);
	nfs3Call(a, &tx.call, Nfs3CallTReadDir);
	tx.handle = *h;
	tx.count = count;
	tx.cookie = cookie;

	memset(&rx, 0, sizeof rx);
	nfs3Call(a, &rx.call, Nfs3CallRReadDir);

	if(sunClientRpc(nfscli, tag, &tx.call, &rx.call, pfreeme) < 0)
		return -1;
	if(rx.status != Nfs3Ok){
		free(*pfreeme);
		*pfreeme = 0;
		nfs3Errstr(rx.status);
		return -1;
	}

	/* readplus failed but read succeeded */
	readplus = -1;

	*unpack = nfs3EntryUnpack;
	*pcount = rx.count;
	*pp = rx.data;
	return 0;
}

/*
 * name <-> int translation
 */
typedef struct Map Map;
typedef struct User User;
typedef struct Group Group;

Map *map;
Map emptymap;

struct User
{
	char *name;
	uint uid;
	uint gid;
	uint g[16];
	uint ng;
	uchar *auth;
	int nauth;
};

struct Group
{
	char *name;	/* same pos as in User struct */
	uint gid;	/* same pos as in User struct */
};

struct Map
{
	int nuser;
	int ngroup;
	User *user;
	User **ubyname;
	User **ubyid;
	Group *group;
	Group **gbyname;
	Group **gbyid;
};

User*
finduser(User **u, int nu, char *s)
{
	int lo, hi, mid, n;

	hi = nu;
	lo = 0;
	while(hi > lo){
		mid = (lo+hi)/2;
		n = strcmp(u[mid]->name, s);
		if(n == 0)
			return u[mid];
		if(n < 0)
			lo = mid+1;
		else
			hi = mid;
	}
	return nil;
}

int
strtoid(User **u, int nu, char *s, u32int *id)
{
	u32int x;
	char *p;
	User *uu;

	x = strtoul(s, &p, 10);
	if(*s != 0 && *p == 0){
		*id = x;
		return 0;
	}

	uu = finduser(u, nu, s);
	if(uu == nil)
		return -1;
	*id = uu->uid;
	return 0;
}

char*
idtostr(User **u, int nu, u32int id)
{
	char buf[32];
	int lo, hi, mid;

	hi = nu;
	lo = 0;
	while(hi > lo){
		mid = (lo+hi)/2;
		if(u[mid]->uid == id)
			return estrdup9p(u[mid]->name);
		if(u[mid]->uid < id)
			lo = mid+1;
		else
			hi = mid;
	}
	snprint(buf, sizeof buf, "%ud", id);	
	return estrdup9p(buf);
}		
char*
uidtostr(u32int uid)
{
	return idtostr(map->ubyid, map->nuser, uid);
}

char*
gidtostr(u32int gid)
{
	return idtostr((User**)map->gbyid, map->ngroup, gid);
}

int
strtouid(char *s, u32int *id)
{
	return strtoid(map->ubyname, map->nuser, s, id);
}

int
strtogid(char *s, u32int *id)
{
	return strtoid((User**)map->gbyid, map->ngroup, s, id);
}


int
idcmp(const void *va, const void *vb)
{
	User **a, **b;

	a = (User**)va;
	b = (User**)vb;
	return (*a)->uid - (*b)->uid;
}

int
namecmp(const void *va, const void *vb)
{
	User **a, **b;

	a = (User**)va;
	b = (User**)vb;
	return strcmp((*a)->name, (*b)->name);
}

void
closemap(Map *m)
{
	int i;

	for(i=0; i<m->nuser; i++){
		free(m->user[i].name);
		free(m->user[i].auth);
	}
	for(i=0; i<m->ngroup; i++)
		free(m->group[i].name);
	free(m->user);
	free(m->group);
	free(m->ubyid);
	free(m->ubyname);
	free(m->gbyid);
	free(m->gbyname);
	free(m);
}

Map*
readmap(char *passwd, char *group)
{
	char *s, *f[10], *p, *nextp, *name;
	uchar *q, *eq;
	int i, n, nf, line, uid, gid;
	Biobuf *b;
	Map *m;
	User *u;
	Group *g;
	SunAuthUnix au;

	m = emalloc(sizeof(Map));

	if((b = Bopen(passwd, OREAD)) == nil){
		free(m);
		return nil;
	}
	line = 0;
	for(; (s = Brdstr(b, '\n', 1)) != nil; free(s)){
		line++;
		if(s[0] == '#')
			continue;
		nf = getfields(s, f, nelem(f), 0, ":");
		if(nf < 4)
			continue;
		name = f[0];
		uid = strtol(f[2], &p, 10);
		if(f[2][0] == 0 || *p != 0){
			fprint(2, "%s:%d: non-numeric id in third field\n", passwd, line);
			continue;
		}
		gid = strtol(f[3], &p, 10);
		if(f[3][0] == 0 || *p != 0){
			fprint(2, "%s:%d: non-numeric id in fourth field\n", passwd, line);
			continue;
		}
		if(m->nuser%32 == 0)
			m->user = erealloc(m->user, (m->nuser+32)*sizeof(m->user[0]));
		u = &m->user[m->nuser++];
		u->name = estrdup9p(name);
		u->uid = uid;
		u->gid = gid;
		u->ng = 0;
		u->auth = 0;
		u->nauth = 0;
	}
	Bterm(b);
	m->ubyname = emalloc(m->nuser*sizeof(User*));
	m->ubyid = emalloc(m->nuser*sizeof(User*));
	for(i=0; i<m->nuser; i++){
		m->ubyname[i] = &m->user[i];
		m->ubyid[i] = &m->user[i];
	}
	qsort(m->ubyname, m->nuser, sizeof(m->ubyname[0]), namecmp);
	qsort(m->ubyid, m->nuser, sizeof(m->ubyid[0]), idcmp);

	if((b = Bopen(group, OREAD)) == nil){
		closemap(m);
		return nil;
	}
	line = 0;
	for(; (s = Brdstr(b, '\n', 1)) != nil; free(s)){
		line++;
		if(s[0] == '#')
			continue;
		nf = getfields(s, f, nelem(f), 0, ":");
		if(nf < 4)
			continue;
		name = f[0];
		gid = strtol(f[2], &p, 10);
		if(f[2][0] == 0 || *p != 0){
			fprint(2, "%s:%d: non-numeric id in third field\n", group, line);
			continue;
		}
		if(m->ngroup%32 == 0)
			m->group = erealloc(m->group, (m->ngroup+32)*sizeof(m->group[0]));
		g = &m->group[m->ngroup++];
		g->name = estrdup9p(name);
		g->gid = gid;

		for(p=f[3]; *p; p=nextp){
			if((nextp = strchr(p, ',')) != nil)
				*nextp++ = 0;
			else
				nextp = p+strlen(p);
			u = finduser(m->ubyname, m->nuser, p);
			if(u == nil){
				if(verbose)
					fprint(2, "%s:%d: unknown user %s\n", group, line, p);
				continue;
			}
			if(u->ng >= nelem(u->g)){
				fprint(2, "%s:%d: user %s is in too many groups; ignoring %s\n", group, line, p, name);
				continue;
			}
			u->g[u->ng++] = gid;
		}
	}
	Bterm(b);
	m->gbyname = emalloc(m->ngroup*sizeof(Group*));
	m->gbyid = emalloc(m->ngroup*sizeof(Group*));
	for(i=0; i<m->ngroup; i++){
		m->gbyname[i] = &m->group[i];
		m->gbyid[i] = &m->group[i];
	}
	qsort(m->gbyname, m->ngroup, sizeof(m->gbyname[0]), namecmp);
	qsort(m->gbyid, m->ngroup, sizeof(m->gbyid[0]), idcmp);

	for(i=0; i<m->nuser; i++){
		au.stamp = 0;
		au.sysname = sys;
		au.uid = m->user[i].uid;
		au.gid = m->user[i].gid;
		memmove(au.g, m->user[i].g, sizeof au.g);
		au.ng = m->user[i].ng;
		n = sunAuthUnixSize(&au);
		q = emalloc(n);
		eq = q+n;
		m->user[i].auth = q;
		m->user[i].nauth = n;
		if(sunAuthUnixPack(q, eq, &q, &au) < 0 || q != eq){
			fprint(2, "sunAuthUnixPack failed for %s\n", m->user[i].name);
			free(m->user[i].auth);
			m->user[i].auth = 0;
			m->user[i].nauth = 0;
		}
	}

	return m;
}

Auth*
mkauth(char *user)
{
	Auth *a;
	uchar *p;
	int n;
	SunAuthUnix au;
	User *u;

	u = finduser(map->ubyname, map->nuser, user);
	if(u == nil || u->nauth == 0){
		/* nobody */
		au.stamp = 0;
		au.uid = -1;
		au.gid = -1;
		au.ng = 0;
		au.sysname = sys;
		n = sunAuthUnixSize(&au);
		a = emalloc(sizeof(Auth)+n);
		a->data = (uchar*)&a[1];
		a->ndata = n;
		if(sunAuthUnixPack(a->data, a->data+a->ndata, &p, &au) < 0
		|| p != a->data+a->ndata){
			free(a);
			return nil;
		}
		a->ref = 1;
if(verbose)print("creds for %s: %.*H\n", user, a->ndata, a->data);
		return a;
	}

	a = emalloc(sizeof(Auth)+u->nauth);
	a->data = (uchar*)&a[1];
	a->ndata = u->nauth;
	memmove(a->data, u->auth, a->ndata);
	a->ref = 1;
if(verbose)print("creds for %s: %.*H\n", user, a->ndata, a->data);
	return a;
}

void
freeauth(Auth *a)
{
	if(--a->ref > 0)
		return;
	free(a);
}

/*
 * 9P server
 */
void
responderrstr(Req *r)
{
	char e[ERRMAX];

	rerrstr(e, sizeof e);
	respond(r, e);
}

void
fsdestroyfid(Fid *fid)
{
	FidAux *aux;

	aux = fid->aux;
	if(aux == nil)
		return;
	freeauth(aux->auth);
	free(aux->name);
	free(aux);
}

void
attrToQid(Nfs3Attr *attr, Qid *qid)
{
	qid->path = attr->fileid;
	qid->vers = attr->mtime.sec;
	qid->type = 0;
	if(attr->type == Nfs3FileDir)
		qid->type |= QTDIR;
}

void
attrToDir(Nfs3Attr *attr, Dir *d)
{
	d->mode = attr->mode & 0777;
	if(attr->type == Nfs3FileDir)
		d->mode |= DMDIR;
	d->uid = uidtostr(attr->uid);
	d->gid = gidtostr(attr->gid);
	d->length = attr->size;
	attrToQid(attr, &d->qid);
	d->mtime = attr->mtime.sec;
	d->atime = attr->atime.sec;
	d->muid = nil;
}

void
fsattach(Req *r)
{
	char *path;
	Auth *auth;
	FidAux *aux;
	Nfs3Attr attr;
	Nfs3Handle h;

	path = r->ifcall.aname;
	if(path==nil || path[0]==0)
		path = defaultpath;

	auth = mkauth(r->ifcall.uname);

	if(mountMnt(auth, r->tag, path, &h) < 0
	|| nfsGetattr(auth, r->tag, &h, &attr) < 0){
		freeauth(auth);
		responderrstr(r);
		return;
	}

	aux = emalloc(sizeof(FidAux));
	aux->auth = auth;
	aux->handle = h;
	aux->cookie = 0;
	aux->name = nil;
	memset(&aux->parent, 0, sizeof aux->parent);
	r->fid->aux = aux;
	attrToQid(&attr, &r->fid->qid);
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

void
fsopen(Req *r)
{
	FidAux *aux;
	Nfs3Attr attr;
	Nfs3SetAttr sa;
	u1int have;
	ulong a, b;

	aux = r->fid->aux;
	a = 0;
	switch(r->ifcall.mode&OMASK){
	case OREAD:
		a = 0x0001;
		break;
	case OWRITE:
		a = 0x0004;
		break;
	case ORDWR:
		a = 0x0001|0x0004;
		break;
	case OEXEC:
		a = 0x20;
		break;
	}
	if(r->ifcall.mode&OTRUNC)
		a |= 0x0004;

	if(nfsAccess(aux->auth, r->tag, &aux->handle, a, &b, &have, &attr) < 0
	|| (!have && nfsGetattr(aux->auth, r->tag, &aux->handle, &attr) < 0)){
    Error:
		responderrstr(r);
		return;
	}
	if(a != b){
		respond(r, "permission denied");
		return;
	}
	if(r->ifcall.mode&OTRUNC){
		memset(&sa, 0, sizeof sa);
		sa.setSize = 1;
		if(nfsSetattr(aux->auth, r->tag, &aux->handle, &sa) < 0)
			goto Error;
	}
	attrToQid(&attr, &r->fid->qid);
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

void
fscreate(Req *r)
{
	FidAux *aux;
	u1int have;
	Nfs3Attr attr;
	Nfs3Handle h;
	ulong mode;
	uint gid;
	int (*mk)(Auth*, ulong, Nfs3Handle*, char*, Nfs3Handle*, ulong, uint, u1int*, Nfs3Attr*);

	aux = r->fid->aux;

	/*
	 * Plan 9 has no umask, so let's use the
	 * parent directory bits like Plan 9 does.
	 * What the heck, let's inherit the group too.
	 * (Unix will let us set the group to anything
	 * since we're the owner!)
	 */
	if(nfsGetattr(aux->auth, r->tag, &aux->handle, &attr) < 0){
		responderrstr(r);
		return;
	}
	mode = r->ifcall.perm&0777;
	if(r->ifcall.perm&DMDIR)
		mode &= (attr.mode&0666) | ~0666;
	else
		mode &= (attr.mode&0777) | ~0777;
	gid = attr.gid;

	if(r->ifcall.perm&DMDIR)
		mk = nfsMkdir;
	else
		mk = nfsCreate;

	if((*mk)(aux->auth, r->tag, &aux->handle, r->ifcall.name, &h, mode, gid, &have, &attr) < 0
	|| (!have && nfsGetattr(aux->auth, r->tag, &h, &attr) < 0)){
		responderrstr(r);
		return;
	}
	attrToQid(&attr, &r->fid->qid);
	aux->parent = aux->handle;
	aux->handle = h;
	free(aux->name);
	aux->name = estrdup9p(r->ifcall.name);
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

void
fsreaddir(Req *r)
{
	FidAux *aux;
	uchar *p, *freeme, *ep, *p9, *ep9;
	char *s;
	uint count;
	int n, (*unpack)(uchar*, uchar*, uchar**, Nfs3Entry*);
	Nfs3Entry e;
	u64int cookie;
	Dir d;

	aux = r->fid->aux;
	/*
	 * r->ifcall.count seems a reasonable estimate to
	 * how much NFS entry data we want.  is it?
	 */
	if(r->ifcall.offset)
		cookie = aux->cookie;
	else
		cookie = 0;
	if(nfsReadDir(aux->auth, r->tag, &aux->handle, r->ifcall.count, cookie,
		&p, &count, &unpack, &freeme) < 0){
		responderrstr(r);
		return;
	}
	ep = p+count;

	p9 = (uchar*)r->ofcall.data;
	ep9 = p9+r->ifcall.count;

	/*
	 * BUG: Issue all of the stat requests in parallel.
	 */
	while(p < ep && p9 < ep9){
		if((*unpack)(p, ep, &p, &e) < 0)
			break;
		aux->cookie = e.cookie;
		if(strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0)
			continue;
		for(s=e.name; (uchar)*s >= ' '; s++)
			;
		if(*s != 0)	/* bad character in name */
			continue;
		if(!e.haveAttr && !e.haveHandle)
			if(nfsLookup(aux->auth, r->tag, &aux->handle, e.name, &e.handle, &e.haveAttr, &e.attr) < 0)
				continue;
		if(!e.haveAttr)
			if(nfsGetattr(aux->auth, r->tag, &e.handle, &e.attr) < 0)
				continue;
		memset(&d, 0, sizeof d);
		attrToDir(&e.attr, &d);
		d.name = e.name;
		if((n = convD2M(&d, p9, ep9-p9)) <= BIT16SZ)
			break;
		p9 += n;
	}
	free(freeme);
	r->ofcall.count = p9 - (uchar*)r->ofcall.data;
	respond(r, nil);
}
	
void
fsread(Req *r)
{
	uchar *p, *freeme;
	uint count;
	FidAux *aux;

	if(r->fid->qid.type&QTDIR){
		fsreaddir(r);
		return;
	}

	aux = r->fid->aux;
	if(nfsRead(aux->auth, r->tag, &aux->handle, r->ifcall.count, r->ifcall.offset, &p, &count, &freeme) < 0){
		responderrstr(r);
		return;
	}
	r->ofcall.data = (char*)p;
	r->ofcall.count = count;
	respond(r, nil);
	free(freeme);
}

void
fswrite(Req *r)
{
	uint count;
	FidAux *aux;

	aux = r->fid->aux;
	if(nfsWrite(aux->auth, r->tag, &aux->handle, (uchar*)r->ifcall.data, r->ifcall.count, r->ifcall.offset, &count) < 0){
		responderrstr(r);
		return;
	}
	r->ofcall.count = count;
	respond(r, nil);
}

void
fsremove(Req *r)
{
	int n;
	FidAux *aux;

	aux = r->fid->aux;
	if(aux->name == nil){
		respond(r, "nfs3client botch -- don't know parent handle in remove");
		return;
	}
	if(r->fid->qid.type&QTDIR)
		n = nfsRmdir(aux->auth, r->tag, &aux->parent, aux->name);
	else
		n = nfsRemove(aux->auth, r->tag, &aux->parent, aux->name);
	if(n < 0){
		responderrstr(r);
		return;
	}
	respond(r, nil);
}

void
fsstat(Req *r)
{
	FidAux *aux;
	Nfs3Attr attr;

	aux = r->fid->aux;
	if(nfsGetattr(aux->auth, r->tag, &aux->handle, &attr) < 0){
		responderrstr(r);
		return;
	}
	memset(&r->d, 0, sizeof r->d);
	attrToDir(&attr, &r->d);
	r->d.name = estrdup9p(aux->name ? aux->name : "???");
	respond(r, nil);
}

void
fswstat(Req *r)
{
	int op, sync;
	FidAux *aux;
	Nfs3SetAttr attr;

	memset(&attr, 0, sizeof attr);
	aux = r->fid->aux;

	/* Fill out stat first to catch errors */
	op = 0;
	sync = 1;
	if(~r->d.mode){
		if(r->d.mode&(DMAPPEND|DMEXCL)){
			respond(r, "wstat -- DMAPPEND and DMEXCL bits not supported");
			return;
		}
		op = 1;
		sync = 0;
		attr.setMode = 1;
		attr.mode = r->d.mode & 0777;
	}
	if(r->d.uid && r->d.uid[0]){
		attr.setUid = 1;
		if(strtouid(r->d.uid, &attr.uid) < 0){
			respond(r, "wstat -- unknown uid");
			return;
		}
		op = 1;
		sync = 0;
	}
	if(r->d.gid && r->d.gid[0]){
		attr.setGid = 1;
		if(strtogid(r->d.gid, &attr.gid) < 0){
			respond(r, "wstat -- unknown gid");
			return;
		}
		op = 1;
		sync = 0;
	}
	if(~r->d.length){
		attr.setSize = 1;
		attr.size = r->d.length;
		op = 1;
		sync = 0;
	}
	if(~r->d.mtime){
		attr.setMtime = Nfs3SetTimeClient;
		attr.mtime.sec = r->d.mtime;
		op = 1;
		sync = 0;
	}
	if(~r->d.atime){
		attr.setAtime = Nfs3SetTimeClient;
		attr.atime.sec = r->d.atime;
		op = 1;
		sync = 0;
	}

	/* Try rename first because it's more likely to fail (?) */
	if(r->d.name && r->d.name[0]){
		if(aux->name == nil){
			respond(r, "nfsclient botch -- don't know parent handle in rename");
			return;
		}
		if(nfsRename(aux->auth, r->tag, &aux->parent, aux->name, &aux->parent, r->d.name) < 0){
			responderrstr(r);
			return;
		}
		free(aux->name);
		aux->name = estrdup9p(r->d.name);
		sync = 0;
	}

	/*
	 * Now we have a problem.  The rename succeeded
	 * but the setattr could fail.  Sic transit atomicity.
	 */
	if(op){
		if(nfsSetattr(aux->auth, r->tag, &aux->handle, &attr) < 0){
			responderrstr(r);
			return;
		}
	}

	if(sync){
		/* NFS commit */
		if(nfsCommit(aux->auth, r->tag, &aux->handle) < 0){
			responderrstr(r);
			return;
		}
	}

	respond(r, nil);
}

char*
fswalk1(Fid *fid, char *name, void *v)
{
	u1int have;
	ulong tag;
	FidAux *aux;
	Nfs3Attr attr;
	Nfs3Handle h;

	tag = *(ulong*)v;
	aux = fid->aux;

	if(nfsLookup(aux->auth, tag, &aux->handle, name, &h, &have, &attr) < 0
	|| (!have && nfsGetattr(aux->auth, tag, &h, &attr) < 0)){
		rerrstr(aux->err, sizeof aux->err);
		return aux->err;
	}

	aux->parent = aux->handle;
	aux->handle = h;
	free(aux->name);
	if(strcmp(name, "..") == 0)
		aux->name = nil;
	else
		aux->name = estrdup9p(name);
	attrToQid(&attr, &fid->qid);
	return nil;
}

char*
fsclone(Fid *fid, Fid *newfid, void*)
{
	FidAux *a, *na;

	a = fid->aux;
	na = emalloc9p(sizeof(FidAux));
	*na = *a;
	if(na->name)
		na->name = estrdup9p(na->name);
	newfid->aux = na;
	if(na->auth)
		na->auth->ref++;
	return nil;
}

void
fswalk(Req *r)
{
	walkandclone(r, fswalk1, fsclone, &r->tag);
}

void
fsflush(Req *r)
{
	Req *or;

	/*
	 * Send on the flush channel(s).
	 * The library will make sure the response
	 * is delayed as necessary.
	 */
	or = r->oldreq;
	if(nfscli)
		sendul(nfscli->flushchan, (ulong)or->tag);
	if(mntcli)
		sendul(mntcli->flushchan, (ulong)or->tag);
	respond(r, nil);
}

void
fsdispatch(void *v)
{
	Req *r;

	r = v;
	switch(r->ifcall.type){
	default:	respond(r, "unknown type");	break;
	case Tattach:	fsattach(r);	break;
	case Topen:	fsopen(r);	break;
	case Tcreate:	fscreate(r);	break;
	case Tread:	fsread(r);	break;
	case Twrite:	fswrite(r);	break;
	case Tremove:	fsremove(r);	break;
	case Tflush:	fsflush(r);	break;
	case Tstat:	fsstat(r);	break;
	case Twstat:	fswstat(r);	break;
	case Twalk:	fswalk(r);	break;
	}
}

void
fsthread(void*)
{
	Req *r;

	while((r = recvp(fschan)) != nil)
		threadcreate(fsdispatch, r, SunStackSize);
}

void
fssend(Req *r)
{
	sendp(fschan, r);
}

void
fsdie(Srv*)
{
	threadexitsall(nil);
}

Srv fs =
{
.destroyfid =	fsdestroyfid,
.attach=		fssend,
.open=		fssend,
.create=		fssend,
.read=		fssend,
.write=		fssend,
.remove=		fssend,
.flush=		fssend,
.stat=		fssend,
.wstat=		fssend,
.walk=		fssend,
.end=		fsdie
};

void
usage(void)
{
	fprint(2, "usage: nfs [-DRv] [-p perm] [-s srvname] [-u passwd group] addr [addr]\n");
	fprint(2, "\taddr - address of portmapper server\n");
	fprint(2, "\taddr addr - addresses of mount server and nfs server\n");
	exits("usage");
}

char*
netchangeport(char *addr, uint port, char *buf, uint nbuf)
{
	char *r;

	strecpy(buf, buf+nbuf, addr);
	r = strrchr(buf, '!');
	if(r == nil)
		return nil;
	r++;
	seprint(r, buf+nbuf, "%ud", port);
	return buf;
}

char mbuf[256], nbuf[256];
char *mountaddr, *nfsaddr;
Channel *csync;
int chattyrpc;
void dialproc(void*);

void
threadmain(int argc, char **argv)
{
	char *srvname, *passwd, *group, *addr, *p;
	SunClient *cli;
	int proto;
	uint mport, nport;
	ulong perm;
	Dir d;

	perm = 0600;
	passwd = nil;
	group = nil;
	srvname = nil;
	sys = sysname();
	if(sys == nil)
		sys = "plan9";
	ARGBEGIN{
	default:
		usage();
	case 'D':
		chatty9p++;
		break;
	case 'R':
		chattyrpc++;
		break;
	case 'p':
		perm = strtol(EARGF(usage()), &p, 8);
		if(perm==0 || *p != 0)
			usage();
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	case 'u':
		passwd = EARGF(usage());
		group = EARGF(usage());
		break;
	case 'v':
		verbose++;
		break;
	}ARGEND

	if(argc != 1 && argc != 2)
		usage();

	if(srvname == nil)
		srvname = argv[0];

	fmtinstall('B', sunRpcFmt);
	fmtinstall('C', sunCallFmt);
	fmtinstall('H', encodefmt);
	sunFmtInstall(&portProg);
	sunFmtInstall(&nfs3Prog);
	sunFmtInstall(&nfsMount3Prog);

	if(passwd && (map = readmap(passwd, group)) == nil)
		fprint(2, "warning: reading %s and %s: %r\n", passwd, group);

	if(map == nil)
		map = &emptymap;

	if(argc == 1){
		addr = netmkaddr(argv[0], "udp", "portmap");
		if((cli = sunDial(addr)) == nil)
			sysfatal("dial %s: %r", addr);
		cli->chatty = chattyrpc;
		sunClientProg(cli, &portProg);
		if(strstr(addr, "udp!"))
			proto = PortProtoUdp;
		else
			proto = PortProtoTcp;
		if(getport(cli, NfsMount3Program, NfsMount3Version, proto, &mport) < 0)
			sysfatal("lookup mount program port: %r");
		if(getport(cli, Nfs3Program, Nfs3Version, proto, &nport) < 0)
			sysfatal("lookup nfs program port: %r");
		sunClientClose(cli);
		mountaddr = netchangeport(addr, mport, mbuf, sizeof mbuf);
		nfsaddr = netchangeport(addr, nport, nbuf, sizeof nbuf);
		strcat(mountaddr, "!r");
		strcat(nfsaddr, "!r");
		if(verbose)
			fprint(2, "nfs %s %s\n", mountaddr, nfsaddr);
	}else{
		mountaddr = argv[0];
		nfsaddr = argv[1];
	}

	/* have to dial in another proc because it creates threads */
	csync = chancreate(sizeof(void*), 0);
	proccreate(dialproc, nil, SunStackSize);
	recvp(csync);

	threadpostmountsrv(&fs, srvname, nil, 0);
	if(perm != 0600){
		p = smprint("/srv/%s", srvname);
		if(p){
			nulldir(&d);
			d.mode = perm;
			dirwstat(p, &d);
		}
	}
	threadexits(nil);
}

void
dialproc(void*)
{
	rfork(RFNAMEG);
	rfork(RFNOTEG);
	if((mntcli = sunDial(mountaddr)) == nil)
		sysfatal("dial mount program at %s: %r", mountaddr);
	mntcli->chatty = chattyrpc;
	sunClientProg(mntcli, &nfsMount3Prog);
	if(mountNull(0) < 0)
		sysfatal("execute nop with mnt server at %s: %r", mountaddr);
	
	if((nfscli = sunDial(nfsaddr)) == nil)
		sysfatal("dial nfs program at %s: %r", nfsaddr);
	nfscli->chatty = chattyrpc;
	sunClientProg(nfscli, &nfs3Prog);
	if(nfsNull(0) < 0)
		sysfatal("execute nop with nfs server at %s: %r", nfsaddr);

	fschan = chancreate(sizeof(Req*), 0);
	threadcreate(fsthread, nil, SunStackSize);
	sendp(csync, 0);
}
