/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// dev9p.c ('#9'): a virtio9p protocol translation driver to use QEMU's built-in 9p.


#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"virtio_ring.h"

#include	"virtio_config.h"
#include	"virtio_9p.h"
#include	"virtio_pci.h"

#include	"virtio_lib.h"

char* strdup(char *s);

// We use this version string to communicate with QEMU virtio-9P.

#define    VERSION9PU       "9P2000.u"

// Same as in devmnt.c

#define MAXRPC (IOHDRSZ+128*1024)

// Buffer size

#define BUFSZ (8192 + IOHDRSZ)

// Raise this error when any of the non-implemented functions is called.

#define Edonotcall(f) "Function " #f " should not be called for this device"

extern Dev v9pdevtab;

// Array of defined 9p mounts and their number

static uint32_t nv9p;

static Vqctl **v9ps;

// Flag of one-time initailzation

static int initdone;

// A structure to hold a pair of buffer references. Virtio-9p requires buffers to be submitted
// in pairs for request and response at once. We expect that devmnt issues a write request first, and then
// read request. Write request will return immediately, but the buffer will be only held until read request
// comes. Then both buffers are sent to virtqueue, and the result is returned to the caller.

struct holdbuf
{
	void *rdbuf;				// read buffer (response)
	int32_t rdlen;				// read length (allocated)
	int32_t rfree;				// if true then read buffer was malloc'd and needs to be freed
	void *wrbuf;				// write buffer (request)
	int32_t wrlen;				// write length (supplied)
	int32_t wfree;				// if true then write buffer was malloc'd and needs to be freed
	Proc *proc;					// holding process
	int descr;					// virtqueue descriptor index for the request
};

// PID cache structure. We need a way to find the currently held buffer by a calling process fast.
// We use the last 4 bits of PID for that as a hash. So we can have up to 16 processes hashed.
// If lookup fails, linear search will be used. The cache is a simple LRU: newly entering process
// overwrites the cache entry.

#define PIDCSIZE 16
#define PIDCMASK 0x0F

struct pidcache
{
	int pid;					// actual PID
	struct holdbuf *hb;			// hold buffer structure pointer
};

// Per-mount (virtqueue) structure. It holds an array of hold buffer structures of the same
// length as the number of descriptors in the queue.

static struct v9pmnt
{
	char *tag;					// mount tag (from host)
	char *version;				// 9p version (from host)
	uint32_t msize;				// message size (need this to pad short buffers otherwise QEMU errors out)
	Virtq *vq;					// associated virtqueue
	struct holdbuf *hbufs;		// hold buffers
	struct pidcache pidch[PIDCSIZE];	// PID cache
	Lock pclock;				// PID cache lock
	int pcuse;					// cache usage counter (entering processes)
	int pchit;					// cache hits counter
	int pcmiss;					// cache misses counter
} *mounts;

// Find a hold buffer structure by PID. Nil is returned if no process found.
// First lookup in the cache, then linearly over the whole array.

static struct holdbuf *
hbbypid(int tidx, int pid)
{
	struct holdbuf *ret = nil;
	lock(&mounts[tidx].pclock);
	if(mounts[tidx].pidch[pid & PIDCMASK].pid == pid) {
		ret = mounts[tidx].pidch[pid & PIDCMASK].hb;
		if(ret->proc->pid == pid) {	// != is unlikely mb corruption but we have to get around
			mounts[tidx].pchit++;
			unlock(&mounts[tidx].pclock);
			return ret;
		}
	}
	unlock(&mounts[tidx].pclock);
	mounts[tidx].pcmiss++;
	for(int i = 0; i < PIDCSIZE; i++) {
		if(mounts[tidx].hbufs[i].proc->pid == pid) {
			ret = &mounts[tidx].hbufs[i];
			return ret;
		}
	}
	return nil;
}

// Find a mount tag index, return -1 if none found.

static int 
findtag(char *tag)
{
	for(int i = 0; i < nv9p; i++)
	{
		if(mounts[i].tag && (!strcmp(mounts[i].tag, tag)))
			return i;
	}
print("tag not found: %s\n", tag);
	return -1;
}

