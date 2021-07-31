#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "pool.h"
#include "playlist.h"

typedef struct Wmsg Wmsg;

enum {
	Busy =	0x01,
	Open =	0x02,
	Trunc =	0x04,
	Eof =	0x08,
};

File files[] = {
[Qdir] =	{.dir = {0,0,{Qdir, 0,QTDIR},		0555|DMDIR,	0,0,0,	"."}},
[Qplayctl] =	{.dir = {0,0,{Qplayctl, 0,QTFILE},	0666,		0,0,0,	"playctl"}},
[Qplaylist] =	{.dir = {0,0,{Qplaylist, 0,QTFILE},	0666|DMAPPEND,	0,0,0,	"playlist"}},
[Qplayvol] =	{.dir = {0,0,{Qplayvol, 0,QTFILE},	0666,		0,0,0,	"playvol"}},
[Qplaystat] =	{.dir = {0,0,{Qplaystat, 0,QTFILE},	0444,		0,0,0,	"playstat"}},
};

Channel		*reqs;
Channel		*workers;
Channel		*volumechan;
Channel		*playchan;
Channel		*playlistreq;
Playlist	playlist;
int		volume[8];

char *statetxt[] = {
	[Nostate] =	"panic!",
	[Error] =	"error",
	[Stop] =	"stop",
	[Pause] =	"pause",
	[Play] =	"play",
	[Resume] =	"resume",
	[Skip] =	"skip",
	nil
};

// low-order bits: position in play list, high-order: play state:
Pmsg	playstate = {Stop, 0};

char	*rflush(Worker*), *rauth(Worker*),
	*rattach(Worker*), *rwalk(Worker*),
	*ropen(Worker*), *rcreate(Worker*),
	*rread(Worker*), *rwrite(Worker*), *rclunk(Worker*),
	*rremove(Worker*), *rstat(Worker*), *rwstat(Worker*),
	*rversion(Worker*);

