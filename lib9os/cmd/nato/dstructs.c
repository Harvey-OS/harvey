#include "nat.h"

extern int ins2outs(NATSession *ns);

char *msgtypetab[] =  {
	[RTable]		"RTable",
	[TTable]		"TTable", 
	[RUpdate]		"RUpdate",
	[TUpdate]		"TUpdate",
	[RDist]			"RDist",
	[TDist]			"TDist", 
	[RAge]			"RAge",
	[TAge]			"TAge",
	[RRange]		"RRange",
	[TRange]		"TRange",
	[RWithdraw]		"RWithdraw",
	[TWithdraw]		"TWithdraw",
	[RParttable]		"RParttable",
	[TParttable]		"TParttable", 
};

/* ******************************************************
void 
appendT(Timerinfohd *th, Timerinfo *ti)
{
	assert(ti);

	qlock(th);
	ti->prev = th->etail;
	ti->next = nil;
	if (th->ehead == nil)
		th->ehead = ti;
	else 
		th->etail->next = ti;
	th->etail = ti;		
	th->len++;
	qunlock(th);
}

void 
sinsertT(Timerinfohd *th, Timerinfo *ti)
{
	int i;
	Timerinfo *tmp;

	assert(ti);

	qlock(th);
	if (th->len == 0) {			// only element
		ti->prev = nil; 
		ti->next = nil;
		th->ehead = ti;
		th->etail = ti;
	}
	else {
		tmp = th->etail;
		for (i=th->len; i>0; i--) {
			if (tmp->fireat <= ti->fireat)
				break;
			tmp = tmp->prev;
		}
		if (i==0) {			// insert first
			ti->prev = nil;
			ti->next = th->ehead;
			th->ehead->prev = ti;
			th->ehead = ti;
		}
		else {
			ti->prev = tmp;
			ti->next = tmp->next;
			if (tmp->next) {	// insert middle
				tmp->next->prev = ti; }
			else {			// insert last
				th->etail = ti; }
			tmp->next = ti;
		}
	}
	th->len++;	
	qunlock(th);
}

Timerinfo* 
deleteT(Timerinfohd *th)
{
	Timerinfo *tmp;

	qlock(th);
	assert(th->len >= 0);

	if (th->len == 0) {
		qunlock(th);
		return nil;
	}

	tmp = th->ehead;
	th->ehead = tmp->next;
	if (th->ehead == nil)
		th->etail = nil;
	else 
		th->ehead->prev = nil;
	tmp->prev = nil;
	tmp->next = nil;
	th->len--;
	qunlock(th);
	return tmp;
}

Msgbuff* 
_deleteM(Msgbuffhd *mh)
{
	Msgbuff *tmp;

	if (mh->len == 0) {
		return nil;
	}

	tmp = mh->buffhead;
	mh->buffhead = tmp->next;
	if (mh->buffhead == nil)
		mh->bufftail = nil;
	else 
		mh->buffhead->prev = nil;
	tmp->prev = nil;
	tmp->next = nil;
	mh->len--;
	return tmp;
}

Msgbuff* 
deleteM(Msgbuffhd *mh)
{
	Msgbuff *tmp;

	qlock(mh);
	assert(mh->len >= 0);
	tmp = _deleteM(mh);
	qunlock(mh);
	return tmp;
}

void 
_allocinitM(Msgbuffhd *mh, int n)
{
	int i;
	Msgbuff *tmp;

	tmp = (Msgbuff*) malloc(sizeof(Msgbuff));
	assert(tmp);
	tmp->next = nil; tmp->prev = nil; 
	mh->buffhead = tmp; 
	for (i=0; i<n; i++) {
		tmp = (Msgbuff*) malloc(sizeof(Msgbuff));
		assert(tmp);
		tmp->next = nil; tmp->prev = nil; 
	}
	mh->bufftail = tmp;
	mh->alloc = n;
	mh->len = n;
}

void 
allocinitM(Msgbuffhd *mh, int n)
{
	qlock(mh);
	assert((mh->buffhead == nil) && (mh->bufftail == nil) && (n >= 1));
	_allocinitM(mh, n);
	qunlock(mh);
}

Msgbuff* 
allocM(Msgbuffhd *mh)
{
	Msgbuff *tmp;	

	qlock(mh);
	if (mh->len == 0) {
		_allocinitM(mh, NFREEBUFFS);
		mh->alloc += mh->alloc; 
	}
	tmp = _deleteM(mh); 
	qunlock(mh);
	return tmp;
}

void 
freeM(Msgbuffhd *mh, Msgbuff* mb)
{
	assert(mb);

	qlock(mh);
	mb->prev = mh->bufftail;
	mb->next = nil;
	if (mh->buffhead == nil)
		mh->buffhead = mb;
	else 
		mh->bufftail->next = mb;
	mh->bufftail = mb;		
	mh->len++;
	qunlock(mh);
}


int
modifyused(hosti2a *h, int hind, int range_lo, int change)
{
	i2amapper *p;
	Portrange *pr;

	assert((0<=hind) && (hind<MAXEXTADDR));

	p = &(h->i2amap[hind]);
	for (pr = p->myrangefirst; pr; pr = pr->next) {
		if (pr->lo == range_lo) {
			pr->nused += change;
			assert((0 <= pr->nused) && (pr->nused < RANGESZ));
			return 1;
		}
	}
	return -1;
}


int 
insertrange(hosti2a *h, int hind, int lo, int nused)
{
	i2amapper *p;
	Portrange *pr = nil, *q, *r;

	assert((0<=hind) && (hind<MAXEXTADDR));

	p = &(h->i2amap[hind]);
	pr = (Portrange*) malloc(sizeof(Portrange));
	assert(pr != nil);

	p->nranges++;
	p->nconns += nused;
	pr->lo = lo;
	pr->hi_port = lo;
	pr->nused = nused;
	if (p->myrangefirst == nil) {
		p->myrangefirst = pr;
		p->myrangelast = pr;
		return 1;
	}

	for (q = p->myrangefirst; q; q = q->next) {
		if (q->lo > lo) break;
	}
	if (q) {
		r = q->prev;
		if (r) {	// add in the middle
			r->next = pr; pr->prev = r;
			q->prev = pr; pr->next = q;
		}
		else {		// add at the beginning
			q->prev = pr; pr->next = q; 
			pr->prev = nil;
			p->myrangefirst = pr;
		}
	}
	else {			// add at the end
		r = p->myrangelast;
		r->next = pr;
		pr->prev = r;
		pr->next = nil;
		p->myrangelast = pr;
	}
	return 1;
}

int 
deleterange(hosti2a *h, int hind, int lo)
{
	i2amapper *p;
	Portrange *q, *r, *s;

	assert((0<=hind) && (hind<MAXEXTADDR));

	p = &(h->i2amap[hind]);
	for (q = p->myrangefirst; q; q = q->next) {
		if (q->lo >= lo) break;
	}
	if (!q || q->lo > lo) return -1;

	p->nranges--;
	p->nconns -= q->nused;
	r = q->prev; s = q->next;
	if (r) 
		r->next = s;
	else
		p->myrangefirst = s;

	if (s)
		s->prev = r;
	else
		p->myrangelast = r;

	if (q==p->myrangecurrent) 
		p->myrangecurrent = (s ? s : p->myrangefirst);
	free(q);
	return 1;
}

Portrange* 
delgoodrange(hosti2a *h, int hind, float cutoff)
{
	i2amapper *p;
	Portrange *q, *r, *s;

	p = &(h->i2amap[hind]);
	for (q = p->myrangefirst; q; q = q->next) {
		if ((q->nused/RANGESZ) <= cutoff) break;
	}
	if (!q) return nil;

	p->nranges--;
	p->nconns -= q->nused;
	r = q->prev; s = q->next;
	if (r) 
		r->next = s;
	else
		p->myrangefirst = s;

	if (s)
		s->prev = r;
	else
		p->myrangelast = r;

	if (q==p->myrangecurrent) 
		p->myrangecurrent = (s ? s : p->myrangefirst);

	return q;
}

Portrange* 
revokerange(int hind)
{
	int i, j, k = -1;
	float cmin = 1, m;

	assert((0<=hind) && (hind<MAXEXTADDR));
	j = hind;

	for (i=0; i<=MAXNUMNATS; i++) {
		if ((allhosts.hosts[i].i2amap[j].nranges == 0) ||
			((nsec() - allhosts.hosts[i].i2amap[j].lastreq) <= 
			NOWITHDRAWLIM*10^9)) 
			continue;
		m = allhosts.hosts[i].i2amap[j].nconns / 
			(allhosts.hosts[i].i2amap[j].nranges*RANGESZ);
		if (m<cmin) {
			cmin = m;
			k = i;
		}	
	}
	if (k >= 0)
		return delgoodrange(&(allhosts.hosts[k]), j, cmin);
	else 
		return nil;	
}

****************************************************** */

