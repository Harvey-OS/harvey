/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include <fcall.h>
#include "object.h"
#include "parse.h"
#include "print.h"
#include "catset.h"
#include "../debug.h"

// #include <pool.h>

char* user, *mapname, *svrname;
int p[2];
int mfd[2];
int debug = 0; // DBGSERVER|DBGSTATE|DBGPICKLE|DBGPLAY;
Biobuf* f;
char* file;

Object* root;

Object** otab; // object table
int notab;     // no of entries used
int sotab;     // no of entries mallocated (invariant sotab >= notab)
int hotab;     // no of holes in otab;

char usage[] = "Usage: %s [-f] [-l] [mapfile]\n";

char* startdir;

Object** catobjects; /* for quickly finding category objects */
int ncat = 0;

void
post(char* name, char* envname, int srvfd)
{
	int fd;
	char buf[32];

	fd = create(name, OWRITE, 0666);
	if(fd < 0)
		return;
	sprint(buf, "%d", srvfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		sysfatal("srv write: %r");
	close(fd);
	putenv(envname, name);
}

int
robusthandler(void*, char* s)
{
	if(debug)
		fprint(2, "inthandler: %s\n", s);
	return (s && (strstr(s, "interrupted") || strstr(s, "hangup")));
}

int32_t
robustread(int fd, void* buf, int32_t sz)
{
	int32_t r;
	char err[32];

	do {
		r = read(fd, buf, sz);
		if(r < 0)
			rerrstr(err, sizeof(err));
	} while(r < 0 && robusthandler(nil, err));
	return r;
}

void
delobject(Object* o)
{
	/* Free an object and all its descendants */
	Object* oo;
	int i;

	for(i = 0; i < o->nchildren; i++) {
		oo = o->children[i];
		if(oo->parent == o)
			delobject(oo);
	}
	freeobject(o, "r");
}

void
threadmain(int argc, char* argv[])
{
	char* q;
	char* srvname;
	char* mntpt;
	int list;

	mntpt = "/mnt";
	user = strdup(getuser());
	srvname = nil;
	list = 0;

	// mainmem->flags |= POOL_NOREUSE;

	ARGBEGIN
	{
	case 'l':
		list = 1;
		break;
	case 'm':
		mntpt = ARGF();
		break;
	case 'd':
		debug = strtoul(ARGF(), nil, 0);
		break;
	case 's':
		srvname = ARGF();
		break;
	case 'f':
		fflag = 1;
		break;
	default:
		fprint(2, usage, argv0);
		exits("usage");
	}
	ARGEND

	switch(argc) {
	default:
		fprint(2, usage, argv0);
		exits("usage");
	case 0:
		mapname = DEFAULTMAP;
		break;
	case 1:
		mapname = argv[0];
		break;
	}

	quotefmtinstall();

	if((f = Bopen(mapname, OREAD)) == nil)
		sysfatal("%s: %r", mapname);
	free(file);
	file = strdup(mapname);
	free(startdir);
	startdir = strdup(mapname);
	if((q = strrchr(startdir, '/')))
		*q = '\0';
	else
		startdir[0] = '\0';
	inittokenlist();
	getobject(Root, nil);
	Bterm(f);
	f = nil;
	root->parent = root;

	if(list) {
		listfiles(root);
		threadexits(nil);
	}

	if(pipe(p) < 0)
		sysfatal("pipe failed: %r");
	mfd[0] = p[0];
	mfd[1] = p[0];

	threadnotify(robusthandler, 1);
	user = strdup(getuser());

	if(debug)
		fmtinstall('F', fcallfmt);

	procrfork(io, nil, STACKSIZE, RFFDG); // RFNOTEG?

	close(p[0]); /* don't deadlock if child fails */

	if(srvname) {
		srvname = smprint("/srv/jukefs.%s", srvname);
		remove(srvname);
		post(srvname, "jukefs", p[1]);
	}
	if(mount(p[1], -1, mntpt, MBEFORE, "") < 0)
		sysfatal("mount failed: %r");
	threadexits(nil);
}

void
reread(void)
{
	int i;
	extern int catnr;
	char* q;

	assert(f == nil);
	if((f = Bopen(mapname, OREAD)) == nil)
		fprint(2, "reread: %s: %r\n", mapname);
	freetree(root);
	root = nil;
	for(i = 0; i < ntoken; i++) {
		free(tokenlist[i].name);
		catsetfree(&tokenlist[i].categories);
	}
	catnr = 0;
	free(tokenlist);
	free(catobjects);
	catobjects = nil;
	ncat = 0;
	tokenlist = nil;
	ntoken = Ntoken;
	inittokenlist();
	free(file);
	file = strdup(mapname);
	free(startdir);
	startdir = strdup(mapname);
	if((q = strrchr(startdir, '/')))
		*q = '\0';
	else
		startdir[0] = '\0';
	getobject(Root, nil);
	root->parent = root;
	Bterm(f);
	f = nil;
}
