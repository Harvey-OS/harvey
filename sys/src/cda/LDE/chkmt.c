#include <u.h>
#include <libc.h>
#include <stdio.h>

void doexit(int);
void rdata(void);

int fd1, fd2;
main(int argc, char *argv[])
{
	char buf[100];
	int i,j;
 	if(argc != 2) {
		fprintf(stderr, "Too many arguments -- ignored\n");
		doexit(1);
	}
	sprint(buf, "%s.old", argv[1]);
	fd1 = open(buf, 0);
	if(fd1 < 0) {
		fprintf(stderr, "cannot open %s\n", buf);
		doexit(1);
	}
	Finit(fd1, (Fbuffer*)0);
	sprint(buf, "%s.new", argv[1]);
	fd2 = open(buf, 0);
	if(fd2 < 0) {
		fprintf(stderr, "cannot open %s\n", buf);
		doexit(1);
	}
	Finit(fd2, (Fbuffer*)0);
	rdata();
}

void
doexit(int s)
{
	exits(s ? "error" : 0);
}

int cnt[32];

int
clrcnt(void)
{
int i, flag;
	for(i = 0, flag = 0; i < 32; ++i) {
		if(cnt[i] != 0) ++flag;
		cnt[i] = 0;
	}
	return(flag);
}


void
rdata(void)
{
char *p, *q;
char buf[200];
int v, n;
	if((p = Frdline(fd1)) == (char *) 0) doexit(1);
	if((q = Frdline(fd2)) == (char *) 0) doexit(1);
	n = clrcnt();
loop:
	if(p == (char *) 0) doexit(0);
	while(*p != '.' || p[1] != 'o') {
		if((p = Frdline(fd1)) == (char *) 0) doexit(0);
	}
	strcpy(buf, p);
loop1:
	if((p = Frdline(fd1)) == (char *) 0) goto chk2;
	if(*p == '.') goto chk2;
loop2:
	while((*p != 0) && (*p != ':')) ++p;
	if(*p == 0) goto loop1;
	v = 0;
	++p;
	while((*p >= '0') && (*p <= '9')) {
		v = v*10 + (*p - '0');
		++p;
	}
	for(n = 0; v; v >>= 1)
		if(v & 1) ++n;
	++cnt[n];
	goto loop2;
chk2:
	while(*q != '.' || q[1] != 'o') {
		if((q = Frdline(fd2)) == (char *) 0) doexit(1);
	}
	if(strcmp(buf, q) != 0) {
		fprintf(stderr, ".o lines don't match %s %s\n", buf, q);
	}
loop3:
	if((q = Frdline(fd2)) == (char *) 0) goto next;
	if(*q == '.') goto next;
loop4:
	while((*q != 0) && (*q != ':')) ++q;
	if(*q == 0) goto loop3;
	v = 0;
	++q;
	while((*q >= '0') && (*q <= '9')) {
		v = v*10 + (*q - '0');
		++q;
	}
	for(n = 0; v; v >>= 1)
		if(v & 1) ++n;
	--cnt[n];
	goto loop4;
next:
	if(clrcnt()) {
		fprintf(stderr, "mismatch for %s\n", buf);
	}
	goto loop;
}	
			
void
expect(char *s)
{

	fprintf(stderr, "%s expected\n", s);
	doexit(1);
}