int
allochostport(NATSession *ns, int hashind)
{
	int j, k, lo;
	Portrange *p;
	int iport, oport, rport, proto, raddr, exp;

	j = ins2outs(ns);
	if ((j<0) || (j>=numextaddr)) {
		netlog("allochostport: ins2outs failed\n");
		return -1;
	}

	wlock(&(myhosti2a.i2amap[j]));

	p = myhosti2a.i2amap[j].myrangecurrent;
	if (p == nil) {
		netlog("allochostport: j %d, %V\n", j, myhosti2a.i2amap[j].nataddr);
		wunlock(&(myhosti2a.i2amap[j]));
		return -1;
	}

	iport = (int) nhgets(ns->iport);
	oport = (int) nhgets(ns->oport);
	proto = (int) ns->proto;
	netlog("allochostport: il %V %d og %V %d proto %d\n", ns->iaddr, iport, 
			ns->oaddr, oport, proto);
	for (;;) {
		if (p->nused < RANGESZ) {
			lo = p->lo;
			for (k = p->hi_port;
				myhosti2a.i2amap[j].p2s[k] != AVAILABLE;
				k = lo + ((k - lo + 1) % RANGESZ) ); 
			p->hi_port = k;
			p->nused++; 
			break;
		}
		else {
			if (p == myhosti2a.i2amap[j].myrangelast) 	// check end of list
				p = myhosti2a.i2amap[j].myrangefirst;
			else 
				p = p->next;
			
			if (p == myhosti2a.i2amap[j].myrangecurrent) {	// check if only range
				wunlock(&(myhosti2a.i2amap[j]));
				netlog("allochostport: alloc failed\n");
				return -1;
			}
			myhosti2a.i2amap[j].myrangecurrent = p;
		}
	}

	assert( (p->lo <= k) && (k < p->lo + RANGESZ) );
	netlog("allochostport: k %d\n", k);
	myhosti2a.i2amap[j].p2s[k] = hashind;
	ns->raddr = j % 256;
	ns->rport[1] = k % 256; k = k / 256;
	ns->rport[0] = k % 256; 
	ns->action = EMPTYBIT;  
	ns->expires[0] = 0;  
	ns->expires[1] = 0;  
	ns->expires[2] = 0;  
	memcpy(&sessiontable.table[hashind], ns, sizeof(*ns));	
	wunlock(&(myhosti2a.i2amap[j]));
	rport = (int) nhgets(ns->rport);
	raddr = (int) ns->raddr;
	netlog("allochostport: raddr %d %d k %d\n", raddr, rport, k);
	return 1;
}

