#include <u.h>
#include "fns.h"
#include "efi.h"

enum {
	MAXPATH = 128,
};

UINTN MK;
EFI_HANDLE *IH;
EFI_SYSTEM_TABLE *ST;

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
EFI_FILE_PROTOCOL *root;

void
putc(int c)
{
	CHAR16 w[2];

	w[0] = c;
	w[1] = 0;
	eficall(2, ST->ConOut->OutputString, ST->ConOut, w);
}

int
getc(void)
{
	EFI_INPUT_KEY k;

	if(eficall(2, ST->ConIn->ReadKeyStroke, ST->ConIn, &k))
		return 0;
	return k.UnicodeChar;
}

void
usleep(int us)
{
	eficall(1, ST->BootServices->Stall, (UINTN)us);
}

void
unload(void)
{
	eficall(2, ST->BootServices->ExitBootServices, IH, MK);
}

EFI_STATUS
LocateProtocol(EFI_GUID *guid, void *reg, void **pif)
{
	return eficall(3, ST->BootServices->LocateProtocol, guid, reg, pif);
}

void
fsinit(void)
{
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

	fs = nil;
	root = nil;
	if(LocateProtocol(&EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, nil, &fs))
		return;
	if(eficall(2, fs->OpenVolume, fs, &root)){
		root = nil;
		return;
	}
}

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

EFI_FILE_PROTOCOL*
fswalk(EFI_FILE_PROTOCOL *dir, char *name)
{
	CHAR16 wname[MAXPATH];
	EFI_FILE_PROTOCOL *fp;

	towpath(wname, MAXPATH, name);

	fp = nil;
	if(eficall(5, dir->Open, dir, &fp, wname, (UINT64)1, (UINT64)1))
		return nil;
	return fp;
}


int
read(void *f, void *data, int len)
{
	UINTN size;

	size = len;
	if(eficall(3, ((EFI_FILE_PROTOCOL*)f)->Read, f, &size, data))
		return 0;
	return (int)size;
}

void
close(void *f)
{
	eficall(1, ((EFI_FILE_PROTOCOL*)f)->Close, f);
}


static void
memconf(char **cfg)
{
	static uchar memtype[EfiMaxMemoryType] = {
		[EfiReservedMemoryType]		2,
		[EfiLoaderCode]			1,
		[EfiLoaderData]			1,
		[EfiBootServicesCode]		1,
		[EfiBootServicesData]		1,
		[EfiRuntimeServicesCode]	1,
		[EfiRuntimeServicesData]	1,
		[EfiConventionalMemory]		1,
		[EfiUnusableMemory]		2,
		[EfiACPIReclaimMemory]		3,
		[EfiACPIMemoryNVS]		4,
		[EfiMemoryMappedIO]		2,
		[EfiMemoryMappedIOPortSpace]	2,
		[EfiPalCode]			2,
	};
	UINTN mapsize, entsize;
	EFI_MEMORY_DESCRIPTOR *t;
	uchar mapbuf[96*1024], *p, m;
	UINT32 entvers;
	char *s;

	mapsize = sizeof(mapbuf);
	entsize = sizeof(EFI_MEMORY_DESCRIPTOR);
	entvers = 1;
	if(eficall(5, ST->BootServices->GetMemoryMap, &mapsize, mapbuf, &MK, &entsize, &entvers))
		return;

	s = *cfg;
	for(p = mapbuf; mapsize >= entsize; p += entsize, mapsize -= entsize){
		t = (EFI_MEMORY_DESCRIPTOR*)p;

		m = 0;
		if(t->Type < EfiMaxMemoryType)
			m = memtype[t->Type];

		if(m == 0)
			continue;

		if(s == *cfg)
			memmove(s, "*e820=", 6), s += 6;
		s = hexfmt(s, 1, m), *s++ = ' ';
		s = hexfmt(s, 16, t->PhysicalStart), *s++ = ' ';
		s = hexfmt(s, 16, t->PhysicalStart + t->NumberOfPages * 4096ULL), *s++ = ' ';
	}
	*s = '\0';
	if(s > *cfg){
		s[-1] = '\n';
		print(*cfg);
		*cfg = s;
	}
}