char 	*(*fcalls[])(Worker*) = {
	[Tflush]	rflush,
	[Tversion]	rversion,
	[Tauth]		rauth,
	[Tattach]	rattach,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

int	messagesize = Messagesize;
Fid	*fids;


char	Eperm[] =	"permission denied";
char	Enotdir[] =	"not a directory";
char	Enoauth[] =	"authentication not required";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =	"file in use";
char	Eexist[] =	"file exists";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Ebadctl[] =	"unknown control message";
char	Epast[] =	"reading past eof";

Fid	*oldfid(int);
Fid	*newfid(int);
void	volumeupdater(void*);
void	playupdater(void*);

char *playerror;

static int
lookup(char *cmd, char *list[])
{
	int i;

	for (i = 0; list[i] != nil; i++)
		if (strcmp(cmd, list[i]) == 0)
			return i;
	return -1;
}

char*
rversion(Worker *w)
{
	Req *r;
	Fid *f;

	r = w->r;
	if(r->ifcall.msize < 256)
		return "max messagesize too small";
	if(r->ifcall.msize < messagesize)
		messagesize = r->ifcall.msize;
	r->ofcall.msize = messagesize;
	if(strncmp(r->ifcall.version, "9P2000", 6) != 0)
		return "unknown 9P version";
	else
		r->ofcall.version = "9P2000";
	for(f = fids; f; f = f->next)
		if(f->flags & Busy)
			f->flags &= ~(Open|Busy);
	return nil;
}

char*
rauth(Worker*)
{
	return Enoauth;
}

char*
rflush(Worker *w)
{
	Wmsg m;
	int i;
	Req *r;

	r = w->r;
	m.cmd = Flush;
	m.off = r->ifcall.oldtag;
	m.arg = nil;
	for(i = 1; i < nelem(files); i++)
		bcastmsg(files[i].workers, &m);
	if (debug & DbgWorker)
		fprint(2, "flush done on tag %d\n", r->ifcall.oldtag);
	return 0;
}

char*
rattach(Worker *w)
{
	Fid *f;
	Req *r;

	r = w->r;
	r->fid = newfid(r->ifcall.fid);
	f = r->fid;
	f->flags |= Busy;
	f->file = &files[Qdir];
	r->ofcall.qid = f->file->dir.qid;
	if(!aflag && strcmp(r->ifcall.uname, user) != 0)
		return Eperm;
	return 0;
}

static Fid*
doclone(Fid *f, int nfid)
{
	Fid *nf;

	nf = newfid(nfid);
	if(nf->flags & Busy)
		return nil;
	nf->flags |= Busy;
	nf->flags &= ~(Open);
	nf->file = f->file;
	return nf;
}

char*
dowalk(Fid *f, char *name)
{
	int t;

	if (strcmp(name, ".") == 0)
		return nil;
	if (strcmp(name, "..") == 0){
		f->file = &files[Qdir];
		return nil;
	}
	if(f->file != &files[Qdir])
		return Enotexist;
	for (t = 1; t < Nqid; t++){
		if(strcmp(name, files[t].dir.name) == 0){
			f->file = &files[t];
			return nil;
		}
	}
	return Enotexist;
}

char*
rwalk(Worker *w)
{
	Fid *f, *nf;
	char *rv;
	int i;
	File *savefile;
	Req *r;

	r = w->r;
	r->fid = oldfid(r->ifcall.fid);
	if((f = r->fid) == nil)
		return Enotexist;
	if(f->flags & Open)
		return Eisopen;

	r->ofcall.nwqid = 0;
	nf = nil;
	savefile = f->file;
	/* clone if requested */
	if(r->ifcall.newfid != r->ifcall.fid){
		nf = doclone(f, r->ifcall.newfid);
		if(nf == nil)
			return "new fid in use";
		f = nf;
	}

	/* if it's just a clone, return */
	if(r->ifcall.nwname == 0 && nf != nil)
		return nil;

	/* walk each element */
	rv = nil;
	for(i = 0; i < r->ifcall.nwname; i++){
		rv = dowalk(f, r->ifcall.wname[i]);
		if(rv != nil){
			if(nf != nil)	
				nf->flags &= ~(Open|Busy);
			else
				f->file = savefile;
			break;
		}
		r->ofcall.wqid[i] = f->file->dir.qid;
	}
	r->ofcall.nwqid = i;

	/* we only error out if no walk  */
	if(i > 0)
		rv = nil;

	return rv;
}

char *
ropen(Worker *w)
{
	Fid *f, *ff;
	Wmsg m;
	Req *r;

	r = w->r;
	r->fid = oldfid(r->ifcall.fid);
	if((f = r->fid) == nil)
		return Enotexist;
	if(f->flags & Open)
		return Eisopen;

	if(r->ifcall.mode != OREAD)
		if((f->file->dir.mode & 0x2) == 0)
			return Eperm;
	if((r->ifcall.mode & OTRUNC) && f->file == &files[Qplaylist]){
		playlist.nlines = 0;
		playlist.ndata = 0;
		free(playlist.lines);
		free(playlist.data);
		playlist.lines = nil;
		playlist.data = nil;
		f->file->dir.length = 0;
		f->file->dir.qid.vers++;
		/* Mark all fids for this file `Trunc'ed */
		for(ff = fids; ff; ff = ff->next)
			if(ff->file == &files[Qplaylist] && (ff->flags & Open))
				ff->flags |= Trunc;
		m.cmd = Check;
		m.off = 0;
		m.arg = nil;
		bcastmsg(f->file->workers, &m);
	}
	r->ofcall.iounit = 0;
	r->ofcall.qid = f->file->dir.qid;
	f->flags |= Open;
	return nil;
}

char *
rcreate(Worker*)
{
	return Eperm;
}

int
readtopdir(Fid*, uchar *buf, long off, int cnt, int blen)
{
	int i, m, n;
	long pos;

	n = 0;
	pos = 0;
	for (i = 1; i < Nqid; i++){
		m = convD2M(&files[i].dir, &buf[n], blen-n);
		if(off <= pos){
			if(m <= BIT16SZ || m > cnt)
				break;
			n += m;
			cnt -= m;
		}
		pos += m;
	}
	return n;
}

char*
rread(Worker *w)
{
	Fid *f;
	Req *r;
	long off, cnt;
	int n, i;
	Wmsg m;
	char *p;

	r = w->r;
	r->fid = oldfid(r->ifcall.fid);
	if((f = r->fid) == nil)
		return Enotexist;
	r->ofcall.count = 0;
	off = r->ifcall.offset;
	cnt = r->ifcall.count;

	if(cnt > messagesize - IOHDRSZ)
		cnt = messagesize - IOHDRSZ;

	if(f->file == &files[Qdir]){
		n = readtopdir(f, r->indata, off, cnt, messagesize - IOHDRSZ);
		r->ofcall.count = n;
		return nil;
	}

	if(f->file == files + Qplaystat){
		p = getplaystat(r->ofcall.data, r->ofcall.data + sizeof r->indata);
		readbuf(r, r->ofcall.data, p - r->ofcall.data);
		return nil;
	}

	m.cmd = 0;
	while(f->vers == f->file->dir.qid.vers && (f->flags & Eof)){
		/* Wait until file state changes (f->file->dir.qid.vers is incremented) */
		m = waitmsg(w, f->file->workers);
		if(m.cmd == Flush && m.off == r->ifcall.tag)
			return (char*)~0;	/* no answer needed */
		assert(m.cmd != Work);
		yield();
	}
	if(f->file == files + Qplaylist){
		f->flags &= ~Eof;
		if((f->flags & Trunc) && r->ifcall.offset != 0){
			f->flags &= ~Trunc;
			return Epast;
		}
		f->flags &= ~Trunc;
		if(r->ifcall.offset < playlist.ndata)
			readbuf(r, playlist.data, playlist.ndata);
		else if(r->ifcall.offset == playlist.ndata){
			r->ofcall.count = 0;
			/* Arrange for this fid to wait next time: */
			f->vers = f->file->dir.qid.vers;
			f->flags |= Eof;
		}else{
			/* Beyond eof, bad seek? */
			return Epast;
		}
	}else if(f->file == files + Qplayctl){
		f->flags &= ~Eof;
		if(m.cmd == Error){
			snprint(r->ofcall.data, sizeof r->indata, "%s %ud %q",
				statetxt[m.cmd], m.off, m.arg);
			free(m.arg);
		}else if(f->vers == f->file->dir.qid.vers){
			r->ofcall.count = 0;
			/* Arrange for this fid to wait next time: */
			f->flags |= Eof;
			return nil;
		}else{
			snprint(r->ofcall.data, sizeof r->indata, "%s %ud",
				statetxt[playstate.cmd], playstate.off);
			f->vers = f->file->dir.qid.vers;
		}
		r->ofcall.count = strlen(r->ofcall.data);
		if(r->ofcall.count > r->ifcall.count)
			r->ofcall.count = r->ifcall.count;
	}else if(f->file == files + Qplayvol){
		f->flags &= ~Eof;
		if(f->vers == f->file->dir.qid.vers){
			r->ofcall.count = 0;
			/* Arrange for this fid to wait next time: */
			f->flags |= Eof;
		}else{
			p = seprint(r->ofcall.data, r->ofcall.data + sizeof r->indata, "volume	'");
			for(i = 0; i < nelem(volume); i++){
				if(volume[i] == Undef)
					break;
				p = seprint(p, r->ofcall.data + sizeof r->indata, "%d ", volume[i]);
			}
			p = seprint(p, r->ofcall.data + sizeof r->indata, "'");
			r->ofcall.count = p - r->ofcall.data;
			if(r->ofcall.count > r->ifcall.count)
				r->ofcall.count = r->ifcall.count;
			f->vers = f->file->dir.qid.vers;
		}
	}else
		abort();
	return nil;
}

char*
rwrite(Worker *w)
{
	long cnt, i, nf, cmd;
	Pmsg newstate;
	char *fields[3], *p, *q;
	Wmsg m;
	Fid *f;
	Req *r;

	r = w->r;
	r->fid = oldfid(r->ifcall.fid);
	if((f = r->fid) == nil)
		return Enotexist;
	r->ofcall.count = 0;
	cnt = r->ifcall.count;

	if(cnt > messagesize - IOHDRSZ)
		cnt = messagesize - IOHDRSZ;

	if(f->file == &files[Qplayctl]){
		r->ifcall.data[cnt] = '\0';
		if (debug & DbgPlayer)
			fprint(2, "rwrite playctl: %s\n", r->ifcall.data);
		nf = tokenize(r->ifcall.data, fields, 4);
		if (nf == 0){
			r->ofcall.count = r->ifcall.count;
			return nil;
		}
		if (nf == 2)
			i = strtol(fields[1], nil, 0);
		else
			i = playstate.off;
		newstate = playstate;
		if ((cmd = lookup(fields[0], statetxt)) < 0)
			return  Ebadctl;
		switch(cmd){
		case Play:
			newstate.cmd = cmd;
			newstate.off = i;
			break;
		case Pause:
			if (playstate.cmd != Play)
				break;
			// fall through
		case Stop:
			newstate.cmd = cmd;
			newstate.off = playstate.off;
			break;
		case Resume:
			if(playstate.cmd == Stop)
				break;
			newstate.cmd = Resume;
			newstate.off = playstate.off;
			break;
		case Skip:
			if (nf == 2)
				i += playstate.off;
			else
				i = playstate.off +1;
			if(i < 0)
				i = 0;
			else if (i >= playlist.nlines)
				i = playlist.nlines - 1;
			newstate.cmd = Play;
			newstate.off = i;
		}
		if (newstate.off >= playlist.nlines){
			newstate.cmd = Stop;
			newstate.off = playlist.nlines;
		}
		if (debug & DbgPlayer)
			fprint(2, "new state %s-%ud\n",
				statetxt[newstate.cmd], newstate.off);
		if (newstate.m != playstate.m)
			sendul(playc, newstate.m);
		f->file->dir.qid.vers++;
	} else if(f->file == &files[Qplayvol]){
		char *subfields[nelem(volume)];
		int v[nelem(volume)];

		r->ifcall.data[cnt] = '\0';
		if (debug & DbgPlayer)
			fprint(2, "rwrite playvol: %s\n", r->ifcall.data);
		nf = tokenize(r->ifcall.data, fields, 4);
		if (nf == 0){
			r->ofcall.count = r->ifcall.count;
			return nil;
		}
		if (nf != 2 || strcmp(fields[0], "volume") != 0)
			return Ebadctl;
		if (debug & DbgPlayer)
			fprint(2, "new volume '");
		nf = tokenize(fields[1], subfields, nelem(subfields));
		if (nf <= 0 || nf > nelem(volume))
			return "volume";
		for (i = 0; i < nf; i++){
			v[i] = strtol(subfields[i], nil, 0);
			if (debug & DbgPlayer)
				fprint(2, " %d", v[i]);
		}
		if (debug & DbgPlayer)
			fprint(2, "'\n");
		while (i < nelem(volume))
			v[i++] = Undef;
		volumeset(v);
		r->ofcall.count = r->ifcall.count;
		return nil;
	} else if(f->file == &files[Qplaylist]){
		if (debug & DbgPlayer){
			fprint(2, "rwrite playlist: `");
			write(2, r->ifcall.data, cnt);
			fprint(2, "'\n");
		}
		playlist.data = realloc(playlist.data, playlist.ndata + cnt + 2);
		if (playlist.data == 0)
			sysfatal("realloc: %r");
		memmove(playlist.data + playlist.ndata, r->ifcall.data, cnt);
		if (playlist.data[playlist.ndata + cnt-1] != '\n')
			playlist.data[playlist.ndata + cnt++] = '\n';
		playlist.data[playlist.ndata + cnt] = '\0';
		p = playlist.data + playlist.ndata;
		while (*p){
			playlist.lines = realloc(playlist.lines, (playlist.nlines+1)*sizeof(playlist.lines[0]));
			if(playlist.lines == nil)
				sysfatal("realloc: %r");
			playlist.lines[playlist.nlines] = playlist.ndata;
			q = strchr(p, '\n');
			if (q == nil)
				break;
			if(debug & DbgPlayer)
				fprint(2, "[%lud]: ", playlist.nlines);
			playlist.nlines++;
			q++;
			if(debug & DbgPlayer)
				write(2, p, q-p);
			playlist.ndata += q - p;
			p = q;
		}
		f->file->dir.length = playlist.ndata;
		f->file->dir.qid.vers++;
	}else
		return Eperm;
	r->ofcall.count = r->ifcall.count;
	m.cmd = Check;
	m.off = 0;
	m.arg = nil;
	bcastmsg(f->file->workers, &m);
	return nil;
}

char *
rclunk(Worker *w)
{
	Fid *f;

	f = oldfid(w->r->ifcall.fid);
	if(f == nil)
		return Enotexist;
	f->flags &= ~(Open|Busy);
	return 0;
}

char *
rremove(Worker*)
{
	return Eperm;
}

char *
rstat(Worker *w)
{
	Req *r;

	r = w->r;
	r->fid = oldfid(r->ifcall.fid);
	if(r->fid == nil)
		return Enotexist;
	r->ofcall.nstat = convD2M(&r->fid->file->dir, r->indata, messagesize - IOHDRSZ);
	r->ofcall.stat = r->indata;
	return 0;
}

char *
rwstat(Worker*)
{
	return Eperm;
}

Fid *
oldfid(int fid)
{
	Fid *f;

	for(f = fids; f; f = f->next)
		if(f->fid == fid)
			return f;
	return nil;
}

Fid *
newfid(int fid)
{
	Fid *f, *ff;

	ff = nil;
	for(f = fids; f; f = f->next)
		if(f->fid == fid){
			return f;
		}else if(ff == nil && (f->flags & Busy) == 0)
			ff = f;
	if(ff == nil){
		ff = mallocz(sizeof *ff, 1);
		if (ff == nil)
			sysfatal("malloc: %r");
		ff->next = fids;
		ff->readers = 0;
		fids = ff;
	}
	ff->fid = fid;
	ff->file = nil;
	ff->vers = ~0;
	return ff;
}

void
work(Worker *w)
{
	Req *r;
	char *err;
	int n;

	r = w->r;
	r->ofcall.data = (char*)r->indata;
	if(!fcalls[r->ifcall.type])
		err = "bad fcall type";
	else
		err = (*fcalls[r->ifcall.type])(w);
	if(err != (char*)~0){	/* ~0 indicates Flush received */
		if(err){
			r->ofcall.type = Rerror;
			r->ofcall.ename = err;
		}else{
			r->ofcall.type = r->ifcall.type + 1;
			r->ofcall.fid = r->ifcall.fid;
		}
		r->ofcall.tag = r->ifcall.tag;
		if(debug & DbgFs)
			fprint(2, "io:->%F\n", &r->ofcall);/**/
		n = convS2M(&r->ofcall, r->outdata, messagesize);
		if(write(srvfd[0], r->outdata, n) != n)
			sysfatal("mount write");
	}
	reqfree(r);
	w->r = nil;
}

void
worker(void *arg)
{
	Worker *w;
	Wmsg m;

	w = arg;
	recv(w->eventc, &m);
	for(;;){
		assert(m.cmd == Work);
		w->r = m.arg;
		if(debug & DbgWorker)
			fprint(2, "worker 0x%p:<-%F\n", w, &w->r->ifcall);
		work(w);
		if(debug & DbgWorker)
			fprint(2, "worker 0x%p wait for next\n", w);
		m = waitmsg(w, workers);
	}
}

void
allocwork(Req *r)
{
	Worker *w;
	Wmsg m;

	m.cmd = Work;
	m.off = 0;
	m.arg = r;
	if(sendmsg(workers, &m))
		return;
	/* No worker ready to accept request, allocate one */
	w = malloc(sizeof(Worker));
	w->eventc = chancreate(sizeof(Wmsg), 1);
	if(debug & DbgWorker)
		fprint(2, "new worker 0x%p\n", w);/**/
	threadcreate(worker, w, 4096);
	send(w->eventc, &m);
}

void
srvio(void *arg)
{
	char e[32];
	int n;
	Req *r;
	Channel *dispatchc;

	threadsetname("file server IO");
	dispatchc = arg;

	r = reqalloc();
	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read9pmsg(srvfd[0], r->indata, messagesize);
		if(n == 0)
			continue;
		if(n < 0){
			rerrstr(e, sizeof e);
			if (strcmp(e, "interrupted") == 0){
				if (debug & DbgFs) fprint(2, "read9pmsg interrupted\n");
				continue;
			}
			sysfatal("srvio: %s", e);
		}
		if(convM2S(r->indata, n, &r->ifcall) == 0)
			continue;

		if(debug & DbgFs)
			fprint(2, "io:<-%F\n", &r->ifcall);
		sendp(dispatchc, r);
		r = reqalloc();
	}
}

