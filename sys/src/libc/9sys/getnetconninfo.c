#include <u.h>
#include <libc.h>

static char *unknown = "???";

static void
getendpoint(char *dir, char *file, char **sysp, char **servp)
{
	int fd, n;
	char buf[128];
	char *sys, *serv;

	sys = serv = 0;

	snprint(buf, sizeof buf, "%s/%s", dir, file);
	fd = open(buf, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n>0){
			buf[n-1] = 0;
			serv = strchr(buf, '!');
			if(serv){
				*serv++ = 0;
				serv = strdup(serv);
			}
			sys = strdup(buf);
		}
		close(fd);
	}
	if(serv == 0)
		serv = unknown;
	if(sys == 0)
		sys = unknown;
	*servp = serv;
	*sysp = sys;
}

NetConnInfo*
getnetconninfo(char *dir, int fd)
{
	NetConnInfo *nci;
	char *cp;
	Dir *d;
	char spec[10];
	char path[128];
	char netname[128], *p;

	/* get a directory address via fd */
	if(dir == nil || *dir == 0){
		if(fd2path(fd, path, sizeof(path)) < 0)
			return nil;
		cp = strrchr(path, '/');
		if(cp == nil)
			return nil;
		*cp = 0;
		dir = path;
	}

	nci = mallocz(sizeof *nci, 1);
	if(nci == nil)
		return nil;

	/* copy connection directory */
	nci->dir = strdup(dir);
	if(nci->dir == nil)
		goto err;

	/* get netroot */
	nci->root = strdup(dir);
	if(nci->root == nil)
		goto err;
	cp = strchr(nci->root+1, '/');
	if(cp == nil)
		goto err;
	*cp = 0;

	/* figure out bind spec */
	d = dirstat(nci->dir);
	if(d != nil){
		sprint(spec, "#%C%d", d->type, d->dev);
		nci->spec = strdup(spec);
	}
	if(nci->spec == nil)
		nci->spec = unknown;
	free(d);

	/* get the two end points */
	getendpoint(nci->dir, "local", &nci->lsys, &nci->lserv);
	if(nci->lsys == nil || nci->lserv == nil)
		goto err;
	getendpoint(nci->dir, "remote", &nci->rsys, &nci->rserv);
	if(nci->rsys == nil || nci->rserv == nil)
		goto err;

	strecpy(netname, netname+sizeof netname, nci->dir);
	if((p = strrchr(netname, '/')) != nil)
		*p = 0;
	if(strncmp(netname, "/net/", 5) == 0)
		memmove(netname, netname+5, strlen(netname+5)+1);
	nci->laddr = smprint("%s!%s!%s", netname, nci->lsys, nci->lserv);
	nci->raddr = smprint("%s!%s!%s", netname, nci->rsys, nci->rserv);
	if(nci->laddr == nil || nci->raddr == nil)
		goto err;
	return nci;
err:
	freenetconninfo(nci);
	return nil;
}

static void
xfree(char *x)
{
	if(x == nil || x == unknown)
		return;
	free(x);
}

void
freenetconninfo(NetConnInfo *nci)
{
	if(nci == nil)
		return;
	xfree(nci->root);
	xfree(nci->dir);
	xfree(nci->spec);
	xfree(nci->lsys);
	xfree(nci->lserv);
	xfree(nci->rsys);
	xfree(nci->rserv);
	xfree(nci->laddr);
	xfree(nci->raddr);
	free(nci);
}
