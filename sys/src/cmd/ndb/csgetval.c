#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ndbhf.h>

/*
 *  search for a tuple that has the given 'attr=val' and also 'rattr=x'.
 *  copy 'x' into 'buf' and return the whole tuple.
 *
 *  return 0 if not found.
 */
Ndbtuple*
csgetval(char *attr, char *val, char *rattr, char *buf)
{
	Ndbtuple *t, *first, *last;
	int n, fd, linefound;
	char line[1024];

	buf[0] = 0;
	fd = open("/net/cs", ORDWR);
	if(fd < 0)
		return 0;
	fprint(fd, "!%s=%s", attr, val);
	seek(fd, 0, 0);

	first = last = 0;
	linefound = 0;
	for(;;){
		n = read(fd, line, sizeof(line)-2);
		if(n <= 0)
			break;
		line[n] = '\n';
		line[n+1] = 0;

		t = _ndbparseline(line);
		if(t == 0)
			continue;
		if(first)
			last->entry = t;
		else
			first = t;
		last = t;

		while(last->entry)
			last = last->entry;

		for(; t; t = t->entry){
			if(buf[0] == 0 || linefound == 0)
				if(strcmp(rattr, t->attr) == 0)
					strcpy(buf, t->val);
			if(linefound == 0)
				if(strcmp(attr, t->attr) == 0)
					linefound = 1;
		}
	}
	close(fd);
	return first;
}