char *
getplaylist(int n)
{
	Wmsg m;

	m.cmd = Preq;
	m.off = n;
	m.arg = nil;
	send(playlistreq, &m);
	recv(playlistreq, &m);
	if(m.cmd == Error)
		return nil;
	assert(m.cmd == Prep);
	assert(m.arg);
	return m.arg;
}

void
playlistsrv(void*)
{
	Wmsg m;
	char *p, *q, *r;
	char *fields[2];
	int n;
	/* Runs in the srv proc */

	threadsetname("playlistsrv");
	while(recv(playlistreq, &m)){
		assert(m.cmd == Preq);
		m.cmd = Error;
		if(m.off < playlist.nlines){
			p = playlist.data + playlist.lines[m.off];
			q = strchr(p, '\n');
			if (q == nil)
				sysfatal("playlistsrv: no newline character found");
			n = q-p;
			r = malloc(n+1);
			memmove(r, p, n);
			r[n] = 0;
			tokenize(r, fields, nelem(fields));
			assert(fields[0] == r);
			m.cmd = Prep;
			m.arg = r;
		}
		send(playlistreq, &m);
	}
}

void
srv(void*)
{
	Req *r;
	Channel *dispatchc;
	/*
	 * This is the proc with all the action.
	 * When a file request comes in, it is dispatched to this proc
	 * for processing.  Two extra threads field changes in play state
	 * and volume state.
	 * By keeping all the action in this proc, we won't need locks
	 */

	threadsetname("srv");
	close(srvfd[1]);

	dispatchc = chancreate(sizeof(Req*), 1);
	procrfork(srvio, dispatchc, 4096, RFFDG);

	threadcreate(volumeupdater, nil, 4096);
	threadcreate(playupdater, nil, 4096);
	threadcreate(playlistsrv, nil, 4096);

	while(r = recvp(dispatchc))
		allocwork(r);
		
}