/*	assume MAXBUFFSZ > 0; return value < 0 => no lock,
	else lock on br.buff[return value]
*/
Msgbuff*
currentBuff(void)
{
	int i = br.cp;
	Msgbuff *p;

	for (;;) {
		p = br.buffs + i;
		if (canqlock(p)) {
			if (p->current == CURRENT) {
				if ((p->index + sizeof(NATSession)) >= MAXBUFFSZ) {
					p->current = WRITE;
					qunlock(p);
				}
				else {
					return p;
				}
			}
		 	else if (p->current == FREE) {
				p->current = CURRENT;
				p->index = sizeof(NATHdr);
				br.cp = i;
				return p;
			}
			else qunlock(p);
		}
		i = (i+1) % MAXMSGBUFFS;
		if (i  == br.cp) return nil;
	}	
}

int
insert2buff(NATSession *ns)
{
	Msgbuff *p;

	p = currentBuff();
	if (p == nil)
		return -1;
	memcpy(p->buff + p->index, 
			ns, sizeof(NATSession));
	p->index += sizeof(NATSession);
	qunlock(p);
	return 1;
}

void
fillheader(int b)
{
	uchar *h = &(br.buffs[b].buff[0]);
	int pktsz = br.buffs[b].index - sizeof(NATHdr);
	//netlog("fillheader: pktsz %d\n", pktsz);
	assert( (0<=pktsz) && (pktsz <= MAXBUFFSZ));

	msgid++;
	*h = (natid >> 0) & 0xff; h++;
	*h = (msgid >> 24) & 0xff; h++;
	*h = (msgid >> 16) & 0xff; h++;
	*h = (msgid >> 8) & 0xff; h++;
	*h = (msgid >> 0) & 0xff; h++;
	*h = (RUpdate >> 0) & 0xff; h++;
	*h = (pktsz >> 24) & 0xff; h++;
	*h = (pktsz >> 16) & 0xff; h++;
	*h = (pktsz >> 8) & 0xff; h++;
	*h = (pktsz >> 0) & 0xff; h++;
	*h = 0xff; h++;
	*h = 0xff;
}

