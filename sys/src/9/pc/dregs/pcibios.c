/*
 * PCI BIOS32 support for configuration
 *
 * pci bios crud is only used if explicitly requested with *pcibios=1
 * in plan9.ini (and the _32_ table exists), or if neither of the usual
 * configuration mechanisms works (which should never happen).
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "mp.h"

static int pcicfgrw8bios(int, int, int, int);
static int pcicfgrw16bios(int, int, int, int);
static int pcicfgrw32bios(int, int, int, int);

extern int pcimaxbno;
extern int pcimaxdno;

static BIOS32si* pcibiossi;

static int
pcicfgrw8bios(int tbdf, int rno, int data, int read)
{
	BIOS32ci ci;

	if(pcibiossi == nil)
		return -1;

	memset(&ci, 0, sizeof(BIOS32ci));
	ci.ebx = (BUSBNO(tbdf)<<8)|(BUSDNO(tbdf)<<3)|BUSFNO(tbdf);
	ci.edi = rno;
	if(read){
		/* bios32 operations start at 0xb100 */
		ci.eax = 0xB108;
		if(!bios32ci(pcibiossi, &ci)/* && !(ci.eax & 0xFF)*/)
			return ci.ecx & 0xFF;
	}
	else{
		ci.eax = 0xB10B;
		ci.ecx = data & 0xFF;
		if(!bios32ci(pcibiossi, &ci)/* && !(ci.eax & 0xFF)*/)
			return 0;
	}

	return -1;
}

static int
pcicfgrw16bios(int tbdf, int rno, int data, int read)
{
	BIOS32ci ci;

	if(pcibiossi == nil)
		return -1;

	memset(&ci, 0, sizeof(BIOS32ci));
	ci.ebx = (BUSBNO(tbdf)<<8)|(BUSDNO(tbdf)<<3)|BUSFNO(tbdf);
	ci.edi = rno;
	if(read){
		ci.eax = 0xB109;
		if(!bios32ci(pcibiossi, &ci)/* && !(ci.eax & 0xFF)*/)
			return ci.ecx & 0xFFFF;
	}
	else{
		ci.eax = 0xB10C;
		ci.ecx = data & 0xFFFF;
		if(!bios32ci(pcibiossi, &ci)/* && !(ci.eax & 0xFF)*/)
			return 0;
	}

	return -1;
}

static int
pcicfgrw32bios(int tbdf, int rno, int data, int read)
{
	BIOS32ci ci;

	if(pcibiossi == nil)
		return -1;

	memset(&ci, 0, sizeof(BIOS32ci));
	ci.ebx = (BUSBNO(tbdf)<<8)|(BUSDNO(tbdf)<<3)|BUSFNO(tbdf);
	ci.edi = rno;
	if(read){
		ci.eax = 0xB10A;
		if(!bios32ci(pcibiossi, &ci)/* && !(ci.eax & 0xFF)*/)
			return ci.ecx;
	}
	else{
		ci.eax = 0xB10D;
		ci.ecx = data;
		if(!bios32ci(pcibiossi, &ci)/* && !(ci.eax & 0xFF)*/)
			return 0;
	}

	return -1;
}

static BIOS32si*
pcibiosinit(void)
{
	BIOS32ci ci;
	BIOS32si *si;

	if((si = bios32open("$PCI")) == nil)
		return nil;

	memset(&ci, 0, sizeof(BIOS32ci));
	ci.eax = 0xB101;
	if(bios32ci(si, &ci) || ci.edx != ((' '<<24)|('I'<<16)|('C'<<8)|'P')){
		free(si);
		return nil;
	}
	if(ci.eax & 0x01)
		pcimaxdno = 31;
	else
		pcimaxdno = 15;
	pcimaxbno = ci.ecx & 0xff;

	return si;
}

int
pciusebios(void)
{
	pcibiossi = pcibiosinit();
	if(pcibiossi != nil) {
		pcicfgrw8 = pcicfgrw8bios;
		pcicfgrw16 = pcicfgrw16bios;
		pcicfgrw32 = pcicfgrw32bios;
	}
	return pcibiossi == nil? -1: 0;
}
