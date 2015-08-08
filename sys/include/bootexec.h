/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct coffsect {
	char name[8];
	uint32_t phys;
	uint32_t virt;
	uint32_t size;
	uint32_t fptr;
	uint32_t fptrreloc;
	uint32_t fptrlineno;
	uint32_t nrelocnlineno;
	uint32_t flags;
};

/*
 * proprietary exec headers, needed to bootstrap various machines
 */
struct mipsexec {
	int16_t mmagic;     /* (0x160) mips magic number */
	int16_t nscns;      /* (unused) number of sections */
	int32_t timdat;     /* (unused) time & date stamp */
	int32_t symptr;     /* offset to symbol table */
	int32_t nsyms;      /* size of symbol table */
	int16_t opthdr;     /* (0x38) sizeof(optional hdr) */
	int16_t pcszs;      /* flags */
	int16_t amagic;     /* see above */
	int16_t vstamp;     /* version stamp */
	int32_t tsize;      /* text size in bytes */
	int32_t dsize;      /* initialized data */
	int32_t bsize;      /* uninitialized data */
	int32_t mentry;     /* entry pt.				*/
	int32_t text_start; /* base of text used for this file	*/
	int32_t data_start; /* base of data used for this file	*/
	int32_t bss_start;  /* base of bss used for this file	*/
	int32_t gprmask;    /* general purpose register mask	*/
	union {
		int32_t cprmask[4]; /* co-processor register masks
		                       */
		int32_t pcsize;
	};
	int32_t gp_value; /* the gp value used for this object    */
};

struct mips4kexec {
	struct mipsexec h;
	struct coffsect itexts;
	struct coffsect idatas;
	struct coffsect ibsss;
};

struct sparcexec {
	int16_t sjunk;  /* dynamic bit and version number */
	int16_t smagic; /* 0407 */
	uint32_t stext;
	uint32_t sdata;
	uint32_t sbss;
	uint32_t ssyms;
	uint32_t sentry;
	uint32_t strsize;
	uint32_t sdrsize;
};

struct nextexec {
	struct nexthdr {
		uint32_t nmagic;
		uint32_t ncputype;
		uint32_t ncpusubtype;
		uint32_t nfiletype;
		uint32_t ncmds;
		uint32_t nsizeofcmds;
		uint32_t nflags;
	};

	struct nextcmd {
		uint32_t cmd;
		uint32_t cmdsize;
		uint8_t segname[16];
		uint32_t vmaddr;
		uint32_t vmsize;
		uint32_t fileoff;
		uint32_t filesize;
		uint32_t maxprot;
		uint32_t initprot;
		uint32_t nsects;
		uint32_t flags;
	} textc;
	struct nextsect {
		char sectname[16];
		char segname[16];
		uint32_t addr;
		uint32_t size;
		uint32_t offset;
		uint32_t align;
		uint32_t reloff;
		uint32_t nreloc;
		uint32_t flags;
		uint32_t reserved1;
		uint32_t reserved2;
	} texts;
	struct nextcmd datac;
	struct nextsect datas;
	struct nextsect bsss;
	struct nextsym {
		uint32_t cmd;
		uint32_t cmdsize;
		uint32_t symoff;
		uint32_t nsyms;
		uint32_t spoff;
		uint32_t pcoff;
	} symc;
};

struct i386exec {
	struct i386coff {
		uint32_t isectmagic;
		uint32_t itime;
		uint32_t isyms;
		uint32_t insyms;
		uint32_t iflags;
	};
	struct i386hdr {
		uint32_t imagic;
		uint32_t itextsize;
		uint32_t idatasize;
		uint32_t ibsssize;
		uint32_t ientry;
		uint32_t itextstart;
		uint32_t idatastart;
	};
	struct coffsect itexts;
	struct coffsect idatas;
	struct coffsect ibsss;
	struct coffsect icomments;
};