// Emulate bindmount for each mount tag. We attach to devmnt directly, without
// the user calling mount. The logic of devmnt properly sequences write and read
// requests, but this cannot be expected from any other client. So 9p virtqueues
// are not generally accessible, they operate under the hood.

static void
domountvq(int tidx)
{
	struct {
		Chan    *chan;
		Chan    *authchan;
		char    *spec;
		int     flags;
	} bogus;
	int dc = 'M';
	Dev *dev = devtabget(dc, 0);
	if(dev == nil)
		error("no #M device found");
	bogus.spec = mounts[tidx].tag;
	bogus.flags = MCACHE;
	bogus.authchan = nil;
	bogus.chan = newchan();
	bogus.chan->dev = &v9pdevtab;
	bogus.chan->path = newpath(bogus.spec);
	Chan *c0 = dev->attach((char *)&bogus);
	Chan *c1 = namec(strdup("/mnt/xxx"), Amount, 0, 0);
	cmount(&c0, c1, MAFTER, bogus.spec); 
}

static void 
mntvq(void)
{
	if(initdone)
		return;
	initdone = 1;
	for(int i = 0; i < nv9p; i++) {
		print("auto mount %d\n", i);
		domountvq(i);
	}
}

static void
v9pinit(void)
{
	uint32_t nvdev;

	print("virtio-9p initializing\n");
	nvdev = getvdevnum();
	if(nvdev <= 0)
		return;
	v9ps = mallocz(nvdev * sizeof(Vqctl *), 1);
	if(v9ps == nil) {
		print("no memory to allocate v9p\n");
		return;
	}
	nv9p = 0;
	nv9p = getvdevsbypciid(PCI_DEVICE_ID_VIRTIO_9P, v9ps, nvdev);
	if(nv9p <= 0)
		return;
	mounts = mallocz(sizeof(struct v9pmnt) * nv9p, 1);
	if(mounts == nil) {
		print("no memory to allocate v9p\n");
		return;
	}
	print("virtio 9p mounts found: %d\n", nv9p);
	for(int i = 0; i < nv9p; i++) {
		struct virtio_9p_config vcfg;
		int rc = readvdevcfg(v9ps[i], &vcfg, sizeof(vcfg), 0);
		if(rc < 0)
			continue;
		print("config area size %d tag_len %d\n", rc, vcfg.tag_len);
		mounts[i].tag = mallocz(vcfg.tag_len + 1, 1);
		readvdevcfg(v9ps[i], mounts[i].tag, vcfg.tag_len, rc);
		print("tag %s\n", mounts[i].tag);
		mounts[i].vq = v9ps[i]->vqs[0];
		mounts[i].hbufs = mallocz(mounts[i].vq->vr.num, 1);
		finalinitvdev(v9ps[i]);
	}
	initdone = 0;
}

// General virtio request. It takes 2 buffers, one for input and other for output.
// Both buffers should be mappable to physical addresses (that is, malloced from the
// system heap, not in any application's address space). The request will be made in the
// indirect mode because this is the only way that work properly with QEMU 9p.

static int32_t
do_request(int gdescr, int tidx, void *inbuf, int32_t inlen, void *outbuf, int32_t outlen)
{
	uint16_t descr[1];
	Virtq *vq = v9ps[tidx]->vqs[0];
	if(vq == nil) {
		error("No virtqueue (nil address)");
	}
	descr[0] = gdescr;
	struct vring_desc req[2] = {
		{
			.addr = PADDR(outbuf),
			.len = outlen,
			.flags = VRING_DESC_F_NEXT,
			.next = 1
		},
		{
			.addr = PADDR(inbuf),
			.len = inlen,
			.flags = VRING_DESC_F_WRITE,
			.next = 0
		}
	};
	q2descr(vq, descr[0])->addr = PADDR(&req);
	q2descr(vq, descr[0])->len = sizeof(req);
	q2descr(vq, descr[0])->flags = VRING_DESC_F_INDIRECT;
	q2descr(vq, descr[0])->next = 0;
	queuedescr(vq, 1, descr);
	reldescr(vq, 1, descr);
	return 0;
}

