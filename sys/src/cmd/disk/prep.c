/*
 * prep.c - Make unit partition table
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

int ropt, aopt;
int changed;
ulong size;
ulong secsize;
ulong secs;

enum {
	Maxpath=	4*NAMELEN,
	Maxpart=	8,
};

/*
 *  plan 9 partitions
 */
typedef struct Partition Partition;
struct Partition
{
	char name[NAMELEN+1];
	ulong start;
	ulong end;
};
int npart;
Partition ptab[Maxpart+2];

char *secbuf;
Biobuf in;
ulong dosend;	/* end of any active dos partition */

void	error(char*, void*);
int	rddiskinfo(char*);
void	prepare(char*);
int	read_table(int);
void	print_table(char*);
void	write_table(int);
void	auto_table(void);
void	edit_table(void);
void	rddosinfo(char*);
ulong	memsize(void);

void
main (int argc, char **argv)
{
	if(Binit(&in, 0, OREAD) < 0){
		fprint(2, "can't Binit\n");
		exits("Binit");
	}
		

	if(argc < 1) {
		fprint(2, "prep [-ra] special\n");
		exits("usage");
	}

	ARGBEGIN{
	case 'r':
		ropt++;
		break;
	case 'a':
		aopt++;
		break;
	default:
		fprint(2, "prep [-ra] special\n");
		exits("usage");
	}ARGEND;
	if(argc != 1){
		fprint(2, "prep [-ra] special\n");
		exits("usage");
	}

	prepare(argv[0]);

	if(changed) 
		print("\n *** PARTITION TABLE HAS BEEN MODIFIED ***\n");
	exits(0);
}

void
error(char *s, void *x)
{
	char buf1[256];
	char buf[ERRLEN];

	sprint(buf1, s, x);
	errstr(buf);
	fprint(2, "prep: %s: %s\n", buf1, buf);
	exits(buf1);
}

int
rddiskinfo(char *special)
{
	Dir d;
	char name[Maxpath];
	int fd;
	Partition *pp = ptab;

	if(strcmp(special + strlen(special) - 4, "disk") == 0)
		*(special + strlen(special) - 4) = 0;
	sprint(name, "%sdisk", special);
	if(dirstat(name, &d) < 0)
		error("stating %s", name);
	size = d.length;
	sprint(name, "%spartition", special);
	if(dirstat(name, &d) < 0)
		error("stating %s", name);
	secsize = d.length;
	secs = size/secsize;
	if(secbuf == 0)
		secbuf = malloc(secsize+1);


	npart = 2;

	strcpy(pp->name, "disk");
	pp->start = 0;
	pp->end = secs;
	pp++;

	strcpy(pp->name, "partition");
	pp->start = secs - 1;
	pp->end = secs;

	fd = open(name, ORDWR);
	if(fd < 0)
		error("opening %s", name);

	print("sector = %d bytes, disk = %d sectors, all numbers are in sectors\n", secsize, size/secsize);
	return fd;
}

void
prepare(char *special)
{
	int i, fd;
	char *line;
	int delete = 0, automatic = aopt;
	Partition *pp;

	fd = rddiskinfo(special);	
	rddosinfo(special);

	if(aopt == 0)
		switch(read_table(fd)){
		case -1:
			automatic++;
			break;
		default:
			print("Plan 9 partition table exists\n");
			print_table(special);
			print("(a)utopartition, (d)elete, (e)dit or (q)uit ?");
			line = Brdline(&in, '\n');
			if(line == 0)
				exits("exists");
			if(ropt)
				return;
			switch (*line) {
				case 'a':
					delete++;
					automatic++;
					break;
				case 'd':
					delete++;
					break;
				case 'e':
					break;
				default:
					return;
			}
		}

	if(delete){
		pp = &ptab[2];
		for(i = 2; i < Maxpart+2; i++){
			pp->name[0] = 0;
			pp->start = 0;
			pp->end = 0;
			pp++;
		}
		npart = 2;
	}

	if(npart == 2 && dosend && dosend < ptab[0].end){
		strcpy(ptab[2].name, "dos");
		ptab[2].start = 0;
		ptab[2].end = dosend;
	}

	if(automatic)
		print("Setting up default Plan 9 partitions\n");
	else
		print_table(special);

	npart = 2;
	for(;;) {
		if(automatic){
			auto_table();
			automatic = 0;
		} else
			edit_table();
		if(aopt)
			break;

		print_table(special);
		print("\nOk [y/n]:");
		line = Brdline(&in, '\n');
		if(line == 0)
			return;
		if (*line == '\n' || *line == 'y')
			break;
	}
	write_table(fd);
	close(fd);
}

