/* cached-worm file server */
#include "all.h"
#include "io.h"
#include "9p1.h"

extern int oldcachefmt;

Map *devmap;

Biobuf bin;

void
machinit(void)
{
	active.exiting = 0;
}

/*
 * Put a string on the console.
 */
void
puts(char *s, int n)
{
	print("%.*s", n, s);
}

void
prflush(void)
{
}

/*
 * Print a string on the console.
 */
void
putstrn(char *str, int n)
{
	puts(str, n);
}

/*
 * get a character from the console
 */
int
getc(void)
{
	return Bgetrune(&bin);
}

void
panic(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = vseprint(buf, buf + sizeof buf, fmt, arg) - buf;
	va_end(arg);
	buf[n] = '\0';
	print("panic: %s\n", buf);
	exit();
}

int
okay(char *quest)
{
	char *ln;

	print("okay to %s? ", quest);
	if ((ln = Brdline(&bin, '\n')) == nil)
		return 0;
	ln[Blinelen(&bin)-1] = '\0';
	if (isascii(*ln) && isupper(*ln))
		*ln = tolower(*ln);
	return *ln == 'y';
}

static void
mapinit(char *mapfile)
{
	int nf;
	char *ln;
	char *fields[2];
	Biobuf *bp;
	Map *map;

	if (mapfile == nil)
		return;
	bp = Bopen(mapfile, OREAD);
	if (bp == nil)
		sysfatal("can't read %s", mapfile);
	devmap = nil;
	while ((ln = Brdline(bp, '\n')) != nil) {
		ln[Blinelen(bp)-1] = '\0';
		if (*ln == '\0' || *ln == '#')
			continue;
		nf = tokenize(ln, fields, nelem(fields));
		if (nf != 2)
			continue;
		if(testconfig(fields[0]) != 0) {
			print("bad `from' device %s in %s\n",
				fields[0], mapfile);
			continue;
		}
		map = malloc(sizeof *map);
		map->from = strdup(fields[0]);
		map->to =   strdup(fields[1]);
		map->fdev = iconfig(fields[0]);
		map->tdev = nil;
		if (access(map->to, AEXIST) < 0) {
			/*
			 * map->to isn't an existing file, so it had better be
			 * a config string for a device.
			 */
			if(testconfig(fields[1]) == 0)
				map->tdev = iconfig(fields[1]);
		}
		/* else map->to is the replacement file name */
		map->next = devmap;
		devmap = map;
	}
	Bterm(bp);
}

static void
confinit(void)
{
	conf.nmach = 1;

	conf.mem = meminit();

	conf.nuid = 1000;
	conf.nserve = 15;		/* tunable */
	conf.nfile = 30000;
	conf.nlgmsg = 100;
	conf.nsmmsg = 500;

	localconfinit();

	conf.nwpath = conf.nfile*8;
	conf.nauth =  conf.nfile/10;
	conf.gidspace = conf.nuid*3;

	cons.flags = 0;

	if (conf.devmap)
		mapinit(conf.devmap);
}

/*
 * compute BUFSIZE*(NDBLOCK+INDPERBUF+INDPERBUF⁲+INDPERBUF⁳+INDPERBUF⁴)
 * while watching for overflow; in that case, return 0.
 */

static uvlong
adduvlongov(uvlong a, uvlong b)
{
	uvlong r = a + b;

	if (r < a || r < b)
		return 0;
	return r;
}

static uvlong
muluvlongov(uvlong a, uvlong b)
{
	uvlong r = a * b;

	if (a != 0 && r/a != b || r < a || r < b)
		return 0;
	return r;
}

static uvlong
maxsize(void)
{
	int i;
	uvlong max = NDBLOCK, ind = 1;

	for (i = 0; i < NIBLOCK; i++) {
		ind = muluvlongov(ind, INDPERBUF);	/* power of INDPERBUF */
		if (ind == 0)
			return 0;
		max = adduvlongov(max, ind);
		if (max == 0)
			return 0;
	}
	return muluvlongov(max, BUFSIZE);
}

enum {
	INDPERBUF⁲ = ((uvlong)INDPERBUF *INDPERBUF),
	INDPERBUF⁴ = ((uvlong)INDPERBUF⁲*INDPERBUF⁲),
};

