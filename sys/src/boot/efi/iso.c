#include <u.h>
#include "fns.h"
#include "efi.h"

enum {
	Sectsz = 0x800,
	Dirsz = 33,
};

typedef struct Extend Extend;
typedef struct Dir Dir;

struct Extend
{
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

typedef struct {
	UINT32		MediaId;

	BOOLEAN		RemovableMedia;
	BOOLEAN		MediaPresent;
	BOOLEAN		LogicalPartition;
	BOOLEAN		ReadOnly;

	BOOLEAN		WriteCaching;
	BOOLEAN		Pad[3];

	UINT32		BlockSize;
	UINT32		IoAlign;
	UINT64		LastBlock;
} EFI_BLOCK_IO_MEDIA;

typedef struct {
	UINT64		Revision;
	EFI_BLOCK_IO_MEDIA	*Media;
	void		*Reset;
	void		*ReadBlocks;
	void		*WriteBlocks;
	void		*FlushBlocks;
} EFI_BLOCK_IO_PROTOCOL;

static EFI_GUID
EFI_BLOCK_IO_PROTOCOL_GUID = {
	0x964e5b21, 0x6459, 0x11d2,
	0x8e, 0x39, 0x00, 0xa0,
	0xc9, 0x69, 0x72, 0x3b,
};

static EFI_BLOCK_IO_PROTOCOL *bio;

static int
readsect(ulong lba, void *buf)
{
	lba *= Sectsz/bio->Media->BlockSize;
	return eficall(bio->ReadBlocks, bio, (UINTN)bio->Media->MediaId, (UINT64)lba, (UINTN)Sectsz, buf);
}

static int
isoread(void *f, void *data, int len)
{
	Extend *ex = f;

	if(ex->len > 0 && ex->rp >= ex->ep)
		if(readsect(ex->lba++, ex->rp = ex->buf))
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
isoclose(void *f)
{
	Extend *ex = f;

	ex->lba = 0;
	ex->len = 0;
	ex->rp = ex->ep = ex->buf + Sectsz;
}

static int
isowalk(Extend *ex, char *path)
{
	char name[MAXPATH], c, *end;
	int i;
	Dir d;

	isoclose(ex);

	/* find pvd */
	for(i=0x10; i<0x1000; i++){
		if(readsect(i, ex->buf))
			return -1;
		if(memcmp(ex->buf, "\001CD001\001", 7) == 0)
			goto Foundpvd;
	}
	return -1;
Foundpvd:
	ex->lba = *((ulong*)(ex->buf + 156 + 2));
	ex->len = *((ulong*)(ex->buf + 156 + 10));
	if(*path == 0)
		return 0;

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
	return -1;
}

static void*
isoopen(char *path)
{
	static uchar buf[sizeof(Extend)+8];
	Extend *ex = (Extend*)((uintptr)(buf+7)&~7);

	if(isowalk(ex, path))
		return nil;
	return ex;
}

int
isoinit(void **fp)
{
	EFI_BLOCK_IO_MEDIA *media;
	EFI_HANDLE *Handles;
	UINTN Count;
	int i;

	bio = nil;
	Count = 0;
	Handles = nil;
	if(eficall(ST->BootServices->LocateHandleBuffer,
		ByProtocol, &EFI_BLOCK_IO_PROTOCOL_GUID, nil, &Count, &Handles))
		return -1;

	for(i=0; i<Count; i++){
		bio = nil;
		if(eficall(ST->BootServices->HandleProtocol,
			Handles[i], &EFI_BLOCK_IO_PROTOCOL_GUID, &bio))
			continue;
	
		media = bio->Media;
		if(media != nil
		&& media->MediaPresent
		&& media->LogicalPartition == 0
		&& media->BlockSize != 0
		&& isoopen("") != nil)
			goto Found;
	}
	return -1;

Found:
	open = isoopen;
	read = isoread;
	close = isoclose;

	if(fp != nil)
		*fp = isoopen("/cfg/plan9.ini");

	return 0;
}
