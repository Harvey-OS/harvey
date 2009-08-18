#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

typedef struct Data	Data;
typedef struct Pc	Pc;
typedef struct Acc	Acc;

struct Data
{
	ushort	down;
	ushort	right;
	ulong	pc;
	ulong	count;
	ulong	time;
};

struct Pc
{
	Pc	*next;
	ulong	pc;
};

struct Acc
{
	char	*name;
	ulong	pc;
	ulong	ms;
	ulong	calls;
};

Data*	data;
Acc*	acc;
ulong	ms;
long	nsym;
long	ndata;
int	dflag;
int	rflag;
Biobuf	bout;
int	tabstop = 4;
int	verbose;

void	syms(char*);
void	datas(char*);
void	graph(int, ulong, Pc*);
void	plot(void);
char*	name(ulong);
void	indent(int);
char*	defaout(void);

void
main(int argc, char *argv[])
{
	char *s;

	s = getenv("tabstop");
	if(s!=nil && strtol(s,0,0)>0)
		tabstop = strtol(s,0,0);
	ARGBEGIN{
	case 'v':
		verbose = 1;
		break;
	case 'd':
		dflag = 1;
		break;
	case 'r':
		rflag = 1;
		break;
	default:
		fprint(2, "usage: prof [-dr] [8.out] [prof.out]\n");
		exits("usage");
	}ARGEND
	Binit(&bout, 1, OWRITE);
	if(argc > 0)
		syms(argv[0]);
	else
		syms(defaout());
	if(argc > 1)
		datas(argv[1]);
	else
		datas("prof.out");
	if(ndata){
		if(dflag)
			graph(0, data[0].down, 0);
		else
			plot();
	}
	exits(0);
}

void
swapdata(Data *dp)
{
	dp->down = beswab(dp->down);
	dp->right = beswab(dp->right);
	dp->pc = beswal(dp->pc);
	dp->count = beswal(dp->count);
	dp->time = beswal(dp->time);
}

int
acmp(void *va, void *vb)
{
	Acc *a, *b;
	ulong ua, ub;

	a = va;
	b = vb;
	ua = a->ms;
	ub = b->ms;

	if(ua > ub)
		return 1;
	if(ua < ub)
		return -1;
	return 0;
}

void
syms(char *cout)
{
	Fhdr f;
	int fd;

	if((fd = open(cout, 0)) < 0){
		perror(cout);
		exits("open");
	}
	if (!crackhdr(fd, &f)) {
		fprint(2, "can't read text file header\n");
		exits("read");
	}
	if (f.type == FNONE) {
		fprint(2, "text file not an a.out\n");
		exits("file type");
	}
	if (syminit(fd, &f) < 0) {
		fprint(2, "syminit: %r\n");
		exits("syms");
	}
	close(fd);
}

void
datas(char *dout)
{
	int fd;
	Dir *d;
	int i;

	if((fd = open(dout, 0)) < 0){
		perror(dout);
		exits("open");
	}
	d = dirfstat(fd);
	if(d == nil){
		perror(dout);
		exits("stat");
	}
	ndata = d->length/sizeof(data[0]);
	data = malloc(ndata*sizeof(Data));
	if(data == 0){
		fprint(2, "prof: can't malloc data\n");
		exits("data malloc");
	}
	if(read(fd, data, d->length) != d->length){
		fprint(2, "prof: can't read data file\n");
		exits("data read");
	}
	free(d);
	close(fd);
	for (i = 0; i < ndata; i++)
		swapdata(data+i);
}

char*
name(ulong pc)
{
	Symbol s;
	static char buf[16];

	if (findsym(pc, CTEXT, &s))
		return(s.name);
	snprint(buf, sizeof(buf), "#%lux", pc);
	return buf;
}

