#include	<u.h>
#include	<libc.h>

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

typedef	struct	Rs	Rs;

enum
{
	BSIZE	= 48*1024,
	SLEEP	= 10,
	SECTOR	= 2048,
};

int	TARGET	= 1;	/* scsi target of rom writer */
int	MEMSIZE	= 10;	/* in megabytes */
int	NPROC	= 3;	/* number of read-ahead processes */
int	NBUF;		/* (MEMSIZE*1024*1024)/BSIZE/NPROC */

struct	Rs
{
	uchar*	buf;
	int	nin;		/* whole buffers read */
	int	nout;		/* whole buffers consumed */
	int	done;		/* reader is done */
	int	last;		/* partial bytes of last buffer */
};

int	scsifc;
int	scsifd;
int	error;
Rs	rs[50];

void	reader(char*, int);
void	checkerror(void);
void	puts(char*);
void	putn(int);
void	scsi(uchar*, int, uchar*, int);

void
main(int argc, char *argv[])
{
	char *file;
	int i, first;
	long ob, t, total;
	uchar cmd[10], cmdwt[10], cmdwr[10];
	char scsiname[50];
	Dir dir;
	Rs *r;

	file = "cd-rom";
	ARGBEGIN {
	default:
		fprint(2, "unknown option: %c\n", ARGC());
		break;
	case 't':
		TARGET = atoi(ARGF());
		break;
	case 'm':
		MEMSIZE = atoi(ARGF());
		break;
	case 'n':
		NPROC = atoi(ARGF());
		break;
	} ARGEND
	if(argc > 0)
		file = argv[0];

	if(NPROC >= nelem(rs)) {
		fprint(2, "too many processes\n");
		exits("-n");
	}
	NBUF = (MEMSIZE*1024*1024)/BSIZE/NPROC;
	for(r=rs; r<rs+NPROC; r++) {
		r->buf = malloc(NBUF*BSIZE);
		if(r->buf == 0) {
			fprint(2, "malloc failed\n");
			exits("-m");
		}
		r->nin = 0;
		r->nout = 0;
		r->done = 0;
	}

	sprint(scsiname, "#S/%d/cmd", TARGET);
	scsifc = open(scsiname, ORDWR);
	sprint(scsiname, "#S/%d/data", TARGET);
	scsifd = open(scsiname, ORDWR);
	if(scsifc < 0 || scsifd < 0) {
		fprint(2, "cant open scsi chan %r\n");
		exits("open");
	}
	if(dirstat(file, &dir) < 0) {
		fprint(2, "cant stat %s %r\n", file);
		exits("stat");
	}

	/*
	 * reserve track
	 */
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xe4;
	t = (dir.length + SECTOR - 1) / SECTOR;
	cmd[5] = t>>24;
	cmd[6] = t>>16;
	cmd[7] = t>>8;
	cmd[8] = t>>0;
	scsi(cmd, 10, cmd, 0);

	/*
	 * write track
	 */
	memset(cmdwt, 0, sizeof(cmdwt));
	cmdwt[0] = 0xe6;
	cmdwt[5] = 1;
	cmdwt[6] = 1;
	cmdwt[7] = (BSIZE/SECTOR)>>8;
	cmdwt[8] = (BSIZE/SECTOR)>>0;

	/*
	 * write
	 */
	memset(cmdwr, 0, sizeof(cmdwr));
	cmdwr[0] = 0x2a;
	cmdwr[7] = (BSIZE/SECTOR)>>8;
	cmdwr[8] = (BSIZE/SECTOR)>>0;

	/*
	 * start all the reading processes
	 */
	error = 0;
	for(i=0; i<NPROC; i++)
		if(rfork(RFPROC|RFNOWAIT|RFFDG|RFMEM) == 0)
			reader(file, i);

	/*
	 * wait until we have full buffers
	 */
	for(;;) {
		checkerror();
		for(r=rs; r<rs+NPROC; r++)
			if(!r->done && r->nin<NBUF)
				goto wfull;
		break;
	wfull:
		sleep(SLEEP);
	}

	/*
	 * copy buffers to output
	 */
	t = time(0);
	total = 0;
	r = rs;
	ob = 0;
	first = 1;
	for(;;) {
		checkerror();
		if(r->nin - r->nout > 0) {
			if(first) {
				scsi(cmdwt, 10, r->buf+ob, BSIZE);
				first = 0;
			} else {
				scsi(cmdwr, 10, r->buf+ob, BSIZE);
			}
			total += BSIZE;
			r->nout++;

			r++;
			if(r >= rs+NPROC) {
				r = rs;
				ob += BSIZE;
				if(ob >= NBUF*BSIZE)
					ob = 0;
			}
			continue;
		}
		if(r->done) {
			if(r->last) {
				i = (r->last + SECTOR - 1) / SECTOR;
				if(first) {
					cmdwt[7] = i>>8;
					cmdwt[8] = i>>0;
					scsi(cmdwt, 10, r->buf+ob, i*SECTOR);
				} else {
					cmdwr[7] = i>>8;
					cmdwr[8] = i>>0;
					scsi(cmdwr, 10, r->buf+ob, i*SECTOR);
				}
			}
			total += r->last;
			break;
		}
		sleep(SLEEP);
		continue;
	}

	/*
	 * flush cache
	 */
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x35;
	scsi(cmd, 10, cmd, 0);

	t = time(0)-t;
	if(t <= 0)
		t = 1;
	fprint(2, "rate = %ld (%ld/%ld)\n", total/t, total, t);

	/*
	 * fixate
	 */
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xe9;
	cmd[8] = 1;
	scsi(cmd, 10, cmd, 0);

	fprint(2, "done\n");

out:
	error = 1;
	exits(0);
}

