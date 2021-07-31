#include "all.h"

#define	SHORT(x)	r->x = (p[1] | (p[0]<<8)); p += 2
#define	LONG(x)		r->x = (p[3] | (p[2]<<8) |\
				(p[1]<<16) | (p[0]<<24)); p += 4
#define SKIPLONG	p += 4
#define	PTR(x, n)	r->x = (void *)(p); p += ROUNDUP(n)

uchar v4prefix[12] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
};

int
rpcM2S(void *ap, Rpccall *r, int n)
{
	uchar *p = ap;
	int k;

	p += 12;
	LONG(host);
	p += 12;
	LONG(lhost);
	SHORT(port);
	SHORT(lport);
	LONG(xid);
	LONG(mtype);
	switch(r->mtype){
	case CALL:
		LONG(rpcvers);
		if(r->rpcvers != 2)
			break;
		LONG(prog);
		LONG(vers);
		LONG(proc);
		LONG(cred.flavor);
		LONG(cred.count);
		PTR(cred.data, r->cred.count);
		LONG(verf.flavor);
		LONG(verf.count);
		PTR(verf.data, r->verf.count);
		r->up = 0;
		k = n - (p - (uchar *)ap);
		if(k < 0)
			break;
		PTR(args, k);
		break;
	case REPLY:
		LONG(stat);
		switch(r->stat){
		case MSG_ACCEPTED:
			LONG(averf.flavor);
			LONG(averf.count);
			PTR(averf.data, r->averf.count);
			LONG(astat);
			switch(r->astat){
			case SUCCESS:
				k = n - (p - (uchar *)ap);
				if(k < 0)
					break;
				PTR(results, k);
				break;
			case PROG_MISMATCH:
				LONG(plow);
				LONG(phigh);
				break;
			}
			break;
		case MSG_DENIED:
			LONG(rstat);
			switch(r->rstat){
			case RPC_MISMATCH:
				LONG(rlow);
				LONG(rhigh);
				break;
			case AUTH_ERROR:
				LONG(authstat);
				break;
			}
			break;
		}
		break;
	}
	n -= p - (uchar *)ap;
	return n;
}

int
auth2unix(Auth *arg, Authunix *r)
{
	int i, n;
	uchar *p;

	if(arg->flavor != AUTH_UNIX)
		return -1;
	p = arg->data;
	LONG(stamp);
	LONG(mach.n);
	PTR(mach.s, r->mach.n);
	LONG(uid);
	LONG(gid);
	LONG(gidlen);
	n = r->gidlen;
	for(i=0; i<n && i < nelem(r->gids); i++){
		LONG(gids[i]);
	}
	for(; i<n; i++){
		SKIPLONG;
	}
	return arg->count - (p - (uchar *)arg->data);
}

int
string2S(void *arg, String *r)
{
	uchar *p;

	p = arg;
	LONG(n);
	PTR(s, r->n);
	return p - (uchar *)arg;
}

#undef	SHORT
#undef	LONG
#undef	PTR

#define	SHORT(x)	p[1] = r->x; p[0] = r->x>>8; p += 2
#define	LONG(x)		p[3] = r->x; p[2] = r->x>>8; p[1] = r->x>>16; p[0] = r->x>>24; p += 4

#define	PTR(x,n)	memmove(p, r->x, n); p += ROUNDUP(n)

int
rpcS2M(Rpccall *r, int ndata, void *ap)
{
	uchar *p = ap;

	memmove(p, v4prefix, 12);
	p += 12;
	LONG(host);
	memmove(p, v4prefix, 12);
	p += 12;
	LONG(lhost);
	SHORT(port);
	SHORT(lport);
	LONG(xid);
	LONG(mtype);
	switch(r->mtype){
	case CALL:
		LONG(rpcvers);
		LONG(prog);
		LONG(vers);
		LONG(proc);
		LONG(cred.flavor);
		LONG(cred.count);
		PTR(cred.data, r->cred.count);
		LONG(verf.flavor);
		LONG(verf.count);
		PTR(verf.data, r->verf.count);
		PTR(args, ndata);
		break;
	case REPLY:
		LONG(stat);
		switch(r->stat){
		case MSG_ACCEPTED:
			LONG(averf.flavor);
			LONG(averf.count);
			PTR(averf.data, r->averf.count);
			LONG(astat);
			switch(r->astat){
			case SUCCESS:
				PTR(results, ndata);
				break;
			case PROG_MISMATCH:
				LONG(plow);
				LONG(phigh);
				break;
			}
			break;
		case MSG_DENIED:
			LONG(rstat);
			switch(r->rstat){
			case RPC_MISMATCH:
				LONG(rlow);
				LONG(rhigh);
				break;
			case AUTH_ERROR:
				LONG(authstat);
				break;
			}
			break;
		}
		break;
	}
	return p - (uchar *)ap;
}

#undef	SHORT
#undef	LONG
#undef	PTR

#define	LONG(m, x)	fprint(fd, "%s = %ld\n", m, r->x)

#define	PTR(m, count)	fprint(fd, "%s [%ld]\n", m, count)

void
rpcprint(int fd, Rpccall *r)
{
	fprint(fd, "%s: host = %I, port = %ld\n", 
		argv0, r->host, r->port);
	LONG("xid", xid);
	LONG("mtype", mtype);
	switch(r->mtype){
	case CALL:
		LONG("rpcvers", rpcvers);
		LONG("prog", prog);
		LONG("vers", vers);
		LONG("proc", proc);
		LONG("cred.flavor", cred.flavor);
		PTR("cred.data", r->cred.count);
		LONG("verf.flavor", verf.flavor);
		PTR("verf.data", r->verf.count);
		fprint(fd, "args...\n");
		break;
	case REPLY:
		LONG("stat", stat);
		switch(r->stat){
		case MSG_ACCEPTED:
			LONG("averf.flavor", averf.flavor);
			PTR("averf.data", r->averf.count);
			LONG("astat", astat);
			switch(r->astat){
			case SUCCESS:
				fprint(fd, "results...\n");
				break;
			case PROG_MISMATCH:
				LONG("plow", plow);
				LONG("phigh", phigh);
				break;
			}
			break;
		case MSG_DENIED:
			LONG("rstat", rstat);
			switch(r->rstat){
			case RPC_MISMATCH:
				LONG("rlow", rlow);
				LONG("rhigh", rhigh);
				break;
			case AUTH_ERROR:
				LONG("authstat", authstat);
				break;
			}
			break;
		}
	}
}

void
showauth(Auth *ap)
{
	Authunix au;
	int i;

	if(auth2unix(ap, &au) != 0){
		chat("auth flavor=%d, count=%d",
			ap->flavor, ap->count);
		for(i=0; i<ap->count; i++)
			chat(" %.2ux", ((uchar *)ap->data)[i]);
	}else{
		chat("auth: %d %.*s u=%d g=%d",
			au.stamp, utfnlen(au.mach.s, au.mach.n), au.mach.s, au.uid, au.gid);
		for(i=0; i<au.gidlen; i++)
			chat(", %d", au.gids[i]);
	}
	chat("...");
}

int
garbage(Rpccall *reply, char *msg)
{
	chat("%s\n", msg ? msg : "garbage");
	reply->astat = GARBAGE_ARGS;
	return 0;
}

int
error(Rpccall *reply, int errno)
{
	uchar *dataptr = reply->results;

	chat("error %d\n", errno);
	PLONG(errno);
	return dataptr - (uchar *)reply->results;
}