// We expect only 9p messages be written, and only for a non-empty chan path (mount tag).
// Some messages need massaging (like Tversion because QEMU does not support vanilla 9P2000
// and we have to cheat here about the protocol version). In such case some additional logic
// applies based on the extracted message type.

static int32_t
v9pwrite(Chan *c, void *va, int32_t n, int64_t offset)
{
	Proc *up = externup();
	int tidx = findtag(chanpath(c));
	if(tidx < 0 || tidx >= nv9p)
		error(Enonexist);
	uint8_t *msg = va;
	int mtype = GBIT8(msg + 4);
	void *nva;
	int lnva;
	int alloc;
	switch(mtype)
	{
	case Tversion:
			alloc = 1;
			Fcall f = {
				.type = mtype,
				.tag = GBIT16(msg + 5),
				.msize = GBIT32(msg + 7),
				.version = VERSION9PU
			};
			lnva = IOHDRSZ + strlen(f.version) + 20;
			nva = mallocz(lnva, 1);
			convS2M(&f, nva, lnva);
		break;
	default:
		if(n >= mounts[tidx].msize) {
			nva = va;
			lnva = n;
			alloc = 0;
		} else {
			lnva = mounts[tidx].msize;
			nva = mallocz(lnva, 1);
			alloc = 1;
			memmove(nva, va, n);
		}
	}
	uint16_t descr[1];
	struct v9pmnt *pm = mounts + tidx;
	int rc = getdescr(pm->vq, 1, descr);
	if(rc < 1) {
		if(alloc)
			free(nva);
		error("not enough virtqueue descriptors");
	}
	lock(&pm->pclock);
	pm->hbufs[descr[0]].descr = descr[0];
	pm->hbufs[descr[0]].proc = up;
	pm->hbufs[descr[0]].wfree = alloc;
	pm->hbufs[descr[0]].wrbuf = nva;
	pm->hbufs[descr[0]].wrlen = lnva;
	pm->hbufs[descr[0]].rdbuf = nil;
	pm->hbufs[descr[0]].rdlen = 0;
	pm->hbufs[descr[0]].rfree = 0;
	pm->pidch[up->pid & PIDCMASK].hb = &pm->hbufs[descr[0]];
	pm->pidch[up->pid & PIDCMASK].pid = up->pid;
	pm->pcuse++;
	unlock(&pm->pclock);
print("write %s msg %d %d\n", chanpath(c), mtype, lnva);
	return n;
}

static int
v9pgen(Chan *c, char *d, Dirtab* dir, int i, int s, Dir *dp)
{
	return -1;
}


int
statcheckx(uint8_t *buf, uint nbuf)
{
        uint8_t *ebuf;
        int i;

        ebuf = buf + nbuf;

print("nbuf %d STATFIXLEN %d BIT16SZ + GBIT16(buf) %d\n", nbuf, STATFIXLEN, BIT16SZ + GBIT16(buf));

        if(nbuf < STATFIXLEN || nbuf != BIT16SZ + GBIT16(buf))
                {print("x1\n");return -1;}

        buf += STATFIXLEN - 4 * BIT16SZ;


        for(i = 0; i < 4; i++){
                if(buf + BIT16SZ > ebuf)
					{print("x2\n");return -1;}
                buf += BIT16SZ + GBIT16(buf);
        }

print("buf %X ebuf %X\n", buf, ebuf);

        if(buf != ebuf)
                {print("x3\n");return -1;}

        return 0;
}


// We expect only 9p messages be received, and only for a non-empty chan path (mount tag).
// Some messages need massaging (like Rversion because QEMU does not support vanilla 9P2000
// and we have to cheat here about the protocol version). In such case some additional logic
// applies based on the extracted message type. The function checks for a held write buffer,
// absence of such is an error. The length returned may length extracted from the first
// 4 bytes of the message in some cases.

