/* bitio.h - view a stream file as a sequence of binary values, hiding the
   bit- and byte-packing format of the file.  The format of input and output
   files may differ.   Reading and writing are performed by macroes for speed;
   the price for this is that the file formats must be selected at compile time.

SYNOPSIS
	#include <stdio.h>
	#include "bitio.h"

	BITFILE *bopen(stream,type);
	    FILE *stream;
	    char *type;

	int getb(bitfile);
	    BITFILE *bitfile;

	putb(bit,bitfile);
	    int bit;
	    BITFILE *bitfile;

	padb(bitfile,bit,bdy,len);
	    BITFILE *bitfile;
	    int bit,bdy,len;

	bflush(bitfile);
	    BITFILE *bitfile;

	unsigned long bclose(bitfile);
	    BITFILE *bitfile;

COMPILER DEPENDENCIES
   The compiler's data types must include:
	unsigned char:  8 bits each
	unsigned short: 2 unsigned chars each
	unsigned int:   2 unsigned shorts each
DESCRIPTION
	Bopen views the named stream file as a bit file to be read (if type is "r")
   or written (if type is "w").   The stream file must already be fopen(3)ed, and
   the first bit to be read/written will be the first bit in its next byte in
   getc(3)/putc(3) order.  Bopen returns a pointer which identifies the bitfile
   to the other functions.  System or stream I/O to/from the associated filedes
   or stream should not be used until after bclose is called.
	Getb returns the next bit from the named bitfile.  It returns EOF on
   end of file or read error.  EOF may occur on a byte, short, or int boundary,
   depending on file format.
	Putb appends the given bit to the named bitfile.
	Padb writes 'bit' enough times (possibly 0) so that if a bitstring
   of length 'len' were written next it would end on a 'bdy'-bit boundary
   (may do the wrong thing if 'bdy' doesn't divide UINT_MAX).
	Bflush ensures that all written bits have been written to the stream
   via putc(3).  The bitfile remains open.  It does not fflush(3) the
   associated stream.  The output is padded with 0 bits to a byte, short, or int
   boundary, depending on file format.
	Bclose causes any buffered data to be flushed and the buffers freed.
   It returns the total number of bytes (not bits) read/written since bopen()
   was called.  It fflush(3)es, but does not fclose(3) the associated stream.

        Bitfile formats are selected at compile time: see `FORMAT:' at the
   end of this file.  The formats for input and output may differ.  Formats
   include:
   a	each bit is an ASCII character: '0' or '1', in putc(3) order; not padded.
   0	the low-order (0001) bit in each byte is first ("little-endian"), and
	bytes are in putc(3) order; EOF and padding at a byte boundary.
   1	the high-order (0200) bit in each byte is first ("big-endian"), and
	bytes are in putc(3) order; EOF and padding at a byte boundary.
   10	the low-order (0001) bit in each byte is first ("little-endian"), but
	bytes are reversed (in each pair) from putc(3) order;  EOF and padding
	at a short boundary.
   11	the high-order (0200) bit in each byte is first ("big-endian"), but
	bytes are reversed (in each pair) from putc(3) order; EOF and padding
	at a short boundary.
   Planned (data structures are in place; code will be implemented if needed):
   100	the low-order (0001) bit in each byte is first ("little-endian"), and
	bytes (in each pair) are in putc(3) order; but shorts (in each pair)
	are reversed from putc(3) order;  EOF and padding at an int boundary.
   101, 110, 111 - by obvious analogy
BUGS
	Putting to an input bitfile or getting from an output bitfile is
   erroneous, but is not checked for.
*/

typedef struct BITFILE {
	FILE *fp;		/* associated stream */
	char type;		/* one of 'r','w' */
	int ic;			/* byte just read */
	unsigned long nb;	/* no. bytes read/written since bopen() */
	unsigned int n;		/* no. bits written so far (mod UINT_MAX) */
	unsigned char cm;	/* single-bit mask */
	unsigned short sm;	/* single-bit mask */
	unsigned int im;	/* single-bit mask */
	union {	struct {	/* used to reorder char & short order */
			union {	struct {
					unsigned char c0;
					unsigned char c1;
					} cc;
				unsigned short s;
				} s0;
			union {	struct {
					unsigned char c0;
					unsigned char c1;
					} cc;
				unsigned short s;
				} s1;
			} ss;
		unsigned int i;
		} i;
	} BITFILE;
#define Init_BITFILE {NULL,'\0',0,0,0,0,0}
#if MAIN
BITFILE empty_BITFILE = Init_BITFILE;
#else
extern BITFILE empty_BITFILE;
#endif

