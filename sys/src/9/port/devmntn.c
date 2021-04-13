/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

/*
 * References are managed as follows:
 * The channel to the server - a network connection or pipe - has one
 * reference for every Chan open on the server.  The server channel has
 * c->mux set to the Mnt used for muxing control to that server.  Mnts
 * have no reference count; they go away when c goes away.
 * Each channel derived from the mount point has mchan set to c,
 * and increfs/decrefs mchan to manage references on the server
 * connection.
 */

#define MAXRPC (IOHDRSZ + 128 * 1024)

struct Mntrpc {
	Chan *c;	 /* Channel for whom we are working */
	Mntrpc *list;	 /* Free/pending list */
	Fcall request;	 /* Outgoing file system protocol message */
	Fcall reply;	 /* Incoming reply */
	Mnt *m;		 /* Mount device during rpc */
	Rendez r;	 /* Place to hang out */
	u8 *rpc;	 /* I/O Data buffer */
	u32 rpclen;	 /* len of buffer */
	Block *b;	 /* reply blocks */
	char done;	 /* Rpc completed */
	u64 stime;	 /* start time for mnt statistics */
	u32 reqlen; /* request length for mnt statistics */
	u32 replen; /* reply length for mnt statistics */
	Mntrpc *flushed; /* message this one flushes */
};

enum {
	TAGSHIFT = 5, /* u32 has to be 32 bits */
	TAGMASK = (1 << TAGSHIFT) - 1,
	NMASK = (64 * 1024) >> TAGSHIFT,
};

struct Mntalloc {
	Lock Lock;
	Mnt *list;    /* Mount devices in use */
	Mnt *mntfree; /* Free list */
	Mntrpc *rpcfree;
	int nrpcfree;
	int nrpcused;
	u32 id;
	u32 tagmask[NMASK];
} mntalloc;

Mnt *mntchk(Chan *);
void mntdirfix(u8 *, Chan *);
Mntrpc *mntflushalloc(Mntrpc *, u32);
void mntflushfree(Mnt *, Mntrpc *);
void mntfree(Mntrpc *);
void mntgate(Mnt *);
void mntpntfree(Mnt *);
void mntqrm(Mnt *, Mntrpc *);
Mntrpc *mntralloc(Chan *, u32);
i32 mntrdwr(int, Chan *, void *, i32, i64);
int mntrpcread(Mnt *, Mntrpc *);
void mountio(Mnt *, Mntrpc *);
void mountmux(Mnt *, Mntrpc *);
void mountrpc(Mnt *, Mntrpc *);
int rpcattn(void *);
Chan *mntchann(void);

extern char Esbadstat[];
extern char Enoversion[];

void (*mntstats)(int, Chan *, u64, u32);

static void
mntreset(void)
{
	mntalloc.id = 1;
	mntalloc.tagmask[0] = 1;		    /* don't allow 0 as a tag */
	mntalloc.tagmask[NMASK - 1] = 0x80000000UL; /* don't allow NOTAG */
	fmtinstall('F', fcallfmt);
	fmtinstall('D', dirfmt);
	/* We can't install %M since eipfmt does and is used in the kernel [sape] */

	if(mfcinit != nil)
		mfcinit();
}

static Chan *
mntattach(char *muxattach)
{
	return mntattachversion(muxattach, "9P2000.L", Tlattach, mntchann);
}

