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

// Shared with devmnt

extern char Enoversion[];
extern int alloctag(void);
extern void freetag(int t);

extern Dev v9pdevtab;

// Array of defined 9p mounts and their number

static uint32_t nv9p;

static Vqctl **v9ps;

static struct v9pmnt
{
	char *tag;
	char *version;
	uint32_t msize;
} *mounts;

// Find a mount tag index, return -1 if none found.

static int 
findtag(char *tag)
{
	for(int i = 0; i < nv9p; i++)
	{
		if(mounts[i].tag && (!strcmp(mounts[i].tag, tag)))
			return i;
	}
	return -1;
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
		finalinitvdev(v9ps[i]);
	}
}

// General virtio request. It takes 2 buffers, one for input and other for output.
// Both buffers should be mappable to physical addresses (that is, malloced from the
// system heap, not in any application's address space). The request will be made in the
// indirect mode because this is the only way that work properly with QEMU 9p.

static int32_t
do_request(int tidx, void *inbuf, int32_t inlen, void *outbuf, int32_t outlen)
{
	uint16_t descr[1];
	Virtq *vq = v9ps[tidx]->vqs[0];
	int rc = getdescr(vq, 1, descr);
	if(rc < 1) {
		error("Insufficient number of descriptors in virtqueue");
		return -1;
	}
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

// Send a version message over the given virtqueue.

static int
v9pversion(int tidx)
{
	uint8_t *msg, *rsp;
	usize k;
	Fcall f;
	f.type = Tversion;
	f.tag = NOTAG;
	f.msize = MAXRPC;
	f.version = VERSION9PU;
	msg = mallocz(BUFSZ, 1);
	rsp = mallocz(BUFSZ, 1);
	if(msg == nil || rsp == nil)
		exhausted("version memory");
	k = convS2M(&f, msg, BUFSZ);
	if(k == 0) {
		free(msg);
		free(rsp);
		error("bad version conversion on send");
	}
	int rc = do_request(tidx, rsp, BUFSZ, msg, BUFSZ);
	int retval = rc;
	free(msg);
	if(rc >= 0) {
		k = convM2S(rsp, BUFSZ, &f);
		if(f.type == Rerror) {
			free(rsp);
			error(f.ename);
			return -1;
		} else if(f.type != Rversion) {
			free(rsp);
			error("unexpected reply type in fversion");
			return -1;
		}
	}
	if(f.msize > MAXRPC) {
		free(rsp);
		error("server tries to increase msize in fversion");
		return -1;
	}
	if(f.msize<256 || f.msize>1024*1024) {
		free(rsp);
		error("nonsense value of msize in fversion");
		return -1;
	}
	mounts[tidx].msize = f.msize;
	mounts[tidx].version = strdup(f.version);
	free(rsp);
	return retval;
}

static Chan*
v9pattach(char *spec)
{
print("v9pattach %s\n", spec);
	int tidx = findtag(spec);
	if(tidx < 0) {
		error(Enonexist);
		return nil;
	}
	if(!mounts[tidx].version) {
		int rc = v9pversion(tidx);
		if(rc < 0) {
			error(Enoversion);
			return nil;
		}
	}
	Chan *ch = devattach(v9pdevtab.dc, spec);
	Fcall r;
	r.type = Tattach;
	r.tag = alloctag();
	r.fid = ch->fid;
	r.afid = NOFID;
	r.uname = "";
	r.aname = "";
	uint8_t *msg, *rsp;
	uint32_t ms = mounts[tidx].msize;
	msg = mallocz(ms, 1);
	rsp = mallocz(ms, 1);
	if(msg == nil || rsp == nil)
		exhausted("attach memory");
	usize k = convS2M(&r, msg, ms);
	if(k == 0) {
		free(msg);
		free(rsp);
		freetag(r.tag);
		error("bad attach conversion on send");
	}
	int rc = do_request(tidx, rsp, ms, msg, ms);
	freetag(r.tag);
	free(msg);
	if(rc >= 0) {
		k = convM2S(rsp, ms, &r);
		if(r.type == Rerror) {
			free(rsp);
			error(r.ename);
			return nil;
		}
		ch->qid = r.qid;
	}
	free(rsp);
	return ch;
}


Dev v9pdevtab = {
	.dc = '9',
	.name = "9p",

	.reset = devreset,
	.init = v9pinit,
	.shutdown = devshutdown,
	.attach = v9pattach,
//	.walk = v9pwalk,
//	.stat = v9pstat,
//	.open = v9popen,
	.create = devcreate,
//	.close = v9pclose,
//	.read = v9pread,
	.bread = devbread,
//	.write = v9pwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