/* Code common to all formats: */
#if MAIN
BITFILE *bopen_rw(s,t)
    FILE *s;
    char *t;
{   BITFILE *f;
	if((f=(BITFILE *)malloc(sizeof(BITFILE)))==NULL) {
		err("bopen: can't alloc");
		return(NULL);
		};
	*f = empty_BITFILE;
	f->fp = s;
	f->type = *t;
	return(f);
	}
#else
BITFILE *bopen_rw();
#endif

/* Code particular to each format: */

/* Format a:  ASCII file, one printable char ('0' or '1') per bit: */
#define bopen_r_a(s) bopen_rw((s),"r")
#define bopen_w_a(s) bopen_rw((s),"w")
#define getb_a(f) ( (((f)->ic=getc((f)->fp))!=EOF)? \
			((f)->nb++, \
			 ((f)->ic=='0')? \
				0: \
				(((f)->ic=='1')? 1: EOF)): \
			EOF )
#define putb_a(b,f) ( (f)->nb++, ((b))? putc('1',(f)->fp): putc('0',(f)->fp) )
#define bflush_a(f) {}

/* Format 0:  the low-order bit (0001) in each byte is first ("little-endian"), and
   bytes are in putc(3) order; */
#if MAIN
BITFILE *bopen_r_0(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"r"))!=NULL) {
		f->cm=0000;
		};
	return(f);
	}
#else
BITFILE *bopen_r_0();
#endif
#if MAIN
BITFILE *bopen_w_0(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"w"))!=NULL) {
		f->i.ss.s0.cc.c0=0000;
		f->cm=0001;
		};
	return(f);
	}
#else
BITFILE *bopen_w_0();
#endif
#define getb_0(f) ( ((f)->cm)? \
			( ((f)->cm&(f)->ic)? \
				((f)->cm<<=1,1): \
				((f)->cm<<=1,0) ): \
			( (((f)->ic=getc((f)->fp))==EOF)? \
				EOF: \
				( (f)->nb++, \
				  (f)->cm=0001, \
				  ((f)->cm&(f)->ic)? \
					((f)->cm<<=1,1): \
					((f)->cm<<=1,0) ) ) )
#define putb_0(b,f) { \
	if((b)) (f)->i.ss.s0.cc.c0 |= (f)->cm; \
	if( !((f)->cm<<=1) ) { \
		putc((f)->i.ss.s0.cc.c0,(f)->fp); \
		(f)->nb++; \
		(f)->i.ss.s0.cc.c0=0000; (f)->cm=0001; \
		}; \
	(f)->n++; \
	}
#define bflush_0(f) padb((f),0,8,0)

/* Format 1:  the high-order bit (0200) in each byte is first ("big-endian"), and
   bytes are in putc(3) order; */
#if MAIN
BITFILE *bopen_r_1(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"r"))!=NULL) {
		f->cm=0000;
		};
	return(f);
	}
#else
BITFILE *bopen_r_1();
#endif
#if MAIN
BITFILE *bopen_w_1(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"w"))!=NULL) {
		f->i.ss.s0.cc.c0=0000;
		f->cm=0200;
		};
	return(f);
	}
#else
BITFILE *bopen_w_1();
#endif
#define getb_1(f) ( ((f)->cm)? \
			( ((f)->cm&(f)->ic)? \
				((f)->cm>>=1,1): \
				((f)->cm>>=1,0) ): \
			( (((f)->ic=getc((f)->fp))==EOF)? \
				EOF: \
				( (f)->nb++, \
				  (f)->cm=0200, \
				  ((f)->cm&(f)->ic)? \
					((f)->cm>>=1,1): \
					((f)->cm>>=1,0) ) ) )
#define putb_1(b,f) { \
	if((b)) (f)->i.ss.s0.cc.c0 |= (f)->cm; \
	if( !((f)->cm>>=1) ) { \
		putc((f)->i.ss.s0.cc.c0,(f)->fp); \
		(f)->nb++; \
		(f)->i.ss.s0.cc.c0=0000; (f)->cm=0200; \
		}; \
	(f)->n++; \
	}
#define bflush_1(f) padb((f),0,8,0)

/* Format 10: the low-order (0001) bit in each byte is first ("little-endian"), and
   bytes are reversed (in each pair) from putc(3) order;
 */
#if MAIN
BITFILE *bopen_r_10(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"r"))!=NULL) {
		f->sm=0000000;
		};
	return(f);
	}