void
checkerror(void)
{
	if(error) {
		puts("error set: exiting\n");
		exits(0);
	}
}

void
puts(char *s)
{
	write(2, s, strlen(s));
}

void
putn(int n)
{
	char x[10];

	strcpy(x, "x: ");
	x[0] = n+'0';
	puts(x);
}

void
reader(char *file, int pn)
{
	Rs *r;
	int f, n;
	long of, ob;

	f = open(file, OREAD);
	if(f < 0) {
		putn(pn);
		puts("cant open ");
		puts(file);
		puts("\n");
		error = 1;
		exits(0);
	}
	r = rs+pn;
	of = pn*BSIZE;
	ob = 0;
	for(;;) {
		checkerror();
		if(r->nin - r->nout >= NBUF) {
			sleep(SLEEP);
			continue;
		}
		seek(f, of, 0);
		n = read(f, r->buf+ob, BSIZE);
		if(n == BSIZE) {
			r->nin++;
			of += NPROC*BSIZE;
			ob += BSIZE;
			if(ob >= NBUF*BSIZE)
				ob = 0;
			continue;
		}
		if(n < 0) {
			putn(pn);
			puts("io error reading\n");
			error = 1;
			exits(0);
		}
		r->last = n;
		r->done = 1;
		exits(0);
	}
}

void
scsi(uchar *cmd, int ccount, uchar *data, int dcount)
{
	uchar resp[6], sense[255];
	long status;

	if(write(scsifc, cmd, ccount) != ccount) {
		fprint(2, "scsi write ctl %r\n");
		exits("wctl");
	}
	if(write(scsifd, data, dcount) != dcount) {
		fprint(2, "scsi write data %r\n");
		exits("wdata");
	}
	if(read(scsifc, resp, 4) != 4) {
		fprint(2, "scsi read ctl %r\n");
		exits("rctl");
	}
	status = (resp[0]<<24) | (resp[1]<<16) | (resp[2]<<8) | (resp[3]<<0);
	if(status == 0x6000)
		return;
	if(status != 0x6002) {
		fprint(2, "bad status %.4x\n", status);
		exits("status");
	}

	/*
	 * request sense
	 */
	memset(resp, 0, sizeof(resp));
	resp[0] = 0x03;
	resp[4] = sizeof(sense);

	if(write(scsifc, resp, sizeof(resp)) != sizeof(resp)) {
		fprint(2, "scsi write ctl1 %r\n");
		exits("wctl");
	}
	if(read(scsifd, sense, sizeof(sense)) <= 0) {
		fprint(2, "scsi read data %r\n");
		exits("rdata");
	}
	if(read(scsifc, resp, 4) != 4) {
		fprint(2, "scsi read ctl %r\n");
		exits("rctl");
	}
	fprint(2, "scsi reqsense: key #%2.2ux code #%2.2ux #%2.2ux\n",
		sense[2], sense[12], sense[13]);
	if((sense[2] & 0x0F) == 0x00 || (sense[2] & 0x0F) == 0x06)
		return;
	fprint(2, "exiting\n");
	exits("scsi");
}
