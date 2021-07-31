#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../ip/ip.h"

typedef struct DS DS;
static Chan*	call(char*, char*, DS*);
static void	_dial_string_parse(char*, DS*);

enum
{
	Maxstring=	128,
};

struct DS
{
	char	buf[Maxstring];			/* dist string */
	char	*netdir;
	char	*proto;
	char	*rem;
	char	*local;				/* other args */
	char	*dir;
	Chan	**ctlp;
};

/*
 *  the dialstring is of the form '[/net/]proto!dest'
 */
Chan*
chandial(char *dest, char *local, char *dir, Chan **ctlp)
{
	DS ds;
	char clone[Maxpath];

	ds.local = local;
	ds.dir = dir;
	ds.ctlp = ctlp;

	_dial_string_parse(dest, &ds);
	if(ds.netdir == 0)
		ds.netdir = "/net";

	/* no connection server, don't translate */
	snprint(clone, sizeof(clone), "%s/%s/clone", ds.netdir, ds.proto);
	return call(clone, ds.rem, &ds);
}

static Chan*
call(char *clone, char *dest, DS *ds)
{
	int n;
	Chan *dchan, *cchan;
	char name[Maxpath], data[Maxpath], *p;

	cchan = namec(clone, Aopen, ORDWR, 0);

	/* get directory name */
	if(waserror()){
		cclose(cchan);
		nexterror();
	}
	n = devtab[cchan->type]->read(cchan, name, sizeof(name)-1, 0);
	name[n] = 0;
	for(p = name; *p == ' '; p++)
		;
	sprint(name, "%lud", strtoul(p, 0, 0));
	p = strrchr(clone, '/');
	*p = 0;
	if(ds->dir)
		snprint(ds->dir, Maxpath, "%s/%s", clone, name);
	snprint(data, sizeof(data), "%s/%s/data", clone, name);

	/* connect */
	if(ds->local)
		snprint(name, sizeof(name), "connect %s %s", dest, ds->local);
	else
		snprint(name, sizeof(name), "connect %s", dest);
	devtab[cchan->type]->write(cchan, name, strlen(name), 0);

	/* open data connection */
	dchan = namec(data, Aopen, ORDWR, 0);
	if(ds->ctlp)
		*ds->ctlp = cchan;
	else
		cclose(cchan);
	poperror();
	return dchan;

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
		if(*ds->buf != '/' && *ds->buf != '#'){
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
