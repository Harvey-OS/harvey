#include "all.h"

#define	SHORT(x)	r->x = (p[1] | (p[0]<<8)); p += 2
#define	LONG(x)		r->x = (p[3] | (p[2]<<8) |\
				(p[1]<<16) | (p[0]<<24)); p += 4
#define SKIPLONG	p += 4
#define	PTR(x, n)	r->x = (void *)(p); p += ROUNDUP(n)

int
rpcM2S(void *ap, Rpccall *r, int n)
{
	int k;
	uchar *p;
	Udphdr *up;

	/* copy IPv4 header fields from Udphdr */
	up = ap;
	p = &up->raddr[IPaddrlen - IPv4addrlen];
	LONG(host);
	USED(p);
	p = &up->laddr[IPaddrlen - IPv4addrlen];
	LONG(lhost);
	USED(p);
	/* ignore up->ifcaddr */
	p = up->rport;
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
	char *s;

	p = arg;
	LONG(n);
	PTR(s, r->n);
	/* must NUL terminate */
	s = malloc(r->n+1);
	if(s == nil)
		panic("malloc(%ld) failed in string2S\n", r->n+1);
	memmove(s, r->s, r->n);
	s[r->n] = '\0';
	r->s = strstore(s);
	free(s);
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
	uchar *p;
	Udphdr *up;

	/* copy header fields to Udphdr */
	up = ap;
	memmove(up->raddr, v4prefix, IPaddrlen);
	p = &up->raddr[IPaddrlen - IPv4addrlen];
	LONG(host);
	USED(p);
	memmove(up->laddr, v4prefix, IPaddrlen);
	p = &up->laddr[IPaddrlen - IPv4addrlen];
	LONG(lhost);
	USED(p);
	memmove(up->ifcaddr, IPnoaddr, sizeof up->ifcaddr);
	p = up->rport;
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
		chat("auth flavor=%ld, count=%ld",
			ap->flavor, ap->count);
		for(i=0; i<ap->count; i++)
			chat(" %.2ux", ((uchar *)ap->data)[i]);
	}else{
		chat("auth: %ld %.*s u=%ld g=%ld",
			au.stamp, utfnlen(au.mach.s, au.mach.n), au.mach.s, au.uid, au.gid);
		for(i=0; i<au.gidlen; i++)
			chat(", %ld", au.gids[i]);
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