void
writepacket(int fd, int b)
{
	int version, sid, msgid = 0, mtype = 0, pktsz = 0, ns, 
		port = 0, proto = 0, action = 0, raddr, i;
	long expires = 0; 
	char addr[20];
	uchar *h = &(br.buffs[b].buff[0]), *hh;

	version = *h; h++;
	sid = *h; h++;
	msgid = (*h << 24) | (*(h+1) << 16) | (*(h+2) << 8) | (*(h+3) << 0); h += 4;
	mtype = *h; h++;
	pktsz = (*h << 24) | (*(h+1) << 16) | (*(h+2) << 8) | (*(h+3) << 0); h += 4;
	h++; // slack

	assert( (0<=pktsz) && (pktsz <= MAXBUFFSZ));
	ns = pktsz / sizeof(NATSession);
	fprint(fd, "NATHdr: version %d sid %d mesgid %d msgtype %s nmesg %d\n",
			version, sid, msgid, msgtypetab[mtype], ns);
	for (i=0; i<ns; i++) {
		sprint(addr, "%d.%d.%d.%d", *h, *(h+1), *(h+2), *(h+3));
		h += 4;
		port = (*h << 8) | (*(h+1) << 0);
		h += 2;
		proto = *h; 
		h++;
		fprint(fd, "I %s %d %d ", addr, port, proto);

		sprint(addr, "%d.%d.%d.%d", *h, *(h+1), *(h+2), *(h+3));
		h += 4;
		port = (*h << 8) | (*(h+1) << 0);
		h += 2;
		action = *h; 
		h++;
		fprint(fd, "O %s %d %d ", addr, port, action);

		port = (*h << 8) | (*(h+1) << 0);
		h += 2;
		raddr = *h; 
		h++;
		hh = &(myhosti2a.i2amap[raddr].nataddr[0]);
		sprint(addr, "%d.%d.%d.%d", *hh, *(hh+1), *(hh+2), *(hh+3));
		expires = ((*h << 20) | (*(h+1) << 12) | (*(h+2) << 4)) + EXPOFFSET;
		fprint(fd, "R %s %d %l \n", addr, port, expires);
	}
}

void 
writebuff(void)
{
	int start = (br.lw) % MAXMSGBUFFS;
	int lw = (start + 1) % MAXMSGBUFFS;
	int i = 0;
	for (;;) {
		qlock(&(br.buffs[lw]));
		if ((br.buffs[lw].current != FREE) || (i >= MAXMSGBUFFS)) 
			break;
		else {
			qunlock(&(br.buffs[lw]));
			lw = (lw + 1) % MAXMSGBUFFS; i++;
		}
	}
	//netlog("writebuff: lw %d\n", lw);
	fillheader(lw);
	writepacket(2, lw);
	qunlock(&(br.buffs[lw]));
	br.lw = lw;
	br.buffs[lw].current = FREE;	
}

uint 
a2ihash(uchar *add)
{
	uint j = add[0];
	
	j = 65559*j + add[1];	
	j = 65559*j + add[2];	
	j = 65559*j + add[3];	

	return j;
}

void 
inita2i(int nb)
{
	int i, tmp;

	for (i=0; i<A2ISZ; i++) {
		a2imap[i].ind = -1;
	}

	for (i=0; i<nb; i++) {
		tmp = a2ihash(myhosti2a.i2amap[i].nataddr) % A2ISZ;
		for (;;) {
			if (a2imap[tmp].ind == -1)
				break;
			tmp++; tmp %= A2ISZ;
		}
		a2imap[tmp].ind = i;
		memcpy(&(a2imap[tmp].nataddr[0]), &(myhosti2a.i2amap[i].nataddr[0]), 4);
		netlog("inita2i: rlook %d\t %V\n", a2imap[tmp].ind, &(a2imap[tmp].nataddr[0]));
	}
}

uchar 
addr2ind(uchar *addr)
{
	uint tmp;

	tmp = a2ihash(addr) % A2ISZ;
	for (;;) {
		if ((a2imap[tmp].ind == -1) ||
			((a2imap[tmp].nataddr[0] == addr[0]) && 
			 (a2imap[tmp].nataddr[1] == addr[1]) && 
			 (a2imap[tmp].nataddr[2] == addr[2]) && 
			 (a2imap[tmp].nataddr[3] == addr[3])) )
			break;
		tmp = (tmp+1) % A2ISZ;
	}
	return a2imap[tmp].ind;
}

int
ins2outs(NATSession *ns)
{
	return ( (RP^2*(ns->iaddr[0])+RP*(ns->iaddr[1])+(ns->iaddr[2])) % numextaddr );
}

