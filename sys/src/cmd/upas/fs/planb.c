/*
 * Plan B (mail2fs) mail box format.
 *
 * BUG: this does not reconstruct the
 * raw text for attachments.  So imap and others
 * will be unable to access any attachment using upas/fs.
 * As an aid, we add the path to the message directory
 * to the message body, so the user could build the path
 * for any attachment and open it.
 */

#include "common.h"
#include <ctype.h>
#include <plumb.h>
#include <libsec.h>
#include "dat.h"

static int
readmessage(Message *m, char *msg)
{
	int fd, i, n;
	char *buf, *name, *p;
	char hdr[128], sdigest[SHA1dlen*2+1];
	Dir *d;

	buf = nil;
	d = nil;
	name = smprint("%s/raw", msg);
	if(name == nil)
		return -1;
	if(m->filename != nil)
		s_free(m->filename);
	m->filename = s_copy(name);
	fd = open(name, OREAD);
	if(fd < 0)
		goto Fail;
	n = read(fd, hdr, sizeof(hdr)-1);
	if(n <= 0)
		goto Fail;
	hdr[n] = 0;
	close(fd);
	fd = -1;
	p = strchr(hdr, '\n');
	if(p != nil)
		*++p = 0;
	if(strncmp(hdr, "From ", 5) != 0)
		goto Fail;
	free(name);
	name = smprint("%s/text", msg);
	if(name == nil)
		goto Fail;
	fd = open(name, OREAD);
	if(fd < 0)
		goto Fail;
	d = dirfstat(fd);
	if(d == nil)
		goto Fail;
	buf = malloc(strlen(hdr) + d->length + strlen(msg) + 10); /* few extra chars */
	if(buf == nil)
		goto Fail;
	strcpy(buf, hdr);
	p = buf+strlen(hdr);
	n = readn(fd, p, d->length);
	if(n < 0)
		goto Fail;
	sprint(p+n, "\n[%s]\n", msg);
	n += 2 + strlen(msg) + 2;
	close(fd);
	free(name);
	free(d);
	free(m->start);
	m->start = buf;
	m->lim = m->end = p+n;
	if(*(m->end-1) == '\n')
		m->end--;
	*m->end = 0;
	m->bend = m->rbend = m->end;
	sha1((uchar*)m->start, m->end - m->start, m->digest, nil);
	for(i = 0; i < SHA1dlen; i++)
		sprint(sdigest+2*i, "%2.2ux", m->digest[i]);
	m->sdigest = s_copy(sdigest);
	return 0;
Fail:
	if(fd >= 0)
		close(fd);
	free(name);
	free(buf);
	free(d);
	return -1;
}

/*
 * Deleted messages are kept as spam instead.
 */
static void
archive(Message *m)
{
	char *dir, *p, *nname;
	Dir d;

	dir = strdup(s_to_c(m->filename));
	nname = nil;
	if(dir == nil)
		return;
	p = strrchr(dir, '/');
	if(p == nil)
		goto Fail;
	*p = 0;
	p = strrchr(dir, '/');
	if(p == nil)
		goto Fail;
	p++;
	if(*p < '0' || *p > '9')
		goto Fail;
	nname = smprint("s.%s", p);
	if(nname == nil)
		goto Fail;
	nulldir(&d);
	d.name = nname;
	dirwstat(dir, &d);
Fail:
	free(dir);
	free(nname);
}

int
purgembox(Mailbox *mb, int virtual)
{
	Message *m, *next;
	int newdels;

	/* forget about what's no longer in the mailbox */
	newdels = 0;
	for(m = mb->root->part; m != nil; m = next){
		next = m->next;
		if(m->deleted > 0 && m->refs == 0){
			if(m->inmbox){
				newdels++;
				/*
				 * virtual folders are virtual,
				 * we do not archive
				 */
				if(virtual == 0)
					archive(m);
			}
			delmessage(mb, m);
		}
	}
	return newdels;
}

