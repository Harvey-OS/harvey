#include <u.h>
#include <libc.h>
#include <bio.h>

#include "scsireq.h"

#define MIN(a, b)	((a) < (b) ? (a): (b))

Biobuf bin, bout;
static char rwbuf[MaxIOsize];
static int verbose = 1;
long maxiosize = MaxIOsize;

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
	Bprint(&bout, " %2.2uX %2.2uX %2.2uX %2.2uX %2.2uX %2.2uX\n",
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
		close(fd[1]);
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
		execl("/bin/rc", "rc", "-c", file, nil);
		exits("exec");
	}
	close(fd[0]);
	return fd[1];
}

int
waitfor(int pid)
{
	int msg;
	Waitmsg *w;

	while((w = wait()) != nil){
		if(w->pid != pid){
			free(w);
			continue;
		}
		msg = (w->msg[0] != '\0');
		free(w);
		return msg;
	}
	return -1;
}

static long
cmdread(ScsiReq *rp, int argc, char *argv[])
{
	long n, iosize;
	vlong nbytes, total;
	int fd, pid;
	char *p;

	iosize = maxiosize;
	nbytes = ~0ULL >> 1;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		nbytes = strtoll(argv[1], &p, 0);
		if(nbytes == 0 && p == argv[1]){
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
	print("bsize=%lud\n", rp->lbsize);
	total = 0;
	while(nbytes){
		n = iosize;
		if(n > nbytes)
			n = nbytes;
		if((n = SRread(rp, rwbuf, n)) <= 0){
			if(total == 0)
				total = -1;
			break;
		}
		if(write(fd, rwbuf, n) != n){
			if(total == 0)
				total = -1;
			if(rp->status == STok)
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
	long n;
	vlong nbytes, total;
	int fd, pid;
	char *p;

	nbytes = ~0ULL >> 1;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		nbytes = strtoll(argv[1], &p, 0);
		if(nbytes == 0 && p == argv[1]){
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
		n = maxiosize;
		if(n > nbytes)
			n = nbytes;
		if((n = readn(fd, rwbuf, n)) <= 0){
			if(total == 0)
				total = -1;
			break;
		}
		if(SRwrite(rp, rwbuf, n) != n){
			if(total == 0)
				total = -1;
			if(rp->status == STok)
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
	if(argc && ((howmany = strtol(argv[0], &p, 0)) == 0 && p == argv[0])){
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
			Bprint(&bout, " %2.2uX", rp->inquiry[i]);
		p = &rp->inquiry[8];
		n = MIN(n, sizeof(rp->inquiry)-8);
		while(n && (*p == ' ' || *p == '\t' || *p == '\n')){
			n--;
			p++;
		}
		Bprint(&bout, "\t%.*s\n", n, (char*)p);
	}
	return status;
}

static long
cmdmodeselect6(ScsiReq *rp, int argc, char *argv[])
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
	if(!(rp->flags & Finqok) && SRinquiry(rp) == -1)
		Bprint(&bout, "warning: couldn't determine whether SCSI-1/SCSI-2 mode");
	return SRmodeselect6(rp, list, nbytes);
}

static long
cmdmodeselect10(ScsiReq *rp, int argc, char *argv[])
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
	if(!(rp->flags & Finqok) && SRinquiry(rp) == -1)
		Bprint(&bout, "warning: couldn't determine whether SCSI-1/SCSI-2 mode");
	return SRmodeselect10(rp, list, nbytes);
}

static long
cmdmodesense6(ScsiReq *rp, int argc, char *argv[])
{
	uchar list[MaxDirData], *lp, page;
	long i, n, nbytes, status;
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
	if((status = SRmodesense6(rp, page, list, nbytes)) == -1)
		return -1;
	lp = list;
	nbytes = list[0];
	Bprint(&bout, " Header\n   ");
	for(i = 0; i < 4; i++){				/* header */
		Bprint(&bout, " %2.2uX", *lp);
		lp++;
	}
	Bputc(&bout, '\n');

	if(list[3]){					/* block descriptors */
		for(n = 0; n < list[3]/8; n++){
			Bprint(&bout, " Block %ld\n   ", n);
			for(i = 0; i < 8; i++)
				Bprint(&bout, " %2.2uX", lp[i]);
			Bprint(&bout, "    (density %2.2uX", lp[0]);
			Bprint(&bout, " blocks %d", (lp[1]<<16)|(lp[2]<<8)|lp[3]);
			Bprint(&bout, " length %d)", (lp[5]<<16)|(lp[6]<<8)|lp[7]);
			lp += 8;
			nbytes -= 8;
			Bputc(&bout, '\n');
		}
	}

	while(nbytes > 0){				/* pages */
		i = *(lp+1);
		nbytes -= i+2;
		Bprint(&bout, " Page %2.2uX %d\n   ", *lp & 0x3F, *(lp+1));
		lp += 2;
		for(n = 0; n < i; n++){
			if(n && ((n & 0x0F) == 0))
				Bprint(&bout, "\n   ");
			Bprint(&bout, " %2.2uX", *lp);
			lp++;
		}
		if(n && (n & 0x0F))
			Bputc(&bout, '\n');
	}
	return status;
}

static long
cmdmodesense10(ScsiReq *rp, int argc, char *argv[])
{
	uchar *list, *lp, page;
	long blen, i, n, nbytes, status;
	char *p;

	nbytes = MaxDirData;
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
	list = malloc(nbytes);
	if(list == 0){
		rp->status = STnomem;
		return -1;
	}
	if((status = SRmodesense10(rp, page, list, nbytes)) == -1)
		return -1;
	lp = list;
	nbytes = ((list[0]<<8)|list[1]);
	Bprint(&bout, " Header\n   ");
	for(i = 0; i < 8; i++){				/* header */
		Bprint(&bout, " %2.2uX", *lp);
		lp++;
	}
	Bputc(&bout, '\n');

	blen = (list[6]<<8)|list[7];
	if(blen){					/* block descriptors */
		for(n = 0; n < blen/8; n++){
			Bprint(&bout, " Block %ld\n   ", n);
			for(i = 0; i < 8; i++)
				Bprint(&bout, " %2.2uX", lp[i]);
			Bprint(&bout, "    (density %2.2uX", lp[0]);
			Bprint(&bout, " blocks %d", (lp[1]<<16)|(lp[2]<<8)|lp[3]);
			Bprint(&bout, " length %d)", (lp[5]<<16)|(lp[6]<<8)|lp[7]);
			lp += 8;
			nbytes -= 8;
			Bputc(&bout, '\n');
		}
	}

	/*
	 * Special for ATA drives, page 0 is the drive info in 16-bit
	 * chunks, little-endian, 256 in total. No decoding for now.
	 */
	if(page == 0){
		for(n = 0; n < nbytes; n += 2){
			if(n && ((n & 0x1F) == 0))
				Bprint(&bout, "\n");
			Bprint(&bout, " %4.4uX", (*(lp+1)<<8)|*lp);
			lp += 2;
		}
		Bputc(&bout, '\n');
	}
	else while(nbytes > 0){				/* pages */
		i = *(lp+1);
		nbytes -= i+2;
		Bprint(&bout, " Page %2.2uX %d\n   ", *lp & 0x3F, *(lp+1));
		lp += 2;
		for(n = 0; n < i; n++){
			if(n && ((n & 0x0F) == 0))
				Bprint(&bout, "\n   ");
			Bprint(&bout, " %2.2uX", *lp);
			lp++;
		}
		if(n && (n & 0x0F))
			Bputc(&bout, '\n');
	}
	free(list);
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
cmdingest(ScsiReq *rp, int argc, char *argv[])
{
	return start(rp, argc, argv, 3);
}

static long
cmdcapacity(ScsiReq *rp, int argc, char *argv[])
{
	uchar d[8];
	long n;

	USED(argc, argv);
	if((n = SRrcapacity(rp, d)) == -1)
		return -1;
	Bprint(&bout, " %ud %ud\n",
		d[0]<<24|d[1]<<16|d[2]<<8|d[3],
		d[4]<<24|d[5]<<16|d[6]<<8|d[7]);
	return n;
}

static long
cmdblank(ScsiReq *rp, int argc, char *argv[])
{
	uchar type, track;
	char *sp;

	type = track = 0;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if((type = strtoul(argv[1], &sp, 0)) == 0 && sp == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		if(type < 0 || type > 6){
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
	return SRblank(rp, type, track);
}

static long
cmdsynccache(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRsynccache(rp);
}

static long
cmdrtoc(ScsiReq *rp, int argc, char *argv[])
{
	uchar d[100*8+4], format, track, *p;
	char *sp;
	long n, nbytes;
	int tdl;

	format = track = 0;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if((format = strtoul(argv[1], &sp, 0)) == 0 && sp == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		if(format < 0 || format > 4){
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
	if((nbytes = SRTOC(rp, d, sizeof(d), format, track)) == -1){
		if(rp->status == STok)
			Bprint(&bout, "\t(probably empty)\n");
		return -1;
	}
	tdl = (d[0]<<8)|d[1];
	switch(format){

	case 0:
		Bprint(&bout, "\ttoc/pma data length: 0x%uX\n", tdl);
		Bprint(&bout, "\tfirst track number: %d\n", d[2]);
		Bprint(&bout, "\tlast track number: %d\n", d[3]);
		for(p = &d[4], n = tdl-2; n; n -= 8, p += 8){
			Bprint(&bout, "\ttrack number: 0x%2.2uX\n", p[2]);
			Bprint(&bout, "\t\tcontrol: 0x%2.2uX\n", p[1] & 0x0F);
			Bprint(&bout, "\t\tblock address: 0x%uX\n",
				(p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7]);
		}
		break;

	case 1:
		Bprint(&bout, "\tsessions data length: 0x%uX\n", tdl);
		Bprint(&bout, "\tnumber of finished sessions: %d\n", d[2]);
		Bprint(&bout, "\tunfinished session number: %d\n", d[3]);
		for(p = &d[4], n = tdl-2; n; n -= 8, p += 8){
			Bprint(&bout, "\tsession number: 0x%2.2uX\n", p[0]);
			Bprint(&bout, "\t\tfirst track number in session: 0x%2.2uX\n",
				p[2]);
			Bprint(&bout, "\t\tlogical start address: 0x%uX\n",
				(p[5]<<16)|(p[6]<<8)|p[7]);
		}
		break;

	case 2:
		Bprint(&bout, "\tfull TOC data length: 0x%uX\n", tdl);
		Bprint(&bout, "\tnumber of finished sessions: %d\n", d[2]);
		Bprint(&bout, "\tunfinished session number: %d\n", d[3]);
		for(p = &d[4], n = tdl-2; n > 0; n -= 11, p += 11){
			Bprint(&bout, "\tsession number: 0x%2.2uX\n", p[0]);
			Bprint(&bout, "\t\tcontrol: 0x%2.2uX\n", p[1] & 0x0F);
			Bprint(&bout, "\t\tADR: 0x%2.2uX\n", (p[1]>>4) & 0x0F);
			Bprint(&bout, "\t\tTNO: 0x%2.2uX\n", p[2]);
			Bprint(&bout, "\t\tPOINT: 0x%2.2uX\n", p[3]);
			Bprint(&bout, "\t\tMin: 0x%2.2uX\n", p[4]);
			Bprint(&bout, "\t\tSec: 0x%2.2uX\n", p[5]);
			Bprint(&bout, "\t\tFrame: 0x%2.2uX\n", p[6]);
			Bprint(&bout, "\t\tZero: 0x%2.2uX\n", p[7]);
			Bprint(&bout, "\t\tPMIN: 0x%2.2uX\n", p[8]);
			Bprint(&bout, "\t\tPSEC: 0x%2.2uX\n", p[9]);
			Bprint(&bout, "\t\tPFRAME: 0x%2.2uX\n", p[10]);
		}
		break;
	case 3:
		Bprint(&bout, "\tPMA data length: 0x%uX\n", tdl);
		for(p = &d[4], n = tdl-2; n > 0; n -= 11, p += 11){
			Bprint(&bout, "\t\tcontrol: 0x%2.2uX\n", p[1] & 0x0F);
			Bprint(&bout, "\t\tADR: 0x%2.2uX\n", (p[1]>>4) & 0x0F);
			Bprint(&bout, "\t\tTNO: 0x%2.2uX\n", p[2]);
			Bprint(&bout, "\t\tPOINT: 0x%2.2uX\n", p[3]);
			Bprint(&bout, "\t\tMin: 0x%2.2uX\n", p[4]);
			Bprint(&bout, "\t\tSec: 0x%2.2uX\n", p[5]);
			Bprint(&bout, "\t\tFrame: 0x%2.2uX\n", p[6]);
			Bprint(&bout, "\t\tZero: 0x%2.2uX\n", p[7]);
			Bprint(&bout, "\t\tPMIN: 0x%2.2uX\n", p[8]);
			Bprint(&bout, "\t\tPSEC: 0x%2.2uX\n", p[9]);
			Bprint(&bout, "\t\tPFRAME: 0x%2.2uX\n", p[10]);
		}
		break;

	case 4:
		Bprint(&bout, "\tATIP data length: 0x%uX\n", tdl);
		break;

	}
	for(n = 0; n < nbytes; n++){
		if(n && ((n & 0x0F) == 0))
			Bprint(&bout, "\n");
		Bprint(&bout, " %2.2uX", d[n]);
	}
	if(n && (n & 0x0F))
		Bputc(&bout, '\n');
	return nbytes;
}

static long
cmdrdiscinfo(ScsiReq *rp, int argc, char*[])
{
	uchar d[MaxDirData];
	int dl;
	long n, nbytes;

	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 0:
		break;
	}
	if((nbytes = SRrdiscinfo(rp, d, sizeof(d))) == -1)
		return -1;

	dl = (d[0]<<8)|d[1];
	Bprint(&bout, "\tdata length: 0x%uX\n", dl);
	Bprint(&bout, "\tinfo[2] 0x%2.2uX\n", d[2]);
	switch(d[2] & 0x03){

	case 0:
		Bprint(&bout, "\t\tEmpty\n");
		break;

	case 1:
		Bprint(&bout, "\t\tIncomplete disc (Appendable)\n");
		break;

	case 2:
		Bprint(&bout, "\t\tComplete (CD-ROM or last session is closed and has no next session pointer)\n");
		break;

	case 3:
		Bprint(&bout, "\t\tReserved\n");
		break;
	}
	switch((d[2]>>2) & 0x03){

	case 0:
		Bprint(&bout, "\t\tEmpty Session\n");
		break;

	case 1:
		Bprint(&bout, "\t\tIncomplete Session\n");
		break;

	case 2:
		Bprint(&bout, "\t\tReserved\n");
		break;

	case 3:
		Bprint(&bout, "\t\tComplete Session (only possible when disc Status is Complete)\n");
		break;
	}
	if(d[2] & 0x10)
		Bprint(&bout, "\t\tErasable\n");
	Bprint(&bout, "\tNumber of First Track on Disc %ud\n", d[3]);
	Bprint(&bout, "\tNumber of Sessions %ud\n", d[4]);
	Bprint(&bout, "\tFirst Track Number in Last Session %ud\n", d[5]);
	Bprint(&bout, "\tLast Track Number in Last Session %ud\n", d[6]);
	Bprint(&bout, "\tinfo[7] 0x%2.2uX\n", d[7]);
	if(d[7] & 0x20)
		Bprint(&bout, "\t\tUnrestricted Use Disc\n");
	if(d[7] & 0x40)
		Bprint(&bout, "\t\tDisc Bar Code Valid\n");
	if(d[7] & 0x80)
		Bprint(&bout, "\t\tDisc ID Valid\n");
	Bprint(&bout, "\tinfo[8] 0x%2.2uX\n", d[8]);
	switch(d[8]){

	case 0x00:
		Bprint(&bout, "\t\tCD-DA or CD-ROM Disc\n");
		break;

	case 0x10:
		Bprint(&bout, "\t\tCD-I Disc\n");
		break;

	case 0x20:
		Bprint(&bout, "\t\tCD-ROM XA Disc\n");
		break;

	case 0xFF:
		Bprint(&bout, "\t\tUndefined\n");
		break;

	default:
		Bprint(&bout, "\t\tReserved\n");
		break;
	}
	Bprint(&bout, "\tLast Session lead-in Start Time M/S/F: 0x%2.2uX/0x%2.2uX/0x%2.2uX\n",
		d[17], d[18], d[19]);
	Bprint(&bout, "\tLast Possible Start Time for Start of lead-out M/S/F: 0x%2.2uX/0x%2.2uX/0x%2.2uX\n",
		d[21], d[22], d[23]);

	for(n = 0; n < nbytes; n++){
		if(n && ((n & 0x0F) == 0))
			Bprint(&bout, "\n");
		Bprint(&bout, " %2.2uX", d[n]);
	}
	if(n && (n & 0x0F))
		Bputc(&bout, '\n');

	return nbytes;
}

static long
cmdrtrackinfo(ScsiReq *rp, int argc, char *argv[])
{
	uchar d[MaxDirData], track;
	char *sp;
	long n, nbytes;
	int dl;

	track = 0;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 1:
		if((track = strtoul(argv[0], &sp, 0)) == 0 && sp == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 0:
		break;
	}
	if((nbytes = SRrtrackinfo(rp, d, sizeof(d), track)) == -1)
		return -1;

	dl = (d[0]<<8)|d[1];
	Bprint(&bout, "\tdata length: 0x%uX\n", dl);
	Bprint(&bout, "\Track Number %d\n", d[2]);
	Bprint(&bout, "\Session Number %d\n", d[3]);
	Bprint(&bout, "\tinfo[4] 0x%2.2uX\n", d[5]);
	Bprint(&bout, "\t\tTrack Mode 0x%2.2uX: ", d[5] & 0x0F);
	switch(d[5] & 0x0F){
	case 0x00:
	case 0x02:
		Bprint(&bout, "2 audio channels without pre-emphasis\n");
		break;
	case 0x01:
	case 0x03:
		Bprint(&bout, "2 audio channels with pre-emphasis of 50/15µs\n");
		break;
	case 0x08:
	case 0x0A:
		Bprint(&bout, "audio channels without pre-emphasis (reserved in CD-R/RW)\n");
		break;
	case 0x09:
	case 0x0B:
		Bprint(&bout, "audio channels with pre-emphasis of 50/15µs (reserved in CD-R/RW)\n");
		break;
	case 0x04:
	case 0x06:
		Bprint(&bout, "Data track, recorded uninterrupted\n");
		break;
	case 0x05:
	case 0x07:
		Bprint(&bout, "Data track, recorded incremental\n");
		break;
	default:
		Bprint(&bout, "(mode unknown)\n");
		break;
	}
	if(d[5] & 0x10)
		Bprint(&bout, "\t\tCopy\n");
	if(d[5] & 0x20)
		Bprint(&bout, "\t\tDamage\n");
	Bprint(&bout, "\tinfo[6] 0x%2.2uX\n", d[6]);
	Bprint(&bout, "\t\tData Mode 0x%2.2uX: ", d[6] & 0x0F);
	switch(d[6] & 0x0F){
	case 0x01:
		Bprint(&bout, "Mode 1 (ISO/IEC 10149)\n");
		break;
	case 0x02:
		Bprint(&bout, "Mode 2 (ISO/IEC 10149 or CD-ROM XA)\n");
		break;
	case 0x0F:
		Bprint(&bout, "Data Block Type unknown (no track descriptor block)\n");
		break;
	default:
		Bprint(&bout, "(Reserved)\n");
		break;
	}
	if(d[6] & 0x10)
		Bprint(&bout, "\t\tFP\n");
	if(d[6] & 0x20)
		Bprint(&bout, "\t\tPacket\n");
	if(d[6] & 0x40)
		Bprint(&bout, "\t\tBlank\n");
	if(d[6] & 0x80)
		Bprint(&bout, "\t\tRT\n");
	Bprint(&bout, "\tTrack Start Address 0x%8.8uX\n",
		(d[8]<<24)|(d[9]<<16)|(d[10]<<8)|d[11]);
	if(d[7] & 0x01)
		Bprint(&bout, "\tNext Writeable Address 0x%8.8uX\n",
			(d[12]<<24)|(d[13]<<16)|(d[14]<<8)|d[15]);
	Bprint(&bout, "\tFree Blocks 0x%8.8uX\n",
		(d[16]<<24)|(d[17]<<16)|(d[18]<<8)|d[19]);
	if((d[6] & 0x30) == 0x30)
		Bprint(&bout, "\tFixed Packet Size 0x%8.8uX\n",
			(d[20]<<24)|(d[21]<<16)|(d[22]<<8)|d[23]);
	Bprint(&bout, "\tTrack Size 0x%8.8uX\n",
		(d[24]<<24)|(d[25]<<16)|(d[26]<<8)|d[27]);

	for(n = 0; n < nbytes; n++){
		if(n && ((n & 0x0F) == 0))
			Bprint(&bout, "\n");
		Bprint(&bout, " %2.2uX", d[n]);
	}
	if(n && (n & 0x0F))
		Bputc(&bout, '\n');

	return nbytes;
}

static long
cmdcdpause(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRcdpause(rp, 0);
}

static long
cmdcdresume(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRcdpause(rp, 1);
}

static long
cmdcdstop(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SRcdstop(rp);
}

static long
cmdcdplay(ScsiReq *rp, int argc, char *argv[])
{
	long length, start;
	char *sp;
	int raw;

	raw = 0;
	start = 0;
	if(argc && strcmp("-r", argv[0]) == 0){
		raw = 1;
		argc--, argv++;
	}

	length = 0xFFFFFFFF;
	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 2:
		if(!raw || ((length = strtol(argv[1], &sp, 0)) == 0 && sp == argv[1])){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 1:
		if((start = strtol(argv[0], &sp, 0)) == 0 && sp == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 0:
		break;
	}

	return SRcdplay(rp, raw, start, length);
}

static long
cmdcdload(ScsiReq *rp, int argc, char *argv[])
{
	char *p;
	ulong slot;

	slot = 0;
	if(argc && (slot = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
		rp->status = Status_BADARG;
		return -1;
	}
	return SRcdload(rp, 1, slot);
}

static long
cmdcdunload(ScsiReq *rp, int argc, char *argv[])
{
	char *p;
	ulong slot;

	slot = 0;
	if(argc && (slot = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
		rp->status = Status_BADARG;
		return -1;
	}
	return SRcdload(rp, 0, slot);
}

static long
cmdcdstatus(ScsiReq *rp, int argc, char *argv[])
{
	uchar *list, *lp;
	long nbytes, status;
	int i, slots;

	USED(argc, argv);

	nbytes = 4096;
	list = malloc(nbytes);
	if(list == 0){
		rp->status = STnomem;
		return -1;
	}
	status = SRcdstatus(rp, list, nbytes);
	if(status == -1){
		free(list);
		return -1;
	}

	lp = list;
	Bprint(&bout, " Header\n   ");
	for(i = 0; i < 8; i++){				/* header */
		Bprint(&bout, " %2.2uX", *lp);
		lp++;
	}
	Bputc(&bout, '\n');

	slots = ((list[6]<<8)|list[7])/4;
	Bprint(&bout, " Slots\n   ");
	while(slots--){
		Bprint(&bout, " %2.2uX %2.2uX %2.2uX %2.2uX\n   ",
			*lp, *(lp+1), *(lp+2), *(lp+3));
		lp += 4;
	}

	free(list);
	return status;
}

static long
cmdgetconf(ScsiReq *rp, int argc, char *argv[])
{
	uchar *list;
	long nbytes, status;

	USED(argc, argv);

	nbytes = 4096;
	list = malloc(nbytes);
	if(list == 0){
		rp->status = STnomem;
		return -1;
	}
	status = SRgetconf(rp, list, nbytes);
	if(status == -1){
		free(list);
		return -1;
	}
	/* to be done... */
	free(list);
	return status;
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
	Bprint(&bout, "%ud %ud\n", d[0], (d[1]<<24)|(d[2]<<16)|(d[3]<<8)|d[4]);
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
	Bprint(&bout, "buffer length: 0x%uX\n", d[0]);
	Bprint(&bout, "number of tracks: 0x%uX\n", d[1]);
	ul = (d[2]<<24)|(d[3]<<16)|(d[4]<<8)|d[5];
	Bprint(&bout, "start address: 0x%luX\n", ul);
	ul = (d[6]<<24)|(d[7]<<16)|(d[8]<<8)|d[9];
	Bprint(&bout, "track length: 0x%luX\n", ul);
	Bprint(&bout, "track mode: 0x%uX\n", d[0x0A] & 0x0F);
	Bprint(&bout, "track status: 0x%uX\n", (d[0x0A]>>4) & 0x0F);
	Bprint(&bout, "data mode: 0x%uX\n", d[0x0B] & 0x0F);
	ul = (d[0x0C]<<24)|(d[0x0D]<<16)|(d[0x0E]<<8)|d[0x0F];
	Bprint(&bout, "free blocks: 0x%luX\n", ul);
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
	n = MIN(nbytes, maxiosize);
	if((n = readn(fd, rwbuf, n)) == -1){
		fprint(2, "file read failed %r\n");
		close(fd);
		return -1;
	}
	if((x = SRwtrack(rp, rwbuf, n, track, mode)) != n){
		fprint(2, "wtrack: write incomplete: asked %ld, did %ld\n", n, x);
		if(rp->status == STok)
			rp->status = Status_SW;
		close(fd);
		return -1;
	}
	nbytes -= n;
	total += n;
	while(nbytes){
		n = MIN(nbytes, maxiosize);
		if((n = read(fd, rwbuf, n)) == -1){
			break;
		}
		if((x = SRwrite(rp, rwbuf, n)) != n){
			fprint(2, "write: write incomplete: asked %ld, did %ld\n", n, x);
			if(rp->status == STok)
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
cmdeinit(ScsiReq *rp, int argc, char *argv[])
{
	USED(argc, argv);
	return SReinitialise(rp);
}

static long
cmdmmove(ScsiReq *rp, int argc, char *argv[])
{
	int transport, source, destination, invert;
	char *p;

	invert = 0;

	switch(argc){

	default:
		rp->status = Status_BADARG;
		return -1;

	case 4:
		if((invert = strtoul(argv[3], &p, 0)) == 0 && p == argv[3]){
			rp->status = Status_BADARG;
			return -1;
		}
		/*FALLTHROUGH*/

	case 3:
		if((transport = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		if((source = strtoul(argv[1], &p, 0)) == 0 && p == argv[1]){
			rp->status = Status_BADARG;
			return -1;
		}
		if((destination = strtoul(argv[2], &p, 0)) == 0 && p == argv[2]){
			rp->status = Status_BADARG;
			return -1;
		}
		break;
	}

	return SRmmove(rp, transport, source, destination, invert);
}

static long
cmdestatus(ScsiReq *rp, int argc, char *argv[])
{
	uchar *list, *lp, type;
	long d, i, n, nbytes, status;
	char *p;

	type = 0;
	nbytes = 4096;

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
		if((type = strtoul(argv[0], &p, 0)) == 0 && p == argv[0]){
			rp->status = Status_BADARG;
			return -1;
		}
		break;

	case 0:
		break;
	}

	list = malloc(nbytes);
	if(list == 0){
		rp->status = STnomem;
		return -1;
	}
	status = SRestatus(rp, type, list, nbytes);
	if(status == -1){
		free(list);
		return -1;
	}

	lp = list;
	nbytes = ((lp[5]<<16)|(lp[6]<<8)|lp[7])-8;
	Bprint(&bout, " Header\n   ");
	for(i = 0; i < 8; i++){				/* header */
		Bprint(&bout, " %2.2uX", *lp);
		lp++;
	}
	Bputc(&bout, '\n');

	while(nbytes > 0){				/* pages */
		i = ((lp[5]<<16)|(lp[6]<<8)|lp[7]);
		nbytes -= i+8;
		Bprint(&bout, " Type");
		for(n = 0; n < 8; n++)			/* header */
			Bprint(&bout, " %2.2uX", lp[n]);
		Bprint(&bout, "\n   ");
		d = (lp[2]<<8)|lp[3];
		lp += 8;
		for(n = 0; n < i; n++){
			if(n && (n % d) == 0)
				Bprint(&bout, "\n   ");
			Bprint(&bout, " %2.2uX", *lp);
			lp++;
		}
		if(n && (n % d))
			Bputc(&bout, '\n');
	}

	free(list);
	return status;
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

static int atatable[4] = {
	'C', 'D', 'E', 'F',
};
static int scsitable[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 
};
static int unittable[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 
};

static long
cmdprobe(ScsiReq *rp, int argc, char *argv[])
{
	char buf[32];
	ScsiReq scsireq;
	char *ctlr, *unit;
	
	USED(argc, argv);
	rp->status = STok;
	scsireq.flags = 0;

	for(ctlr="CDEF0123456789abcdef"; *ctlr; ctlr++) {
		/*
		 * I can guess how many units you have.
		 */
		if(*ctlr >= 'C' && *ctlr <= 'F')
			unit = "01";
		else if((*ctlr >= '0' && *ctlr <= '9')
		     || (*ctlr >= 'a' && *ctlr <= 'f'))
			unit = "0123456789abcdef";
		else
			unit = "012345678";
			
		for(; *unit; unit++){
			sprint(buf, "/dev/sd%c%c", *ctlr, *unit);
			if(SRopenraw(&scsireq, buf) == -1)
				/*
				return -1;
				 */
				continue;
			SRreqsense(&scsireq);
			switch(scsireq.status){
	
			default:
				break;
	
			case STok:
			case Status_SD:
				Bprint(&bout, "%s: ", buf);
				cmdinquiry(&scsireq, 0, 0);
				break;
			}	
			SRclose(&scsireq);
		}
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
	int raw;
	long status;

	raw = 0;
	if(argc && strcmp("-r", argv[0]) == 0){
		raw = 1;
		argc--, argv++;
	}
	if(argc != 1){
		rp->status = Status_BADARG;
		return -1;
	}
	if(raw == 0){
		if((status = SRopen(rp, argv[0])) != -1 && verbose)
			Bprint(&bout, "%sblock size: %ld\n",
				rp->flags&Fbfixed? "fixed ": "", rp->lbsize);
	}
	else {
		status = SRopenraw(rp, argv[0]);
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
	{ "modeselect6",cmdmodeselect6,	1,		/*[0x15] */
	  "modeselect6 bytes...",
	},
	{ "modeselect",	cmdmodeselect10, 1,		/*[0x55] */
	  "modeselect bytes...",
	},
	{ "modesense6",	cmdmodesense6,	1,		/*[0x1A]*/
	  "modesense6 [page [nbytes]]",
	},
	{ "modesense",	cmdmodesense10, 1,		/*[0x5A]*/
	  "modesense [page [nbytes]]",
	},
	{ "start",	cmdstart,	1,		/*[0x1B]*/
	  "start [code]",
	},
	{ "stop",	cmdstop,	1,		/*[0x1B]*/
	  "stop",
	},
	{ "eject",	cmdeject,	1,		/*[0x1B]*/
	  "eject",
	},
	{ "ingest",	cmdingest,	1,		/*[0x1B]*/
	  "ingest",
	},
	{ "capacity",	cmdcapacity,	1,		/*[0x25]*/
	  "capacity",
	},

	{ "blank",	cmdblank,	1,		/*[0xA1]*/
	  "blank [track/LBA [type]]",
	},
//	{ "synccache",	cmdsynccache,	1,		/*[0x35]*/
//	  "synccache",
//	},
	{ "rtoc",	cmdrtoc,	1,		/*[0x43]*/
	  "rtoc [track/session-number [format]]",
	},
	{ "rdiscinfo",	cmdrdiscinfo,	1,		/*[0x51]*/
	  "rdiscinfo",
	},
	{ "rtrackinfo",	cmdrtrackinfo,	1,		/*[0x52]*/
	  "rtrackinfo [track]",
	},

	{ "cdpause",	cmdcdpause,	1,		/*[0x4B]*/
	  "cdpause",
	},
	{ "cdresume",	cmdcdresume,	1,		/*[0x4B]*/
	  "cdresume",
	},
	{ "cdstop",	cmdcdstop,	1,		/*[0x4E]*/
	  "cdstop",
	},
	{ "cdplay",	cmdcdplay,	1,		/*[0xA5]*/
	  "cdplay [track-number] or [-r [LBA [length]]]",
	},
	{ "cdload",	cmdcdload,	1,		/*[0xA6*/
	  "cdload [slot]",
	},
	{ "cdunload",	cmdcdunload,	1,		/*[0xA6]*/
	  "cdunload [slot]",
	},
	{ "cdstatus",	cmdcdstatus,	1,		/*[0xBD]*/
	  "cdstatus",
	},
//	{ "getconf",	cmdgetconf,	1,		/*[0x46]*/
//	  "getconf",
//	},

//	{ "fwaddr",	cmdfwaddr,	1,		/*[0xE2]*/
//	  "fwaddr [track [mode [npa]]]",
//	},
//	{ "treserve",	cmdtreserve,	1,		/*[0xE4]*/
//	  "treserve nbytes",
//	},
//	{ "trackinfo",	cmdtrackinfo,	1,		/*[0xE5]*/
//	  "trackinfo [track]",
//	},
//	{ "wtrack",	cmdwtrack,	1,		/*[0xE6]*/
//	  "wtrack [|]file [nbytes [track [mode]]]",
//	},
//	{ "load",	cmdload,	1,		/*[0xE7]*/
//	  "load",
//	},
//	{ "unload",	cmdunload,	1,		/*[0xE7]*/
//	  "unload",
//	},
//	{ "fixation",	cmdfixation,	1,		/*[0xE9]*/
//	  "fixation [toc-type]",
//	},
	{ "einit",	cmdeinit,	1,		/*[0x07]*/
	  "einit",
	},
	{ "estatus",	cmdestatus,	1,		/*[0xB8]*/
	  "estatus",
	},
	{ "mmove",	cmdmmove,	1,		/*[0xA5]*/
	  "mmove transport source destination [invert]",
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
	  "open [-r] sddev",
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
	fprint(2, "%s: usage: %s [-q] [-m maxiosize] [/dev/sdXX]\n", argv0, argv0);
	exits("usage");
}

static struct {
	int	status;
	char*	description;
} description[] = {
	STnomem,	"buffer allocation failed",
	STtimeout,	"bus timeout",
	STharderr,	"controller error of some kind",
	STok,		"good",
	STcheck,	"check condition",
	STcondmet,	"condition met/good",
	STbusy,		"busy ",
	STintok,	"intermediate/good",
	STintcondmet,	"intermediate/condition met/good",
	STresconf,	"reservation conflict",
	STterminated,	"command terminated",
	STqfull,	"queue full",

	Status_SD,	"sense-data available",
	Status_SW,	"internal software error",
	Status_BADARG,	"bad argument to request",

	0, 0,
};

void
main(int argc, char *argv[])
{
	ScsiReq target;
	char *ap, *av[256];
	int ac, i;
	ScsiCmd *cp;
	long status;

	ARGBEGIN {
	case 'q':
		verbose = 0;
		break;
	case 'm':
		ap = ARGF();
		if(ap == nil)
			usage();
		maxiosize = atol(ap);
		if(maxiosize < 512 || maxiosize > MaxIOsize)
			usage();
		break;
	default:
		usage();
	} ARGEND

	if(Binit(&bin, 0, OREAD) == Beof || Binit(&bout, 1, OWRITE) == Beof){
		fprint(2, "%s: can't init bio: %r\n", argv0);
		exits("Binit");
	}

	memset(&target, 0, sizeof(target));
	if(argc && cmdopen(&target, argc, argv) == -1) {
		fprint(2, "open failed\n");
		usage();
	}
	Bflush(&bout);

	while(ap = Brdline(&bin, '\n')){
		ap[Blinelen(&bin)-1] = 0;
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
			for(i = 0; description[i].description; i++){
				if(target.status != description[i].status)
					continue;
				if(target.status == Status_SD)
					makesense(&target);
				else
					Bprint(&bout, "%s\n", description[i].description);
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
