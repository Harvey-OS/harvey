#include "stdinc.h"

#include "9.h"
#include "dat.h"
#include "fns.h"

enum {
	NConInit	= 128,
	NMsgInit	= 20,
	NMsgProcInit	= 4,
	NMsizeInit	= 8192+IOHDRSZ,
};

static struct {
	VtLock*	lock;
	Con**	con;			/* arena */
	int	ncon;			/* how many in arena */
	int	hi;			/* high watermark */
	int	cur;			/* hint for allocation */

	u32int	msize;
} cbox;

static struct {
	VtLock*	lock;

	Msg*	free;
	VtRendez* alloc;

	Msg*	head;
	Msg*	tail;
	VtRendez* work;

	int	maxmsg;
	int	nmsg;
	int	maxproc;
	int	nproc;

	u32int	msize;			/* immutable */
} mbox;

static void
msgFree(Msg* m)
{
	vtLock(mbox.lock);
	if(mbox.nmsg > mbox.maxmsg){
		vtMemFree(m->data);
		vtMemFree(m);
		mbox.nmsg--;
		vtUnlock(mbox.lock);
		return;
	}
	m->next = mbox.free;
	mbox.free = m;
	if(m->next == nil)
		vtWakeup(mbox.alloc);
	vtUnlock(mbox.lock);
}

static void
conFree(Con* con)
{
	if(con->fd >= 0){
		close(con->fd);
		con->fd = -1;
	}

	assert(con->version == nil);
	assert(con->mhead == nil);
	assert(con->nmsg == 0);
	assert(con->nfid == 0);
	assert(con->state == CsMoribund);

	con->state = CsDead;
}

static void
msgProc(void*)
{
	int n;
	Msg *m;
	char *e;
	Con *con;

	vtThreadSetName("msg");

	vtLock(mbox.lock);
	while(mbox.nproc <= mbox.maxproc){
		while(mbox.head == nil)
			vtSleep(mbox.work);
		m = mbox.head;
		mbox.head = m->next;
		m->next = nil;

		e = nil;

		con = m->con;
		vtLock(con->lock);
		assert(con->state != CsDead);
		con->nmsg++;

		if(m->t.type == Tversion){
			con->version = m;
			con->state = CsDown;
			while(con->mhead != nil)
				vtSleep(con->active);
			assert(con->state == CsDown);
			if(con->version == m){
				con->version = nil;
				con->state = CsInit;
			}
			else
				e = "Tversion aborted";
		}
		else if(con->state != CsUp)	
			e = "connection not ready";

		/*
		 * Add Msg to end of active list.
		 */
		if(con->mtail != nil){
			m->prev = con->mtail;
			con->mtail->next = m;
		}
		else{
			con->mhead = m;
			m->prev = nil;
		}
		con->mtail = m;
		m->next = nil;

		vtUnlock(con->lock);
		vtUnlock(mbox.lock);

		/*
		 * Dispatch if not error already.
		 */
		m->r.tag = m->t.tag;
		if(e == nil && !(*rFcall[m->t.type])(m))
			e = vtGetError();
		if(e != nil){
			m->r.type = Rerror;
			m->r.ename = e;
		}
		else
			m->r.type = m->t.type+1;

		vtLock(con->lock);
		/*
		 * Remove Msg from active list.
		 */
		if(m->prev != nil)
			m->prev->next = m->next;
		else
			con->mhead = m->next;
		if(m->next != nil)
			m->next->prev = m->prev;
		else
			con->mtail = m->prev;
		m->prev = m->next = nil;
		if(con->mhead == nil)
			vtWakeup(con->active);

		if(Dflag)
			fprint(2, "msgProc: r %F\n", &m->r);

		if((con->state == CsNew || con->state == CsUp) && !m->flush){
			/*
			 * TODO: optimise this copy away somehow for
			 * read, stat, etc.
			 */
			assert(n = convS2M(&m->r, con->data, con->msize));
			if(write(con->fd, con->data, n) != n){
				if(con->fd >= 0){
					close(con->fd);
					con->fd = -1;
				}
			}
		}

		con->nmsg--;
		if(con->state == CsMoribund && con->nmsg == 0){
			vtUnlock(con->lock);
			conFree(con);
		}
		else
			vtUnlock(con->lock);

		vtLock(mbox.lock);
		m->next = mbox.free;
		mbox.free = m;
		if(m->next == nil)
			vtWakeup(mbox.alloc);
	}
	mbox.nproc--;
	vtUnlock(mbox.lock);
}