static int
mustshow(char* name)
{
	if(isdigit(name[0]))
		return 1;
	if(0 && name[0] == 'a' && name[1] == '.')
		return 1;
	if(0 && name[0] == 's' && name[1] == '.')
		return 1;
	return 0;
}

static int
readpbmessage(Mailbox *mb, char *msg, int doplumb)
{
	Message *m, **l;
	char *x;

	m = newmessage(mb->root);
	m->mallocd = 1;
	m->inmbox = 1;
	if(readmessage(m, msg) < 0){
		delmessage(mb, m);
		mb->root->subname--;
		return -1;
	}
	for(l = &mb->root->part; *l != nil; l = &(*l)->next)
		if(strcmp(s_to_c((*l)->filename), s_to_c(m->filename)) == 0 &&
		    *l != m){
			if((*l)->deleted < 0)
				(*l)->deleted = 0;
			delmessage(mb, m);
			mb->root->subname--;
			return -1;
		}
	x = strchr(m->start, '\n');
	if(x == nil)
		m->header = m->end;
	else
		m->header = x + 1;
	m->mheader = m->mhend = m->header;
	parseunix(m);
	parse(m, 0, mb, 0);
	logmsg("new", m);

	/* chain in */
	*l = m;
	if(doplumb)
		mailplumb(mb, m, 0);
	return 0;
}

static int
dcmp(Dir *a, Dir *b)
{
	char *an, *bn;

	an = a->name;
	bn = b->name;
	if(an[0] != 0 && an[1] == '.')
		an += 2;
	if(bn[0] != 0 && bn[1] == '.')
		bn += 2;
	return strcmp(an, bn);
}

static void
readpbmbox(Mailbox *mb, int doplumb)
{
	int fd, i, j, nd, nmd;
	char *month, *msg;
	Dir *d, *md;

	fd = open(mb->path, OREAD);
	if(fd < 0){
		fprint(2, "%s: %s: %r\n", argv0, mb->path);
		return;
	}
	nd = dirreadall(fd, &d);
	close(fd);
	if(nd > 0)
		qsort(d, nd, sizeof d[0], (int (*)(void*, void*))dcmp);
	for(i = 0; i < nd; i++){
		month = smprint("%s/%s", mb->path, d[i].name);
		if(month == nil)
			break;
		fd = open(month, OREAD);
		if(fd < 0){
			fprint(2, "%s: %s: %r\n", argv0, month);
			free(month);
			continue;
		}
		md = dirfstat(fd);
		if(md != nil && (md->qid.type & QTDIR) != 0){
			free(md);
			md = nil;
			nmd = dirreadall(fd, &md);
			for(j = 0; j < nmd; j++)
				if(mustshow(md[j].name)){
					msg = smprint("%s/%s", month, md[j].name);
					readpbmessage(mb, msg, doplumb);
					free(msg);
				}
		}
		close(fd);
		free(month);
		free(md);
		md = nil;
	}
	free(d);
}

