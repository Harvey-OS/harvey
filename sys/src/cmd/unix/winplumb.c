#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "shell32.lib")

char	*argv0 = "winplumb";
char errbuf[256];
unsigned long parseip(char*, char*);
typedef unsigned long ulong;
void oserror(void);

void
hnputl(void *p, unsigned long v)
{
	unsigned char *a;

	a = p;
	a[0] = v>>24;
	a[1] = v>>16;
	a[2] = v>>8;
	a[3] = v;
}

void
hnputs(void *p, unsigned short v)
{
	unsigned char *a;

	a = p;
	a[0] = v>>8;
	a[1] = v;
}

unsigned long
nhgetl(void *p)
{
	unsigned char *a;
	a = p;
	return (a[0]<<24)|(a[1]<<16)|(a[2]<<8)|(a[3]<<0);
}

unsigned short
nhgets(void *p)
{
	unsigned char *a;
	a = p;
	return (a[0]<<8)|(a[1]<<0);
}


int
main(int argc, char **argv)
{
	char *addr, *p, *q, to[4];
	char buf[2048];
	int port, fd, nfd, one, len, n, tot;
	ulong ip;
	struct sockaddr_in sin;
	WSADATA wasdat;

	if(argc != 1 && argc != 2){
	usage:
		fprintf(stderr, "usage: winplumb [tcp!ipaddr!port]\n");
		ExitThread(1);
	}

	if(argc == 1)
		addr = "tcp!*!17890";
	else
		addr = argv[1];

	strcpy(buf, addr);
	p = strchr(buf, '!');
	if(p == 0)
		goto usage;
	q = strchr(p+1, '!');
	if(q == 0)
		goto usage;
	*p++ = 0;
	*q++ = 0;

	if(strcmp(buf, "tcp") != 0)
		goto usage;

	port = atoi(q);
	if(strcmp(p, "*") == 0)
		ip = 0;
	else
		ip = parseip(to, p);

	WSAStartup(MAKEWORD(1, 1), &wasdat);


	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0){
		oserror();
		fprintf(stderr, "socket: %s\n", errbuf);
		ExitThread(1);
	}

	one = 1;
	if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof one) != 0){
		oserror();
		fprintf(stderr, "setsockopt nodelay: %s\n", errbuf);
	}

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof one) != 0){
		oserror();
		fprintf(stderr, "setsockopt reuse: %s\n", errbuf);
	}
	memset(&sin, 0, sizeof sin);
	sin.sin_family = AF_INET;
	hnputs(&sin.sin_port, port);
	hnputl(&sin.sin_addr, ip);
	if(bind(fd, (struct sockaddr*)&sin, sizeof sin) < 0){
		oserror();
		fprintf(stderr, "bind: %s\n", errbuf);
		ExitThread(1);
	}

	if(listen(fd, 5) < 0){
		oserror();
		fprintf(stderr, "listen: %s\n", errbuf);
		ExitThread(1);
	}

	for(;;){
		len = sizeof sin;
		nfd = accept(fd, (struct sockaddr*)&sin, &len);
		if(nfd < 0){
			oserror();
			fprintf(stderr, "accept: %s\n", errbuf);
			continue;
		}
		tot = 0;
		while(tot == 0 || buf[tot-1] != '\n'){
			n = recv(nfd, buf+tot, sizeof buf-tot, 0);
			if(n < 0)
				break;
			tot += n;
		}
		if(buf[tot-1] == '\n'){
			buf[tot-1] = 0;
			p = strchr(buf, ' ');
			if(p)
				*p++ = 0;
			ShellExecute(0, 0, buf, p, 0, SW_SHOWNORMAL);
		}
		closesocket(nfd);
	}
}


#define CLASS(p) ((*(unsigned char*)(p))>>6)


unsigned long
parseip(char *to, char *from)
{
	int i;
	char *p;

	p = from;
	memset(to, 0, 4);
	for(i = 0; i < 4 && *p; i++){
		to[i] = strtoul(p, &p, 0);
		if(*p == '.')
			p++;
	}
	switch(CLASS(to)){
	case 0:	/* class A - 1 byte net */
	case 1:
		if(i == 3){
			to[3] = to[2];
			to[2] = to[1];
			to[1] = 0;
		} else if (i == 2){
			to[3] = to[1];
			to[1] = 0;
		}
		break;
	case 2:	/* class B - 2 byte net */
		if(i == 3){
			to[3] = to[2];
			to[2] = 0;
		}
		break;
	}
	return nhgetl(to);
}

void
oserror(void)
{
	int e, r, i;
	char buf[200];

	e = GetLastError();
	
	r = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), 0);

	if(r == 0)
		sprintf(buf, "windows error %d", e);


	for(i = strlen(buf)-1; i>=0 && buf[i] == '\n' || buf[i] == '\r'; i--)
		buf[i] = 0;

	strcpy(errbuf, buf);
}

extern int	main(int, char*[]);
static int	args(char *argv[], int n, char *p);

int PASCAL
WinMain(HANDLE hInst, HANDLE hPrev, LPSTR arg, int nshow)
{
	int argc, n;
	char *p, **argv;

	/* conservative guess at the number of args */
	for(argc=5,p=arg; *p; p++)
		if(*p == ' ' || *p == '\t')
			argc++;

	argv = malloc(argc*sizeof(char*));
	argc = args(argv+1, argc, arg);
	argc++;
	argv[0] = argv0;
	main(argc, argv);
	ExitThread(0);
	return 0;
}

/*
 * Break the command line into arguments
 * The rules for this are not documented but appear to be the following
 * according to the source for the microsoft C library.
 * Words are seperated by space or tab
 * Words containing a space or tab can be quoted using "
 * 2N backslashes + " ==> N backslashes and end quote
 * 2N+1 backslashes + " ==> N backslashes + literal "
 * N backslashes not followed by " ==> N backslashes
 */
static int
args(char *argv[], int n, char *p)
{
	char *p2;
	int i, j, quote, nbs;

	for(i=0; *p && i<n-1; i++) {
		while(*p == ' ' || *p == '\t')
			p++;
		quote = 0;
		argv[i] = p2 = p;
		for(;*p; p++) {
			if(!quote && (*p == ' ' || *p == '\t'))
				break;
			for(nbs=0; *p == '\\'; p++,nbs++)
				;
			if(*p == '"') {
				for(j=0; j<(nbs>>1); j++)
					*p2++ = '\\';
				if(nbs&1)
					*p2++ = *p;
				else
					quote = !quote;
			} else {
				for(j=0; j<nbs; j++)
					*p2++ = '\\';
				*p2++ = *p;
			}
		}
		/* move p up one to avoid pointing to null at end of p2 */
		if(*p)
			p++;
		*p2 = 0;	
	}
	argv[i] = 0;

	return i;
}