static Walkqid *
mntwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Proc *up = externup();
	int i, alloc;
	Mnt *mnt;
	Mntrpc *r;
	Walkqid *wq;

	if(nc != nil)
		print("mntwalk: nc != nil\n");
	if(nname > MAXWELEM)
		error("devmnt: too many name elements");
	alloc = 0;
	wq = smalloc(sizeof(Walkqid) + (nname - 1) * sizeof(Qid));
	if(waserror()){
		if(alloc && wq->clone != nil)
			cclose(wq->clone);
		free(wq);
		return nil;
	}

	alloc = 0;
	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(nc == nil){
		nc = devclone(c);
		/*
		 * Until the other side accepts this fid,
		 * we can't mntclose it.
		 * nc->dev remains nil for now.
		 */
		alloc = 1;
	}
	wq->clone = nc;

	if(waserror()){
		mntfree(r);
		nexterror();
	}
	r->request.type = Twalk;
	r->request.fid = c->fid;
	r->request.newfid = nc->fid;
	r->request.nwname = nname;
	memmove(r->request.wname, name, nname * sizeof(char *));

	mountrpc(mnt, r);

	if(r->reply.nwqid > nname)
		error("too many QIDs returned by walk");
	if(r->reply.nwqid < nname){
		if(alloc)
			cclose(nc);
		wq->clone = nil;
		if(r->reply.nwqid == 0){
			free(wq);
			wq = nil;
			goto Return;
		}
	}

	/* move new fid onto mnt device and update its qid */
	if(wq->clone != nil){
		if(wq->clone != c){
			wq->clone->dev = c->dev;
			//if(wq->clone->dev != nil)	//XDYNX
			//	devtabincr(wq->clone->dev);
			wq->clone->mchan = c->mchan;
			incref(&c->mchan->r);
		}
		if(r->reply.nwqid > 0)
			wq->clone->qid = r->reply.wqid[r->reply.nwqid - 1];
	}
	wq->nqid = r->reply.nwqid;
	for(i = 0; i < wq->nqid; i++)
		wq->qid[i] = r->reply.wqid[i];

Return:
	poperror();
	mntfree(r);
	poperror();
	return wq;
}

// Here is the other place the mistakes in 9P2000.L bite us.
// Putting all this system-dependent stuff in 9P is nasty but we can take it.
// size[4] Tgetattr tag[2] fid[4] request_mask[8]
// size[4] Rgetattr tag[2] valid[8] qid[13] mode[4] uid[4] gid[4] nlink[8]
//                  rdev[8] size[8] blksize[8] blocks[8]
//                  atime_sec[8] atime_nsec[8] mtime_sec[8] mtime_nsec[8]
//                  ctime_sec[8] ctime_nsec[8] btime_sec[8] btime_nsec[8]
//                  gen[8] data_version[8]
//
// 9P:
// size[2]    total byte count of the following data
// type[2]    for kernel use
// dev[4]     for kernel use
// qid.type[1]the type of the file (directory, etc.), represented as a bit vector corresponding to the high 8 bits of the file's mode word.
// qid.vers[4]version number for given path
// qid.path[8]the file server's unique identification for the file
// mode[4]   permissions and flags
// atime[4]   last access time
// mtime[4]   last modification time
// length[8]   length of file in bytes
// name[ s ]   file name; must be / if the file is the root directory of the server
// uid[ s ]    owner name
// gid[ s ]    group name
// muid[ s ]   name of the user who last modified the file
static i32
mntstat(Chan *c, u8 *dp, i32 n)
{
	Proc *up = externup();
	Mnt *mnt;
	Mntrpc *r;
	usize nstat, nl;
	char *name;
	u8 *buf;

	if(n < BIT16SZ)
		error(Eshortstat);
	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(waserror()){
		mntfree(r);
		nexterror();
	}
	r->request.type = Tgetattr;
	r->request.fid = c->fid;
	mountrpc(mnt, r);
	name = chanpath(c);
	nl = strlen(name);
	buf = (u8 *)r->reply.data;
	nstat = STATFIXLEN + nl + 16;	     // max uid print + 0 null terminator
	// This looks crazy, right? It's how you tell the
	// caller there is more data to read. But you give it
	// no data. That avoids the mess of partial reads of
	// stat structs. The calling layer will likely hide the
	// mess under the rock.
	if(nstat > n){
		nstat = BIT16SZ;
		PBIT16(dp, nstat - 2);
	} else {
		// N.B. STATFIXLEN includes the strings lengths
		// AND the leading 16-bit size. The leading 16-bit size
		// is of a packet with 0 length strings is STATFIXLEN - 2
		// The offsets below include the leading 16-bit size
		u64 _64;
		u32 _32;
		int ul;
		// This is an offset for where the strings go.
		int o = STATFIXLEN - 4 * BIT16SZ + BIT16SZ;
		char *base = strrchr(name, '/');
		// If there is on no /, use the name, else use the
		// part of the string after the slash. This works
		// even if the name ends in /.
		if(base != nil)
			name = base + 1;
		nl = strlen(name);
		//hexdump(buf, 149);
		memset(dp, 0, n);
		// The Qid format is compatible, so just copy it.
		memmove(dp + 8, buf + 8, sizeof(Qid));
		// mode. It needs adjustment for DMDIR.
		_32 = GBIT32(buf + 21);
		if(buf[8] & QTDIR){
			_32 |= DMDIR;
		}
		PBIT32(dp + 21, _32);

		// The time wire format is compatible but sadly the sizes
		// differ. We have to get it and put it.
		// Plan 9 has a y2032 problem!
		_64 = GBIT64(buf + 48);
		PBIT32(dp + 25, (u32)_64);
		_64 = GBIT64(buf + 64);
		PBIT32(dp + 29, (u32)_64);

		// file length. Compatible bit encoding.
		memmove(dp + 33, buf + 49, sizeof(u64));

		// put the name as a string.
		sprint((char *)dp + o, "%s", name);
		// There are four BIT16SZ lengths at the
		// end. If they are > 0 then the strings are interspersed
		// between them.
		PBIT16(dp + o - 2, nl);
		o += nl + BIT16SZ;

		// UID
		_32 = GBIT32(buf + 25);
		// The next name is at o
		sprint((char *)dp + o, "%d", _32);
		// strlen safe as we zero the buffer?
		ul = strlen((char *)dp + o);
		PBIT16(dp + o - 2, ul);
		o += ul + BIT16SZ;
		nstat = o;

		// GID
		_32 = GBIT32(buf + 29);
		// The next name is at o
		sprint((char *)dp + o, "%d", _32);
		// strlen safe as we zero the buffer?
		ul = strlen((char *)dp + o);
		PBIT16(dp + o - 2, ul);
		o += ul;
		nstat = o;

		// MUID is empty
		nstat += 1 * BIT16SZ;

		// nstat includes whole stat size, but
		// the size in the record does not.
		// Subtract BIT16SZ for that reason.
		PBIT16(dp, nstat - BIT16SZ);

		validstat(dp, nstat);
		mntdirfix(dp, c);
		//hexdump(dp,nstat);
	}
	mntfree(r);
	poperror();

	return nstat;
}

