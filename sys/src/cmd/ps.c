#include <u.h>
#include <libc.h>
#include <bio.h>

void	ps(char*);
void	error(char*);
int	cmp(void*, void*);

Biobuf	bin;
int	pflag;

void
main(int argc, char *argv[])
{
	int fd, i, n, tot, none = 1;
	char dir[70*DIRLEN], *mem;


	ARGBEGIN {
	case 'p':
		pflag++;
		break;
	} ARGEND;
	Binit(&bin, 1, OWRITE);
	if(chdir("/proc")==-1)
		error("/proc");
	fd=open(".", OREAD);
	if(fd<0)
		error("/proc");
	mem = sbrk(0);
	tot = 0;
	while((n = read(fd, dir, sizeof dir)) > 0){
		if(brk((void*)(mem + tot + n)) == -1)
			error("out of memory");
		memmove(mem+tot, dir, n);
		tot += n;
	}
	qsort(mem, tot/DIRLEN, DIRLEN, cmp);
	for(i=0; i<tot; i+=DIRLEN){
		ps(mem+i);
		none = 0;
	}

	if(none)
		error("no processes; bad #p");
	exits(0);
}

void
ps(char *s)
{
	int fd;
	char buf[64];
	int basepri, pri;
	long utime, stime, size;
	char pbuf[8];
	char status[2*NAMELEN+12+9*12];
	char *p;

	sprint(buf, "%s/status", s);
	fd = open(buf, OREAD);
	if(fd<0)
		return;
	memset(status, 0, sizeof status);
	if(read(fd, status, sizeof status) > 0){
		p = strchr(status, ' ');
		if(!p)
			goto Return;
		*p = 0;
		p = strchr(status+NAMELEN, ' ');
		if(!p)
			goto Return;
		*p = 0;
		status[2*NAMELEN+12-1] = 0;
		utime = atol(status+2*NAMELEN+12)/1000;
		stime = atol(status+2*NAMELEN+12+1*12)/1000;
		size  = atol(status+2*NAMELEN+12+6*12);
		if(pflag){
			basepri = atol(status+2*NAMELEN+12+7*12);
			pri = atol(status+2*NAMELEN+12+8*12);
			sprint(pbuf, " %2d %2d", basepri, pri);
		} else
			pbuf[0] = 0;
		Bprint(&bin, "%-10s %8s %4ld:%.2ld %3ld:%.2ld%s %7ldK %-.8s %s\n",
				status+NAMELEN,
				s,
				utime/60, utime%60,
				stime/60, stime%60,
				pbuf,
				size,
				status+2*NAMELEN,
				status);
	}
    Return:
	close(fd);
}

void
error(char *s)
{
	fprint(2, "ps: %s: ", s);
	perror("error");
	exits(s);
}

int
cmp(void *va, void *vb)
{
	char *a, *b;

	a = va;
	b = vb;
	return atoi(a) - atoi(b);
}
