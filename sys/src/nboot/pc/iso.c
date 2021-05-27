#include <u.h>
#include "fns.h"

enum {
	Sectsz = 0x800,
	Maxpath = 256,
	Dirsz = 33,
};

typedef struct Extend Extend;
typedef struct Dir Dir;

struct Extend
{
	int drive;
	ulong lba;
	ulong len;
	uchar *rp;
	uchar *ep;
	uchar buf[Sectsz];
};

struct Dir
{
	uchar dirlen;
	uchar extlen;

	uchar lba[8];
	uchar len[8];

	uchar date[7];

	uchar flags[3];

	uchar seq[4];

	uchar namelen;
};

int readsect(ulong drive, ulong lba, void *buf);

void
unload(void)
{
}

int
read(void *f, void *data, int len)
{
	Extend *ex = f;

	if(ex->len > 0 && ex->rp >= ex->ep)
		if(readsect(ex->drive, ex->lba++, ex->rp = ex->buf))
			return -1;
	if(ex->len < len)
		len = ex->len;
	if(len > (ex->ep - ex->rp))
		len = ex->ep - ex->rp;
	memmove(data, ex->rp, len);
	ex->rp += len;
	ex->len -= len;
	return len;
}

void
close(void *f)
{
	Extend *ex = f;

	ex->drive = 0;
	ex->lba = 0;
	ex->len = 0;
	ex->rp = ex->ep = ex->buf + Sectsz;
}

static int
isowalk(Extend *ex, int drive, char *path)
{
	char name[Maxpath], c, *end;
	int i;
	Dir d;

	close(ex);
	ex->drive = drive;

	/* find pvd */
	for(i=0x10; i<0x1000; i++){
		if(readsect(drive, i, ex->buf))
			return -1;
		if(*ex->buf == 1)
			break;
	}
	ex->lba = *((ulong*)(ex->buf + 156 + 2));
	ex->len = *((ulong*)(ex->buf + 156 + 10));

	for(;;){
		if(readn(ex, &d, Dirsz) != Dirsz)
			break;
		if(d.dirlen == 0)
			break;
		if(readn(ex, name, d.namelen) != d.namelen)
			break;
		i = d.dirlen - (Dirsz + d.namelen);
		while(i-- > 0)
			read(ex, &c, 1);
		for(i=0; i<d.namelen; i++){
			c = name[i];
			if(c >= 'A' && c <= 'Z'){
				c -= 'A';
				c += 'a';
			}
			name[i] = c;
		}
		name[i] = 0;
		while(*path == '/')
			path++;
		if((end = strchr(path, '/')) == 0)
			end = path + strlen(path);
		i = end - path;
		if(d.namelen == i && memcmp(name, path, i) == 0){
			ex->rp = ex->ep;
			ex->lba = *((ulong*)d.lba);
			ex->len = *((ulong*)d.len);
			if(*end == 0)
				return 0;
			else if(d.flags[0] & 2){
				path = end;
				continue;
			}
			break;
		}
	}
	close(ex);
	return -1;
}

void
start(void *sp)
{
	char path[Maxpath], *kern;
	int drive;
	Extend ex;
	void *f;

	/* drive passed in DL */
	drive = ((ushort*)sp)[5] & 0xFF;

	/*
	 * load full bootblock as only the frist 2K get
	 * loaded from bios. the code is specially arranged
	 * to have all the important routines in the first
	 * 2K of the 9bootiso image. (strings have been
	 * placed in l.s to make sure they will be < 2K)
	 */
	if(isowalk(&ex, drive, bootname)){
		print(bootname);
		putc('?');
		halt();
	}
	readn(&ex, origin, ex.len);
	close(&ex);

	if(isowalk(f = &ex, drive, "/cfg/plan9.ini")){
		print("no config\n");
		f = 0;
	}
	for(;;){
		kern = configure(f, path); f = 0;
		if(isowalk(&ex, drive, kern)){
			print("not found\n");
			continue;
		}
		print(bootkern(&ex));
		print("\n");
	}
}

