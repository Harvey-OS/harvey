#include <u.h>
#include <libc.h>
#include <bio.h>

#include "scsireq.h"

#define MIN(a, b)	((a) < (b) ? (a): (b))

int bus;
Biobuf bin, bout;
static char rwbuf[MaxIOsize];
static int verbose = 1;

typedef struct {
	char *name;
	long (*f)(ScsiReq *, int, char *[]);
	int open;
	char *help;
} ScsiCmd;
static ScsiCmd scsicmd[];

static long
cmdready(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRready(rp);
}

static long
cmdrewind(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRrewind(rp);
}

static long
cmdreqsense(ScsiReq *rp, int argc, char *argv[])
{
	long nbytes;

	USED(argc, argv);
	if((nbytes = SRreqsense(rp)) != -1)
		makesense(rp);
	return nbytes;
}

static long
cmdformat(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRformat(rp);
}

static long
cmdrblimits(ScsiReq *rp, int argc, char *argv[])
{
	uchar l[6];
	long n;

	USED(argc, argv);
	if((n = SRrblimits(rp, l)) == -1)
		return -1;
	Bprint(&bout, " %2.2ux %2.2ux %2.2ux %2.2ux %2.2ux %2.2ux\n",
		l[0], l[1], l[2], l[3], l[4], l[5]);
	return n;
}

static int
mkfile(char *file, int omode, int *pid)
{
	int fd[2];

	if(*file != '|'){
		*pid = -1;
		if(omode == OWRITE)
			return create(file, OWRITE, 0666);
		else if(omode == OREAD)
			return open(file, OREAD);
		return -1;
	}

	file++;
	if(*file == 0 || pipe(fd) == -1)
		return -1;
	if((*pid = fork()) == -1){
		close(fd[0]);
		return -1;
	}
	if(*pid == 0){
		switch(omode){

		case OREAD:
			dup(fd[0], 1);
			break;

		case OWRITE:
			dup(fd[0], 0);
			break;
		}
		close(fd[0]);
		close(fd[1]);
		execl("/bin/rc", "rc", "-c", file, 0);
		exits("exec");
	}
	close(fd[0]);
	return fd[1];
}

int
waitfor(int pid)
{
	int rpid;
	Waitmsg w;

	while((rpid = wait(&w)) != pid && rpid != -1)
		;
	return w.msg[0];
}