static int32_t
v9pread(Chan *c, void *va, int32_t n, int64_t offset)
{
	if(!strcmp(chanpath(c), "#9"))
		return devdirread(c, va, n, (Dirtab *)0, 0L, v9pgen);
	Proc *up = externup();
	int tidx = findtag(chanpath(c));
	if(tidx < 0 || tidx >= nv9p)
		error(Enonexist);
	struct holdbuf *hb = hbbypid(tidx, up->pid);
	if(hb == nil)
		error("read request without previously held write request");
	hb->rdbuf = va;
	hb->rdlen = n;
	do_request(hb->descr, tidx, hb->rdbuf, hb->rdlen, hb->wrbuf, hb->wrlen);
	if(hb->wfree)
		free(hb->wrbuf);
	uint8_t *msg = va;
	int mtype = GBIT8(msg + 4);
	uint32_t mlen = GBIT32(msg);
	Fcall f;
	switch(mtype)
	{
	case Rerror:
		convM2S(msg, n, &f);
		error(f.ename);
		break;
	case Rversion:
		convM2S(msg, n, &f);
		mounts[tidx].version = strdup(f.version);
		mounts[tidx].msize = f.msize;
		f.version = VERSION9P;
		convS2M(&f, va, n);
		mlen = GBIT32(msg);
		break;
	case Rstat:
		mlen = GBIT16(msg);
		uint nbuf = GBIT16(msg + 9);
		uint8_t *buf = msg + 9;
		Dir d;
		char strs[1024];
		convM2D(buf, nbuf, &d, strs);
		d.uid = eve;
		d.gid = eve;
		d.muid = eve;
		uint dms = convD2M(&d, buf, nbuf);
		PBIT16(msg + 7, dms);
		mlen = 9 + dms;
		PBIT32(msg, mlen);
	default:
		;
	}
print("read %s msg %d %d\n", chanpath(c), mtype, mlen);
	return mlen;
}

static int
v9pversion(int tidx)
{
	error(Edonotcall(version));
	return 0;
}

// First attach of #9 will force mounting of all defined tags with devmnt. It cannot be done in devinit
// because error() called in a kernel process without user context causes double fault and kernel crash.
// So, to have the host shares actually mounted one has to issue something like ls '#9'.

static Chan*
v9pattach(char *spec)
{
	mntvq();
	if(strlen(spec) == 0)
		return devattach(v9pdevtab.dc, "");
print("stray attach %s\n", spec);
	error(Edonotcall(attach));
	return nil;
}

static Chan*
v9popen(Chan *c, int omode)
{
	if(!strcmp(chanpath(c), "#9"))
		return devopen(c, omode, (Dirtab*)0, 0, v9pgen);
	error(Edonotcall(open));
	return nil;
}

static Walkqid*
v9pwalk(Chan* c, Chan *nc, char** name, int nname)
{
	if(!strcmp(chanpath(c), "#9"))
		return devwalk(c, nc, name, nname, (Dirtab *)0, 0, v9pgen);
print("walk %s\n", chanpath(c));
	error(Edonotcall(walk));
	return nil;
}

static int32_t
v9pstat(Chan* c, uint8_t* dp, int32_t n)
{
	if(!strcmp(chanpath(c), "#9"))
		return devstat(c, dp, n, (Dirtab *)0, 0L, v9pgen);
	error(Edonotcall(stat));
	return 0;
}

static void
v9pclose(Chan* c)
{
	if(!strcmp(chanpath(c), "#9"))
		return;
	error(Edonotcall(close));
}

Dev v9pdevtab = {
	.dc = '9',
	.name = "9p",

	.reset = devreset,
	.init = v9pinit,
	.shutdown = devshutdown,
	.attach = v9pattach,
	.walk = v9pwalk,
	.stat = v9pstat,
	.open = v9popen,
	.create = devcreate,
	.close = v9pclose,
	.read = v9pread,
	.bread = devbread,
	.write = v9pwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
