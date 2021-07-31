#include	<u.h>
#include	<libc.h>

#define	MINUTE(x)	((long)(x)*60L)
#define	HOUR(x)		(MINUTE(x)*60L)
#define	YEAR(x)		(HOUR(x)*24L*360L)

int	verb;
int	uflag;
int	diff;
int	diffb;
char*	ndump;
char*	sflag;

void	ysearch(char*);
long	starttime(char*);
void	lastbefore(ulong, char*, char*);
char*	prtime(ulong);

void
main(int argc, char *argv[])
{
	char buf[100];
	Tm *tm;
	Dir dir;
	Waitmsg w;
	int i;

	ndump = "dump";
	ARGBEGIN {
	default:
		goto usage;
	case 'v':
		verb = 1;
		break;
	case 'd':
		ndump = ARGF();
		break;
	case 'D':
		diff = 1;
		break;
	case 'b':
		diffb = 1;
		break;
	case 's':
		sflag = ARGF();
		break;
	case 'u':
		uflag = 1;
		break;
	} ARGEND

	if(argc == 0) {
	usage:
		fprint(2, "usage: history [-vuD] [-d 9fsname] [-s yymmdd] files\n");
		exits(0);
	}

	tm = localtime(time(0));
	sprint(buf, "/n/%s/%.4d/", ndump, tm->year+1900);
	if(dirstat(buf, &dir)) {
		if(verb)
			print("mounting dump\n");
		if(rfork(RFFDG|RFPROC) == 0) {
			execl("/bin/rc", "rc", "9fs", ndump, 0);
			exits(0);
		}
		while(wait(&w) != -1)
			;
	}

	for(i=0; i<argc; i++)
		ysearch(argv[i]);
	exits(0);
}

void
ysearch(char *file)
{
	char fil[400], buf[500], pair[2][500];
	Dir dir;
	ulong otime, dt;
	int toggle, started;

	fil[0] = 0;
	if(file[0] != '/') {
		getwd(strchr(fil, 0), 100);
		strcat(fil, "/");
	}
	strcat(fil, file);
	dir.mtime = starttime(sflag);
	started = 0;
	toggle = 0;
	for(;;) {
		otime = dir.mtime;
		lastbefore(otime, fil, buf);
		if(dirstat(buf, &dir))
			return;
		dt = HOUR(12);
		while(otime <= dir.mtime) {
			if(verb)
				print("backup %ld, %ld\n", dir.mtime, otime-dt);
			lastbefore(otime-dt, fil, buf);
			if(dirstat(buf, &dir))
				return;
			dt += HOUR(12);
		}
		strcpy(pair[toggle], buf);
		if(diff && started){
			switch(rfork(RFFDG|RFPROC)){
			case 0:
				if(diffb)
					execl("/bin/diff", "diff", "-nb", pair[toggle ^ 1], pair[toggle], 0);
				else
					execl("/bin/diff", "diff", "-n", pair[toggle ^ 1], pair[toggle], 0);
				fprint(2, "can't exec diff: %r\n");
				exits(0);
			case -1:
				fprint(2, "can't fork diff: %r\n");
				break;
			default:
				while(wait(nil) != -1)
					;
				break;
			}
		}
		print("%s %s %lld\n", prtime(dir.mtime), buf, dir.length);
		toggle ^= 1;
		started = 1;
	}
}

void
lastbefore(ulong t, char *f, char *b)
{
	Tm *tm;
	Dir dir;
	int vers, try;
	ulong t0;

	t0 = t;
	if(verb)
		print("%ld lastbefore %s\n", t0, f);
	for(try=0; try<10; try++) {
		tm = localtime(t);
		sprint(b, "/n/%s/%.4d/%.2d%.2d", ndump,
			tm->year+1900, tm->mon+1, tm->mday);
		if(dirstat(b, &dir) || dir.mtime > t0) {
			if(verb)
				print("%ld earlier %s\n", dir.mtime, b);
			t -= HOUR(24);
			continue;
		}
		for(vers=0;; vers++) {
			sprint(b, "/n/%s/%.4d/%.2d%.2d%d", ndump,
				tm->year+1900, tm->mon+1, tm->mday, vers+1);
			if(dirstat(b, &dir) || dir.mtime > t0)
				break;
			if(verb)
				print("%ld later %s\n", dir.mtime, b);
		}
		sprint(b, "/n/%s/%.4d/%.2d%.2d%s", ndump,
			tm->year+1900, tm->mon+1, tm->mday, f);
		if(vers)
			sprint(b, "/n/%s/%.4d/%.2d%.2d%d%s", ndump,
				tm->year+1900, tm->mon+1, tm->mday, vers, f);
		return;
	}
	strcpy(b, "XXX");	/* error */
}

char*
prtime(ulong t)
{
	static char buf[100];
	char *b;
	Tm *tm;

	if(uflag)
		tm = gmtime(t);
	else
		tm = localtime(t);
	b = asctime(tm);
	memcpy(buf, b+4, 24);
	buf[24] = 0;
	return buf;
}

long
starttime(char *s)
{
	Tm *tm;
	long t, dt;
	int i, yr, mo, da;

	t = time(0);
	if(s == 0)
		return t;
	for(i=0; i<6; i++)
		if(s[i] < '0' || s[i] > '9') {
			fprint(2, "bad start time: %s\n", s);
			return t;
		}
	yr = (s[0]-'0')*10 + s[1]-'0';
	mo = (s[2]-'0')*10 + s[3]-'0' - 1;
	da = (s[4]-'0')*10 + s[5]-'0';

	t = 0;
	dt = YEAR(10);
	for(i=0; i<50; i++) {
		tm = localtime(t+dt);
		if(yr > tm->year ||
		  (yr == tm->year && mo > tm->mon) ||
		  (yr == tm->year && mo == tm->mon) && da > tm->mday) {
			t += dt;
			continue;
		}
		dt /= 2;
		if(dt == 0)
			break;
	}
	t += HOUR(12);	/* .5 day to get to noon of argument */
	return t;
}