static void
acpiconf(char **cfg)
{
	EFI_CONFIGURATION_TABLE *t;
	uintptr pa;
	char *s;
	int n;

	pa = 0;
	t = ST->ConfigurationTable;
	n = ST->NumberOfTableEntries;
	while(--n >= 0){
		if(memcmp(&t->VendorGuid, &ACPI_10_TABLE_GUID, sizeof(EFI_GUID)) == 0){
			if(pa == 0)
				pa = (uintptr)t->VendorTable;
		} else if(memcmp(&t->VendorGuid, &ACPI_20_TABLE_GUID, sizeof(EFI_GUID)) == 0)
			pa = (uintptr)t->VendorTable;
		t++;
	}

	if(pa){
		s = *cfg;
		memmove(s, "*acpi=0x", 8), s += 8;
		s = hexfmt(s, 0, pa), *s++ = '\n';
		*s = '\0';
		print(*cfg);
		*cfg = s;
	}
}


static int
topbit(ulong mask)
{
	int bit = 0;

	while(mask != 0){
		mask >>= 1;
		bit++;
	}
	return bit;
}

static int
lowbit(ulong mask)
{
	int bit = 0;

	while((mask & 1) == 0){
		mask >>= 1;
		bit++;
	}
	return bit;
}

static void
screenconf(char **cfg)
{
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	ulong mr, mg, mb, mx, mc;
	int bits, depth;
	char *s;

	gop = nil;
	if(LocateProtocol(&EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, nil, &gop) || gop == nil)
		return;

	if((info = gop->Mode->Info) == nil)
		return;

	switch(info->PixelFormat){
	default:
		return;	/* unsupported */

	case PixelRedGreenBlueReserved8BitPerColor:
		mr = 0x000000ff;
		mg = 0x0000ff00;
		mb = 0x00ff0000;
		mx = 0xff000000;
		break;

	case PixelBlueGreenRedReserved8BitPerColor:
		mb = 0x000000ff;
		mg = 0x0000ff00;
		mr = 0x00ff0000;
		mx = 0xff000000;
		break;

	case PixelBitMask:
		mr = info->PixelInformation.RedMask;
		mg = info->PixelInformation.GreenMask;
		mb = info->PixelInformation.BlueMask;
		mx = info->PixelInformation.ReservedMask;
		break;
	}

	if((depth = topbit(mr | mg | mb | mx)) == 0)
		return;

	s = *cfg;
	memmove(s, "*bootscreen=", 12), s += 12;
	s = decfmt(s, 0, info->PixelsPerScanLine), *s++ = 'x';
	s = decfmt(s, 0, info->VerticalResolution), *s++ = 'x';
	s = decfmt(s, 0, depth), *s++ = ' ';

	while(depth > 0){
		if(depth == topbit(mr)){
			mc = mr;
			*s++ = 'r';
		} else if(depth == topbit(mg)){
			mc = mg;
			*s++ = 'g';
		} else if(depth == topbit(mb)){
			mc = mb;
			*s++ = 'b';
		} else if(depth == topbit(mx)){
			mc = mx;
			*s++ = 'x';
		} else {
			break;
		}
		bits = depth - lowbit(mc);
		s = decfmt(s, 0, bits);
		depth -= bits;
	}
	*s++ = ' ';

	*s++ = '0', *s++ = 'x';
	s = hexfmt(s, 0, gop->Mode->FrameBufferBase), *s++ = '\n';
	*s = '\0';

	print(*cfg);
	*cfg = s;
}

void
eficonfig(char **cfg)
{
	screenconf(cfg);
	acpiconf(cfg);
	memconf(cfg);
}

EFI_STATUS
main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st)
{
	char path[MAXPATH], *kern;
	void *f;

	IH = ih;
	ST = st;

	fsinit();

	f = fswalk(root, "/plan9.ini");
	for(;;){
		kern = configure(f, path);
		f = fswalk(root, kern);
		if(f == nil){
			print("not found\n");
			continue;
		}
		print(bootkern(f));
		print("\n");
		f = nil;
	}
}
