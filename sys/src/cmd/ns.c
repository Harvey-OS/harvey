#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>

#pragma	varargck	type	"P"	char*

int	nsrv;
Dir	*srv;
Biobuf	stdout;

typedef struct Mount Mount;

struct Mount
{
	char	*cmd;
	char	*flag;
	char	*new;
	char	*old;
	char	*spec;
};

void	xlatemnt(Mount*);
char	*quote(char*);

int	rflag;

void
usage(void)
{
	fprint(2, "usage: ns [-r] [pid]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Mount *m;
	int line, fd, n, pid;
	char buf[1024], *av[5];

	ARGBEGIN{
	case 'r':
		rflag++;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1){
		pid = atoi(argv[0]);
		if(pid == 0)
			usage();
	}else
		pid = getpid();

	Binit(&stdout, 1, OWRITE);

	sprint(buf, "/proc/%d/ns", pid);
	fd = open(buf, OREAD);
	if(fd < 0) {
		fprint(2, "ns: open %s: %r\n", buf);
		exits("open ns");
	}

	for(line=1; ; line++) {
		n = read(fd, buf, sizeof(buf));
		if(n == sizeof(buf)) {
			fprint(2, "ns: ns string too long\n");
			exits("read ns");
		}
		if(n < 0) {
			fprint(2, "ns: read %r\n");
			exits("read ns");
		}
		if(n == 0)
			break;
		buf[n] = '\0';

		m = mallocz(sizeof(Mount), 1);
		if(m == nil) {
			fprint(2, "ns: no memory: %r\n");
			exits("no memory");
		}

		n = tokenize(buf, av, 5);
		switch(n){
		case 2:
			if(strcmp(av[0], "cd") == 0){
				Bprint(&stdout, "%s %s\n", av[0], av[1]);
				continue;
			}
			/* fall through */
		default:
			fprint(2, "ns: unrecognized format of ns file: %d elements on line %d\n", n, line);
			exits("format");
		case 5:
			m->cmd = strdup(av[0]);
			m->flag = strdup(av[1]);
			m->new = strdup(av[2]);
			m->old = strdup(av[3]);
			m->spec = strdup(av[4]);
			break;
		case 4:
			if(av[1][0] == '-'){
				m->cmd = strdup(av[0]);
				m->flag = strdup(av[1]);
				m->new = strdup(av[2]);
				m->old = strdup(av[3]);
				m->spec = strdup("");
			}else{
				m->cmd = strdup(av[0]);
				m->flag = strdup("");
				m->new = strdup(av[1]);
				m->old = strdup(av[2]);
				m->spec = strdup(av[3]);
			}
			break;
		case 3:
			m->cmd = strdup(av[0]);
			m->flag = strdup("");
			m->new = strdup(av[1]);
			m->old = strdup(av[2]);
			m->spec = strdup("");
			break;
		}

		if(!rflag && strcmp(m->cmd, "mount")==0)
			xlatemnt(m);

		Bprint(&stdout, "%s %s %s %s %s\n", m->cmd, m->flag,
			quote(m->new), quote(m->old), quote(m->spec));

		free(m->cmd);
		free(m->flag);
		free(m->new);
		free(m->old);
		free(m->spec);
		free(m);
	}

	exits(nil);
}

void
xlatemnt(Mount *m)
{
	int n, fd;
	char *s, *t, *net, *port;
	char buf[256];

	if(strncmp(m->new, "/net/", 5) != 0)
		return;

	s = strdup(m->new);
	net = s+5;
	for(t=net; *t!='/'; t++)
		if(*t == '\0')
			goto Return;
	*t = '\0';
	port = t+1;
	for(t=port; *t!='/'; t++)
		if(*t == '\0')
			goto Return;
	*t = '\0';
	if(strcmp(t+1, "data") != 0)
		goto Return;
	snprint(buf, sizeof buf, "/net/%s/%s/remote", net, port);
	fd = open(buf, OREAD);
	if(fd < 0)
		goto Return;
	n = read(fd, buf, sizeof buf);
	close(fd);
	if(n<=1 || n>sizeof buf)
		goto Return;
	if(buf[n-1] == '\n')
		--n;
	buf[n] = '\0';
	t = malloc(strlen(net)+1+n+1);
	if(t == nil)
		goto Return;
	sprint(t, "%s!%s", net, buf);
	free(m->new);
	m->new = t;

Return:	
	free(s);
}

char*
quote(char *s)
{
	static char buf[3][1024];
	static int i;
	char *p, *ep;

	if(strpbrk(s, " '\\\t#$") == nil)
		return s;
	i = (i+1)%3;
	p = &buf[i][0];
	ep = &buf[i][1024];
	*p++ = '\'';
	while(p < ep-5){
		switch(*s){
		case '\0':
			goto out;
		case '\'':
			*p++ = '\'';
			break;
		}
		*p++ = *s++;
	}
    out:
	*p++ = '\'';
	*p = '\0';
	return buf[i];
}