static void
printsizes(void)
{
	uvlong max = maxsize();

	print("\tblock size = %d; ", RBUFSIZE);
	if (max == 0)
		print("max file size exceeds 2⁶⁴ bytes\n");
	else {
		uvlong offlim = 1ULL << (sizeof(Off)*8 - 1);

		if (max >= offlim)
			max = offlim - 1;
		print("max file size = %,llud\n", (Wideoff)max);
	}
	if (INDPERBUF⁲/INDPERBUF != INDPERBUF)
		print("overflow computing INDPERBUF⁲\n");
	if (INDPERBUF⁴/INDPERBUF⁲ != INDPERBUF⁲)
		print("overflow computing INDPERBUF⁴\n");
	print("\tINDPERBUF = %d, INDPERBUF^4 = %,lld, ", INDPERBUF,
		(Wideoff)INDPERBUF⁴);
	print("CEPERBK = %d\n", CEPERBK);
	print("\tsizeofs: Dentry = %d, Cache = %d\n",
		sizeof(Dentry), sizeof(Cache));
}

void
usage(void)
{
	fprint(2, "usage: %s [-cf][-a ann-str][-m dev-map] config-dev\n",
		argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, nets = 0;
	char *ann;

	rfork(RFNOTEG);
	formatinit();
	machinit();
	conf.confdev = "n";		/* Devnone */

	ARGBEGIN{
	case 'a':			/* announce on this net */
		ann = EARGF(usage());
		if (nets >= Maxnets) {
			fprint(2, "%s: too many networks to announce: %s\n",
				argv0, ann);
			exits("too many nets");
		}
		annstrs[nets++] = ann;
		break;
	case 'c':			/* use new, faster cache layout */
		oldcachefmt = 0;
		break;
	case 'f':			/* enter configuration mode first */
		conf.configfirst++;
		break;
	case 'm':			/* name device-map file */
		conf.devmap = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if (argc != 1)
		usage();
	conf.confdev = argv[0];	/* config string for dev holding full config */

	Binit(&bin, 0, OREAD);
	confinit();

	print("\nPlan 9 %d-bit cached-worm file server with %d-deep indir blks\n",
		sizeof(Off)*8 - 1, NIBLOCK);
	printsizes();

	qlock(&reflock);
	qunlock(&reflock);
	serveq = newqueue(1000, "9P service");	/* tunable */
	raheadq = newqueue(1000, "readahead");	/* tunable */

	mbinit();
	netinit();
	scsiinit();

	files = malloc(conf.nfile * sizeof *files);
	for(i=0; i < conf.nfile; i++) {
		qlock(&files[i]);
		qunlock(&files[i]);
	}

	wpaths = malloc(conf.nwpath * sizeof(*wpaths));
	uid = malloc(conf.nuid * sizeof(*uid));
	gidspace = malloc(conf.gidspace * sizeof(*gidspace));
	authinit();

	print("iobufinit\n");
	iobufinit();

	arginit();
	boottime = time(nil);

	print("sysinit\n");
	sysinit();

	/*
	 * Ethernet i/o processes
	 */
	netstart();

	/*
	 * read ahead processes
	 */
	newproc(rahead, 0, "rah");

	/*
	 * server processes
	 */
	for(i=0; i < conf.nserve; i++)
		newproc(serve, 0, "srv");

	/*
	 * worm "dump" copy process
	 */
	newproc(wormcopy, 0, "wcp");

	/*
	 * processes to read the console
	 */
	consserve();

	/*
	 * "sync" copy process
	 * this doesn't return.
	 */
	procsetname("scp");
	synccopy();
}

/*
 * read ahead processes.
 * read message from q and then
 * read the device.
 */
int
rbcmp(void *va, void *vb)
{
	Rabuf *ra, *rb;

	ra = *(Rabuf**)va;
	rb = *(Rabuf**)vb;
	if(rb == 0)
		return 1;
	if(ra == 0)
		return -1;
	if(ra->dev > rb->dev)
		return 1;
	if(ra->dev < rb->dev)
		return -1;
	if(ra->addr > rb->addr)
		return 1;
	if(ra->addr < rb->addr)
		return -1;
	return 0;
}

void
rahead(void *)
{
	Rabuf *rb[50];
	Iobuf *p;
	int i, n;

	for (;;) {
		rb[0] = fs_recv(raheadq, 0);
		for(n = 1; n < nelem(rb); n++) {
			if(raheadq->count <= 0)
				break;
			rb[n] = fs_recv(raheadq, 0);
		}
		qsort(rb, n, sizeof rb[0], rbcmp);
		for(i = 0; i < n; i++) {
			if(rb[i] == 0)
				continue;
			p = getbuf(rb[i]->dev, rb[i]->addr, Brd);
			if(p)
				putbuf(p);
			lock(&rabuflock);
			rb[i]->link = rabuffree;
			rabuffree = rb[i];
			unlock(&rabuflock);
		}
	}
}

/*
 * main filesystem server loop.
 * entered by many processes.
 * they wait for message buffers and
 * then process them.
 */
void
serve(void *)
{
	int i;
	Chan *cp;
	Msgbuf *mb;

	for (;;) {
		qlock(&reflock);
		/* read 9P request from a network input process */
		mb = fs_recv(serveq, 0);
		assert(mb->magic == Mbmagic);
		/* fs kernel sets chan in /sys/src/fs/ip/il.c:/^getchan */
		cp = mb->chan;
		if (cp == nil)
			panic("serve: nil mb->chan");
		rlock(&cp->reflock);
		qunlock(&reflock);

		rlock(&mainlock);

		if (mb->data == nil)
			panic("serve: nil mb->data");
		/* better sniffing code in /sys/src/cmd/disk/kfs/9p12.c */
		if(cp->protocol == nil){
			/* do we recognise the protocol in this packet? */
			/* better sniffing code: /sys/src/cmd/disk/kfs/9p12.c */
			for(i = 0; fsprotocol[i] != nil; i++)
				if(fsprotocol[i](mb) != 0) {
					cp->protocol = fsprotocol[i];
					break;
				}
			if(cp->protocol == nil){
				print("no protocol for message\n");
				for(i = 0; i < 12; i++)
					print(" %2.2uX", mb->data[i]);
				print("\n");
			}
		} else
			/* process the request, generate an answer and reply */
			cp->protocol(mb);

		mbfree(mb);
		runlock(&mainlock);
		runlock(&cp->reflock);
	}
}

void
exit(void)
{
	lock(&active);
	active.exiting = 1;
	unlock(&active);

	print("halted at %T.\n", time(nil));
	postnote(PNGROUP, getpid(), "die");
	exits(nil);
}

enum {
	DUMPTIME = 5,	/* 5 am */
	WEEKMASK = 0,	/* every day (1=sun, 2=mon, 4=tue, etc.) */
};

/*
 * calculate the next dump time.
 * minimum delay is 100 minutes.
 */
Timet
nextdump(Timet t)
{
	Timet nddate = nextime(t+MINUTE(100), DUMPTIME, WEEKMASK);

	if(!conf.nodump)
		print("next dump at %T\n", nddate);
	return nddate;
}

/*
 * process to copy dump blocks from
 * cache to worm. it runs flat out when
 * it gets work, but only looks for
 * work every 10 seconds.
 */
void
wormcopy(void *)
{
	int f, dorecalc = 1;
	Timet dt, t = 0, nddate = 0, ntoytime = 0;
	Filsys *fs;

	for (;;) {
		if (dorecalc) {
			dorecalc = 0;
			t = time(nil);
			nddate = nextdump(t);		/* chatters */
			ntoytime = time(nil);
		}
		dt = time(nil) - t;
		if(dt < 0 || dt > MINUTE(100)) {
			if(dt < 0)
				print("time went back\n");
			else
				print("time jumped ahead\n");
			dorecalc = 1;
			continue;
		}
		t += dt;
		f = 0;
		if(t > ntoytime)
			ntoytime = time(nil) + HOUR(1);
		else if(t > nddate) {
			if(!conf.nodump) {
				print("automatic dump %T\n", t);
				for(fs=filsys; fs->name; fs++)
					if(fs->dev->type == Devcw)
						cfsdump(fs);
			}
			dorecalc = 1;
		} else {
			rlock(&mainlock);
			for(fs=filsys; fs->name; fs++)
				if(fs->dev->type == Devcw)
					f |= dumpblock(fs->dev);
			runlock(&mainlock);
			if(!f)
				delay(10000);
			wormprobe();
		}
	}
}

/*
 * process to synch blocks
 * it puts out a block/cache-line every second
 * it waits 10 seconds if caught up.
 * in both cases, it takes about 10 seconds
 * to get up-to-date.
 */
void
synccopy(void)
{
	int f;

	for (;;) {
		rlock(&mainlock);
		f = syncblock();
		runlock(&mainlock);
		if(!f)
			delay(10000);
		else
			delay(1000);
	}
}

Devsize
inqsize(char *file)
{
	int nf;
	char *ln, *end, *data = malloc(strlen(file) + 5 + 1);
	char *fields[4];
	Devsize rv = -1;
	Biobuf *bp;

	strcpy(data, file);
	end = strstr(data, "/data");
	if (end == nil)
		strcat(data, "/ctl");
	else
		strcpy(end, "/ctl");
	bp = Bopen(data, OREAD);
	if (bp) {
		while (rv < 0 && (ln = Brdline(bp, '\n')) != nil) {
			ln[Blinelen(bp)-1] = '\0';
			nf = tokenize(ln, fields, nelem(fields));
			if (nf == 3 && strcmp(fields[0], "geometry") == 0)
				rv = atoi(fields[2]);
		}
		Bterm(bp);
	}
	free(data);
	return rv;
}
