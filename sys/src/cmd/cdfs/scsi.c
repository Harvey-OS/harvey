/*
 * Now thread-safe.
 *
 * The codeqlock guarantees that once codes != nil, that pointer will never 
 * change nor become invalid.
 *
 * The QLock in the Scsi structure moderates access to the raw device.
 * We should probably export some of the already-locked routines, but
 * there hasn't been a need.
 */

#include <u.h>
#include <libc.h>
#include <disk.h>

enum {
	/* commands */
	Testrdy		= 0x00,
	Reqsense	= 0x03,
	Write10		= 0x2a,
	Writever10	= 0x2e,
	Readtoc		= 0x43,

	/* sense[2] (key) sense codes */
	Sensenone	= 0,
	Sensenotrdy	= 2,
	Sensebadreq	= 5,

	/* sense[12] (asc) sense codes */
	Lunnotrdy	= 0x04,
	Recovnoecc	= 0x17,
	Recovecc	= 0x18,
	Badcdb		= 0x24,
	Newmedium	= 0x28,
	Nomedium	= 0x3a,
};

int scsiverbose;

#define codefile "/sys/lib/scsicodes"

static char *codes;
static QLock codeqlock;

static void
getcodes(void)
{
	Dir *d;
	int n, fd;

	if(codes != nil)
		return;

	qlock(&codeqlock);
	if(codes != nil) {
		qunlock(&codeqlock);
		return;
	}

	if((d = dirstat(codefile)) == nil || (fd = open(codefile, OREAD)) < 0) {
		qunlock(&codeqlock);
		return;
	}

	codes = malloc(1+d->length+1);
	if(codes == nil) {
		close(fd);
		qunlock(&codeqlock);
		free(d);
		return;
	}

	codes[0] = '\n';	/* for searches */
	n = readn(fd, codes+1, d->length);
	close(fd);
	free(d);

	if(n < 0) {
		free(codes);
		codes = nil;
		qunlock(&codeqlock);
		return;
	}
	codes[n] = '\0';
	qunlock(&codeqlock);
}
	
char*
scsierror(int asc, int ascq)
{
	char *p, *q;
	static char search[32];
	static char buf[128];

	getcodes();

	if(codes) {
		snprint(search, sizeof search, "\n%.2ux%.2ux ", asc, ascq);
		if(p = strstr(codes, search)) {
			p += 6;
			if((q = strchr(p, '\n')) == nil)
				q = p+strlen(p);
			snprint(buf, sizeof buf, "%.*s", (int)(q-p), p);
			return buf;
		}

		snprint(search, sizeof search, "\n%.2ux00", asc);
		if(p = strstr(codes, search)) {
			p += 6;
			if((q = strchr(p, '\n')) == nil)
				q = p+strlen(p);
			snprint(buf, sizeof buf, "(ascq #%.2ux) %.*s", ascq, (int)(q-p), p);
			return buf;
		}
	}

	snprint(buf, sizeof buf, "scsi #%.2ux %.2ux", asc, ascq);
	return buf;
}


static int
_scsicmd(Scsi *s, uchar *cmd, int ccount, void *data, int dcount, int io, int dolock)
{
	uchar resp[16];
	int n;
	long status;

	if(dolock)
		qlock(s);
	if(write(s->rawfd, cmd, ccount) != ccount) {
		werrstr("cmd write: %r");
		if(dolock)
			qunlock(s);
		return -1;
	}

	switch(io){
	case Sread:
		n = read(s->rawfd, data, dcount);
		/* read toc errors are frequent and not very interesting */
		if(n < 0 && (scsiverbose == 1 ||
		    scsiverbose == 2 && cmd[0] != Readtoc))
			fprint(2, "dat read: %r: cmd 0x%2.2uX\n", cmd[0]);
		break;
	case Swrite:
		n = write(s->rawfd, data, dcount);
		if(n != dcount && scsiverbose)
			fprint(2, "dat write: %r: cmd 0x%2.2uX\n", cmd[0]);
		break;
	default:
	case Snone:
		n = write(s->rawfd, resp, 0);
		if(n != 0 && scsiverbose)
			fprint(2, "none write: %r: cmd 0x%2.2uX\n", cmd[0]);
		break;
	}

	memset(resp, 0, sizeof(resp));
	if(read(s->rawfd, resp, sizeof(resp)) < 0) {
		werrstr("resp read: %r\n");
		if(dolock)
			qunlock(s);
		return -1;
	}
	if(dolock)
		qunlock(s);

	resp[sizeof(resp)-1] = '\0';
	status = atoi((char*)resp);
	if(status == 0)
		return n;

	werrstr("cmd %2.2uX: status %luX dcount %d n %d", cmd[0], status, dcount, n);
	return -1;
}

int
scsicmd(Scsi *s, uchar *cmd, int ccount, void *data, int dcount, int io)
{
	return _scsicmd(s, cmd, ccount, data, dcount, io, 1);
}