static void
readpbvmbox(Mailbox *mb, int doplumb)
{
	int fd, nr;
	long sz;
	char *data, *ln, *p, *nln, *msg;
	Dir *d;

	fd = open(mb->path, OREAD);
	if(fd < 0){
		fprint(2, "%s: %s: %r\n", argv0, mb->path);
		return;
	}
	d = dirfstat(fd);
	if(d == nil){
		fprint(2, "%s: %s: %r\n", argv0, mb->path);
		close(fd);
		return;
	}
	sz = d->length;
	free(d);
	if(sz > 2 * 1024 * 1024){
		sz = 2 * 1024 * 1024;
		fprint(2, "%s: %s: bug: folder too big\n", argv0, mb->path);
	}
	data = malloc(sz+1);
	if(data == nil){
		close(fd);
		fprint(2, "%s: no memory\n", argv0);
		return;
	}
	nr = readn(fd, data, sz);
	close(fd);
	if(nr < 0){
		fprint(2, "%s: %s: %r\n", argv0, mb->path);
		free(data);
		return;
	}
	data[nr] = 0;

	for(ln = data; *ln != 0; ln = nln){
		nln = strchr(ln, '\n');
		if(nln != nil)
			*nln++ = 0;
		else
			nln = ln + strlen(ln);
		p = strchr(ln , ' ');
		if(p != nil)
			*p = 0;
		p = strchr(ln, '\t');
		if(p != nil)
			*p = 0;
		p = strstr(ln, "/text");
		if(p != nil)
			*p = 0;
		msg = smprint("/mail/box/%s/msgs/%s", user, ln);
		if(msg == nil){
			fprint(2, "%s: no memory\n", argv0);
			continue;
		}
		readpbmessage(mb, msg, doplumb);
		free(msg);
	}
	free(data);
}

static char*
readmbox(Mailbox *mb, int doplumb, int virt)
{
	int fd;
	Dir *d;
	Message *m;
	static char err[Errlen];

	if(debug)
		fprint(2, "read mbox %s\n", mb->path);
	fd = open(mb->path, OREAD);
	if(fd < 0){
		errstr(err, sizeof(err));
		return err;
	}

	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		errstr(err, sizeof(err));
		return err;
	}
	if(mb->d != nil){
		if(d->qid.path == mb->d->qid.path &&
		   d->qid.vers == mb->d->qid.vers){
			close(fd);
			free(d);
			return nil;
		}
		free(mb->d);
	}
	close(fd);
	mb->d = d;
	mb->vers++;
	henter(PATH(0, Qtop), mb->name,
		(Qid){PATH(mb->id, Qmbox), mb->vers, QTDIR}, nil, mb);
	snprint(err, sizeof err, "reading '%s'", mb->path);
	logmsg(err, nil);

	for(m = mb->root->part; m != nil; m = m->next)
		if(m->deleted == 0)
			m->deleted = -1;
	if(virt == 0)
		readpbmbox(mb, doplumb);
	else
		readpbvmbox(mb, doplumb);

	/*
	 * messages removed from the mbox; flag them to go.
	 */
	for(m = mb->root->part; m != nil; m = m->next)
		if(m->deleted < 0 && doplumb){
			m->inmbox = 0;
			m->deleted = 1;
			mailplumb(mb, m, 1);
		}
	logmsg("mbox read", nil);
	return nil;
}

static char*
mbsync(Mailbox *mb, int doplumb)
{
	char *rv;

	rv = readmbox(mb, doplumb, 0);
	purgembox(mb, 0);
	return rv;
}

static char*
mbvsync(Mailbox *mb, int doplumb)
{
	char *rv;

	rv = readmbox(mb, doplumb, 1);
	purgembox(mb, 1);
	return rv;
}

char*
planbmbox(Mailbox *mb, char *path)
{
	char *list;
	static char err[64];

	if(access(path, AEXIST) < 0)
		return Enotme;
	list = smprint("%s/list", path);
	if(access(list, AEXIST) < 0){
		free(list);
		return Enotme;
	}
	free(list);
	mb->sync = mbsync;
	if(debug)
		fprint(2, "planb mbox %s\n", path);
	return nil;
}

char*
planbvmbox(Mailbox *mb, char *path)
{
	int fd, nr, i;
	char buf[64];
	static char err[64];

	fd = open(path, OREAD);
	if(fd < 0)
		return Enotme;
	nr = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(nr < 7)
		return Enotme;
	buf[nr] = 0;
	for(i = 0; i < 6; i++)
		if(buf[i] < '0' || buf[i] > '9')
			return Enotme;
	if(buf[6] != '/')
		return Enotme;
	mb->sync = mbvsync;
	if(debug)
		fprint(2, "planb virtual mbox %s\n", path);
	return nil;
}