// TODO: can we merge with the one in devmnt.c
static Chan *
mntopencreate(int type, Chan *c, char *name, int omode, int perm)
{
	Proc *up = externup();
	Mnt *mnt;
	Mntrpc *r;

	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(waserror()){
		mntfree(r);
		nexterror();
	}
	r->request.type = type;
	r->request.fid = c->fid;
	// 9P2000.L -- another mess?
	if(omode == OEXEC)
		r->request.mode = OREAD;
	else
		r->request.mode = omode;

	if(type == Tcreate){
		r->request.perm = perm;
		r->request.name = name;
	}
	mountrpc(mnt, r);

	c->qid = r->reply.qid;
	c->offset = 0;
	c->mode = openmode(omode);
	c->iounit = r->reply.iounit;
	c->writeoffset = 0;
	c->buffend = 0;
	c->writebuff = nil;
	c->buffsize = mnt->msize;
	if(c->iounit == 0 || c->iounit > mnt->msize - IOHDRSZ)
		c->iounit = mnt->msize - IOHDRSZ;
	c->flag |= COPEN;
	poperror();
	mntfree(r);

	if(c->flag & CCACHE)
		mfcopen(c);

	return c;
}

static Chan *
mntopen(Chan *c, int omode)
{
	// 9P2000.L differentiates by file type,
	// unlike Plan 9. We have to treat, e.g.,
	// directories and files differently (!).
	// Using different system calls for files
	// and directories is a great idea, said
	// no one ever.
	u8 t = c->qid.type;
	if(t & QTDIR){
	} else if(t){
		error("only dirs or regular files");
	}
	return mntopencreate(Tlopen, c, nil, omode, 0);
}

static void
mntcreate(Chan *c, char *name, int omode, int perm)
{
	mntopencreate(Tlcreate, c, name, omode, perm);
}

static void
mntclunk(Chan *c, int t)
{
	Proc *up = externup();
	Mnt *mnt;
	Mntrpc *r;

	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(waserror()){
		mntfree(r);
		nexterror();
	}

	r->request.type = t;
	r->request.fid = c->fid;
	mountrpc(mnt, r);
	mntfree(r);
	poperror();
}

static void
mntclose(Chan *c)
{
	if(c->buffend > 0){
		mntrdwr(Twrite, c, c->writebuff, c->buffend, c->writeoffset);
	}
	c->buffend = 0;
	free(c->writebuff);
	c->writebuff = nil;
	mntclunk(c, Tclunk);
}

static void
mntremove(Chan *c)
{
	mntclunk(c, Tremove);
}