static int
_scsiready(Scsi *s, int dolock)
{
	char err[ERRMAX];
	uchar cmd[6], resp[16];
	int status, i;

	if(dolock)
		qlock(s);
	werrstr("");
	for(i=0; i<3; i++) {
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = Testrdy;	/* unit ready */
		if(write(s->rawfd, cmd, sizeof(cmd)) != sizeof(cmd)) {
			if(scsiverbose)
				fprint(2, "ur cmd write: %r\n");
			werrstr("short unit-ready raw write");
			continue;
		}
		write(s->rawfd, resp, 0);
		if(read(s->rawfd, resp, sizeof(resp)) < 0) {
			if(scsiverbose)
				fprint(2, "ur resp read: %r\n");
			continue;
		}
		resp[sizeof(resp)-1] = '\0';
		status = atoi((char*)resp);
		if(status == 0 || status == 0x02) {
			if(dolock)
				qunlock(s);
			return 0;
		}
		if(scsiverbose)
			fprint(2, "target: bad status: %x\n", status);
	}
	rerrstr(err, sizeof err);
	if(err[0] == '\0')
		werrstr("unit did not become ready");
	if(dolock)
		qunlock(s);
	return -1;
}

int
scsiready(Scsi *s)
{
	return _scsiready(s, 1);
}

int
scsi(Scsi *s, uchar *cmd, int ccount, void *v, int dcount, int io)
{
	uchar req[6], sense[255], *data;
	int tries, code, key, n;
	char *p;

	data = v;
	SET(key, code);
	qlock(s);
	for(tries=0; tries<2; tries++) {
		n = _scsicmd(s, cmd, ccount, data, dcount, io, 0);
		if(n >= 0) {
			qunlock(s);
			return n;
		}

		/*
		 * request sense
		 */
		memset(req, 0, sizeof(req));
		req[0] = Reqsense;
		req[4] = sizeof(sense);
		memset(sense, 0xFF, sizeof(sense));
		if((n=_scsicmd(s, req, sizeof(req), sense, sizeof(sense), Sread, 0)) < 14)
			if(scsiverbose)
				fprint(2, "reqsense scsicmd %d: %r\n", n);
	
		if(_scsiready(s, 0) < 0)
			if(scsiverbose)
				fprint(2, "unit not ready\n");
	
		key = sense[2] & 0xf;
		code = sense[12];			/* asc */
		if(code == Recovnoecc || code == Recovecc) { /* recovered errors */
			qunlock(s);
			return dcount;
		}

		/* retry various odd cases */
		if(code == Newmedium && cmd[0] == Readtoc) {
			/* read toc and media changed */
			s->nchange++;
			s->changetime = time(0);
		} else if((cmd[0] == Write10 || cmd[0] == Writever10) &&
		    key == Sensenotrdy &&
		    code == Lunnotrdy && sense[13] == 0x08) {
			/* long write in progress, per mmc-6 */
			tries = 0;
			sleep(1);
		} else if(cmd[0] == Write10 || cmd[0] == Writever10)
			break;		/* don't retry worm writes */
	}

	/* drive not ready, or medium not present */
	if(cmd[0] == Readtoc && key == Sensenotrdy &&
	    (code == Nomedium || code == Lunnotrdy)) {
		s->changetime = 0;
		qunlock(s);
		return -1;
	}
	qunlock(s);

	if(cmd[0] == Readtoc && key == Sensebadreq && code == Badcdb)
		return -1;			/* blank media */

	p = scsierror(code, sense[13]);

	werrstr("cmd #%.2ux: %s", cmd[0], p);

	if(scsiverbose)
		fprint(2, "scsi cmd #%.2ux: %.2ux %.2ux %.2ux: %s\n",
			cmd[0], key, code, sense[13], p);

//	if(key == Sensenone)
//		return dcount;
	return -1;
}

Scsi*
openscsi(char *dev)
{
	Scsi *s;
	int rawfd, ctlfd, l, n;
	char *name, *p, buf[512];

	l = strlen(dev)+1+3+1;
	name = malloc(l);
	if(name == nil)
		return nil;

	snprint(name, l, "%s/raw", dev);
	if((rawfd = open(name, ORDWR)) < 0) {
		free(name);
		return nil;
	}

	snprint(name, l, "%s/ctl", dev);
	if((ctlfd = open(name, ORDWR)) < 0) {
	Error:
		free(name);
		close(rawfd);
		return nil;
	}

	n = readn(ctlfd, buf, sizeof buf);
	close(ctlfd);
	if(n <= 0) {
		if(n == 0)
			werrstr("eof on %s", name);
		goto Error;
	}

	if(strncmp(buf, "inquiry ", 8) != 0 || (p = strchr(buf, '\n')) == nil) {
		werrstr("inquiry mal-formatted in %s", name);
		goto Error;
	}
	*p = '\0';
	free(name);
	name = nil;

	if((p = strdup(buf+8)) == nil)
		goto Error;

	s = mallocz(sizeof(*s), 1);
	if(s == nil) {
	Error1:
		free(p);
		goto Error;
	}

	s->rawfd = rawfd;
	s->inquire = p;
	s->changetime = time(0);
	
	if(scsiready(s) < 0)
		goto Error1;

	return s;
}

void
closescsi(Scsi *s)
{
	close(s->rawfd);
	free(s->inquire);
	free(s);
}
