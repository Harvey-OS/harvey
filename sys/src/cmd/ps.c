#include <u.h>
#include <libc.h>
#include <bio.h>

void	ps(char*);
void	error(char*);
int	cmp(void*, void*);

Biobuf	bout;
int	pflag;
int	aflag;

void
main(int argc, char *argv[])
{
	int fd, i, tot, none = 1;
	Dir *dir, **mem;


	ARGBEGIN {
	case 'a':
		aflag++;
		break;
	case 'p':
		pflag++;
		break;
	} ARGEND;
	Binit(&bout, 1, OWRITE);
	if(chdir("/proc")==-1)
		error("/proc");
	fd=open(".", OREAD);
	if(fd<0)
		error("/proc");
	tot = dirreadall(fd, &dir);
	if(tot <= 0){
		fprint(2, "ps: empty directory /proc\n");
		exits("empty");
	}
	mem = malloc(tot*sizeof(Dir*));
	for(i=0; i<tot; i++)
		mem[i] = dir++;

	qsort(mem, tot, sizeof(Dir*), cmp);
	for(i=0; i<tot; i++){
		ps(mem[i]->name);
		none = 0;
	}

	if(none)
		error("no processes; bad #p");
	exits(0);
}

void
ps(char *s)
{
	int i, n, fd;
	char buf[64];
	int basepri, pri;
	ulong utime, stime, size;
	char pbuf[8];
#define NAMELEN 28
	char status[2*NAMELEN+12+9*12+1];
	char args[256];
	char *p;

	sprint(buf, "%s/status", s);
	fd = open(buf, OREAD);
	if(fd<0)
		return;
	n = read(fd, status, sizeof status-1);
	close(fd);
	if(n <= 0)
		return;
	status[n] = '\0';
	p = strchr(status, ' ');
	if(!p)
		return;
	*p = 0;
	p = strchr(status+NAMELEN, ' ');
	if(!p)
		return;
	*p = 0;
	status[2*NAMELEN+12-1] = 0;
	utime = strtoul(status+2*NAMELEN+12, 0, 0)/1000;
	stime = strtoul(status+2*NAMELEN+12+1*12, 0, 0)/1000;
	size  = strtoul(status+2*NAMELEN+12+6*12, 0, 0);
	if(pflag){
		basepri = strtoul(status+2*NAMELEN+12+7*12, 0, 0);
		pri = strtoul(status+2*NAMELEN+12+8*12, 0, 0);
		sprint(pbuf, " %2d %2d", basepri, pri);
	} else
		pbuf[0] = 0;
	Bprint(&bout, "%-10s %8s %4lud:%.2lud %3lud:%.2lud%s %7ludK %-.8s ",
			status+NAMELEN,
			s,
			utime/60, utime%60,
			stime/60, stime%60,
			pbuf,
			size,
			status+2*NAMELEN);

	if(aflag == 0){
    Noargs:
		Bprint(&bout, "%s\n", status);
		return;
	}

	sprint(buf, "%s/args", s);
	fd = open(buf, OREAD);
	if(fd < 0)
		goto Badargs;
	n = read(fd, args, sizeof args-1);
	close(fd);
	if(n < 0)
		goto Badargs;
	if(n == 0)
		goto Noargs;
	args[n] = '\0';
	for(i=0; i<n; i++)
		if(args[i] == '\n')
			args[i] = ' ';
	Bprint(&bout, "%s\n", args);
	return;

    Badargs:
	Bprint(&bout, "%s ?\n", status);
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
	Dir **a, **b;

	a = va;
	b = vb;
	return atoi((*a)->name) - atoi((*b)->name);
}
