#include "lib9.h"

typedef struct DS DS;

static int	call(char*, char*, DS*);
static int	csdial(DS*);
static void	_dial_string_parse(char*, DS*);

enum{
	Maxstring=	128,
	Maxpath=	256
};

struct DS {
	/* dist string */
	char	buf[Maxstring];
	char	*netdir;
	char	*proto;
	char	*rem;

	/* other args */
	char	*local;
	char	*dir;
	int	*cfdp;
};


/*
 *  the dialstring is of the form '[/net/]proto!dest'
 */
int
dial(char *dest, char *local, char *dir, int *cfdp)
{
	DS ds;
	int rv;
	char err[ERRLEN], alterr[ERRLEN];

	ds.local = local;
	ds.dir = dir;
	ds.cfdp = cfdp;

	_dial_string_parse(dest, &ds);


	if(ds.netdir)
		return csdial(&ds);

	ds.netdir = "/net";
	rv = csdial(&ds);
	if(rv >= 0)
		return rv;
	errstr(err);
/*
	if(strstr(err, "translate") == 0 && strstr(err, "unreachable") == 0){
		werrstr(err);
		return rv;
	}
*/
	ds.netdir = "/net.alt";
	rv = csdial(&ds);
	if(rv >= 0)
		return rv;

	errstr(alterr);
	if(strstr(alterr, "translate") || strstr(alterr, "file does not exist"))
		werrstr(err);
	else
		werrstr(alterr);
	return rv;
}

static int
csdial(DS *ds)
{
	int n, fd, rv;
	char *p, buf[Maxstring], clone[Maxpath], err[ERRLEN], besterr[ERRLEN];

	/*
	 *  open connection server
	 */
	snprint(buf, sizeof(buf), "%s/cs", ds->netdir);
	fd = open(buf, ORDWR);
	if(fd < 0){
		/* no connection server, don't translate */
		snprint(clone, sizeof(clone), "%s/%s/clone", ds->netdir, ds->proto);
		return call(clone, ds->rem, ds);
	}

	/*
	 *  ask connection server to translate
	 */
	sprint(buf, "%s!%s", ds->proto, ds->rem);
	if(write(fd, buf, strlen(buf)) < 0){
		werrstr("%s: %r", buf);
		close(fd);
		return -1;
	}

	/*
	 *  loop through each address from the connection server till
	 *  we get one that works.
	 */
	*besterr = 0;
	rv = -1;
	seek(fd, 0, 0);
	while((n = read(fd, buf, sizeof(buf) - 1)) > 0){
		buf[n] = 0;
		p = strchr(buf, ' ');
		if(p == 0)
			continue;
		*p++ = 0;
		rv = call(buf, p, ds);
		if(rv >= 0)
			break;
		errstr(err);
		if(strstr(err, "file does not exist") == 0)
			memmove(besterr, err, ERRLEN);
	}
	close(fd);

	if(rv < 0 && *besterr)
		werrstr(besterr);
	else
		werrstr(err);
	return rv;
}

static int
call(char *clone, char *dest, DS *ds)
{
	int fd, cfd, n;
	char name[3*NAMELEN+5], data[3*NAMELEN+10], *p;

	cfd = open(clone, ORDWR);
	if(cfd < 0){
		werrstr("%s: %r", clone);
		return -1;
	}

	/* get directory name */
	n = read(cfd, name, sizeof(name)-1);
	if(n < 0){
		close(cfd);
		return -1;
	}
	name[n] = 0;
	for(p = name; *p == ' '; p++)
		;
	sprint(name, "%d", strtoul(p, 0, 0));
	p = strrchr(clone, '/');
	*p = 0;
	if(ds->dir)
		snprint(ds->dir, 2*NAMELEN, "%s/%s", clone, name);
	snprint(data, sizeof(data), "%s/%s/data", clone, name);

	/* connect */
	if(ds->local)
		snprint(name, sizeof(name), "connect %s %s", dest, ds->local);
	else
		snprint(name, sizeof(name), "connect %s", dest);
	if(write(cfd, name, strlen(name)) < 0){
		werrstr("%s failed: %r", name);
		close(cfd);
		return -1;
	}

	/* open data connection */
	fd = open(data, ORDWR);
	if(fd < 0){
		werrstr("can't open %s: %r", data);
		close(cfd);
		return -1;
	}
	if(ds->cfdp)
		*ds->cfdp = cfd;
	else
		close(cfd);
	return fd;
}

/*
 *  parse a dial string
 */
static void
_dial_string_parse(char *str, DS *ds)
{
	char *p, *p2;

	strncpy(ds->buf, str, Maxstring);
	ds->buf[Maxstring-1] = 0;

	p = strchr(ds->buf, '!');
	if(p == 0) {
		ds->netdir = 0;
		ds->proto = "net";
		ds->rem = ds->buf;
	} else {
		if(*ds->buf != '/'){
			ds->netdir = 0;
			ds->proto = ds->buf;
		} else {
			for(p2 = p; *p2 != '/'; p2--)
				;
			*p2++ = 0;
			ds->netdir = ds->buf;
			ds->proto = p2;
		}
		*p = 0;
		ds->rem = p + 1;
	}
}
