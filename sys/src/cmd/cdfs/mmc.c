#include <u.h>
#include <libc.h>
#include <disk.h>
#include "dat.h"
#include "fns.h"

enum
{
	Pagesz		= 255,
};

static Dev mmcdev;

typedef struct Mmcaux Mmcaux;
struct Mmcaux {
	uchar page05[Pagesz];
	int page05ok;

	int pagecmdsz;
	ulong mmcnwa;

	int nropen;
	int nwopen;
	long ntotby;
	long ntotbk;
};

static ulong
bige(void *p)
{
	uchar *a;

	a = p;
	return (a[0]<<24)|(a[1]<<16)|(a[2]<<8)|(a[3]<<0);
}

static ushort
biges(void *p)
{
	uchar *a;

	a = p;
	return (a[0]<<8) | a[1];
}

static void
hexdump(void *v, int n)
{
	int i;
	uchar *p;

	p = v;
	for(i=0; i<n; i++){
		print("%.2ux ", p[i]);
		if((i%8) == 7)
			print("\n");
	}
	if(i%8)
		print("\n");
}

static int
mmcgetpage10(Drive *drive, int page, void *v)
{
	uchar cmd[10], resp[512];
	int n, r;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x5A;
	cmd[2] = page;
	cmd[8] = 255;
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < 8)
		return -1;

	r = (resp[6]<<8) | resp[7];
	n -= 8+r;

	if(n < 0)
		return -1;
	if(n > Pagesz)
		n = Pagesz;

	memmove(v, &resp[8+r], n);
	return n;
}

static int
mmcgetpage6(Drive *drive, int page, void *v)
{
	uchar cmd[6], resp[512];
	int n;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x1A;
	cmd[2] = page;
	cmd[4] = 255;
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < 4)
		return -1;

	n -= 4+resp[3];
	if(n < 0)
		return -1;
	if(n > Pagesz)
		n = Pagesz;

	memmove(v, &resp[4+resp[3]], n);
	return n;
}

static int
mmcsetpage10(Drive *drive, int page, void *v)
{
	uchar cmd[10], *p, *pagedata;
	int len, n;

	pagedata = v;
	assert(pagedata[0] == page);
	len = 8+2+pagedata[1];
	p = emalloc(len);
	memmove(p+8, pagedata, pagedata[1]);
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x55;
	cmd[1] = 0x10;
	cmd[8] = len;

//	print("cmd\n");
//	hexdump(cmd, 10);
//	print("page\n");
//	hexdump(p, len);

	n = scsi(drive, cmd, sizeof(cmd), p, len, Swrite);
	free(p);
	if(n < len)
		return -1;
	return 0;
}

static int
mmcsetpage6(Drive *drive, int page, void *v)
{
	uchar cmd[6], *p, *pagedata;
	int len, n;
	
	pagedata = v;
	assert(pagedata[0] == page);
	len = 4+2+pagedata[1];
	p = emalloc(len);
	memmove(p+4, pagedata, pagedata[1]);
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x15;
	cmd[1] = 0x10;
	cmd[4] = len;

	n = scsi(drive, cmd, sizeof(cmd), p, len, Swrite);
	free(p);
	if(n < len)
		return -1;
	return 0;
}

static int
mmcgetpage(Drive *drive, int page, void *v)
{
	Mmcaux *aux;

	aux = drive->aux;
	switch(aux->pagecmdsz) {
	case 10:
		return mmcgetpage10(drive, page, v);
	case 6:
		return mmcgetpage6(drive, page, v);
	default:
		assert(0);
	}
	return -1;
}

static int
mmcsetpage(Drive *drive, int page, void *v)
{
	Mmcaux *aux;

	aux = drive->aux;
	switch(aux->pagecmdsz) {
	case 10:
		return mmcsetpage10(drive, page, v);
	case 6:
		return mmcsetpage6(drive, page, v);
	default:
		assert(0);
	}
	return -1;
}

