#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define VFLAG(...)	if(vflag) print(__VA_ARGS__)

#define UPTR2INT(p)	((uintptr)(p))

static int vflag = 0;

typedef struct BIOS32sdh {		/* BIOS32 Service Directory Header */
	u8int	signature[4];		/* "_32_" */
	u8int	physaddr[4];		/* physical address of entry point */
	u8int	revision;
	u8int	length;			/* of header in paragraphs */
	u8int	checksum;		/* */
	u8int	reserved[5];
} BIOS32sdh;

typedef struct BIOS32si {		/* BIOS32 Service Interface */
	u8int*	base;			/* base address of service */
	int	length;			/* length of service */
	u32int	offset;			/* service entry-point from base */

	u16int	ptr[3];			/* far pointer m16:32 */
} BIOS32si;

static Lock bios32lock;
static u16int bios32ptr[3];
static void* bios32entry;

int
bios32ci(BIOS32si* si, BIOS32ci* ci)
{
	int r;

	lock(&bios32lock);
	r = bios32call(ci, si->ptr);
	unlock(&bios32lock);

	return r;
}

static void*
rsdchecksum(void* addr, int length)
{
	u8int *p, sum;

	sum = 0;
	for(p = addr; length-- > 0; p++)
		sum += *p;
	if(sum == 0)
		return addr;

	return nil;
}

static void*
rsdscan(u8int* addr, int len, char* signature)
{
	int sl;
	u8int *e, *p;

	e = addr+len;
	sl = strlen(signature);
	for(p = addr; p+sl < e; p += 16){
		if(memcmp(p, signature, sl))
			continue;
		return p;
	}

	return nil;
}

static int
bios32locate(void)
{
	uintptr ptr;
	BIOS32sdh *sdh;

	VFLAG("bios32link\n");
	if((sdh = rsdscan(BIOSSEG(0xE000), 0x20000, "_32_")) == nil)
		return -1;
	if(rsdchecksum(sdh, sizeof(BIOS32sdh)) == nil)
		return -1;
	VFLAG("sdh @ %#p, entry %#ux\n", sdh, L32GET(sdh->physaddr));

	bios32entry = vmap(L32GET(sdh->physaddr), 4096+1);
	VFLAG("entry @ %#p\n", bios32entry);
	ptr = UPTR2INT(bios32entry);
	bios32ptr[0] = ptr & 0xffff;
	bios32ptr[1] = (ptr>>16) & 0xffff;
	bios32ptr[2] = KESEL;
	VFLAG("bios32link: ptr %ux %ux %ux\n",
		bios32ptr[0], bios32ptr[1], bios32ptr[2]);

	return 0;
}

void
BIOS32close(BIOS32si* si)
{
	vunmap(si->base, si->length);
	free(si);
}

BIOS32si*
bios32open(char* id)
{
	uint ptr;
	BIOS32ci ci;
	BIOS32si *si;

	lock(&bios32lock);
	if(bios32ptr[2] == 0 && bios32locate() < 0){
		unlock(&bios32lock);
		return nil;
	}

	VFLAG("bios32si: %s\n", id);
	memset(&ci, 0, sizeof(BIOS32ci));
	ci.eax = (id[3]<<24|(id[2]<<16)|(id[1]<<8)|id[0]);

	bios32call(&ci, bios32ptr);
	unlock(&bios32lock);

	VFLAG("bios32si: eax %ux\n", ci.eax);
	if(ci.eax & 0xff)
		return nil;
	VFLAG("bios32si: base %#ux length %#ux offset %#ux\n",
		ci.ebx, ci.ecx, ci.edx);

	if((si = malloc(sizeof(BIOS32si))) == nil)
		return nil;
	if((si->base = vmap(ci.ebx, ci.ecx)) == nil){
		free(si);
		return nil;
	}
	si->length = ci.ecx;

	ptr = UPTR2INT(si->base)+ci.edx;
	si->ptr[0] = ptr & 0xffff;
	si->ptr[1] = (ptr>>16) & 0xffff;
	si->ptr[2] = KESEL;
	VFLAG("bios32si: eax entry %ux\n", ptr);

	return si;
}