static long
cmdread(ScsiReq *rp, int argc, char *argv[])
{
	long n, nbytes, total, iosize;
	int fd, pid;
	char *p;

	iosize = MaxIOsize;
	nbytes = 0x7FFFFFFF & ~iosize;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if((nbytes = strtol(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((fd = mkfile(argv[0], OWRITE, &pid)) == -1){
			rp->status = Status_BADARG;
			return -1;
		}
		break;
	}
	print("bsize=%d\n", rp->lbsize);
	total = 0;
	while(nbytes){
		n = MIN(nbytes, iosize);
		if((n = SRread(rp, rwbuf, n)) == -1){
			if(total == 0)
				total = -1;
			break;
		}
		if(write(fd, rwbuf, n) != n){
			if(total == 0)
				total = -1;
			if(rp->status == Status_OK)
				rp->status = Status_SW;
			break;
		}
		nbytes -= n;
		total += n;
	}
	close(fd);
	if(pid >= 0 && waitfor(pid)){
		rp->status = Status_SW;
		return -1;
	}
	return total;
}

static long
cmdwrite(ScsiReq *rp, int argc, char *argv[])
{
	long n, nbytes, total;
	int fd, pid;
	char *p;

	nbytes = 0x7FFFFFFF & ~MaxIOsize;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if((nbytes = strtol(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((fd = mkfile(argv[0], OREAD, &pid)) == -1){
			rp->status = Status_BADARG;
			return -1;
		}
		break;
	}
	total = 0;
	while(nbytes){
		n = MIN(nbytes, MaxIOsize);
		if((n = read(fd, rwbuf, n)) == -1){
			if(total == 0)
				total = -1;
			break;
		}
		if(SRwrite(rp, rwbuf, n) != n){
			if(total == 0)
				total = -1;
			if(rp->status == Status_OK)
				rp->status = Status_SW;
			break;
		}
		nbytes -= n;
		total += n;
	}
	close(fd);
	if(pid >= 0 && waitfor(pid)){
		rp->status = Status_SW;
		return -1;
	}
	return total;
}

static long
cmdseek(ScsiReq *rp, int argc, char *argv[])
{
	char *p;
	long offset;
	int type;

	type = 0;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if((type = strtol(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((offset = strtol(argv[0], &p, 0)) == 0 && p == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		break;
	}
	return SRseek(rp, offset, type);
}

static long
cmdfilemark(ScsiReq *rp, int argc, char *argv[])
{
	char *p;
	ulong howmany;

	howmany = 1;
	if(argc && (howmany = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
		rp->status = Status_BADARG;
		return -1;
	}
	return SRfilemark(rp, howmany);
}

static long
cmdspace(ScsiReq *rp, int argc, char *argv[])
{
	uchar code;
	long howmany;
	char option, *p;

	code = 0x00;
	howmany = 1;
	while(argc && (*argv)[0] == '-'){
		while(option = *++argv[0]){
			switch(option){

			case '-':
				break;

			case 'b':
				code = 0x00;
				break;

			case 'f':
				code = 0x01;
				break;

			default:
				rp->status = Status_BADARG;
				return -1;
			}
			break;
		}
		argc--; argv++;
		if(option == '-')
			break;
	}
	if(argc || ((howmany = strtol(argv[0], &p, 0)) == 0 && p == argv[0])){
		USED(howmany);
		rp->status = Status_BADARG;
		return -1;
	}
	return SRspace(rp, code, howmany);
}

static long
cmdinquiry(ScsiReq *rp, int argc, char *argv[])
{
	long status;
	int i, n;
	uchar *p;

	USED(argc, argv);
	if((status = SRinquiry(rp)) != -1){
		n = rp->inquiry[4]+4;
		for(i = 0; i < MIN(8, n); i++)
			Bprint(&bout, " %2.2ux", rp->inquiry[i]);
		p = &rp->inquiry[8];
		n = MIN(n, sizeof(rp->inquiry)-8);
		while(n && (*p == ' ' || *p == '\t' || *p == '\n')){
			n--;
			p++;
		}
		Bprint(&bout, "\t%.*s\n", n, p);
	}
	return status;
}

static long
cmdmodeselect(ScsiReq *rp, int argc, char *argv[])
{
	uchar list[MaxDirData];
	long nbytes, ul;
	char *p;

	memset(list, 0, sizeof(list));
	for(nbytes = 0; argc; argc--, argv++, nbytes++){
		if((ul = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		list[nbytes] = ul;
	}
	return SRmodeselect(rp, list, nbytes);
}

static long
cmdmodesense(ScsiReq *rp, int argc, char *argv[])
{
	uchar list[MaxDirData], *lp, page;
	long i, nbytes, status;
	char *p;

	nbytes = sizeof(list);
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if((nbytes = strtoul(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((page = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		break;

	case 0:
		page = 0x3F;
		break;
	}
	if((status = SRmodesense(rp, page, list, nbytes)) == -1)
		return -1;
	lp = list;
	nbytes = list[0]-1;
	for(i = 0; i < 4; i++){				/* header */
		Bprint(&bout, " %2.2ux", *lp);
		lp++;
	}
	nbytes -= 4;
	Bputc(&bout, '\n');
	for(i = 0; i < 8; i++){				/* block descriptor */
		Bprint(&bout, " %2.2ux", *lp);
		lp++;
	}
	nbytes -= 8;
	Bputc(&bout, '\n');
	while(nbytes > 0){
		i = *(lp+1);
		nbytes -= i+2;
		Bprint(&bout, " %2.2ux %2.2ux", *lp, *(lp+1));
		lp += 2;
		while(i--){
			Bprint(&bout, " %2.2ux", *lp);
			lp++;
		}
		Bputc(&bout, '\n');
	}
	return status;
}

static long
start(ScsiReq *rp, int argc, char *argv[], uchar code)
{
	char *p;

	if(argc && (code = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
		rp->status = Status_BADARG;
		return -1;
	}
	return SRstart(rp, code);
}

static long
cmdstart(ScsiReq *rp, int argc, char *argv[])
{
	return start(rp, argc, argv, 1);
}

static long
cmdstop(ScsiReq *rp, int argc, char *argv[])
{
	return start(rp, argc, argv, 0);
}

static long
cmdeject(ScsiReq *rp, int argc, char *argv[])
{
	return start(rp, argc, argv, 2);
}

static long
cmdcapacity(ScsiReq *rp, int argc, char *argv[])
{
	uchar d[6];
	long n;

	USED(argc, argv);
	if((n = SRrcapacity(rp, d)) == -1)
		return -1;
	Bprint(&bout, " %ld %ld\n", d[0]<<24|d[1]<<16|d[2]<<8|d[3],
		 d[4]<<24|d[5]<<16|d[6]<<8|d[7]);
	return n;
}

static long
cmdflushcache(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRflushcache(rp);
}

static long
cmdrdiscinfo(ScsiReq *rp, int argc, char *argv[])
{
	uchar d[MaxDirData], ses, track, *p;
	char *sp;
	long n, nbytes;

	ses = track = 0;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if((ses = strtoul(argv[1], &sp, 0)) == 0 && sp == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((track = strtoul(argv[0], &sp, 0)) == 0 && sp == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 0:
		break;
	}
	if((nbytes = SRrdiscinfo(rp, d, ses, track)) == -1)
		return -1;
	if(ses == 0){
		Bprint(&bout, "\ttoc/pma data length: 0x%ux\n", (d[0]<<8)|d[1]);
		Bprint(&bout, "\tfirst track number: %d\n", d[2]);
		Bprint(&bout, "\tlast track number: %d\n", d[3]);
		for(p = &d[4], n = nbytes-4; n; n -= 8, p += 8){
			Bprint(&bout, "\ttrack number: 0x%2.2ux\n", p[2]);
			Bprint(&bout, "\t\tcontrol: 0x%2.2ux\n", p[1] & 0x0F);
			Bprint(&bout, "\t\tblock address: 0x%lux\n",
				(p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7]);
		}
	}
	else{
		Bprint(&bout, "\tsessions data length: 0x%ux\n", (d[0]<<8)|d[1]);
		Bprint(&bout, "\tnumber of finished sessions: %d\n", d[2]);
		Bprint(&bout, "\tunfinished session number: %d\n", d[3]);
		for(p = &d[4], n = nbytes-4; n; n -= 8, p += 8){
			Bprint(&bout, "\tsession number: 0x%2.2ux\n", p[0]);
			Bprint(&bout, "\t\tfirst track number in session: 0x%2.2ux\n",
				p[2]);
			Bprint(&bout, "\t\tlogical start address: 0x%lux\n",
				(p[5]<<16)|(p[6]<<8)|p[7]);
		}
	}
	for(n = 0; n < nbytes; n++)
		Bprint(&bout, " %2.2ux", d[n]);
	Bprint(&bout, "\n");
	return nbytes;
}

static long
cmdfwaddr(ScsiReq *rp, int argc, char *argv[])
{
	uchar d[MaxDirData], npa, track, mode;
	long n;
	char *p;

	npa = mode = track = 0;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 3:
		if((npa = strtoul(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 2:
		if((mode = strtoul(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((track = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		break;

	case 0:
		break;
	}
	if((n = SRfwaddr(rp, track, mode, npa, d)) == -1)
		return -1;
	Bprint(&bout, "%ud %ld\n", d[0], (d[1]<<24)|(d[2]<<16)|(d[3]<<8)|d[4]);
	return n;
}

static long
cmdtreserve(ScsiReq *rp, int argc, char *argv[])
{
	long nbytes;
	char *p;

	if(argc != 1 || ((nbytes = strtoul(argv[0], &p, 0)) == 0 && p == argv[0])){
		rp->status = Status_BADARG;
		return -1;
	}
	return SRtreserve(rp, nbytes);
}

static long
cmdtrackinfo(ScsiReq *rp, int argc, char *argv[])
{
	uchar d[MaxDirData], track;
	long n;
	ulong ul;
	char *p;

	track = 0;
	if(argc && (track = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
		rp->status = Status_BADARG;
		return -1;
	}
	if((n = SRtinfo(rp, track, d)) == -1)
		return -1;
	Bprint(&bout, "buffer length: 0x%ux\n", d[0]);
	Bprint(&bout, "number of tracks: 0x%ux\n", d[1]);
	ul = (d[2]<<24)|(d[3]<<16)|(d[4]<<8)|d[5];
	Bprint(&bout, "start address: 0x%lux\n", ul);
	ul = (d[6]<<24)|(d[7]<<16)|(d[8]<<8)|d[9];
	Bprint(&bout, "track length: 0x%lux\n", ul);
	Bprint(&bout, "track mode: 0x%ux\n", d[0x0A] & 0x0F);
	Bprint(&bout, "track status: 0x%ux\n", (d[0x0A]>>4) & 0x0F);
	Bprint(&bout, "data mode: 0x%ux\n", d[0x0B] & 0x0F);
	ul = (d[0x0C]<<24)|(d[0x0D]<<16)|(d[0x0E]<<8)|d[0x0F];
	Bprint(&bout, "free blocks: 0x%lux\n", ul);
	return n;
}

static long
cmdwtrack(ScsiReq *rp, int argc, char *argv[])
{
	uchar mode, track;
	long n, nbytes, total, x;
	int fd, pid;
	char *p;

	mode = track = 0;
	nbytes = 0;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 4:
		if((mode = strtoul(argv[3], &p, 0)) == 0 && p == argv[3]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 3:
		if((track = strtoul(argv[2], &p, 0)) == 0 && p == argv[2]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 2:
		if((nbytes = strtoul(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((fd = mkfile(argv[0], OREAD, &pid)) == -1){
			rp->status = Status_BADARG;
			return -1;
		}
		break;
	}
	total = 0;
	n = MIN(nbytes, MaxIOsize);
	if((n = read(fd, rwbuf, n)) == -1){
		fprint(2, "file read failed %r\n");
		close(fd);
		return -1;
	}
	if((x = SRwtrack(rp, rwbuf, n, track, mode)) != n){
		fprint(2, "wtrack: write incomplete: asked %d, did %d\n", n, x);
		if(rp->status == Status_OK)
			rp->status = Status_SW;
		close(fd);
		return -1;
	}
	nbytes -= n;
	total += n;
	while(nbytes){
		n = MIN(nbytes, MaxIOsize);
		if((n = read(fd, rwbuf, n)) == -1){
			break;
		}
		if((x = SRwrite(rp, rwbuf, n)) != n){
			fprint(2, "write: write incomplete: asked %d, did %d\n", n, x);
			if(rp->status == Status_OK)
				rp->status = Status_SW;
			break;
		}
		nbytes -= n;
		total += n;
	}
	close(fd);
	if(pid >= 0 && waitfor(pid)){
		rp->status = Status_SW;
		return -1;
	}
	return total;
}

static long
cmdload(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRmload(rp, 0);
}

static long
cmdunload(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRmload(rp, 1);
}

static long
cmdfixation(ScsiReq *rp, int argc, char *argv[])
{
	uchar type;
	char *p;

	type = 0;
	if(argc && (type = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
		rp->status = Status_BADARG;
		return -1;
	}
	return SRfixation(rp, type);
}

static long
cmdhelp(ScsiReq *rp, int argc, char *argv[])
{
	ScsiCmd *cp;
	char *p;

	USED(rp);
	if(argc)
		p = argv[0];
	else
		p = 0;
	for(cp = scsicmd; cp->name; cp++){
		if(p == 0 || strcmp(p, cp->name) == 0)
			Bprint(&bout, "%s\n", cp->help);
	}
	return 0;
}

static long
cmdprobe(ScsiReq *rp, int argc, char *argv[])
{
	ScsiReq scsireq;
	uchar id;
	
	USED(argc, argv);
	rp->status = Status_OK;
	scsireq.flags = 0;
	for(id = 0; id < CtlrID; id++){
		if(SRopenraw(&scsireq, id) == -1)
			return -1;
		SRreqsense(&scsireq);
		switch(scsireq.status){

		default:
			break;

		case Status_OK:
		case Status_SD:
			Bprint(&bout, "%d: ", id);
			cmdinquiry(&scsireq, 0, 0);
			break;
		}	
		SRclose(&scsireq);
	}
	return 0;
}

static long
cmdclose(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRclose(rp);
}

static long
cmdopen(ScsiReq *rp, int argc, char *argv[])
{
	char *p;
	int id, raw;
	long status;

	raw = 0;
	if(argc && strcmp("-r", argv[0]) == 0){
		raw = 1;
		argc--, argv++;
	}
	if(argc != 1 || ((id = strtoul(argv[0], &p, 0)) == 0 && p == argv[0])){
		rp->status = Status_BADARG;
		return -1;
	}
	if(raw == 0){
		if((status = SRopen(rp, id)) != -1 && verbose)
			Bprint(&bout, "block size: %ld\n", rp->lbsize);
	}
	else {
		status = SRopenraw(rp, id);
		rp->lbsize = 512;
	}
	return status;
}

static ScsiCmd scsicmd[] = {
	{ "ready",	cmdready,	1,		/*[0x00]*/
	  "ready",
	},
	{ "rewind",	cmdrewind,	1,		/*[0x01]*/
	  "rewind",
	},
	{ "rezero",	cmdrewind,	1,		/*[0x01]*/
	  "rezero",
	},
	{ "reqsense",	cmdreqsense,	1,		/*[0x03]*/
	  "reqsense",
	},
	{ "format",	cmdformat,	0,		/*[0x04]*/
	  "format",
	},
	{ "rblimits",	cmdrblimits,	1,		/*[0x05]*/
	  "rblimits",
	},
	{ "read",	cmdread,	1,		/*[0x08]*/
	  "read [|]file [nbytes]",
	},
	{ "write",	cmdwrite,	1,		/*[0x0A]*/
	  "write [|]file [nbytes]",
	},
	{ "seek",	cmdseek,	1,		/*[0x0B]*/
	  "seek offset [whence]",
	},
	{ "filemark",	cmdfilemark,	1,		/*[0x10]*/
	  "filemark [howmany]",
	},
	{ "space",	cmdspace,	1,		/*[0x11]*/
	  "space [-f] [-b] [[--] howmany]",
	},
	{ "inquiry",	cmdinquiry,	1,		/*[0x12]*/
	  "inquiry",
	},
	{ "modeselect",	cmdmodeselect,	1,		/*[0x15] */
	  "modeselect bytes...",
	},
	{ "modesense",	cmdmodesense,	1,		/*[0x1A]*/
	  "modesense [page [nbytes]]",
	},
	{ "start",	cmdstart,	1,		/*[0x1B]*/
	  "start [code]",
	},
	{ "stop",	cmdstop,	1,		/*[0x1B]*/
	  "stop [code]",
	},
	{ "eject",	cmdeject,	1,		/*[0x1B]*/
	  "eject [code]",
	},
	{ "capacity",	cmdcapacity,	1,		/*[0x25]*/
	  "capacity",
	},

	{ "flushcache",	cmdflushcache,	1,		/*[0x35]*/
	  "flushcache",
	},
	{ "rdiscinfo",	cmdrdiscinfo,	1,		/*[0x43]*/
	  "rdiscinfo [track/session-number [ses]]",
	},
	{ "fwaddr",	cmdfwaddr,	1,		/*[0xE2]*/
	  "fwaddr [track [mode [npa]]]",
	},
	{ "treserve",	cmdtreserve,	1,		/*[0xE4]*/
	  "treserve nbytes",
	},
	{ "trackinfo",	cmdtrackinfo,	1,		/*[0xE5]*/
	  "trackinfo [track]",
	},
	{ "wtrack",	cmdwtrack,	1,		/*[0xE6]*/
	  "wtrack [|]file [nbytes [track [mode]]]",
	},
	{ "load",	cmdload,	1,		/*[0xE7]*/
	  "load",
	},
	{ "unload",	cmdunload,	1,		/*[0xE7]*/
	  "unload",
	},
	{ "fixation",	cmdfixation,	1,		/*[0xE9]*/
	  "fixation [toc-type]",
	},

	{ "help",	cmdhelp,	0,
	  "help",
	},
	{ "probe",	cmdprobe,	0,
	  "probe",
	},
	{ "close",	cmdclose,	1,
	  "close",
	},
	{ "open",	cmdopen,	0,
	  "open target-id",
	},
	{ 0, 0 },
};

#define	SEP(c)	(((c)==' ')||((c)=='\t')||((c)=='\n'))

static char *
tokenise(char *s, char **start, char **end)
{
	char *to;
	Rune r;
	int n;

	while(*s && SEP(*s))				/* skip leading white space */
		s++;
	to = *start = s;
	while(*s){
		n = chartorune(&r, s);
		if(SEP(r)){
			if(to != *start)		/* we have data */
				break;
			s += n;				/* null string - keep looking */
			while(*s && SEP(*s))	
				s++;
			to = *start = s;
		} 
		else if(r == '\''){
			s += n;				/* skip leading quote */
			while(*s){
				n = chartorune(&r, s);
				if(r == '\''){
					if(s[1] != '\'')
						break;
					s++;		/* embedded quote */
				}
				while (n--)
					*to++ = *s++;
			}
			if(!*s)				/* no trailing quote */
				break;
			s++;				/* skip trailing quote */ 
		}
		else  {
			while(n--)
				*to++ = *s++;
		}
	}
	*end = to;
	return s;
}

static int
parse(char *s, char *fields[], int nfields)
{
	int c, argc;
	char *start, *end;

	argc = 0;
	c = *s;
	while(c){
		s = tokenise(s, &start, &end);
		c = *s++;
		if(*start == 0)
			break;
		if(argc >= nfields-1)
			return -1;
		*end = 0;
		fields[argc++] = start;
	}
	fields[argc] = 0;
	return argc;
}

static void
usage(void)
{
	fprint(2, "%s: usage: %s [-b bus] [-q] [scsi-id]\n", argv0, argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	ScsiReq target;
	char *ap, *av[50];
	int ac;
	ScsiCmd *cp;
	long status;

	ARGBEGIN {
	case 'b':
		bus = atoi(ARGF());
		break;

	case 'q':
		verbose = 0;
		break;

	default:
		usage();
	} ARGEND

	if(Binit(&bin, 0, OREAD) == Beof || Binit(&bout, 1, OWRITE) == Beof){
		fprint(2, "%s: can't init bio: %r\n", argv0);
		exits("Binit");
	}

	memset(&target, 0, sizeof(target));
	if(argc && cmdopen(&target, argc, argv) == -1)
		usage();
	Bflush(&bout);

	while(ap = Brdline(&bin, '\n')){
		ap[BLINELEN(&bin)-1] = 0;
		switch(ac = parse(ap, av, nelem(av))){

		default:
			for(cp = scsicmd; cp->name; cp++){
				if(strcmp(cp->name, av[0]) == 0)
					break;
			}
			if(cp->name == 0){
				Bprint(&bout, "eh?\n");
				break;
			}
			if((target.flags & Fopen) == 0 && cp->open){
				Bprint(&bout, "no current target\n");
				break;
			}
			if((status = (*cp->f)(&target, ac-1, &av[1])) != -1){
				if(verbose)
					Bprint(&bout, "ok %ld\n", status);
				break;
			}
			switch(target.status){

			case Status_OK:
				if(verbose)
					Bprint(&bout, "ok %ld\n", status);
				break;

			case Status_SD:
				makesense(&target);
				break;

			case Status_NX:
				Bprint(&bout, "select timeout\n");
				break;

			case Status_HW:
				Bprint(&bout, "hardware error\n");
				break;

			case Status_SW:
				Bprint(&bout, "software error\n");
				break;

			case Status_BADARG:
				Bprint(&bout, "bad argument\n");
				break;

			case Status_RO:
				Bprint(&bout, "device is read-only\n");
				break;

			default:
				Bprint(&bout, "status %ld #%2.2ux\n", status, target.status);
				break;
			}
			break;

		case -1:
			Bprint(&bout, "eh?\n");
			break;

		case 0:
			break;
		}
		Bflush(&bout);
	}
	exits(0);
}