static i32
mntwstat(Chan *c, u8 *dp, i32 n)
{
	Proc *up = externup();
	Mnt *mnt;
	Mntrpc *r;

	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(waserror()){
		mntfree(r);
		nexterror();
	}
	r->request.type = Twstat;
	r->request.fid = c->fid;
	r->request.nstat = n;
	r->request.stat = dp;
	mountrpc(mnt, r);
	poperror();
	mntfree(r);
	return n;
}

static i32
mntread(Chan *c, void *buf, i32 n, i64 off)
{
	Proc *up = externup();
	u8 *p, *e;
	int nc, cache, isdir;

	isdir = 0;
	cache = c->flag & CCACHE;
	if(c->qid.type & QTDIR){
		cache = 0;
		isdir = 1;
	}

	p = buf;
	if(cache){
		nc = mfcread(c, buf, n, off);
		if(nc > 0){
			n -= nc;
			if(n == 0)
				return nc;
			p += nc;
			off += nc;
		}
		n = mntrdwr(Tread, c, p, n, off);
		mfcupdate(c, p, n, off);
		return n + nc;
	}

	// Flush if we're reading this file.  Would be nice to see if
	// read could be satisfied from buffer.
	if(c->buffend > 0){
		mntrdwr(Twrite, c, c->writebuff, c->buffend, c->writeoffset);
	}
	// Long story.
	// Reads on directories have taken a long path. In the original 45 years ago Unix I cut my teeth on,
	// one could open a file and read it. If it were a directory, data was returned as 14 bytes of name
	// and 2 bytes of inumber.
	// There were obvious problems here, the most basic being that the on-disk format was directly
	// returned to a program. Hence readdir, which was an attempt at an abstraction: it returned
	// the names and little else. So what happens in, say, ls -l? it looks something like this:
	// entries := readdir()
	// for each entry, stat() the entry.
	// Now it was well known in the 1980s that this is a disaster on a network: there is at
	// least one packet for each directory entry.
	// For directory reads, it's much, much better to grow the amount
	// of data in a packet, even data not used, than to add lots of packets when more
	// info is needed. Measurement has shown this over and over. Hence NFS3 READDIRPLUS.
	// Plan 9 and 9P, circa 1991, introduced a major improvement:
	// directories are like files, and one can now read them again. This is really important:
	// o means you can import a directory over 9p and just read from it
	// o removes a need for a readdir system call
	// o avoids a special op in 9p to read directories
	// o no need to have the kernel unpack the information, just relay to user space
	// Were readdir brought into 9P, the addition would impact 9P, the kernel, all user
	// libraries, and all user programs, in not very good ways.
	// This is one of the subtle design elements of Plan 9 that not even many Plan 9 users
	// think about.
	// Directory reads return stat records, which are an endian- and word-length independent
	// representation of the file metadata.
	// 9p supports this kind of read directly.
	// A read of a directory on Plan 9 returns metadata, in other words, so there are no
	// extra stats per entry as in the Unix model.
	// Unix and Linux, sadly, retained the old model. A Linux readdir returns dirents.
	// If you want to know more, stat each entry. We're back to the bad old days.
	// What's this look like on 9P?
	// 1. read the dir
	// 2. for each element, walk to it.
	// 3. stat it.
	// 9P2000.L had a chance to fix this in a way that would have worked well for Plan 9
	// as well as some hypothetical future Linux with a more powerful readdir model,
	// but sadly it instead baked in those limitations.
	// We don't want to reflect that mess to user mode in Harvey, so we have to
	// convert the 9P2000.L information to plan 9 stat structs, losing information
	// in the process.
	// 9P2000.L stat information looks like this:
	// qid[13] offset[8] type[1] name[s]
	//
	// Plan 9 stat looks like this:
	//
	// size[2]    total byte count of the following data
	// type[2]    for kernel use
	// dev[4]     for kernel use
	// qid.type[1] the type of the file (directory, etc.), represented as a bit vector corresponding to the high 8 bits of the file's mode word.
	// qid.vers[4] version number for given path
	// qid.path[8] the file server's unique identification for the file
	// mode[4]   permissions and flags
	// atime[4]   last access time
	// mtime[4]   last modification time
	// length[8]   length of file in bytes
	// name[ s ]   file name; must be / if the file is the root directory of the server
	// uid[ s ]    owner name
	// gid[ s ]    group name
	// muid[ s ]   name of the user who last modified the file
	//
	// Important point here: note that in 9P2000.L, they had to add a stat structure.
	// There is no such thing needed in 9P2000; it's the province of the kernel, not
	// the protocol. Hopefully this illustrates the differences in the model.
	//
	// The size of a 9P stat struct is:
	// ../../../include/fcall.h:#define STATFIXLEN     (BIT16SZ+QIDSZ+5*BIT16SZ+4*BIT32SZ+1*BIT64SZ)    amount of fixed length data in a stat buffer */
	//	size + qidsz + what is that 5? type and 4 sizes for strings + dev,mod,atime,mtime + length
	// Note that for now we can only fill in the name; to better fill this in we'll need to do the
	// walk/stat operation.

	// Rough algorithm:
	// zero buf. Useful so that the default length strings for uid, gid, muid will be empty.
	// compute length of record.
	// copy the name out. copy the qid out. copy the record size out. Continue.
	memset(buf, 0, n);
	if(isdir){
		u8 *b = buf;

		int tot = 0;
		int a = n / 8;
		if(a == 0)
			a = 128;
		u8 *m = mallocz(n, 0);
		if(waserror()){
			free(m);
		}
		memset(m, 0, n);
		n = mntrdwr(Treaddir, c, m, a, off);
		if(n == 0){
			free(m);
			poperror();
			return tot;
		}
		if(n < 4)
			error(Esbadstat);
		// TODO: we're going to have to stat each returned
		// entry because this 9P2000.L design requires us to.
		// Why is it that way? To match how Linux works.
		// And Linux works the way Unix worked in 1972.
		// Fantastic.
		p = m;
		e = &m[n];
		while(p < e){
			int namesz = GBIT16(p + 22);
			// Move the Qid. No need to change it; 9P2000.L and 9P Qids are the same.
			memmove(b + 8, p, sizeof(Qid));
			// Move the name, including the name's length.
			memmove(b + 41, p + 22, namesz + BIT16SZ);
			// Story the whole record length, not including the
			// the 16-bit length field.
			// "size[2]    total byte count of the following data"
			PBIT16(b, STATFIXLEN + namesz - BIT16SZ);
			// advance p by the size of the 9P2000.L record
			p += 24 + namesz;
			// advance b by the size of the Plan 9 stat record
			b += STATFIXLEN + namesz;
		}
		if(p != e)
			error(Esbadstat);
		n = b - (u8 *)buf;
		free(m);
		poperror();
	} else {
		n = mntrdwr(Tread, c, buf, n, off);
	}

	return n;
}