static void
conProc(void* v)
{
	Msg *m;
	Con *con;
	int eof, fd, n;

	vtThreadSetName("con");

	con = v;
	if(Dflag)
		fprint(2, "conProc: con->fd %d\n", con->fd);
	fd = con->fd;
	eof = 0;

	vtLock(mbox.lock);
	while(!eof){
		while(mbox.free == nil){
			if(mbox.nmsg >= mbox.maxmsg){
				vtSleep(mbox.alloc);
				continue;
			}
			m = vtMemAllocZ(sizeof(Msg));
			m->data = vtMemAlloc(mbox.msize);
			m->msize = mbox.msize;
			mbox.nmsg++;
			mbox.free = m;
			break;
		}
		m = mbox.free;
		mbox.free = m->next;
		m->next = nil;
		vtUnlock(mbox.lock);

		m->con = con;
		m->flush = 0;

		while((n = read9pmsg(fd, m->data, con->msize)) == 0)
			;
		if(n < 0){
			m->t.type = Tversion;
			m->t.fid = NOFID;
			m->t.tag = NOTAG;
			m->t.msize = con->msize;
			m->t.version = "9PEoF";
			eof = 1;
		}
		else if(convM2S(m->data, n, &m->t) != n){
			if(Dflag)
				fprint(2, "conProc: convM2S error: %s\n",
					con->name);
			msgFree(m);
			vtLock(mbox.lock);
			continue;
		}
		if(Dflag)
			fprint(2, "conProc: t %F\n", &m->t);

		vtLock(mbox.lock);
		if(mbox.head == nil){
			mbox.head = m;
			if(!vtWakeup(mbox.work) && mbox.nproc < mbox.maxproc){
				if(vtThread(msgProc, nil) > 0)
					mbox.nproc++;
			}
			vtWakeup(mbox.work);
		}
		else
			mbox.tail->next = m;
		mbox.tail = m;
	}
	vtUnlock(mbox.lock);
}

Con*
conAlloc(int fd, char* name)
{
	Con *con;
	int cur, i;

	vtLock(cbox.lock);
	cur = cbox.cur;
	for(i = 0; i < cbox.hi; i++){
		/*
		 * Look for any unallocated or CsDead up to the
		 * high watermark; cur is a hint where to start.
		 * Wrap around the whole arena.
		 */
		if(cbox.con[cur] == nil || cbox.con[cur]->state == CsDead)
			break;
		if(++cur >= cbox.hi)
			cur = 0;
	}
	if(i >= cbox.hi){
		/*
		 * None found.
		 * If the high watermark is up to the limit of those
		 * allocated, increase the size of the arena.
		 * Bump up the watermark and take the next.
		 */
		if(cbox.hi >= cbox.ncon){
			cbox.con = vtMemRealloc(cbox.con,
					(cbox.ncon+NConInit)*sizeof(Con*));
			memset(&cbox.con[cbox.ncon], 0, NConInit*sizeof(Con*));
			cbox.ncon += NConInit;
		}
		cur = cbox.hi++;
	}

	/*
	 * Do one-time initialisation if necessary.
	 * Put back a new hint.
	 * Do specific initialisation and start the proc.
	 */
	con = cbox.con[cur];
	if(con == nil){
		con = vtMemAllocZ(sizeof(Con));
		con->lock = vtLockAlloc();
		con->data = vtMemAlloc(cbox.msize);
		con->msize = cbox.msize;
		con->active = vtRendezAlloc(con->lock);
		con->fidlock = vtLockAlloc();
		cbox.con[cur] = con;
	}
	assert(con->mhead == nil);
	assert(con->nmsg == 0);
	assert(con->fhead == nil);
	assert(con->nfid == 0);

	con->state = CsNew;

	if(++cur >= cbox.hi)
		cur = 0;
	cbox.cur = cur;

	con->fd = fd;
	if(con->name != nil){
		vtMemFree(con->name);
		con->name = nil;
	}
	if(name != nil)
		con->name = vtStrDup(name);
	con->aok = 0;
	vtUnlock(cbox.lock);

	if(vtThread(conProc, con) < 0){
		conFree(con);
		return nil;
	}

	return con;
}

static int
cmdMsg(int argc, char* argv[])
{
	char *p;
	int maxmsg, maxproc;
	char *usage = "usage: msg [-m nmsg] [-p nproc]";

	maxmsg = maxproc = 0;

	ARGBEGIN{
	default:
		return cliError(usage);
	case 'm':
		p = ARGF();
		if(p == nil)
			return cliError(usage);
		maxmsg = strtol(argv[0], &p, 0);
		if(maxmsg <= 0 || p == argv[0] || *p != '\0')
			return cliError(usage);
		break;
	case 'p':
		p = ARGF();
		if(p == nil)
			return cliError(usage);
		maxproc = strtol(argv[0], &p, 0);
		if(maxproc <= 0 || p == argv[0] || *p != '\0')
			return cliError(usage);
		break;
	}ARGEND
	if(argc)
		return cliError(usage);

	vtLock(mbox.lock);
	if(maxmsg)
		mbox.maxmsg = maxmsg;
	maxmsg = mbox.maxmsg;
	if(maxproc)
		mbox.maxproc = maxproc;
	maxproc = mbox.maxproc;
	vtUnlock(mbox.lock);

	consPrint("\tmsg -m %d -p %d\n", maxmsg, maxproc);

	return 1;
}

void
procInit(void)
{
	mbox.lock = vtLockAlloc();
	mbox.alloc = vtRendezAlloc(mbox.lock);
	mbox.work = vtRendezAlloc(mbox.lock);

	mbox.maxmsg = NMsgInit;
	mbox.maxproc = NMsgProcInit;
	mbox.msize = NMsizeInit;

	cliAddCmd("msg", cmdMsg);

	cbox.lock = vtLockAlloc();
	cbox.con = nil;
	cbox.ncon = 0;
	cbox.msize = NMsizeInit;
}
