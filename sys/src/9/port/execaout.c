/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../port/edf.h"
#include	<trace.h>

#include	<a.out.h>

// These comments in // distinguish new from original information.
// Let's go over a.out. a.out format has a header which defines the three segments
// of a binary: text, data, bss. The BSS is a "virtual" segment, which tells how large
// the bss area is. The header has a magic number (a two byte jump around the header
// in the original) which marks the architecture type of the a.out, defined in
// 9/$ARCH/dat.h.
// Unlike ELF, the segments are defined in relation to each other. Numbers are big-endian,
// in a nod to Plan 9's origins on big-endian systems. The text segment is loaded into
// memory including the header, and the entry point is defined in the header -- it is not
// fixed at 0 or 0x20 as on some Unix systems.
// Let's look at a typical program:
// package main func main(){}
// The header for this program looks like this:
// 00 00 8a 97
// 00 08 d5 fe
// 00 00 21 40
// 00 01 e6 60
// 00 00 d0 db
// 00 24 30 70
// 00 00 00 00
// 00 00 00 00
// 00 00 00 00
// 00 24 30 70
// In this case it is an amd64 Plan 9 binary. Note that magic:
// 00 00 8a 97 is big-endian, so in little it is 0x8a97. The 0x8000 marks it as a new
// style header:
// #define HDR_MAGIC	0x00008000		/* header expansion */
// The 7 is from the macro:
// #define	_MAGIC(f, b)	((f)|((((4*(b))+0)*(b))+7))
// so, 0x8000 | (b * 4 * b) | 7 -> 0x8000 | 0xa90 | 7
// Hence, the magic on amd64.
// The reset of the a.out header is like this, in big-endian:
// struct	Exec
// {
// 	int32_t	magic; 00 00 8a 97		/* magic number */
// 	int32_t	text;  00 08 d5 fe	 	/* size of text segment */
// 	int32_t	data;  00 00 21 40	 	/* size of initialized data */
// 	int32_t	bss;   00 01 e6 60	  	/* size of uninitialized data */
// 	int32_t	syms;  00 00 d0 db	 	/* size of symbol table */
// 	int32_t	entry; 00 24 30 70	 	/* 32-bit entry point */
// 	int32_t	spsz;  00 00 00 00		/* size of pc/sp offset table */
//	int32_t	pcsz;  00 00 00 00		/* size of pc/line number table */
//      uint64_t hdr;  00 00 00 00              /* expansion (64-bit entry point) */
//                     00 24 30 70
//};
// Note that Go did not get it quite right! The 32-bit entry should be zero on
// expanded a.out headers.
// A few other things to note:
// ELF typically aligns segments on page boundaries. Plan 9 a.out does not:
// the data segment must be moved to its proper place. One can not mmap the
// data segment, as one can in ELF. This is a major disadvantage of a.out.
// In the discussion below, we annotate code with these values so we can
// try to verify them.

// This is part of the grossness of the 64-bit port.
// The header is actually a bit bigger than the exec struct,
// since we kind ran out of room.
// This was a botch from the original plan 9; they had 64-bit
// CPUs available and could have got this right.
typedef struct {
	Exec Exec;
	uint64_t hdr[1];
} Hdr;

static uint64_t
vl2be(uint64_t v)
{
	uint8_t *p;

	p = (uint8_t*)&v;
	return ((uint64_t)((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3])<<32)
	      |((uint64_t)(p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7]);
}

static uint32_t
l2be(int32_t l)
{
	uint8_t *cp;

	cp = (uint8_t*)&l;
	return (cp[0]<<24) | (cp[1]<<16) | (cp[2]<<8) | cp[3];
}

/*
 * called from sysexec, when we know that we're stuck with a.out.
 * This was execac, but the NIX stuff is better managed for ELF binaries.
 * It would be nice, someday, to bring the NIX stuff back. In the era
 * of 1024-core machines, NIX makes more sense than ever.
 *
 * We should do it for Jim.
 *
 */
