#define KMESGSIZE	64
#define PCICONSSIZE	64
#define STAGESIZE	64

enum {
	Promptsecs	= 60,
};

#include "../pc/dat.h"

#define NAMELEN 28

#define	GSHORT(p)	(((p)[1]<<8)|(p)[0])
#define	GLSHORT(p)	(((p)[0]<<8)|(p)[1])

#define	GLONG(p)	((GSHORT(p+2)<<16)|GSHORT(p))
#define	GLLONG(p)	(((ulong)GLSHORT(p)<<16)|GLSHORT(p+2))
#define	PLLONG(p,v)	(p)[3]=(v);(p)[2]=(v)>>8;(p)[1]=(v)>>16;(p)[0]=(v)>>24

#define	PLVLONG(p,v)	(p)[7]=(v);(p)[6]=(v)>>8;(p)[5]=(v)>>16;(p)[4]=(v)>>24;\
			(p)[3]=(v)>>32; (p)[2]=(v)>>40;\
			(p)[1]=(v)>>48; (p)[0]=(v)>>56;

enum {
	Stkpat =	0,
};

extern int	v_flag;

enum {
	Maxfile = 4096,
};

/* from 9load */

enum {	/* returned by bootpass */
	MORE, ENOUGH, FAIL
};
enum {
	INITKERNEL,
	READEXEC,
	READ9TEXT,
	READ9DATA,
	READGZIP,
	READLZIP,
	READEHDR,		/* elf states ... */
	READPHDR,
	READEPAD,
	READEDATA,		/* through here */
	READE64HDR,		/* elf64 states ... */
	READ64PHDR,
	READE64PAD,
	READE64DATA,		/* through here */
	TRYBOOT,
	TRYEBOOT,		/* another elf state */
	TRYE64BOOT,		/* another elf state */
	INIT9LOAD,
	READ9LOAD,
	FAILED
};

typedef struct Execbytes Execbytes;
struct	Execbytes			/* a.out.h's Exec as bytes, not longs */
{
	uchar	magic[4];		/* magic number */
	uchar	text[4];	 	/* size of text segment */
	uchar	data[4];	 	/* size of initialized data */
	uchar	bss[4];	  		/* size of uninitialized data */
	uchar	syms[4];	 	/* size of symbol table */
	uchar	entry[4];	 	/* entry point */
	uchar	spsz[4];		/* size of sp/pc offset table */
	uchar	pcsz[4];		/* size of pc/line number table */
};

typedef struct {
	Execbytes;
	uvlong uvl[1];			/* optional if HDR_MAGIC set */
} Exechdr;

typedef struct Boot Boot;
struct Boot {
	int state;

	Exechdr hdr;
	uvlong	entry;

	char *bp;	/* base ptr */
	char *wp;	/* write ptr */
	char *ep;	/* end ptr */
};

extern int	debugload;
extern Chan	*conschan;
extern char	*defaultpartition;
extern int	iniread;
extern int	noclock;
extern int	pxe;
extern int	vga;

extern int	biosinited;
