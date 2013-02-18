#include "nat.h"

extern uchar addr2ind(uchar *addr);
extern int allochostport(NATSession *ns, int hashind);
extern int insert2buff(NATSession *ns);

static double nsess, peaksess, totsess, nfin, nfin2, nofin;
static int nprobe, nlookup;
static Biobuf bout;

void
NATinit(char *fname)
{
	int i, j, k, *ptr;
	Portrange *p, *q;

	q = nil;
	fmtinstall('F', FiveTuple_conv);
	Binit(&bout, 1, OWRITE);
	Bout = &bout;

	natid = 0;	// to be obtained differently for multiple boxes case
	msgid = 0;

	// read in available external addresses and stick into myhosti2a
	initi2a(fname);

	// complete initialization of myhosti2a
	for (j=0; j<numextaddr; j++) {
		myhosti2a.i2amap[j].nconns = 0;
		myhosti2a.i2amap[j].nranges = 0;
		myhosti2a.i2amap[j].lastreq = 0;

		ptr = (int*) malloc(MAXPORT*sizeof(int));
		assert(ptr);
		for(i = 0; i<MAXPORT; i++)
			*(ptr+i) = AVAILABLE;
		myhosti2a.i2amap[j].p2s = ptr;

		p = (Portrange*) malloc(sizeof(Portrange));
		p->lo = MID_PORT;
		p->hi_port = p->lo;
		p->nused = 0;
		p->prev = nil;
		myhosti2a.i2amap[j].myrangefirst = p;
		myhosti2a.i2amap[j].myrangecurrent = p;
		// Can q but undefined?
		for (k=MID_PORT+RANGESZ; k+RANGESZ<=HI_PORT; k = k+RANGESZ) {
			q = (Portrange*) malloc(sizeof(Portrange));
			assert(q!=nil);
			q->lo = k;
			q->hi_port = q->lo;
			q->nused = 0;
			q->prev = p;
			p->next = q;
			p = q;
		}
		q->next = nil;
		myhosti2a.i2amap[j].myrangelast = p;
		netlog("NATinit: mallocd for addr %V\n", myhosti2a.i2amap[j].nataddr);
	}

	// initialize a2imap
	netlog("NATinit: before inita2i\n");
	inita2i(numextaddr);

	// initialize buffer ring
	br.cp = 0;
	br.lw = MAXMSGBUFFS - 1;
	for (j=0; j<MAXBUFFSZ; j++) {
		br.buffs[j].current = FREE;
		br.buffs[j].index = sizeof(NATHdr);;
	}
}


// used for non-udp packets
static uint
hash(FiveTuple *f)
{
	uint j = ((f->iaddr[3]^f->iport[0])<<24) | ((f->iport[1])<<16) |
		 ((f->oaddr[3]^f->oport[0]^f->proto)<<8) | f->oport[1];
	j = j + (j>>13);
	return j & 0xffff;
}

// used for udp packets, pretend oaddr == oport == 0.
static uint
hash00(FiveTuple *f)
{
	uint j = ((f->iaddr[3]^f->iport[0])<<24) | ((f->iport[1])<<16) |
		 (f->proto<<8);
	j = j + (j>>13);
	return j & 0xffff;
}

int
port2session(FiveTuple *f)
{
	uchar h[4]; 
	int j;
	int *p, hashval;

	memcpy(h, f->iaddr, 4);
	j = addr2ind(h);
	if ((j<0) || (j>=MAXEXTADDR)) {
		return AVAILABLE;
	}

	rlock(&myhosti2a.i2amap[j]);
	p = myhosti2a.i2amap[j].p2s + nhgets(f->iport);
	hashval = *p;
	runlock(&myhosti2a.i2amap[j]);
	netlog("port2session: ret %d\n", hashval);
	return hashval;
}

NATSession*
hashlookup(FiveTuple *f, int *hashval, int *deleted, int findslot)
{
	int sentinel;
	NATSession *s;
	FiveTuple *sf;

	sentinel = *hashval;
	*deleted = 0;

loop:	
	s = sessiontable.table + *hashval;
	sf = (FiveTuple*)(s->iaddr);
	if(verbose)
		netlog("hashlookup1: \t\t%5d %.2ux %F\n", *hashval, s->action, sf);
	if( (s->action & EMPTYBIT) == 0) {// if empty
		if (findslot == FINDSLOT) {
			netlog("hashlookup2: ret 1 hv %d\n", *hashval);
			return s;
		}
		netlog("hashlookup3: EMPTYBIT\n");
		return nil;
	}
	if( (s->action & DELETEDBIT) != 0 ){ // if deleted
		if(*deleted == 0)
			*deleted = *hashval;
		if (findslot == FINDSLOT) {
			netlog("hashlookup4: ret 1 hv %d\n", *hashval);
			return s;
		}
		goto sessionnext;
	}
	if(f->proto==IP_TCPPROTO && memcmp(f->oaddr, s->oaddr, 6)!=0)
		goto sessionnext;
	if(memcmp(f, sf, 7) == 0) {// if match
		netlog("hashlookup5: ret 1 hv %d\n", *hashval);
		return s;
	}

sessionnext:
	if(verbose)
		netlog("hashlookup6: \t%6d %.2ux %F\n", *hashval, s->action, sf);
	*hashval = (*hashval+(f->iport[1])+1)%MAXSESSIONS;
	if(*hashval == sentinel){
		fprint(2, "hash table filled!\n");
		netlog("hashlookup7: ret -1 hv %d\n", *hashval);
		return nil;
	}
	goto loop;
}