int
aoutldseg(Chan *c, void *v, uintptr_t *entryp, Ldseg **rp, char *mach, uint32_t minpgsz)
{
	Hdr *hdr = v;
	Ldseg *ldseg;
	int32_t hdrsz, magic, textsz, datasz, bsssz;
	uintptr_t textlim, datalim, bsslim;

	magic = l2be(hdr->Exec.magic);
	if (magic != AOUT_MAGIC) {
		return -1;
	}
	print("aoutldseg\n");
	if (magic & HDR_MAGIC) {
		// 0x0000000000243070 -> 70 30 24 little endian.
		// BE is easier to read of course.
		*entryp = vl2be(hdr->hdr[0]);
		// Expanded header, so size is size of the header
		// struct above, i.e. 40 (0x28)
		hdrsz = sizeof(Hdr);
	}
	else{
		*entryp = l2be(hdr->Exec.entry);
		hdrsz = sizeof(Exec);
	}
	// From here on out, all is hex.
	textsz = l2be(hdr->Exec.text); // 8d5fe
	datasz = l2be(hdr->Exec.data); // 2140
	bsssz = l2be(hdr->Exec.bss);   // 1e660

	// This is the highest address of text. Text is loaded at
	// 200000 virtual on most architectures. textsz is from the file.
	// Since we load the header in the text segment, we add that in too.
	textlim = UTROUND(UTZERO+hdrsz+textsz); // UTROUND(200000 + 28 + 8d5fe)
	                                        // = UTROUND(28d626) = 400000
	datalim = BIGPGROUND(textlim+datasz);   // BIGPGROUND(400000 + 2140) = 600000
	bsslim = BIGPGROUND(textlim+datasz+bsssz); // BIGPGROUND(400000 + 2140 + 1e660)
	                                           // = 600000
	// bsslim and datalim can be the same. bss can be the zeros that
	// fill the last page in the data segment.

	/*
	 * Check the binary header for consistency,
	 * e.g. the entry point is within the text segment and
	 * the segments don't overlap each other.
	 */
	// entry is 243070
	// if entry <  200000 + 28, means start is in header, bad.
	// if entry > 200000 + 28 + 8d5fe would mean above end of text, bad.
	// NOTE: you can't check textlim. That's the in-memory image limit, which
	// can be larger than the actual size of text.
	// The limit is defined by what's in the file!
	print("%#lx %#lx %#lx %#lx %#lx %#lx \n",
			textlim, datalim, bsslim,
			textsz, datasz, bsssz);
	print("entrypoint %#lx\n", *entryp);
	if(*entryp < UTZERO+hdrsz || *entryp >= UTZERO+hdrsz+textsz){
		print("Bad entry point\n");
		return 0;
	}

	// if 8d5fe is > 400000 that's bad.
	// if 2140 is > 600000 that's bad.
	// if 1e660 is > 600000 that's bad.
	if(textsz >= textlim || datasz > datalim || bsssz > bsslim
	   // if 400000 is >= (0x00007ffffffff000ull & ~(BIGPGSZ-1)), that's bad.
	   // if 600000 is >= (0x00007ffffffff000ull & ~(BIGPGSZ-1)), that's bad.
	   // if 600000 is >= (0x00007ffffffff000ull & ~(BIGPGSZ-1)), that's bad.
	   || textlim >= USTKTOP || datalim >= USTKTOP || bsslim >= USTKTOP
	   // if 600000 is < 400000 that's bad.
	   // if 600000 is < 600000 that's bad.
	   // Note this last test covers the case that bss starts in the
	   // middle of a data text page. That's ok.
	   || datalim < textlim || bsslim < datalim) {
		return 0;
	}

	// Tests pass. Allocate two segments.
	// The bss segment is automagically created
	// by exec -- no need to worry about it.
	ldseg = malloc(2 * sizeof ldseg[0]);
	if(ldseg == nil){
		print("aloutldseg: malloc fail\n");
		return 0;
	}


	/*
	 * There are three -- count them, three, segments.
	 * We only need to return info about text and data,
	 * thanks to Aki's outstanding work on elf.
	 */

	/* Text.  Shared. Attaches to cache image if possible
	 * but prepaged if EXAC
	 */
	/* img = attachimage(SG_TEXT|SG_READ, chan, up->color, UTZERO, (textlim-UTZERO)/BIGPGSZ); */
	// pagesize is always BIGPGSIZE on harvey.
	ldseg[0].pgsz = BIGPGSZ;
	// the memory size is 400000 - 200000 (UTZERO starts 200000)
	ldseg[0].memsz = textlim - UTZERO;
	// the file size is the header size and text size; we read the
	// whole front of the file into the text segment.
	ldseg[0].filesz = hdrsz+textsz;
	// text segment starts at the start of the file.
	// This is confusing. we have two things that are the same?
	ldseg[0].pg0fileoff = 0;
	ldseg[0].pg0off = 0;
	// our virtual address is UTZERO.
	ldseg[0].pg0vaddr = UTZERO;
	ldseg[0].type = SG_LOAD | SG_EXEC | SG_READ;

	/* Data. Shared. */
	/* s = newseg(SG_DATA, textlim, (datalim-textlim)/BIGPGSZ); */
	/* s->fstart = hdrsz+textsz; */
	/* s->flen = datasz; */
	// big page size, again.
	ldseg[1].pgsz = BIGPGSZ;
	// size in mem is 600000 - 400000 = 200000
	ldseg[1].memsz = datalim - textlim;
	// Size in the file is datasz
	ldseg[1].filesz = datasz;
	// Offset in the file is hrdsize + textsiz
	// TODO: aligned to 32 bits? I think so.
	// 28 + 8d5fe = 8d626
	// This is confusing. we have two things that are the same?
	ldseg[1].pg0fileoff = hdrsz + textsz;
	ldseg[1].pg0off = 0;
	// vaddr stars at textlim.
	ldseg[1].pg0vaddr = textlim;
	ldseg[1].type = SG_LOAD | SG_WRITE | SG_READ;

	*rp = ldseg;
	return 2;
}

