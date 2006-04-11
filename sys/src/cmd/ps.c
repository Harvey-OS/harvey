#include <u.h>
#include <libc.h>
#include <bio.h>

void	ps(char*);
void	error(char*);
int	cmp(void*, void*);

Biobuf	bout;
int	pflag;
int	aflag;
int	rflag;

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
	case 'r':
		rflag++;
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
	ulong utime, stime, rtime, size;
	int argc, basepri, fd, i, n, pri;
	char args[256], *argv[16], buf[64], pbuf[8], rbuf[20], rbuf1[20], status[4096];

	sprint(buf, "%s/status", s);
	fd = open(buf, OREAD);
	if(fd<0)
		return;
	n = read(fd, status, sizeof status-1);
	close(fd);
	if(n <= 0)
		return;
	status[n] = '\0';

	if((argc = tokenize(status, argv, nelem(argv)-1)) < 12)
		return;
	argv[argc] = 0;

	/*
	 * 0  text
	 * 1  user
	 * 2  state
	 * 3  cputime[6]
	 * 9  memory
	 * 10 basepri
	 * 11 pri
	 */
	utime = strtoul(argv[3], 0, 0)/1000;
	stime = strtoul(argv[4], 0, 0)/1000;
	rtime = strtoul(argv[5], 0, 0)/1000;
	size  = strtoul(argv[9], 0, 0);
	if(pflag){
		basepri = strtoul(argv[10], 0, 0);
		pri = strtoul(argv[11], 0, 0);
		sprint(pbuf, " %2d %2d", basepri, pri);
	} else
		pbuf[0] = 0;

	if(rflag){
		if(rtime >= 86400)
			sprint(rbuf, " %lud:%02lud:%02lud:%02lud", rtime/86400, (rtime/3600)%24, (rtime/60)%60, rtime%60);
		else if(rtime >= 3600)
			sprint(rbuf, " %lud:%02lud:%02lud", rtime/3600, (rtime/60)%60, rtime%60);
		else
			sprint(rbuf, " %lud:%02lud", rtime/60, rtime%60);
		sprint(rbuf1, "%12s", rbuf);
	}else
		rbuf1[0] = 0;

	Bprint(&bout, "%-10s %8s%s %4lud:%.2lud %3lud:%.2lud %s %7ludK %-8.8s ",
			argv[1],
			s,
			rbuf1,
			utime/60, utime%60,
			stime/60, stime%60,
			pbuf,
			size,
			argv[2]);

	if(aflag == 0){
    Noargs:
		Bprint(&bout, "%s\n", argv[0]);
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
	Bprint(&bout, "%s ?\n", argv[0]);
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