// given arriving packet, lookup or create session and rewrite f
// This code was intended to match the IXP1200 microcode version.
int
NATsession(double t, int dir, FiveTuple *f, ushort synfin, int *raddrind)
{
	int hashval, deleted;
	NATSession *s;
	double timeout;
	char *newold;
	int locktype, succ, exptime, exp; // found?
	NATSession ns;

	s = nil;
	USED(s);
	newold = "";
	if(f->proto == IP_TCPPROTO)
		netlog("%c %F%s\n", (dir==OUTBOUND)?'O':'I', f, tcpflag(synfin));

	netlog("NATsession: entered\n");
	if(dir != OUTBOUND){  // inbound
		rlock(&sessiontable);
		locktype = RDLOCK;
		hashval = port2session(f);
		if(hashval == AVAILABLE){
			runlock(&sessiontable);
			netlog("NATsession: inbound NAT_DROP no session\n");
			return NAT_DROP;  // no session
		}
		s = sessiontable.table + hashval;
		if(s->proto == IP_TCPPROTO && (f->proto != IP_TCPPROTO ||
				memcmp(f->oaddr, s->oaddr, 6) != 0) ){
			runlock(&sessiontable);
			netlog("NATsession: inbound NAT_DROP outside TCP end mismatch\n");
			return NAT_DROP;  // outside end of TCP didn't match
		}
		memcpy(f->iaddr, s->iaddr, 6);  // global -> local addr,port
		goto sessionfound;
	}

	if(f->proto==IP_UDPPROTO)
		hashval = hash00(f)%MAXSESSIONS;
	else
		hashval = hash(f)%MAXSESSIONS;

	rlock(&sessiontable);
	locktype = RDLOCK;

	s = hashlookup(f, &hashval, &deleted, FINDSESN);
	if (s == nil) {			// new session
		runlock(&sessiontable);
		wlock(&sessiontable);
	 	locktype = WRLOCK;
		s = hashlookup(f, &hashval, &deleted, FINDSLOT);
		if (s == nil)  {
			fprint(2,"hashlookup failed at %.6f\n", t);
			netlog("NATsession: outbound NAT_DROP hash fail\n");
			return NAT_DROP;
		}
		memcpy(ns.iaddr, f->iaddr, 4);
		memcpy(ns.iport, f->iport, 2);
		ns.proto = f->proto; //memcpy(ns.proto, f->proto, 1);
		memcpy(ns.oaddr, f->oaddr, 4);
		memcpy(ns.oport, f->oport, 2);
		succ = allochostport(&ns, hashval);
		if(succ <= 0) {
			fprint(2,"NATalloc failed at %.6f\n", t);
			return NAT_DROP;
		}
		*raddrind = (int) ns.raddr;
		s = sessiontable.table + hashval;
		insert2buff(&ns);
		if(verbose > 1)
			newold = "new";
		totsess++;
		nsess++;
		if(peaksess < nsess)
			peaksess = nsess;
	}

sessionfound:
	assert(s!=nil);
	if(dir == OUTBOUND){
		memcpy(f->iaddr,(char*)&(myhosti2a.i2amap[s->raddr].nataddr[0]), 4); 
		memcpy(f->iport, s->rport, 2);  // local -> global port
		if(verbose > 1)
			netlog("NATsession: %s\t\t -> %d [%d]\n", 
				newold, nhgets(s->rport), hashval);
	}
	// deltat
	if((s->action&FINBIT) == FINBIT) // FIN in both directions
		timeout = FIN_TIMEOUT;
	else if(s->proto == IP_TCPPROTO)
		timeout = TCP_TIMEOUT;
	else
		timeout = UDP_TIMEOUT;

	// note FIN; set timeout
	qlock(&(sessiontable.finlock));
	if(synfin&TCP_RST){
		s->action |= FINBIT;
	}else if(synfin&TCP_FIN){
		if(dir == OUTBOUND)
			s->action |= FINOBIT;
		else
			s->action |= FINIBIT;
	}
	if((s->action&FINBIT) == FINBIT) // FIN in both directions
		timeout = FIN_TIMEOUT;
	exptime = ((int) floor(t + timeout)) >> 4;
	assert(exptime < 2^24);
	s->expires[0] = (exptime >> 16) & 0xff;
	s->expires[1] = (exptime >> 8) & 0xff;
	s->expires[2] = (exptime >> 0) & 0xff;
	//memcpy(ns.expires, s->expires, 3);
	//insert2buff(&ns);
	exp = (s->expires[0] << 16) | (s->expires[1] << 8) | (s->expires[2] << 0);
	netlog("NATsession: t update %d\n", exp);
	qunlock(&(sessiontable.finlock));
	if (locktype == RDLOCK) 
		runlock(&sessiontable);
	else 
		wunlock(&sessiontable);

	return NAT_REWRITE;
}

