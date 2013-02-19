#include <u.h>
#include <libc.h>

static
char*
skip(char *p)
{

	while(*p == ' ')
		p++;
	while(*p != ' ' && *p != 0)
		p++;
	return p;
}

/*
 *  after a fork with fd's copied, both fd's are pointing to
 *  the same Chan structure.  Since the offset is kept in the Chan
 *  structure, the seek's and read's in the two processes can be
 *  are competing moving the offset around.  Hence the unusual loop
 *  in the middle of this routine.
 */
long
times(long *t)
{
	char b[200], *p;
	static int f = -1;
	int i, retries;
	ulong r;

	memset(b, 0, sizeof(b));
	for(retries = 0; retries < 100; retries++){
		if(f < 0)
			f = open("/dev/cputime", OREAD|OCEXEC);
		if(f < 0)
			break;
		if(seek(f, 0, 0) < 0 || (i = read(f, b, sizeof(b))) < 0){
			close(f);
			f = -1;
		} else {
			if(i != 0)
				break;
		}
	}
	p = b;
	if(t)
		t[0] = atol(p);
	p = skip(p);
	if(t)
		t[1] = atol(p);
	p = skip(p);
	r = atol(p);
	if(t){
		p = skip(p);
		t[2] = atol(p);
		p = skip(p);
		t[3] = atol(p);
	}
	return r;
}