/*
 *  make a boot area, swap space, and file system (leave DOS partition)
 */
#define BOOTSECS 2048
void
auto_table(void)
{
	ulong total, next, swap;
	Partition *pp;

	total = ptab[0].end - ptab[0].start - 1;
	next = 0;
	pp = &ptab[2];
	if(strcmp(pp->name, "dos") == 0){
		next += pp->end - pp->start;
		pp++;
	}

	if(total-next < BOOTSECS)
		error("not enough room for boot area", 0);
	strcpy(pp->name, "boot");
	pp->start = next;
	next += BOOTSECS;
	pp->end = next;
	pp++;

	/*
	 *  grab at most 20% of remaining for swap
	 */
	swap = (total-next)*20/100;
	if(swap > 2*memsize())
		swap = 2*memsize();
	strcpy(pp->name, "swap");
	pp->start = next;
	next += swap;
	pp->end = next;
	pp++;

	/*
	 *  one page for nvram
	 */
	strcpy(pp->name, "nvram");
	pp->start = next;
	next += 1;
	pp->end = next;
	pp++;

	/*
	 *  the rest is file system
	 */
	strcpy(pp->name, "fs");
	pp->start = next;
	next = total;
	pp->end = next;
	pp++;

	npart = pp - ptab;
}

void
edit_table(void)
{
	int i, j;
	Partition *pp;
	char *line;
	ulong sofar = 0;

	pp = &ptab[2];
	for(i = 2; i < Maxpart+2; i++){
		for(;;){
			print(" %d name (%s [- to delete, * to quit]):", i, pp->name);
			line = Brdline(&in, '\n');
			if(pp->name[0] == 0){
				pp->start = sofar;
				pp->end = secs - 1;
			}
			if(line == 0 || *line == '*')
				return;
			line[Blinelen(&in)-1] = 0;
			if(*line == '-'){
				pp->name[0] = 0;
				for(j = i; j < Maxpart+1; j++)
					ptab[j] = ptab[j+1];
			} else if(*line != 0){
				strncpy(pp->name, line, NAMELEN);
				break;
			} else if(pp->name[0])
				break;
		}

		do {
			print(" %d start (%d):", i, pp->start);
			line = Brdline(&in, '\n');
			if(line == 0)
				return;
			line[Blinelen(&in)-1] = 0;
			if(*line != 0)  
				pp->start = atoi(line);
			if(pp->end == 0)
				pp->end = pp->start;
		}while(pp->start < 0 || pp->start >= ptab[0].end);

		do {
			print(" %d length (%d):", i, pp->end-pp->start);
			line = Brdline(&in, '\n');
			if(line == 0)
				return;
			line[Blinelen(&in)-1] = 0;
			if(*line != 0)  
				pp->end = pp->start + atoi(line);
		}while(pp->end < 0 || pp->end > ptab[0].end ||
			pp->end < pp->start);
		sofar = pp->end;

		npart = i+1;
		pp++;
	}
}
	
int
read_table(int fd)
{
	int i;
	int lines;
	char *line[Maxpart+1];
	char *field[3];
	Partition *pp;

	if((i = read(fd, secbuf, secsize)) < 0)
		error("reading partition table", 0);
	secbuf[i] = 0;

	setfields("\n");
	lines = getfields(secbuf, line, Maxpart+1);
	setfields(" \t");

	if(strcmp(line[0], "plan9 partitions") != 0)
		return -1;

	pp = &ptab[npart];
	for(i = 1; i < lines; i++){
		if(getfields(line[i], field, 3) != 3)
			break;
		if(strlen(field[0]) > NAMELEN)
			break;
		strcpy(pp->name, field[0]);
		pp->start = strtoul(field[1], 0, 0);
		pp->end = strtoul(field[2], 0, 0);
		if(pp->start > pp->end || pp->start >= ptab[0].end)
			break;
		npart++;
		pp++;
	}

	return npart;
}