int
mmcstatus(Drive *drive)
{
	uchar cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xBD;
	return scsi(drive, cmd, sizeof(cmd), nil, 0, Sread);
}

void
mmcgetspeed(Drive *drive)
{
	int n, maxread, curread, maxwrite, curwrite;
	uchar buf[Pagesz];

	n = mmcgetpage(drive, 0x2A, buf);
	maxread = (buf[8]<<8)|buf[9];
	curread = (buf[14]<<8)|buf[15];
	maxwrite = (buf[18]<<8)|buf[19];
	curwrite = (buf[20]<<8)|buf[21];

	if(n < 22 || (maxread && maxread < 170) || (curread && curread < 170))
		return;	/* bogus data */

	drive->readspeed = curread;
	drive->writespeed = curwrite;
	drive->maxreadspeed = maxread;
	drive->maxwritespeed = maxwrite;
}

Drive*
mmcprobe(Scsi *scsi)
{
	Mmcaux *aux;
	Drive *drive;
	uchar buf[Pagesz];
	int cap;

	/* BUG: confirm mmc better? */

	drive = emalloc(sizeof(Drive));
	drive->Scsi = *scsi;
	drive->Dev = mmcdev;
	aux = emalloc(sizeof(Mmcaux));
	drive->aux = aux;

	/* attempt to read CD capabilities page */
	if(mmcgetpage10(drive, 0x2A, buf) >= 0)
		aux->pagecmdsz = 10;
	else if(mmcgetpage6(drive, 0x2A, buf) >= 0)
		aux->pagecmdsz = 6;
	else {
		werrstr("not an mmc device");
		free(drive);
		return nil;
	}

	cap = 0;
	if(buf[3] & 3)	/* 2=cdrw, 1=cdr */
		cap |= Cwrite;
	if(buf[5] & 1)
		cap |= Ccdda;

//	print("read %d max %d\n", biges(buf+14), biges(buf+8));
//	print("write %d max %d\n", biges(buf+20), biges(buf+18));

	/* cache page 05 (write parameter page) */
	if((cap & Cwrite) && mmcgetpage(drive, 0x05, aux->page05) >= 0)
		aux->page05ok = 1;
	else
		cap &= ~Cwrite;

	drive->cap = cap;

	mmcgetspeed(drive);
	return drive;
}

static int
mmctrackinfo(Drive *drive, int t, int i)
{
	uchar cmd[10], resp[255];
	int n, type, bs;
	uchar tmode;
	ulong beg, size;
	Mmcaux *aux;

	aux = drive->aux;
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x52;	/* get track info */
	cmd[1] = 1;
	cmd[2] = t>>24;
	cmd[3] = t>>16;
	cmd[4] = t>>8;
	cmd[5] = t;
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp);
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < 28) {
		if(vflag)
			print("trackinfo %d fails n=%d %r\n", t, n);
		return -1;
	}

	beg = bige(&resp[8]);
	size = bige(&resp[24]);

	tmode = resp[5] & 0x0D;
//	dmode = resp[6] & 0x0F;

	if(vflag)
		print("track %d type 0x%x\n", t, tmode);
	type = TypeNone;
	bs = BScdda;
	switch(tmode){
	case 0:
		type = TypeAudio;
		bs = BScdda;
		break;
	case 1:	/* 2 audio channels, with pre-emphasis 50/15 μs */
		if(vflag)
			print("audio channels with preemphasis on track %d (u%.3d)\n", t, i);
		type = TypeNone;
		break;
	case 4:	/* data track, recorded uninterrupted */
		type = TypeData;
		bs = BScdrom;
		break;
	case 5:	/* data track, recorded interrupted */
	default:
		if(vflag)
			print("unknown track type %d\n", tmode);
	}

	drive->track[i].mtime = drive->changetime;
	drive->track[i].beg = beg;
	drive->track[i].end = beg+size;
	drive->track[i].type = type;
	drive->track[i].bs = bs;
	drive->track[i].size = (vlong)(size-2)*bs;	/* -2: skip lead out */

	if(resp[6] & (1<<6)) {
		drive->track[i].type = TypeBlank;
		drive->writeok = 1;
	}

	if(t == 0xFF)
		aux->mmcnwa = bige(&resp[12]);

	return 0;
}

