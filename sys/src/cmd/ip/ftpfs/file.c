#include <u.h>
#include <libc.h>
#include <String.h>
#include "ftpfs.h"

enum
{
	Chunk=		1024,		/* chunk size for buffered data */
	Nfile=		128,		/* maximum number of cached files */
};

/* a file (with cached data) */
struct File
{
	char	*mem;		/* part of file cached in memory */
	ulong	len;		/* length of cached data */
	long	off;		/* current offset into tpath */
	short	fd;		/* fd to cache file */
	char	inuse;
	char	dirty;
	ulong	atime;		/* time of last access */
	Node	*node;
	char 	*template;
};

static File	files[Nfile];
static ulong	now;
static int	ntmp;

/*
 *  lookup a file, create one if not found.  if there are no
 *  free files, free the last oldest clean one.
 */
static File*
fileget(Node *node)
{
	File *fp;
	File *oldest;

	fp = node->fp;
	if(fp)
		return fp;

	oldest = 0;
	for(fp = files; fp < &files[Nfile]; fp++){
		if(fp->inuse == 0)
			break;
		if(fp->dirty == 0 && (oldest == 0 || oldest->atime > fp->atime))
			oldest = fp;
	}

	if(fp == &files[Nfile]){
		uncache(oldest->node);
		fp = oldest;
	}
	node->fp = fp;
	fp->node = node;
	fp->atime = now++;
	fp->inuse = 1;
	fp->fd = -1;
	if(fp->mem){
		free(fp->mem);
		fp->mem = nil;
	}
	return fp;
}

/*
 *  free a cached file
 */
void
filefree(Node *node)
{
	File *fp;

	fp = node->fp;
	if(fp == 0)
		return;

	if(fp->fd > 0){
		ntmp--;
		close(fp->fd);
		remove(fp->template);
		free(fp->template);
		fp->template = 0;
	}
	fp->fd = -1;
	if(fp->mem){
		free(fp->mem);
		fp->mem = nil;
	}
	fp->len = 0;
	fp->inuse = 0;
	fp->dirty = 0;

	node->fp = 0;
}

/*
 *  satisfy read first from in memory chunk and then from temporary
 *  file.  It's up to the caller to make sure that the file is valid.
 */
int
fileread(Node *node, char *a, long off, int n)
{
	int sofar;
	int i;
	File *fp;

	fp = node->fp;
	if(fp == 0)
		fatal("fileread");

	if(off + n > fp->len)
		n = fp->len - off;

	for(sofar = 0; sofar < n; sofar += i, off += i, a += i){
		if(off >= fp->len)
			return sofar;
		if(off < Chunk){
			i = n;
			if(off + i > Chunk)
				i = Chunk - off;
			memmove(a, fp->mem + off, i);
			continue;
		}
		if(fp->off != off)
			if(seek(fp->fd, off, 0) < 0){
				fp->off = -1;
				return -1;
			}
		i = read(fp->fd, a, n-sofar);
		if(i < 0){
			fp->off = -1;
			return -1;
		}
		if(i == 0)
			break;
		fp->off = off + i;
	}
	return sofar;
}

void
uncachedir(Node *parent, Node *child)
{
	Node *sp;

	if(parent == 0 || parent == child)
		return;
	for(sp = parent->children; sp; sp = sp->sibs)
		if(sp->opens == 0)
		if(sp != child)
		if(sp->fp != nil)
		if(sp->fp->dirty == 0)
		if(sp->fp->fd >= 0){
			filefree(sp);
			UNCACHED(sp);
		}
}

static int
createtmp(File *fp)
{
	char template[32];

	strcpy(template, "/tmp/ftpXXXXXXXXXXX");
	mktemp(template);
	if(strcmp(template, "/") == 0){
		fprint(2, "ftpfs can't create tmp file %s: %r\n", template);
		return -1;
	}
	if(ntmp >= 16)
		uncachedir(fp->node->parent, fp->node);

	fp->fd = create(template, ORDWR|ORCLOSE, 0600);
	fp->template = strdup(template);
	fp->off = 0;
	ntmp++;
	return fp->fd;
}

/*
 *  write cached data (first Chunk bytes stay in memory)
 */
int
filewrite(Node *node, char *a, long off, int n)
{
	int i, sofar;
	File *fp;

	fp = fileget(node);

	if(fp->mem == nil){
		fp->mem = malloc(Chunk);
		if(fp->mem == nil)
			return seterr("out of memory");
	}

	for(sofar = 0; sofar < n; sofar += i, off += i, a += i){
		if(off < Chunk){
			i = n;
			if(off + i > Chunk)
				i = Chunk - off;
			memmove(fp->mem + off, a, i);
			continue;
		}
		if(fp->fd < 0)
			if(createtmp(fp) < 0)
				return seterr("can't create temp file");
		if(fp->off != off)
			if(seek(fp->fd, off, 0) < 0){
				fp->off = -1;
				return seterr("can't seek temp file");
			}
		i = write(fp->fd, a, n-sofar);
		if(i <= 0){
			fp->off = -1;
			return seterr("can't write temp file");
		}
		fp->off = off + i;
	}

	if(off > fp->len)
		fp->len = off;
	if(off > node->d->length)
		node->d->length = off;
	return sofar;
}

/*
 *  mark a file as dirty
 */
void
filedirty(Node *node)
{
	File *fp;

	fp = fileget(node);
	fp->dirty = 1;
}

/*
 *  mark a file as clean
 */
void
fileclean(Node *node)
{
	if(node->fp)
		node->fp->dirty = 0;
}

int
fileisdirty(Node *node)
{
	return node->fp && node->fp->dirty;
}
