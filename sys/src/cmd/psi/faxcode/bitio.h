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
/**************************************************************/
/* FORMAT: may be selected here (input and output may differ) */

/* Input: */
#define bopen_r(s) bopen_r_0((s))
#define getb(f) getb_0(f)

/* Output: changed 0 to 1 */
#ifndef SINGLE
#define w_1
#define bopen_w(s) bopen_w_1((s))
#define putb(b,f) putb_1((b),(f))
#define bflush(f) bflush_1(f)
#else
#define w_0
#define bopen_w(s) bopen_w_0((s))
#define putb(b,f) putb_0((b),(f))
#define bflush(f) bflush_0(f)
#endif
/**************************************************************/


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
	}BITFILE;
#define Init_BITFILE {NULL,'\0',0,0,0,0,0}
BITFILE *bopen_rw(FILE *, char *);
unsigned long mybclose(BITFILE *);

/* Code particular to each format: */

/* Format 0:  the low-order bit (0001) in each byte is first ("little-endian"), and
   bytes are in putc(3) order; */
#ifdef w_0

BITFILE *bopen_w_0(FILE *s)
{   BITFILE *f;
	if((f=bopen_rw(s,"w"))!=NULL) {
		f->i.ss.s0.cc.c0=0000;
		f->cm=0001;
		};
	return(f);
	}
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
#endif

/* Format 1:  the high-order bit (0200) in each byte is first ("big-endian"), and
   bytes are in putc(3) order; */

#ifdef w_1

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
#endif

/**************************************************************/

/* Code common to all formats: */
/*#define bopen(s,t) ((*(t)=='r')? bopen_r((s)): ((*(t)=='w')? bopen_w((s)): (NULL)))*/
#define bopen(s,t) bopen_w(s)
#define padb(f,b,B,l) { while(((f)->n+(l))%(B)) putb((b),(f)); }

unsigned long fwr_bm_g31(FILE *,Bitmap *);
void BOF_to_g31(BITFILE *);
void rlel_to_g31(RLE_Line *,int,BITFILE *);
void EOF_to_g31(BITFILE *);