static int
mmcreadtoc(Drive *drive, int type, int track, void *data, int nbytes)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x43;
	cmd[1] = type;
	cmd[6] = track;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;

	return scsi(drive, cmd, sizeof(cmd), data, nbytes, Sread);
}

static int
mmcreaddiscinfo(Drive *drive, void *data, int nbytes)
{
	uchar cmd[10];
	int n;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x51;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	n = scsi(drive, cmd, sizeof(cmd), data, nbytes, Sread);
	if(n < 24) {
		if(n >= 0)
			werrstr("rdiscinfo returns %d", n);
		return -1;
	}

	return n;
}

static Msf
rdmsf(uchar *p)
{
	Msf msf;

	msf.m = p[0];
	msf.s = p[1];
	msf.f = p[2];
	return msf;
}

static int
mmcgettoc(Drive *drive)
{
	uchar resp[1024];
	int i, n, first, last;
	ulong tot;
	Track *t;

	/*
	 * if someone has swapped the cd,
	 * mmcreadtoc will get ``medium changed'' and the
	 * scsi routines will set nchange and changetime in the
	 * scsi device.
	 */
	mmcreadtoc(drive, 0, 0, resp, sizeof(resp));
	if(drive->Scsi.changetime == 0) {	/* no media present */
		drive->ntrack = 0;
		return 0;
	}

	if(drive->nchange == drive->Scsi.nchange && drive->changetime != 0)
		return 0;

	drive->ntrack = 0;
	drive->nameok = 0;
	drive->nchange = drive->Scsi.nchange;
	drive->changetime = drive->Scsi.changetime;
	drive->writeok = 0;

	for(i=0; i<nelem(drive->track); i++){
		memset(&drive->track[i].mbeg, 0, sizeof(Msf));
		memset(&drive->track[i].mend, 0, sizeof(Msf));
	}

	/*
	 * find number of tracks
	 */
	if((n=mmcreadtoc(drive, 0x02, 0, resp, sizeof(resp))) < 4) {
		/*
		 * on a blank disc in a cd-rw, use readdiscinfo
		 * to find the track info.
		 */
		if(mmcreaddiscinfo(drive, resp, sizeof(resp)) < 0)
			return -1;
		if(resp[4] != 1) 
			print("multi-session disc %d\n", resp[4]);
		first = resp[3];
		last = resp[6];
		if(vflag)
			print("blank disc %d %d\n", first, last);
		drive->writeok = 1;
	} else {
		first = resp[2];
		last = resp[3];

		if(n >= 4+8*(last-first+2)) {
			for(i=0; i<=last-first+1; i++)	/* <=: track[last-first+1] = end */
				drive->track[i].mbeg = rdmsf(resp+4+i*8+5);
			for(i=0; i<last-first+1; i++)
				drive->track[i].mend = drive->track[i+1].mbeg;
		}
	}

	if(vflag)
		print("first %d last %d\n", first, last);

	if(first == 0 && last == 0)
		first = 1;

	if(first <= 0 || first >= Maxtrack) {
		werrstr("first table %d not in range", first);
		return -1;
	}
	if(last <= 0 || last >= Maxtrack) {
		werrstr("last table %d not in range", last);
		return -1;
	}

	if(drive->cap & Cwrite) {	/* CDR drives are easy */
		for(i = first; i <= last; i++)
			mmctrackinfo(drive, i, i-first);
	} else {
		/*
		 * otherwise we need to infer endings from the
		 * beginnings of other tracks.
		 */
		for(i = first; i <= last; i++) {
			memset(resp, 0, sizeof(resp));
			if(mmcreadtoc(drive, 0x00, i, resp, sizeof(resp)) < 0)
				break;
			t = &drive->track[i-first];
			t->mtime = drive->changetime;
			t->type = TypeData;
			t->bs = BScdrom;
			t->beg = bige(resp+8);
			if(!(resp[5] & 4)) {
				t->type = TypeAudio;
				t->bs = BScdda;
			}
		}

		if((long)drive->track[0].beg < 0)	/* i've seen negative track 0's */
			drive->track[0].beg = 0;

		tot = 0;
		memset(resp, 0, sizeof(resp));
		if(mmcreadtoc(drive, 0x00, 0xAA, resp, sizeof(resp)) < 0)
			print("bad\n");
		if(resp[6])
			tot = bige(resp+8);

		for(i=last; i>=first; i--) {
			t = &drive->track[i-first];
			t->end = tot;
			tot = t->beg;
			if(t->end <= t->beg) {
				t->beg = 0;
				t->end = 0;
			}
			t->size = (t->end - t->beg - 2) * (vlong)t->bs;	/* -2: skip lead out */
		}
	}

	drive->firsttrack = first;
	drive->ntrack = last+1-first;
	return 0;
}

