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
csgetvalue(char *netroot, char *attr, char *val, char *rattr, char *buf, int len)
{
	Ndbtuple *t, *first, *last;
	int n, linefound;
	char line[1024];
	int fd;

	buf[0] = 0;

	if(netroot)
		snprint(line, sizeof(line), "%s/cs", netroot);
	else
		strcpy(line, "/net/cs");
	fd = open(line, ORDWR);
	if(fd < 0)
		return 0;
	seek(fd, 0, 0);
	snprint(line, sizeof(line), "!%s=%s %s=*", attr, val, rattr);
	if(write(fd, line, strlen(line)) < 0){
		close(fd);
		return 0;
	}
	seek(fd, 0, 0);

	werrstr("");
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
				if(strcmp(rattr, t->attr) == 0){
					strncpy(buf, t->val, len);
					buf[len-1] = 0;
					if(strlen(t->val) >= len)
						werrstr("return value truncated");
				}
			if(linefound == 0)
				if(strcmp(attr, t->attr) == 0)
					linefound = 1;
		}
	}
	close(fd);

	setmalloctag(first, getcallerpc(&netroot));
	return first;
}

Ndbtuple*
csgetval(char *netroot, char *attr, char *val, char *rattr, char *buf)
{
	return csgetvalue(netroot, attr, val, rattr, buf, Ndbvlen);
}
