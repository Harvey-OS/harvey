#include <u.h>
#include "fns.h"
#include "efi.h"

typedef struct {
	UINT64		Revision;
	void		*Open;
	void		*Close;
	void		*Delete;
	void		*Read;
	void		*Write;
	void		*GetPosition;
	void		*SetPosition;
	void		*GetInfo;
	void		*SetInfo;
	void		*Flush;
	void		*OpenEx;
	void		*ReadEx;
	void		*WriteEx;
	void		*FlushEx;
} EFI_FILE_PROTOCOL;

typedef struct {
	UINT64		Revision;
	void		*OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

static
EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = {
	0x0964e5b22, 0x6459, 0x11d2,
	0x8e, 0x39, 0x00, 0xa0,
	0xc9, 0x69, 0x72, 0x3b,
};

static
EFI_FILE_PROTOCOL *fsroot;

static void
towpath(CHAR16 *w, int nw, char *s)
{
	int i;

	for(i=0; *s && i<nw-1; i++){
		*w = *s++;
		if(*w == '/')
			*w = '\\';
		w++;
	}
	*w = 0;
}

static void*
fsopen(char *name)
{
	CHAR16 wname[MAXPATH];
	EFI_FILE_PROTOCOL *fp;

	if(fsroot == nil)
		return nil;

	towpath(wname, MAXPATH, name);

	fp = nil;
	if(eficall(fsroot->Open, fsroot, &fp, wname, (UINT64)1, (UINT64)1))
		return nil;
	return fp;
}

static int
fsread(void *f, void *data, int len)
{
	UINTN size;

	size = len > 4096 ? 4096 : len;
	if(eficall(((EFI_FILE_PROTOCOL*)f)->Read, f, &size, data))
		return 0;
	return (int)size;
}

static void
fsclose(void *f)
{
	eficall(((EFI_FILE_PROTOCOL*)f)->Close, f);
}

int
fsinit(void **pf)
{
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
	EFI_FILE_PROTOCOL *root;
	EFI_HANDLE *Handles;
	void *f;
	UINTN Count;
	int i;

	Count = 0;
	Handles = nil;
	if(eficall(ST->BootServices->LocateHandleBuffer,
		ByProtocol, &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, nil, &Count, &Handles))
		return -1;

	/*
	 * assuming the ESP is the first entry in the handle buffer, so go backwards
	 * to scan for plan9.ini in other (9fat) filesystems first. if nothing is found
	 * we'll be defaulting to the ESP.
	 */
	fsroot = nil;
	for(i=Count-1; i>=0; i--){
		root = nil;
		fs = nil;
		if(eficall(ST->BootServices->HandleProtocol,
			Handles[i], &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, &fs))
			continue;
		if(eficall(fs->OpenVolume, fs, &root))
			continue;
		fsroot = root;
		f = fsopen("/plan9.ini");
		if(f != nil){
			if(pf != nil)
				*pf = f;
			else
				fsclose(f);
			break;
		}
	}
	if(fsroot == nil)
		return -1;

	read = fsread;
	close = fsclose;
	open = fsopen;

	return 0;
}