static int
mmcsetbs(Drive *drive, int bs)
{
	uchar *p;
	Mmcaux *aux;

	aux = drive->aux;

	assert(aux->page05ok);

	p = aux->page05;
	p[2] = 0x01;				/* track-at-once */
//	if(xflag)
//		p[2] |= 0x10;			/* test-write */
	
	switch(bs){
	case BScdrom:
		p[3] = (p[3] & ~0x07)|0x04;	/* data track, uninterrupted */
		p[4] = 0x08;			/* mode 1 CD-ROM */
		p[8] = 0;			/* session format CD-DA or CD-ROM */
		break;

	case BScdda:
		p[3] = (p[3] & ~0x07)|0x00;	/* 2 audio channels without pre-emphasis */
		p[4] = 0x00;			/* raw data */
		p[8] = 0;			/* session format CD-DA or CD-ROM */
		break;

	case BScdxa:
		p[3] = (p[3] & ~0x07)|0x04;	/* data track, uninterrupted */
		p[4] = 0x09;			/* mode 2 */
		p[8] = 0x20;			/* session format CD-ROM XA */
		break;

	default:
		assert(0);
	}

	if(mmcsetpage(drive, 0x05, p) < 0)
		return -1;
}

static long
mmcread(Buf *buf, void *v, long nblock, long off)
{
	Drive *drive;
	int bs;
	uchar cmd[12];
	long n, nn;
	Otrack *o;

	o = buf->otrack;
	drive = o->drive;
	bs = o->track->bs;
	off += o->track->beg;

	if(nblock >= (1<<10)) {
		werrstr("mmcread too big");
		if(vflag)
			fprint(2, "mmcread too big\n");
		return -1;
	}

	/* truncate nblock modulo size of track */
	if(off > o->track->end - 2) {
		werrstr("read past end of track");
		if(vflag)
			fprint(2, "end of track (%ld->%ld off %ld)", o->track->beg, o->track->end-2, off);
		return -1;
	}
	if(off == o->track->end - 2)
		return 0;

	if(off+nblock > o->track->end - 2)
		nblock = o->track->end - 2 - off;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xBE;
	cmd[2] = off>>24;
	cmd[3] = off>>16;
	cmd[4] = off>>8;
	cmd[5] = off>>0;
	cmd[6] = nblock>>16;
	cmd[7] = nblock>>8;
	cmd[8] = nblock>>0;
	cmd[9] = 0x10;

	switch(bs){
	case BScdda:
		cmd[1] = 0x04;
		break;

	case BScdrom:
		cmd[1] = 0x08;
		break;

	case BScdxa:
		cmd[1] = 0x0C;
		break;

	default:
		werrstr("unknown bs %d", bs);
		return -1;
	}

	n = nblock*bs;
	nn = scsi(drive, cmd, sizeof(cmd), v, n, Sread);
	if(nn != n) {
		werrstr("short read %ld/%ld", nn, n);
		if(vflag)
			print("read off %lud nblock %ld bs %d failed\n", off, nblock, bs);
		return -1;
	}

	return nblock;
}

