/*
 * Process in-band messages about window title changes.
 * The messages are of the form:
 *
 *	\033];xxx\007
 *
 * where xxx is the new directory.  This format was chosen
 * because it changes the label on xterm windows.
 */

#include <u.h>
#include <libc.h>

struct {
	char *file;
	char name[512];
} keep[] = {
	{ "/dev/label" },
	{ "/dev/wdir" }
};

char *prog = "/bin/rwd";

void
usage(void)
{
	fprint(2, "usage: conswdir [/bin/rwd]\n");
	exits("usage");
}

void
save(void)
{
	int i, fd;
	for(i = 0; i < nelem(keep); i++){
		*keep[i].name = 0;
		if((fd = open(keep[i].file, OREAD)) != -1){
			read(fd, keep[i].name, sizeof(keep[i].name));
			close(fd);
		}
	}
}

void
rest(void)
{
	int i, fd;
	for(i = 0; i < nelem(keep); i++)
		if((fd = open(keep[i].file, OWRITE)) != -1){
			write(fd, keep[i].name, strlen(keep[i].name));
			close(fd);
		}

}

void
setpath(char *s)
{
	switch(rfork(RFPROC|RFFDG|RFNOWAIT)){
	case 0:
		execl(prog, prog, s, nil);
		_exits(nil);
	}
}

enum
{
	None,
	Esc,
	Brack,
	Semi,
	Bell,
};

int
process(char *buf, int n, int *pn)
{
	char *p;
	char path[4096];
	int start, state;

	start = 0;
	state = None;
	for(p=buf; p<buf+n; p++){
		switch(state){
		case None:
			if(*p == '\033'){
				start = p-buf;
				state++;
			}
			break;
		case Esc:
			if(*p == ']')
				state++;
			else
				state = None;
			break;
		case Brack:
			if(*p == ';')
				state++;
			else
				state = None;
			break;
		case Semi:
			if(*p == '\007')
				state++;
			else if((uchar)*p < 040)
				state = None;
			break;
		}
		if(state == Bell){
			memmove(path, buf+start+3, p - (buf+start+3));
			path[p-(buf+start+3)] = 0;
			p++;
			memmove(buf+start, p, n-(p-buf));
			n -= p-(buf+start);
			p = buf+start;
			p--;
			start = 0;
			state = None;
			setpath(path);
		}
	}
	/* give up if we go too long without seeing the close */
	*pn = n;
	if(state == None || p-(buf+start) >= 2048)
		return (p - buf);
	else
		return start;
}

static void
catchint(void*, char *msg)
{
	if(strstr(msg, "interrupt"))
		noted(NCONT);
	else if(strstr(msg, "kill"))
		noted(NDFLT);
	else
		noted(NCONT);
}

void
main(int argc, char **argv)
{
	char buf[4096];
	int n, m;

	notify(catchint);

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1)
		prog = argv[0];

	save();
	n = 0;
	for(;;){
		m = read(0, buf+n, sizeof buf-n);
		if(m < 0){
			rerrstr(buf, sizeof buf);
			if(strstr(buf, "interrupt"))
				continue;
			break;
		}
		n += m;
		m = process(buf, n, &n);
		if(m > 0){
			write(1, buf, m);
			memmove(buf, buf+m, n-m);
			n -= m;
		}
	}
	rest();
	exits(nil);
}
