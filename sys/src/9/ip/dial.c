#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"kernel.h"

typedef struct DS DS;
static int	call(char*, char*, DS*);
static void	_dial_string_parse(char*, DS*);

enum
{
	Maxstring=	128,
	Maxpath=	256,
};

struct DS
{
	char	buf[Maxstring];			/* dist string */
	char	*netdir;
	char	*proto;
	char	*rem;
	char	*local;				/* other args */
	char	*dir;
	int	*cfdp;
};

/*
 *  the dialstring is of the form '[/net/]proto!dest'
 */
int
kdial(char *dest, char *local, char *dir, int *cfdp)
{
	DS ds;
	char clone[Maxpath];

	ds.local = local;
	ds.dir = dir;
	ds.cfdp = cfdp;

	_dial_string_parse(dest, &ds);
	if(ds.netdir == 0)
		ds.netdir = "/net";

	/* no connection server, don't translate */
	snprint(clone, sizeof(clone), "%s/%s/clone", ds.netdir, ds.proto);
	return call(clone, ds.rem, &ds);
}

static int
call(char *clone, char *dest, DS *ds)
{
	int fd, cfd, n;
	char name[3*NAMELEN+5], data[3*NAMELEN+10], *p;

	cfd = kopen(clone, ORDWR);
	if(cfd < 0){
		kwerrstr("%s: %r", clone);
		return -1;
	}

	/* get directory name */
	n = kread(cfd, name, sizeof(name)-1);
	if(n < 0){
		kclose(cfd);
		return -1;
	}
	name[n] = 0;
	for(p = name; *p == ' '; p++)
		;
	sprint(name, "%lud", strtoul(p, 0, 0));
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
	if(kwrite(cfd, name, strlen(name)) < 0){
		kwerrstr("%s failed: %r", name);
		kclose(cfd);
		return -1;
	}

	/* open data connection */
	fd = kopen(data, ORDWR);
	if(fd < 0){
		kwerrstr("can't open %s: %r", data);
		kclose(cfd);
		return -1;
	}
	if(ds->cfdp)
		*ds->cfdp = cfd;
	else
		kclose(cfd);
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