void
print_table(char *special)
{
	int i, j;
	ulong s1, s2, e1, e2;
	ulong ps;
	Partition *pp;
	char buf[2*NAMELEN];
	char *basename;
	char overlap[Maxpart+3];
	char *op;

	basename = strrchr(special, '/');
	if(basename == 0)
		basename = special;

	print("\nNr Name                 Overlap       Start      End        %%     Size\n");
	for(i = 0; i < npart; i++) {
		pp = &ptab[i];
		sprint(buf, "%s%s", basename, pp->name);
		s1 = pp->start;
		e1 = pp->end;
		op = overlap;
		for(j = 0; j < npart; j++){
			s2 = ptab[j].start;
			e2 = ptab[j].end;
			if(s1 < e2 && e1 > s2)
				*op++ = '0' + j;
			else
				*op++ = '-';
		}
		for(; j < Maxpart+2; j++)
			*op++ = ' ';
		*op = 0;
		ps = pp->end - pp->start;
		print("%2d %-20.20s %s %8d %8d %8d %8d\n", i, buf, overlap,
			pp->start, pp->end, (ps*100)/secs, ps);
	}
}

void
write_table(int fd)
{
	int off;
	int i;
	Partition *pp;

	if(ropt)
		return;

	if(seek(fd, 0, 0) < 0)
		error("seeking table", 0);

	memset(secbuf, 0, secsize);
	off = sprint(secbuf, "plan9 partitions\n");
	for(i = 2; i < npart; i++){
		pp = &ptab[i];
		off += sprint(&secbuf[off], "%s %d %d\n", pp->name, pp->start, pp->end);
	}

	if(write(fd, secbuf, secsize) != secsize)
		error("writing table", 0);

	changed = 1;
}

/*
 *  DOS boot sector and partition table
 */
typedef struct DOSpart	DOSpart;
struct DOSpart
{
	uchar flag;		/* active flag */
	uchar shead;		/* starting head */
	uchar scs[2];		/* starting cylinder/sector */
	uchar type;		/* partition type */
	uchar ehead;		/* ending head */
	uchar ecs[2];		/* ending cylinder/sector */
	uchar start[4];		/* starting sector */
	uchar len[4];		/* length in sectors */
};
typedef struct DOSptab	DOSptab;
struct DOSptab
{
	DOSpart p[4];
	uchar magic[2];		/* 0x55 0xAA */
};
enum
{
	DOSactive=	0x80,	/* active partition */

	DOS12=		1,	/* 12-bit FAT partiton */
	DOS16=		4,	/* 16-bit FAT partiton */
	DOSext=		5,	/* extended partiton */
	DOShuge=	6,	/* "huge" partiton */

	DOSmagic0=	0x55,
	DOSmagic1=	0xAA,
};

#define GL(p) ((p[3]<<24)|(p[2]<<16)|(p[1]<<8)|p[0])

void
rddosinfo(char *special)
{
	char name[Maxpath];
	int fd;
	char *t;
	DOSptab b;
	DOSpart *dp;
	ulong end;

	sprint(name, "%sdisk", special);
	fd = open(name, OREAD);
	if(fd < 0)
		error("opening %s", name);
	seek(fd, 446, 0);
	if(read(fd, &b, sizeof(b)) != sizeof(b))
		return;
	close(fd);
	if(b.magic[0] != DOSmagic0 || b.magic[1] != DOSmagic1)
		return;

	print("\nDOS partition table exists\n");
	print("\nNr Type           Start      Len\n");
	for(dp = b.p; dp < &b.p[4]; dp++){
		switch(dp->type){
		case 0:
			continue;
		case DOS12:
			t = "12 bit FAT";
			break;
		case DOS16:
			t = "16 bit FAT";
			break;
		case DOSext:
			t = "extended";
			break;
		case DOShuge:
			t = "huge";
			break;
		default:
			t = "unknown";
			break;
		}
		print("%2d %-11.11s %8d %8d %s\n", dp-b.p, t, 
			GL(dp->start), GL(dp->len),
			dp->flag==DOSactive ? "Active" : "");
		end = GL(dp->start) + GL(dp->len);
		if(dp->flag == DOSactive && end > dosend)
			dosend = end;
	}
	print("\n");
}

/*
 *  return memory size in sectors
 */
ulong
memsize(void)
{
	int fd, n, by2pg, secs;
	char *p;
	char buf[128];

	p = getenv("cputype");
	if(p && strcmp(p, "68020") == 0)
		by2pg = 8*1024;
	else
		by2pg = 4*1024;

	secs = 4*1024*1024/secsize;
	
	fd = open("/dev/swap", OREAD);
	if(fd < 0)
		return secs;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return secs;
	buf[n] = 0;
	p = strchr(buf, '/');
	if(p)
		secs = strtoul(p+1, 0, 0) * (by2pg/secsize);
	return secs;
}