static i32
mntwrite(Chan *c, void *buf, i32 n, i64 off)
{
	int result = n;
	int offset = 0;
	if(c->writebuff == nil){
		c->writebuff = (unsigned char *)malloc(c->buffsize);
		if(c->writebuff == nil){
			print("devmntn: write buffer allocation of %d bytes failed\n", c->buffsize);
			return mntrdwr(Twrite, c, buf, n, off);
		}
	}
	if(off != c->writeoffset + c->buffend){
		// non-continuous write - flush cached data
		mntrdwr(Twrite, c, c->writebuff, c->buffend, c->writeoffset);
		c->buffend = 0;
		c->writeoffset = off;
	}
	while(n + c->buffend >= c->buffsize){
		offset = c->mux->msize - c->buffend;
		n -= offset;
		memmove(&c->writebuff[c->buffend], buf, offset);
		mntrdwr(Twrite, c, c->writebuff, c->mux->msize, c->writeoffset);
		c->writeoffset += offset;
	}
	memmove(&c->writebuff[c->buffend], buf + offset, n);
	c->buffend += n;
	return result;
}

Chan *
mntchann(void)
{
	Chan *c;

	c = devattach('N', 0);
	lock(&mntalloc.Lock);
	c->devno = mntalloc.id++;
	unlock(&mntalloc.Lock);

	if(c->mchan)
		panic("mntchan non-zero %#p", c->mchan);
	return c;
}

Dev mntndevtab = {
	.dc = 'N',
	.name = "mntn",

	.reset = mntreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = mntattach,
	.walk = mntwalk,
	.stat = mntstat,
	.open = mntopen,
	.create = mntcreate,
	.close = mntclose,
	.read = mntread,
	.bread = devbread,
	.write = mntwrite,
	.bwrite = devbwrite,
	.remove = mntremove,
	.wstat = mntwstat,
};