static Otrack*
mmcopenrd(Drive *drive, int trackno)
{
	Otrack *o;
	Mmcaux *aux;

	if(trackno < 0 || trackno >= drive->ntrack) {
		werrstr("track number out of range");
		return nil;
	}

	aux = drive->aux;
	if(aux->nwopen) {
		werrstr("disk in use for writing");
		return nil;
	}

	o = emalloc(sizeof(Otrack));
	o->drive = drive;
	o->track = &drive->track[trackno];
	o->nchange = drive->nchange;
	o->omode = OREAD;
	o->buf = bopen(mmcread, OREAD, o->track->bs, Nblock);
	o->buf->otrack = o;

	aux->nropen++;

	return o;
}

static long
mmcxwrite(Otrack *o, void *v, long nblk)
{
	uchar cmd[10];
	Mmcaux *aux;

	assert(o->omode == OWRITE);

	aux = o->drive->aux;
	aux->ntotby += nblk*o->track->bs;
	aux->ntotbk += nblk;
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x2a;	/* write */
	cmd[2] = aux->mmcnwa>>24;
	cmd[3] = aux->mmcnwa>>16;
	cmd[4] = aux->mmcnwa>>8;
	cmd[5] = aux->mmcnwa;
	cmd[7] = nblk>>8;
	cmd[8] = nblk>>0;
	if(vflag)
		print("%lld: write %ld at 0x%lux\n", nsec(), nblk, aux->mmcnwa);
	aux->mmcnwa += nblk;
	return scsi(o->drive, cmd, sizeof(cmd), v, nblk*o->track->bs, Swrite);
}

static long
mmcwrite(Buf *buf, void *v, long nblk, long)
{
	return mmcxwrite(buf->otrack, v, nblk);
}

static Otrack*
mmccreate(Drive *drive, int type)
{
	int bs;
	Mmcaux *aux;
	Track *t;
	Otrack *o;

	aux = drive->aux;

	if(aux->nropen || aux->nwopen) {
		werrstr("drive in use");
		return nil;
	}

	switch(type){
	case TypeAudio:
		bs = BScdda;
		break;
	case TypeData:
		bs = BScdrom;
		break;
	default:
		werrstr("bad type %d", type);
		return nil;
	}

	if(mmctrackinfo(drive, 0xFF, Maxtrack)) {		/* the invisible track */
		werrstr("CD not writable");
		return nil;
	}
	if(mmcsetbs(drive, bs) < 0) {
		werrstr("cannot set bs mode");
		return nil;
	}
	if(mmctrackinfo(drive, 0xFF, Maxtrack)) {		/* the invisible track */
		werrstr("CD not writable 2");
		return nil;
	}

	aux->ntotby = 0;
	aux->ntotbk = 0;

	t = &drive->track[drive->ntrack++];
	t->size = 0;
	t->bs = bs;
	t->beg = aux->mmcnwa;
	t->end = 0;
	t->type = type;
	drive->nameok = 0;

	o = emalloc(sizeof(Otrack));
	o->drive = drive;
	o->nchange = drive->nchange;
	o->omode = OWRITE;
	o->track = t;
	o->buf = bopen(mmcwrite, OWRITE, bs, Nblock);
	o->buf->otrack = o;

	aux->nwopen++;

	if(vflag)
		print("mmcinit: nwa = 0x%luX\n", aux->mmcnwa);

	return o;
}

