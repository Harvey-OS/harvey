#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>
#include "imap4d.h"

void
boxVerify(Box *box)
{
	Msg *m;
	ulong seq, uid, recent;

	if(box == nil)
		return;
	recent = 0;
	seq = 0;
	uid = 0;
	for(m = box->msgs; m != nil; m = m->next){
		if(m->seq == 0)
			fprint(2, "m->seq == 0: m->seq=%lud\n", m->seq);
		else if(m->seq <= seq)
			fprint(2, "m->seq=%lud out of order: last=%lud\n", m->seq, seq);
		seq = m->seq;

		if(m->uid == 0)
			fprint(2, "m->uid == 0: m->seq=%lud\n", m->seq);
		else if(m->uid <= uid)
			fprint(2, "m->uid=%lud out of order: last=%lud\n", m->uid, uid);
		uid = m->uid;

		if(m->flags & MRecent)
			recent++;
	}
	if(seq != box->max)
		fprint(2, "max=%lud, should be %lud\n", box->max, seq);
	if(uid >= box->uidnext)
		fprint(2, "uidnext=%lud, maxuid=%lud\n", box->uidnext, uid);
	if(recent != box->recent)
		fprint(2, "recent=%lud, should be %lud\n", box->recent, recent);
}

void
openfiles(void)
{
	Dir d;
	int i;

	for(i = 0; i < 20; i++)
		if(dirfstat(i, &d) >= 0)
			fprint(2, "fd[%d]='%s' type=%c dev=%d user='%s group='%s'\n", i, d.name, d.type, d.dev, d.uid, d.gid);
}

void
ls(char *file)
{
	Dir d[NDirs];
	int fd, i, nd;

	fd = open(file, OREAD);
	if(fd < 0)
		return;

	/*
	 * read box to find all messages
	 * each one has a directory, and is in numerical order
	 */
	if(dirfstat(fd, d) < 0){
		close(fd);
		return;
	}
	if(!(d->mode & CHDIR)){
		fprint(2, "file %s\n", file);
		close(fd);
		return;
	}
	while((nd = dirread(fd, d, sizeof(Dir) * NDirs)) >= sizeof(Dir)){
		nd /= sizeof(Dir);
		for(i = 0; i < nd; i++){
			fprint(2, "%s/%s %c\n", file, d[i].name, "-d"[(d[i].mode & CHDIR) == CHDIR]);
		}
	}
	close(fd);
}