#else
BITFILE *bopen_r_10();
#endif
#if MAIN
BITFILE *bopen_w_10(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"w"))!=NULL) {
		f->i.ss.s0.s=0000000;
		f->sm=0000001;
		};
	return(f);
	}
#else
BITFILE *bopen_w_10();
#endif
#define getb_10(f) ( ((f)->sm)? \
			( ((f)->sm&(f)->i.ss.s0.s)? \
				((f)->sm<<=1,1): \
				((f)->sm<<=1,0) ): \
			( (((f)->ic=getc((f)->fp))==EOF)? \
				EOF: \
				( (f)->nb++, \
				  (f)->i.ss.s0.cc.c1=(f)->ic&0377, \
				  ( (((f)->ic=getc((f)->fp))==EOF)? \
					EOF: \
					( (f)->nb++, \
					  (f)->i.ss.s0.cc.c0=(f)->ic&0377, \
				  	  (f)->sm=0000001, \
				  	  ((f)->sm&(f)->i.ss.s0.s)? \
						((f)->sm<<=1,1): \
						((f)->sm<<=1,0) ) ) ) ) )
#define putb_10(b,f) { \
	if((b)) (f)->i.ss.s0.s |= (f)->sm; \
	if( !((f)->sm<<=1) ) { \
		putc((f)->i.ss.s0.cc.c1,(f)->fp); \
		(f)->nb++; \
		putc((f)->i.ss.s0.cc.c0,(f)->fp); \
		(f)->nb++; \
		(f)->i.ss.s0.s=0000000; (f)->sm=0000001; \
		}; \
	(f)->n++; \
	}
#define bflush_10(f) padb((f),0,16,0)

/* Format 11:  the high-order (0200) bit in each byte is first ("little-endian"),
   and bytes are reversed (in each pair) from putc(3) order.
 */
#if MAIN
BITFILE *bopen_r_11(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"r"))!=NULL) {
		f->sm=0000000;
		};
	return(f);
	}
#else
BITFILE *bopen_r_11();
#endif
#if MAIN
BITFILE *bopen_w_11(s)
    FILE *s;
{   BITFILE *f;
	if((f=bopen_rw(s,"w"))!=NULL) {
		f->i.ss.s0.s=0000000;
		f->sm=0100000;
		};
	return(f);
	}
#else
BITFILE *bopen_w_11();
#endif
#define getb_11(f) ( ((f)->sm)? \
			( ((f)->sm&(f)->i.ss.s0.s)? \
				((f)->sm>>=1,1): \
				((f)->sm>>=1,0) ): \
			( (((f)->ic=getc((f)->fp))==EOF)? \
				EOF: \
				( (f)->nb++, \
				  (f)->i.ss.s0.cc.c0=(f)->ic&0377, \
				  ( (((f)->ic=getc((f)->fp))==EOF)? \
					EOF: \
					( (f)->nb++, \
					  (f)->i.ss.s0.cc.c1=(f)->ic&0377, \
				  	  (f)->sm=0100000, \
				  	  ((f)->sm&(f)->i.ss.s0.s)? \
						((f)->sm>>=1,1): \
						((f)->sm>>=1,0) ) ) ) ) )
#define putb_11(b,f) { \
	if((b)) (f)->i.ss.s0.s |= (f)->sm; \
	if( !((f)->sm>>=1) ) { \
		putc((f)->i.ss.s0.cc.c0,(f)->fp); \
		(f)->nb++; \
		putc((f)->i.ss.s0.cc.c1,(f)->fp); \
		(f)->nb++; \
		(f)->i.ss.s0.s=0000000; (f)->sm=0100000; \
		}; \
	(f)->n++; \
	}
#define bflush_11(f) padb((f),0,16,0)

/**************************************************************/
/* FORMAT: may be selected here (input and output may differ) */

/* Input: */
#define bopen_r(s) bopen_r_0((s))
#define getb(f) getb_0(f)
/* Output: */
#define bopen_w(s) bopen_w_0((s))
#define putb(b,f) putb_0((b),(f))
#define bflush(f) bflush_0(f)

/**************************************************************/

/* Code common to all formats: */
#define bopen(s,t) ((*(t)=='r')? bopen_r((s)): ((*(t)=='w')? bopen_w((s)): (NULL)))
#define padb(f,b,B,l) { while(((f)->n+(l))%(B)) putb((b),(f)); }

#if MAIN
unsigned long bclose(f)
    BITFILE *f;
{   unsigned long nb;
	if(f->type=='w') { bflush(f); fflush(f->fp); };
	nb=f->nb;
	free(f);
	return(nb);
	}
#else
unsigned long bclose();
#endif