void
initi2a(char *fname)
{
	int i, j, k;
	char* err1;
	char *line, *tok1, *tok2;
	uchar addr[4];

	bp = Bopen(fname, OREAD);
	assert(bp);

	while((line = Brdline(bp, '\n')) && (line[0] == '#')) ;
	if (!line) {
		fprint(2, "1 Bad Nat init file %s\n", fname);
		exits("init");
	}
	line[Blinelen(bp) - 1] = 0;
	tok1 = strtok(line,":");
	tok2 = strtok(0,":");
	if ((strcmp(tok1,"NUMEXTADDR") != 0) || !(numextaddr = atoi(tok2))) {
		fprint(2, "2 Bad Nat init file %s %s %s\n", fname, tok1, tok2);
		exits("init");
	}
	if (numextaddr > MAXEXTADDR) {
		fprint(2, "3 Bad Nat init data in %s; can support max %d ext addrs\n", fname, MAXEXTADDR);
		exits("init");
	}
	for (i=0; i<numextaddr; i++) {
		if (!(line = Brdline(bp, '\n'))) {;
			fprint(2, "4 Bad Nat init file %s\n", fname);
			exits("init");
		}
		line[Blinelen(bp) - 1] = 0;
		err1 = v4parseip(addr, line);
		memcpy(&(myhosti2a.i2amap[i].nataddr[0]), addr, 4);
		netlog("initi2a: ext addr %d\t %V\n", i, &(myhosti2a.i2amap[i].nataddr[0]));
	}

	while((line = Brdline(bp, '\n')) && (line[0] == '#')) ;
	if (!line) {
		fprint(2, "Bad Nat init file %s\n", fname);
		exits("init");
	}
	line[Blinelen(bp) - 1] = 0;
	tok1 = strtok(line,":");
	tok2 = strtok(0,":");
	if ((strcmp(tok1,"NUMNATS") != 0) || !(numnats = atoi(tok2))) {
		fprint(2, "Bad Nat init file %s\n", fname);
		exits("init");
	}
	if (numnats >= MAXNUMNATS) {
		fprint(2, "Bad Nat init data in %s; can support max %d nat boxes\n", fname, MAXNUMNATS);
		exits("init");
	}

	fprint(2, "Initialized data from file %s\n", fname);
	Bterm(bp);
}

/*

Msgbuff* 
circinitM(Msgbuffhd *mh, int n)
{
	int i; 
	Msgbuff *h, *t, *m;

	assert(n>0);

	m = allocM(mh);
	if (!m)
		return nil;
	h = m; t = m; 
	for (i=1; i<n; i++) {
		m = allocM(mh);
		if (!m)
			return nil;
		m->prev = t;
		t->next = m;
		t = m;
	}
	t->next = h;
	h->prev = t;
	return h;
}


void 
appendP(Owner *ow, Portrange *pr)
{
	assert(pr);

	pr->prev = ow->porttail;
	pr->next = nil;
	if (ow->porthead == nil)
		ow->porthead = pr;
	else 
		ow->porttail->next = pr;
	ow->porttail = pr;		
	ow->nranges++;
	ow->nconns += pr->nused;
}

void
initportalloc(void)
{
	int i, j, k;
	Portrange *p, *q = nil;

	for (i=0; i<=MAXNUMNATS; i++) {
		for (j=0; j<MAXEXTADDR; j++) {
			memcpy(allhosts.hosts[i].i2amap[j].nataddr,...,4);
			allhosts.hosts[i].i2amap[j].myrangefirst = nil;
			allhosts.hosts[i].i2amap[j].myrangelast = nil;
			allhosts.hosts[i].i2amap[j].nconns = 0;
			allhosts.hosts[i].i2amap[j].nranges = 0;
			allhosts.hosts[i].i2amap[j].lastreq = 0;
			allhosts.hosts[i].i2amap[j].p2s = nil;
		}
	}

	for (j=0; j<MAXEXTADDR; j++) {
		p = (Portrange*) malloc(sizeof(Portrange));
		p->lo = MID_PORT;
		p->hi_port = p->lo;
		p->nused = 0;
		p->prev = nil;
		allhosts.hosts[0].i2amap[j].myrangefirst = p;
		allhosts.hosts[0].i2amap[j].myrangecurrent = p;
		for (k=MID_PORT+RANGESZ; k+RANGESZ<=HI_PORT; k = k+RANGESZ) {
			q = (Portrange*) malloc(sizeof(Portrange));
			assert(q!=nil);
			q->lo = MID_PORT;
			q->hi_port = q->lo;
			q->nused = 0;
			q->prev = p;
			p->next = q;
			p = q;
		}
		q->next = nil;
		allhosts.hosts[0].i2amap[j].myrangelast = p;
	}
}


*/