void
playupdater(void*)
{
	Wmsg m;
	/* This is a thread in the srv proc */

	while(recv(playchan, &m)){
		if(debug & DbgPlayer)
			fprint(2, "playupdate: %s %d %s\n", statetxt[m.cmd], m.off, m.arg?m.arg:"");
		if(playstate.m == m.m)
			continue;
		if(m.cmd == Stop && m.off == 0xffff)
			m.off = playlist.nlines;
		if(m.cmd != Error){
			playstate.m = m.m;
			m.cmd = Check;
			assert(m.arg == nil);
		}
		files[Qplayctl].dir.qid.vers++;
		bcastmsg(files[Qplayctl].workers, &m);
	}
}

void
volumeupdater(void*)
{
	Wmsg m;
	int v[nelem(volume)];
	/* This is a thread in the srv proc */

	while(recv(volumechan, v)){
		if(debug & DbgPlayer)
			fprint(2, "volumeupdate: volume now %d %d %d %d\n", volume[0], volume[1], volume[2], volume[3]);
		memmove(volume, v, sizeof(volume));
		files[Qplayvol].dir.qid.vers++;
		m.cmd = Check;
		m.arg = nil;
		bcastmsg(files[Qplayvol].workers, &m);
	}
}

void
playupdate(Pmsg p, char *s)
{
	Wmsg m;

	m.m = p.m;
	m.arg = s ? strdup(s) : nil;
	send(playchan, &m);
}