void
mmcsynccache(Drive *drive)
{
	uchar cmd[10];
	Mmcaux *aux;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x35;	/* flush */
	scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
	if(vflag) {
		aux = drive->aux;
		print("mmcsynccache: bytes = %ld blocks = %ld, mmcnwa 0x%luX\n",
			aux->ntotby, aux->ntotbk, aux->mmcnwa);
	}
/* rsc: seems not to work on some drives; 	mmcclose(1, 0xFF); */
}

static void
mmcclose(Otrack *o)
{
	Mmcaux *aux;
	static uchar zero[2*BSmax];

	aux = o->drive->aux;
	if(o->omode == OREAD)
		aux->nropen--;
	else if(o->omode == OWRITE) {
		aux->nwopen--;
		mmcxwrite(o, zero, 2);	/* write lead out */
		mmcsynccache(o->drive);
		o->drive->nchange = -1;	/* force reread toc */
	}
	free(o);
}

static int
mmcxclose(Drive *drive, int ts, int trackno)
{
	uchar cmd[10];

	/*
	 * ts: 1 == track, 2 == session
	 */
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x5B;
	cmd[2] = ts;
	if(ts == 1)
		cmd[5] = trackno;
	return scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
}

static int
mmcfixate(Drive *drive)
{
	uchar *p;
	Mmcaux *aux;

	if((drive->cap & Cwrite) == 0) {
		werrstr("not a writer");
		return -1;
	}

	drive->nchange = -1;	/* force reread toc */

	aux = drive->aux;
	p = aux->page05;
	p[3] = (p[3] & ~0xC0);
	if(mmcsetpage(drive, 0x05, p) < 0)
		return -1;

/* rsc: seems not to work on some drives; 	mmcclose(1, 0xFF); */
	return mmcxclose(drive, 0x02, 0);
}

static int
mmcsession(Drive *drive)
{
	uchar *p;
	Mmcaux *aux;

	drive->nchange = -1;	/* force reread toc */

	aux = drive->aux;
	p = aux->page05;
	p[3] = (p[3] & ~0xC0);
	if(mmcsetpage(drive, 0x05, p) < 0)
		return -1;

/* rsc: seems not to work on some drives; 	mmcclose(1, 0xFF); */
	return mmcxclose(drive, 0x02, 0);
}

static int
mmcblank(Drive *drive, int quick)
{
	uchar cmd[12];

	drive->nchange = -1;	/* force reread toc */

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xA1;	/* blank */
	/* cmd[1] = 0 means blank the whole disc; = 1 just the header */
	cmd[1] = quick ? 0x01 : 0x00;

	return scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
}

static int
start(Drive *drive, int code)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x1B;
	cmd[4] = code;
	return scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
}

static char*
e(int status)
{
	if(status < 0)
		return geterrstr();
	return nil;
}

static char*
mmcctl(Drive *drive, int argc, char **argv)
{
	if(argc < 1)
		return nil;

	if(strcmp(argv[0], "blank") == 0)
		return e(mmcblank(drive, 0));
	if(strcmp(argv[0], "quickblank") == 0)
		return e(mmcblank(drive, 1));
	if(strcmp(argv[0], "eject") == 0)
		return e(start(drive, 2));
	if(strcmp(argv[0], "ingest") == 0)
		return e(start(drive, 3));
	return "bad arg";
}

static char*
mmcsetspeed(Drive *drive, int r, int w)
{
	char *rv;
	uchar cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xBB;
	cmd[2] = r>>8;
	cmd[3] = r;
	cmd[4] = w>>8;
	cmd[5] = w;
	rv = e(scsi(drive, cmd, sizeof(cmd), nil, 0, Snone));
	mmcgetspeed(drive);
	return rv;
}

static Dev mmcdev = {
	mmcopenrd,
	mmccreate,
	bufread,
	bufwrite,
	mmcclose,
	mmcgettoc,
	mmcfixate,
	mmcctl,
	mmcsetspeed,
};