void
graph(int ind, ulong i, Pc *pc)
{
	long time, count, prgm;
	Pc lpc;

	if(i >= ndata){
		fprint(2, "prof: index out of range %ld [max %ld]\n", i, ndata);
		return;
	}
	count = data[i].count;
	time = data[i].time;
	prgm = data[i].pc;
	if(time < 0)
		time += data[0].time;
	if(data[i].right != 0xFFFF)
		graph(ind, data[i].right, pc);
	indent(ind);
	if(count == 1)
		Bprint(&bout, "%s:%lud\n", name(prgm), time);
	else
		Bprint(&bout, "%s:%lud/%lud\n", name(prgm), time, count);
	if(data[i].down == 0xFFFF)
		return;
	lpc.next = pc;
	lpc.pc = prgm;
	if(!rflag){
		while(pc){
			if(pc->pc == prgm){
				indent(ind+1);
				Bprint(&bout, "...\n");
				return;
			}
			pc = pc->next;
		}
	}
	graph(ind+1, data[i].down, &lpc);
}
/*
 *	assume acc is ordered by increasing text address.
 */
long
symind(ulong pc)
{
	int top, bot, mid;

	bot = 0;
	top = nsym;
	for (mid = (bot+top)/2; mid < top; mid = (bot+top)/2) {
		if (pc < acc[mid].pc)
			top = mid;
		else
		if (mid != nsym-1 && pc >= acc[mid+1].pc)
			bot = mid;
		else
			return mid;
	}
	return -1;
}

ulong
sum(ulong i)
{
	long j, dtime, time;
	int k;
	static indent;

	if(i >= ndata){
		fprint(2, "prof: index out of range %ld [max %ld]\n", i, ndata);
		return 0;
	}
	j = symind(data[i].pc);
	time = data[i].time;
	if(time < 0)
		time += data[0].time;
	if (verbose){
		for(k = 0; k < indent; k++)
			print("	");
		print("%lud: %ld/%lud", i, data[i].time, data[i].count);
		if (j >= 0)
			print("	%s\n", acc[j].name);
		else
			print("	0x%lux\n", data[i].pc);
	}
	dtime = 0;
	if(data[i].down != 0xFFFF){
		indent++;
		dtime = sum(data[i].down);
		indent--;
	}
	j = symind(data[i].pc);
	if (j >= 0) {
		acc[j].ms += time - dtime;
		ms += time - dtime;
		acc[j].calls += data[i].count;
	}
	if(data[i].right == 0xFFFF)
		return time;
	return time + sum(data[i].right);
}

void
plot(void)
{
	Symbol s;

	for (nsym = 0; textsym(&s, nsym); nsym++) {
		acc = realloc(acc, (nsym+1)*sizeof(Acc));
		if(acc == 0){
			fprint(2, "prof: malloc fail\n");
			exits("acc malloc");
		}
		acc[nsym].name = s.name;
		acc[nsym].pc = s.value;
		acc[nsym].calls = acc[nsym].ms = 0;
	}
	sum(data[0].down);
	qsort(acc, nsym, sizeof(Acc), acmp);
	Bprint(&bout, "  %%     Time     Calls  Name\n");
	if(ms == 0)
		ms = 1;
	while (--nsym >= 0) {
		if(acc[nsym].calls)
			Bprint(&bout, "%4.1f %8.3f %8lud\t%s\n",
				(100.0*acc[nsym].ms)/ms,
				acc[nsym].ms/1000.0,
				acc[nsym].calls,
				acc[nsym].name);
	}
}

void
indent(int ind)
{
	int j;

	j = 2*ind;
	while(j >= tabstop){
		Bwrite(&bout, ".\t", 2);
		j -= tabstop;
	}
	if(j)
		Bwrite(&bout, ".                            ", j);
}

char*	trans[] =
{
	"386",		"8.out",
	"68020",		"2.out",
	"alpha",		"7.out",
	"amd64",	"6.out",
	"arm",		"5.out",
	"mips",		"v.out",
	"power",		"q.out",
	"sparc",		"k.out",
	"spim",		"0.out",
	0,0
};

char*
defaout(void)
{
	char *p;
	int i;

	p = getenv("objtype");
	if(p)
	for(i=0; trans[i]; i+=2)
		if(strcmp(p, trans[i]) == 0)
			return trans[i+1];
	return trans[1];
}
