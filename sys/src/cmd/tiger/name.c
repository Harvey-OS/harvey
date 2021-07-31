#include	<u.h>
#include	<libc.h>
#include	<bio.h>

static	tigerlu(char*, char*, char*);

char*
tiger(char *name)
{
	static char buf1[200];
	char *p, buf[100];
	int i, c, n;

	n = strlen(name);
	if(n >= sizeof(buf)-10)
		name = "too long";

	if(n == 5) {
		for(i=0; i<5; i++) {
			c = name[i];
			if(c < '0' || c > '9')
				goto brk;
		}
		sprint(buf1, "/lib/tiger/%.2s/%.5s.h", name, name);
		return buf1;
	brk:;
	}

	p = strchr(name, ',');
	if(p) {
		memcpy(buf1, name, p-name);
		buf1[p-name] = 0;
		p++;
		while(*p == ' ')
			p++;
		if(tigerlu(buf, buf1, p))
			goto bad;
	} else
		if(tigerlu(buf, name, 0))
			goto bad;

	sprint(buf1, "/lib/tiger/%.2s/%.5s.h", buf, buf);
	return buf1;
	
bad:
	strcpy(buf1, name);
	return buf1;
}

static int
tigerlu(char *buf, char *county, char *state)
{
	Biobuf bio;
	char *p, *q, *pi;
	int f, c, d, i, flag;

	f = open("/lib/tiger/codes1", OREAD);
	if(f < 0)
		return 1;
	Binit(&bio, f, OREAD);

	flag = 0;
	for(;;) {
		p = Brdline(&bio, '\n');
		if(p == 0)
			break;

		pi = p;
		for(i=0; i<5; i++) {
			c = *p++;
			if(c < '0' || c > '9')
				goto brk;
		}
		for(;;p++) {
			c = *p;
			if(c != ' ' && c != '\t')
				break;
		}
		for(q=county;;) {
			c = *p++;
			if(c == '\n')
				goto brk;
			if(c >= 'A' && c <= 'Z')
				c += 'a'-'A';
			if(c == ',')
				c = 0;

			d = *q++;
			if(d >= 'A' && d <= 'Z')
				d += 'a'-'A';

			if(c != d)
				goto brk;
			if(c == 0)
				break;
		}
		if(state == 0) {
			memmove(buf, pi, 5);
			flag++;
			goto brk;
		}

		for(q=state;;) {
			c = *p++;
			if(c >= 'A' && c <= 'Z')
				c += 'a'-'A';
			if(c == '\n')
				c = 0;

			d = *q++;
			if(d >= 'A' && d <= 'Z')
				d += 'a'-'A';

			if(c != d)
				goto brk;
			if(c == 0)
				break;
		}
		memmove(buf, pi, 5);
		flag++;
	brk:;
	}

	Bclose(&bio);
	close(f);
	if(flag == 1)
		return 0;
	return 1;
}
