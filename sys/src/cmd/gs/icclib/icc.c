
/* 
 * International Color Consortium Format Library (icclib)
 * For ICC profile version 3.4
 *
 * Author:  Graeme W. Gill
 * Date:    2002/04/22
 * Version: 2.02
 *
 * Copyright 1997 - 2002 Graeme W. Gill
 * See Licence.txt file for conditions of use.
 */

/*
 * TTBD:
 *
 *      Add a "warning mode" to file reading, in which file format
 *      errors are ignored where possible, rather than generating
 *      a fatal error (see ICM_STRICT #define).
 *
 *      NameColor Dump doesn't handle device space correctly - 
 *	    should use appropriate interpretation in case device is Lab etc.
 *
 *      Should recognise & honour unicode 0xFFFE endian marker.
 *      Should generate it on writing too ?
 *
 *		Should fix all write_number failure errors to indicate failed value.
 *		(Partially implemented - need to check all write_number functions)
 *
 *		Make write fail error messages be specific on which element failed.
 *
 *		Should add named color space lookup function support.
 *
 *		Should probably reject reading or writing profiles with majv != 2 ?
 *
 *      Would be nice to add generic ability to add new tag type handling,
 *      so that the base library doesn't need to be modified (ie. VideoCardGamma) ?
 *
 *		Need to add DeviceSettings and OutputResponse tags to bring up to
 *		ICC.1:1998-09 [started but not complete]
 *
 */

#undef ICM_STRICT	/* Not fully implimented - switch off strict checking of file format */

/* Trial: Make the default grid points of the Lab clut be symetrical about */
/*        a/b 0.0, and also make L = 100.0 fall on a grid point. */
/*        This seems a good idea. */

#define SYMETRICAL_DEFAULT_LAB_RANGE

/*
 * Change History:
 * 
 * 2.02
 *      Merged rename of [u]int64 to icm[Ui][I]nt64 (to work around
 *      AIX 5.1L portability bug) from Raph Levien.
 *
 *      Fixed stray , in icmLookupOrder structure definition (from Dan Coby)
 *
 * 2.01
 *		Change TextDescription code to not barf if #undef ICM_STRICT and
 *      Apple scriptcode not padded to 67 bytes.
 *
 *      Add get_ranges() method to all Lu types, not just LuLut.
 *      Fix bug in PCS overide logic that was causing
 *		reverse conversions to apply the wrong conversion.
 *
 *      Added Delta E convenience functions icmLabDE() and
 *      icmCIE94() etc.
 *
 *		Merged Raph Levien's cleanups, to quiet gcc warnings.
 *
 *      Merged another couple of warning cleanups from Jouk Jansen.
 *
 * 2.00
 *      Change absolute conversion to be white point only, and use
 *      Bradford transform by default. (ie. we are now ignoring the
 *      comment in section 6.4.22 of the 1998 spec. about the
 *      media black point being used for absolute colorimetry,
 *      ignoring the recommendation on page 118 in section E.5,
 *      and are taking up the recommendation on page 124 in section
 *      E.16 that a more sophisticated chromatic adaption model be used.)
 *
 *      This is for better compatibility with other CMM's, and to
 *      improve the results when using simple links between
 *      profiles with non-D50 white points. Standard profiles
 *      like sRGB will also be more accurate when interpreted
 *      with absolute colorimetric intent.
 *      This will cause some slight incompatibilty with previous
 *      versions of icclib.
 *
 *      Added ColorSync 2.5 specific VideoCardGamma tag support
 *      (from Neil Okamoto)
 *
 * 1.31
 *      Added file I/O class to allow substitution of alternative ICC profile
 *      file access. Provide standard file class instance, and memory image
 *      instance of file I/O class as default and example. 
 *      Added an optional new_icc_a() object creator, that takes a memory
 *      allocator class instance. This allows an alternate memory heap to
 *      be used with the icc class. 
 *      Renamed object free() methods to del() for more consistency with new().
 *
 * 1.30	
 *      Added workaround for reading some Adobe profiles with confused header DateTime.
 *      Enhanced tag allocate() methods so that they can resize allocations.
 *      Enhanced icmLut_set_tables() to access grid points in a cache friendly order.
 *      Fixed bug in check_icc_legal() that caused bogus errors, removed
 *      uneccessary static declarations in icc.h, and fixed a bug in
 *      icmTable_lookup_bwd() that effected both accuracy and speed. (Thanks to Andrei Frolov)
 *      Removed icmAbsoluteColorimetricXYZ intent, and replaced it with
 *      a PCS overide capability. This adds a new parameter to get_luobj() 
 *      Added Lab translations of some XYZ "dump" strings.
 *      Fix memory leak after failed tag read + rename_tag function
 *      + shared library support changes. (Thanks to Carles Llopis).
 *		Changed all the public 2str utility routines to a single function
 *      that can be used to interpret an enumeration or tag in a human
 *      readable form. 
 *
 * 1.23	
 *      Fixed important bug in Lut read/write. The matrix values had their
 *      rows and columns switched. Not many profiles exercise this code.
 *      Thanks to David Gillman for discovering this problem.
 *      Fixup compiler complains about illegal enum values for icmCurveStyle,
 *      and icmDataStyle. Malloc memory icmLut_lookup_clut_nl for gw[], so that
 *      it is more friendly to systems with a limited stack. (Thanks to Dave White)
 *
 * 1.22	99/11/11 Snapshot of current code.
 *      Added more hooks to support inherited implementation of
 *      color conversion, used in Argyll to support reversing
 *      multi-dimentional table lookups.
 *      Cleaned up color conversion code to make it easier to follow.
 *      Adding simplex interpolation for non-Lab style input space interpolation.
 *      Fix Sun misalignment and realloc problems (Thanks to Allan N. Hessenflow)
 *      Fixed endian problem with Unicode on read and write.
 *      Expanded icmTextDescription_dump() to do hex dump of Unicode and ScriptCode.
 *      Changed over to ICC.1:1998-09 .h file.
 *      Started implementing ICC.1:1998-09, but not complete yet!
 *
 * 1.21	99/2/14
 *     	After re-reading Michael Bourgoin's 1998 SIGGRAPH notes,
 *      I have consolidated the Lut input index, and table value encodings.
 *      The default set_tables() scaling has been adjusted appropriately
 *      for this correction of Lab encoding.
 *      Trying to create an 8 bit XYZ Lut will now fail if icclib helper
 *      functions are used to create it.
 * 
 * 1.20	99/2/7
 *      Added profile color lookup functon.
 *      Added set_tables() support.
 *      Various bug fixes and enhancements.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#ifdef __sun
#include <unistd.h>
#endif
#if defined(__IBMC__) && defined(_M_IX86)
#include <float.h>
#endif
#include "icc.h"

/* ========================================================== */
/* Default system interface object implementations */

/* Standard Stream file I/O icmFile compatible class */
/* Note that this uses malloc, so replace class if */
/* you need a different memory allocator. */

/* Set current position to offset. Return 0 on success, nz on failure. */
static int icmFileStd_seek(
icmFile *pp,
long int offset
) {
	icmFileStd *p = (icmFileStd *)pp;

	return fseek(p->fp, offset, SEEK_SET);
}

/* Read count items of size length. Return number of items successfully read. */
static size_t icmFileStd_read(
icmFile *pp,
void *buffer,
size_t size,
size_t count
) {
	icmFileStd *p = (icmFileStd *)pp;

	return fread(buffer, size, count, p->fp);
}

/* write count items of size length. Return number of items successfully written. */
static size_t icmFileStd_write(
icmFile *pp,
void *buffer,
size_t size,
size_t count
) {
	icmFileStd *p = (icmFileStd *)pp;

	return fwrite(buffer, size, count, p->fp);
}


/* flush all write data out to secondary storage. Return nz on failure. */
static int icmFileStd_flush(
icmFile *pp
) {
	icmFileStd *p = (icmFileStd *)pp;

	return fflush(p->fp);
}

/* we're done with the file object, return nz on failure */
static int icmFileStd_delete(
icmFile *pp
) {
	icmFileStd *p = (icmFileStd *)pp;

	if (p->doclose != 0) {
		if (fclose(p->fp) != 0)
			return 2;
	}

	free(p);
	return 0;
}

/* Create icmFile given a (binary) FILE* */
icmFile *new_icmFileStd_fp(
FILE *fp
) {
	icmFileStd *p;
	if ((p = (icmFileStd *) calloc(1,sizeof(icmFileStd))) == NULL)
		return NULL;
	p->seek  = icmFileStd_seek;
	p->read  = icmFileStd_read;
	p->write = icmFileStd_write;
	p->flush = icmFileStd_flush;
	p->del   = icmFileStd_delete;

	p->fp = fp;
	p->doclose = 0;

	return (icmFile *)p;
}

/* Create icmFile given a file name */
icmFile *new_icmFileStd_name(
char *name,
char *mode
) {
	FILE *fp;
	icmFile *p;
#if defined(O_BINARY)
	char nmode[50];
#endif

	if ((fp = fopen(name,mode)) == NULL)
		return NULL;

#if defined(O_BINARY)
	strcpy(nmode, mode);
	strcat(nmode, "b");
	if ((fp = freopen(name, nmode, fp)) == NULL)
		return NULL;
#endif
	
	p = new_icmFileStd_fp(fp);

	if (p != NULL) {
		icmFileStd *pp = (icmFileStd *)p;
		pp->doclose = 1;
	}
	return p;
}

/* ------------------------------------------------- */
/* Memory image icmFile compatible class */
/* Note that this uses malloc, so replace class if */
/* you need a different memory allocator. */

/* Set current position to offset. Return 0 on success, nz on failure. */
static int icmFileMem_seek(
icmFile *pp,
long int offset
) {
	icmFileMem *p = (icmFileMem *)pp;
	unsigned char *np;

	np = p->start + offset;
	if (np < p->start || np >= p->end)
		return 1;
	p->cur = np;
	return 0;
}

/* Read count items of size length. Return number of items successfully read. */
static size_t icmFileMem_read(
icmFile *pp,
void *buffer,
size_t size,
size_t count
) {
	icmFileMem *p = (icmFileMem *)pp;
	size_t len;

	len = size * count;
	if ((p->cur + len) >= p->end) {		/* Too much */
		if (size > 0)
			count = (p->end - p->cur)/size;
		else
			count = 0;
	}
	len = size * count;
	if (len > 0)
		memcpy (buffer, p->cur, len);
	p->cur += len;
	return count;
}

/* write count items of size length. Return number of items successfully written. */
static size_t icmFileMem_write(
icmFile *pp,
void *buffer,
size_t size,
size_t count
) {
	icmFileMem *p = (icmFileMem *)pp;
	size_t len;

	len = size * count;
	if ((p->cur + len) >= p->end) {		/* Too much */
		if (size > 0)
			count = (p->end - p->cur)/size;
		else
			count = 0;
	}
	len = size * count;
	if (len > 0)
		memcpy (p->cur, buffer, len);
	p->cur += len;
	return count;
}


/* flush all write data out to secondary storage. Return nz on failure. */
static int icmFileMem_flush(
icmFile *pp
) {
	return 0;
}

/* we're done with the file object, return nz on failure */
static int icmFileMem_delete(
icmFile *pp
) {
	icmFileMem *p = (icmFileMem *)pp;

	free(p);
	return 0;
}

/* Create a memory image file access class */
icmFile *new_icmFileMem(
void *base,				/* Pointer to base of memory buffer */
size_t length			/* Number of bytes in buffer */
) {
	icmFileMem *p;
	if ((p = (icmFileMem *) calloc(1,sizeof(icmFileMem))) == NULL)
		return NULL;
	p->seek  = icmFileMem_seek;
	p->read  = icmFileMem_read;
	p->write = icmFileMem_write;
	p->flush = icmFileMem_flush;
	p->del   = icmFileMem_delete;

	p->cur = p->start = base;
	p->end = p->start + length;

	return (icmFile *)p;
}

/* ------------------------------------------------- */
/* Standard Heap allocator icmAlloc compatible class */
/* Just call the standard system function */

static void *icmAllocStd_malloc(
struct _icmAlloc *pp,
size_t size
) {
	return malloc(size);
}

static void *icmAllocStd_calloc(
struct _icmAlloc *pp,
size_t num,
size_t size
) {
	return calloc(num, size);
}

static void *icmAllocStd_realloc(
struct _icmAlloc *pp,
void *ptr,
size_t size
) {
	return realloc(ptr, size);
}


static void icmAllocStd_free(
struct _icmAlloc *pp,
void *ptr
) {
	free(ptr);
}

/* we're done with the AllocStd object */
static void icmAllocStd_delete(
icmAlloc *pp
) {
	icmAllocStd *p = (icmAllocStd *)pp;

	free(p);
}

/* Create icmAllocStd */
icmAlloc *new_icmAllocStd() {
	icmAllocStd *p;
	if ((p = (icmAllocStd *) calloc(1,sizeof(icmAllocStd))) == NULL)
		return NULL;
	p->malloc  = icmAllocStd_malloc;
	p->calloc  = icmAllocStd_calloc;
	p->realloc = icmAllocStd_realloc;
	p->free    = icmAllocStd_free;
	p->del     = icmAllocStd_delete;

	return (icmAlloc *)p;
}

/* ========================================================== */
/* Conversion support functions */
/* Convert between ICC storage types and native C types */
/* Write routine return non-zero if numbers can't be represented */

/* Unsigned */
static unsigned int read_UInt8Number(char *p) {
	unsigned int rv;
	rv = (unsigned int)((ORD8 *)p)[0];
	return rv;
}

static int write_UInt8Number(unsigned int d, char *p) {
	if (d > 255)
		return 1;
	((ORD8 *)p)[0] = (ORD8)d;
	return 0;
}

static unsigned int read_UInt16Number(char *p) {
	unsigned int rv;
	rv = 256 * (unsigned int)((ORD8 *)p)[0]
	   +       (unsigned int)((ORD8 *)p)[1];
	return rv;
}

static int write_UInt16Number(unsigned int d, char *p) {
	if (d > 65535)
		return 1;
	((ORD8 *)p)[0] = (ORD8)(d >> 8);
	((ORD8 *)p)[1] = (ORD8)(d);
	return 0;
}

static unsigned int read_UInt32Number(char *p) {
	unsigned int rv;
	rv = 16777216 * (unsigned int)((ORD8 *)p)[0]
	   +    65536 * (unsigned int)((ORD8 *)p)[1]
	   +      256 * (unsigned int)((ORD8 *)p)[2]
	   +            (unsigned int)((ORD8 *)p)[3];
	return rv;
}

static int write_UInt32Number(unsigned int d, char *p) {
	((ORD8 *)p)[0] = (ORD8)(d >> 24);
	((ORD8 *)p)[1] = (ORD8)(d >> 16);
	((ORD8 *)p)[2] = (ORD8)(d >> 8);
	((ORD8 *)p)[3] = (ORD8)(d);
	return 0;
}

static void read_UInt64Number(icmUint64 *d, char *p) {
	d->h = 16777216 * (unsigned int)((ORD8 *)p)[0]
	     +    65536 * (unsigned int)((ORD8 *)p)[1]
	     +      256 * (unsigned int)((ORD8 *)p)[2]
	     +            (unsigned int)((ORD8 *)p)[3];
	d->l = 16777216 * (unsigned int)((ORD8 *)p)[4]
	     +    65536 * (unsigned int)((ORD8 *)p)[5]
	     +      256 * (unsigned int)((ORD8 *)p)[6]
	     +            (unsigned int)((ORD8 *)p)[7];
}

static int write_UInt64Number(icmUint64 *d, char *p) {
	((ORD8 *)p)[0] = (ORD8)(d->h >> 24);
	((ORD8 *)p)[1] = (ORD8)(d->h >> 16);
	((ORD8 *)p)[2] = (ORD8)(d->h >> 8);
	((ORD8 *)p)[3] = (ORD8)(d->h);
	((ORD8 *)p)[4] = (ORD8)(d->l >> 24);
	((ORD8 *)p)[5] = (ORD8)(d->l >> 16);
	((ORD8 *)p)[6] = (ORD8)(d->l >> 8);
	((ORD8 *)p)[7] = (ORD8)(d->l);
	return 0;
}

static double read_U8Fixed8Number(char *p) {
	ORD32 o32;
	o32 = 256 * (ORD32)((ORD8 *)p)[0]		/* Read big endian 16 bit unsigned */
        +       (ORD32)((ORD8 *)p)[1];
	return (double)o32/256.0;
}

static int write_U8Fixed8Number(double d, char *p) {
	ORD32 o32;
	d = d * 256.0 + 0.5;
	if (d >= 65536.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)((o32) >> 8);
	((ORD8 *)p)[1] = (ORD8)((o32));
	return 0;
}

static double read_U16Fixed16Number(char *p) {
	ORD32 o32;
	o32 = 16777216 * (ORD32)((ORD8 *)p)[0]		/* Read big endian 32 bit unsigned */
        +    65536 * (ORD32)((ORD8 *)p)[1]
	    +      256 * (ORD32)((ORD8 *)p)[2]
	    +            (ORD32)((ORD8 *)p)[3];
	return (double)o32/65536.0;
}

static int write_U16Fixed16Number(double d, char *p) {
	ORD32 o32;
	d = d * 65536.0 + 0.5;
	if (d >= 4294967296.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)((o32) >> 24);
	((ORD8 *)p)[1] = (ORD8)((o32) >> 16);
	((ORD8 *)p)[2] = (ORD8)((o32) >> 8);
	((ORD8 *)p)[3] = (ORD8)((o32));
	return 0;
}


#ifdef NEVER	/* Not currently used anywhere */

/* Signed numbers */
static int read_SInt8Number(char *p) {
	int rv;
	rv = (int)((INR8 *)p)[0];
	return rv;
}

static int write_SInt8Number(int d, char *p) {
	if (d > 127)
		return 1;
	else if (d < -128)
		return 1;
	((INR8 *)p)[0] = (INR8)d;
	return 0;
}

static int read_SInt16Number(char *p) {
	int rv;
	rv = 256 * (int)((INR8 *)p)[0]
	   +       (int)((ORD8 *)p)[1];
	return rv;
}

static int write_SInt16Number(int d, char *p) {
	if (d > 32767)
		return 1;
	else if (d < -32768)
		return 1;
	((INR8 *)p)[0] = (INR8)(d >> 8);
	((ORD8 *)p)[1] = (ORD8)(d);
	return 0;
}

#endif /* NEVER */

static int read_SInt32Number(char *p) {
	int rv;
	rv = 16777216 * (int)((INR8 *)p)[0]
	   +    65536 * (int)((ORD8 *)p)[1]
	   +      256 * (int)((ORD8 *)p)[2]
	   +            (int)((ORD8 *)p)[3];
	return rv;
}

static int write_SInt32Number(int d, char *p) {
	((INR8 *)p)[0] = (INR8)(d >> 24);
	((ORD8 *)p)[1] = (ORD8)(d >> 16);
	((ORD8 *)p)[2] = (ORD8)(d >> 8);
	((ORD8 *)p)[3] = (ORD8)(d);
	return 0;
}

#ifdef NEVER /* Not currently used anywhere */

static void read_SInt64Number(icmInt64 *d, char *p) {
	d->h = 16777216 * (int)((INR8 *)p)[0]
	     +    65536 * (int)((ORD8 *)p)[1]
	     +      256 * (int)((ORD8 *)p)[2]
	     +            (int)((ORD8 *)p)[3];
	d->l = 16777216 * (unsigned int)((ORD8 *)p)[4]
	     +    65536 * (unsigned int)((ORD8 *)p)[5]
	     +      256 * (unsigned int)((ORD8 *)p)[6]
	     +            (unsigned int)((ORD8 *)p)[7];
}

static int write_SInt64Number(icmInt64 *d, char *p) {
	((INR8 *)p)[0] = (INR8)(d->h >> 24);
	((ORD8 *)p)[1] = (ORD8)(d->h >> 16);
	((ORD8 *)p)[2] = (ORD8)(d->h >> 8);
	((ORD8 *)p)[3] = (ORD8)(d->h);
	((ORD8 *)p)[4] = (ORD8)(d->l >> 24);
	((ORD8 *)p)[5] = (ORD8)(d->l >> 16);
	((ORD8 *)p)[6] = (ORD8)(d->l >> 8);
	((ORD8 *)p)[7] = (ORD8)(d->l);
	return 0;
}

#endif /* NEVER */

static double read_S15Fixed16Number(char *p) {
	INR32 i32;
	i32 = 16777216 * (INR32)((INR8 *)p)[0]		/* Read big endian 32 bit signed */
        +    65536 * (INR32)((ORD8 *)p)[1]
	    +      256 * (INR32)((ORD8 *)p)[2]
	    +            (INR32)((ORD8 *)p)[3];
	return (double)i32/65536.0;
}

static int write_S15Fixed16Number(double d, char *p) {
	INR32 i32;
	d = ceil(d * 65536.0);		/* Beware! (int)(d + 0.5) doesn't work! */
	if (d >= 2147483648.0)
		return 1;
	if (d < -2147483648.0)
		return 1;
	i32 = (INR32)d;
	((INR8 *)p)[0] = (INR8)((i32) >> 24);		/* Write big endian 32 bit signed */
	((ORD8 *)p)[1] = (ORD8)((i32) >> 16);
	((ORD8 *)p)[2] = (ORD8)((i32) >> 8);
	((ORD8 *)p)[3] = (ORD8)((i32));
	return 0;
}

/* PCS encoded numbers */

/* 16 bit XYZ  - value range 0.0 - 1.9997 */
static double read_PCSXYZ16Number(char *p) {
	ORD32 o32;
	o32 = 256 * (ORD32)((ORD8 *)p)[0]		/* Read big endian 16 bit unsigned */
        +       (ORD32)((ORD8 *)p)[1];
	return (double)o32/32768.0;
}

static int write_PCSXYZ16Number(double d, char *p) {
	ORD32 o32;
	d = d * 32768.0 + 0.5;
	if (d >= 65536.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)((o32) >> 8);
	((ORD8 *)p)[1] = (ORD8)((o32));
	return 0;
}

#ifdef NEVER /* Not currently used */

/* L part of 8 bit Lab - value range 0.0 - 100.0 */
static double read_PCSL8Number(char *p) {
	ORD32 o32;
	o32 = (ORD32)((ORD8 *)p)[0];		/* Read big endian 8 bit unsigned */
	return (double)o32/2.550;
}

static int write_PCSL8Number(double d, char *p) {
	ORD32 o32;
	d = d * 2.550 + 0.5;
	if (d >= 256.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)((o32));
	return 0;
}

/* ab part of 8 bit Lab - value range -128.0 - 127.0 */
static double read_PCSab8Number(char *p) {
	ORD32 o32;
	o32 = (ORD32)((ORD8 *)p)[0];	/* Read big endian 8 bit unsigned */
	return (double)o32-128.0;
}

static int write_PCSab8Number(double d, char *p) {
	ORD32 o32;
	d = (d+128.0) + 0.5;
	if (d >= 256.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)((o32));
	return 0;
}

#endif /* NEVER */

/* L part of 16 bit Lab - value range 0.0 - 100.0 */
static double read_PCSL16Number(char *p) {
	ORD32 o32;
	o32 = 256 * (ORD32)((ORD8 *)p)[0]		/* Read big endian 16 bit unsigned */
        +       (ORD32)((ORD8 *)p)[1];
	return (double)o32/652.800;				/* 0xff00/100.0 */
}

static int write_PCSL16Number(double d, char *p) {
	ORD32 o32;
	d = d * 652.800 + 0.5;
	if (d >= 65536.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)((o32) >> 8);
	((ORD8 *)p)[1] = (ORD8)((o32));
	return 0;
}

/* ab part of 16 bit Lab - value range -128.0 - 127.9961 */
static double read_PCSab16Number(char *p) {
	ORD32 o32;
	o32 = 256 * (ORD32)((ORD8 *)p)[0]		/* Read big endian 16 bit unsigned */
        +       (ORD32)((ORD8 *)p)[1];
	return ((double)o32/256.0)-128.0;
}

static int write_PCSab16Number(double d, char *p) {
	ORD32 o32;
	d = (d+128.0) * 256.0 + 0.5;
	if (d >= 65536.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)((o32) >> 8);
	((ORD8 *)p)[1] = (ORD8)((o32));
	return 0;
}

/* Device coordinate as 8 bit value range 0.0 - 1.0 */
static double read_DCS8Number(char *p) {
	unsigned int rv;
	rv =   (unsigned int)((ORD8 *)p)[0];
	return (double)rv/255.0;
}

static int write_DCS8Number(double d, char *p) {
	ORD32 o32;
	d = d * 255.0 + 0.5;
	if (d >= 256.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)(o32);
	return 0;
}

/* Device coordinate as 16 bit value range 0.0 - 1.0 */
static double read_DCS16Number(char *p) {
	unsigned int rv;
	rv = 256 * (unsigned int)((ORD8 *)p)[0]
	   +       (unsigned int)((ORD8 *)p)[1];
	return (double)rv/65535.0;
}

static int write_DCS16Number(double d, char *p) {
	ORD32 o32;
	d = d * 65535.0 + 0.5;
	if (d >= 65536.0)
		return 1;
	if (d < 0.0)
		return 1;
	o32 = (ORD32)d;
	((ORD8 *)p)[0] = (ORD8)(o32 >> 8);
	((ORD8 *)p)[1] = (ORD8)(o32);
	return 0;
}

/* ---------------------------------------------------------- */
/* Auiliary function - return a string that represents a tag */
/* Note - returned buffers are static, can only be used 5 */
/* times before buffers get reused. */
char *tag2str(
	int tag
) {
	int i;
	static int si = 0;			/* String buffer index */
	static char buf[5][20];		/* String buffers */
	char *bp;
	unsigned char c[4];

	bp = buf[si++];
	si %= 5;				/* Rotate through buffers */

	c[0] = 0xff & (tag >> 24);
	c[1] = 0xff & (tag >> 16);
	c[2] = 0xff & (tag >> 8);
	c[3] = 0xff & (tag >> 0);
	for (i = 0; i < 4; i++) {	/* Can we represent it as a string ? */
		if (!isprint(c[i]))
			break;
	}
	if (i < 4) {	/* Not printable - use hex */
		sprintf(bp,"0x%x",tag);
	} else {		/* Printable */
		sprintf(bp,"'%c%c%c%c'",c[0],c[1],c[2],c[3]);
	}
	return bp;
}

/* Auiliary function - return a tag created from a string */
int str2tag(
	const char *str
) {
	unsigned long tag;
	tag = (((unsigned long)str[0]) << 24)
	    + (((unsigned long)str[1]) << 16)
	    + (((unsigned long)str[2]) << 8)
	    + (((unsigned long)str[3]));
	return (int)tag;
}

/* helper - return 1 if the string doesn't have a */
/*  null terminator, return 0 if it does. */
/* Note: will return 1 if len == 0 */
static int check_null_string(char *cp, int len) {
	for (; len > 0; len--) {
		if (*cp++ == '\000')
			break;
	}
	if (len == 0)
		return 1;
	return 0;
}

/* helper - return 1 if the string doesn't have a */
/*  null terminator, return 0 if it does. */
/* Note: will return 1 if len == 0 */
/* Unicode version */
static int check_null_string16(char *cp, int len) {
	for (; len > 0; len--) {	/* Length is in characters */
		if (cp[0] == 0 && cp[1] == 0)
			break;
		cp += 2;
	}
	if (len == 0) 
		return 1;
	return 0;
}

/* Color Space to number of component conversion */
/* Return 0 on error */
static unsigned int number_ColorSpaceSignature(icColorSpaceSignature sig) {
	switch(sig) {
		case icSigXYZData:
			return 3;
		case icSigLabData:
			return 3;
		case icSigLuvData:
			return 3;
		case icSigYCbCrData:
			return 3;
		case icSigYxyData:
			return 3;
		case icSigRgbData:
			return 3;
		case icSigGrayData:
			return 1;
		case icSigHsvData:
			return 3;
		case icSigHlsData:
			return 3;
		case icSigCmykData:
			return 4;
		case icSigCmyData:
			return 3;
		case icSig2colorData:
			return 2;
		case icSig3colorData:
			return 3;
		case icSig4colorData:
			return 4;
		case icSig5colorData:
		case icSigMch5Data:
			return 5;
		case icSig6colorData:
		case icSigMch6Data:
			return 6;
		case icSig7colorData:
		case icSigMch7Data:
			return 7;
		case icSig8colorData:
		case icSigMch8Data:
			return 8;
		case icSig9colorData:
			return 9;
		case icSig10colorData:
			return 10;
		case icSig11colorData:
			return 11;
		case icSig12colorData:
			return 12;
		case icSig13colorData:
			return 13;
		case icSig14colorData:
			return 14;
		case icSig15colorData:
			return 15;
		default:
			return 0;
	}
}

/* ------------------------------------------------------- */
/* Flag dump functions */
/* Note - returned buffers are static, can only be used 5 */
/* times before buffers get reused. */

/* Screening Encodings */
static char *string_ScreenEncodings(unsigned long flags) {
	static int si = 0;			/* String buffer index */
	static char buf[5][80];		/* String buffers */
	char *bp, *cp;

	cp = bp = buf[si++];
	si %= 5;				/* Rotate through buffers */

	if (flags & icPrtrDefaultScreensTrue) {
		sprintf(cp,"Default Screen");
	} else {
		sprintf(cp,"No Default Screen");
	}
	cp = cp + strlen(cp);
	if (flags & icLinesPerInch) {
		sprintf(cp,", Lines Per Inch");
	} else {
		sprintf(cp,", Lines Per cm");
	}
	cp = cp + strlen(cp);

	return bp;
}

/* Device attributes */
static char *string_DeviceAttributes(unsigned long flags) {
	static int si = 0;			/* String buffer index */
	static char buf[5][80];		/* String buffers */
	char *bp, *cp;

	cp = bp = buf[si++];
	si %= 5;				/* Rotate through buffers */

	if (flags & icTransparency) {
		sprintf(cp,"Transparency");
	} else {
		sprintf(cp,"Reflective");
	}
	cp = cp + strlen(cp);
	if (flags & icMatte) {
		sprintf(cp,", Matte");
	} else {
		sprintf(cp,", Glossy");
	}
	cp = cp + strlen(cp);

	return bp;
}

/* Profile header flags */
static char *string_ProfileHeaderFlags(unsigned long flags) {
	static int si = 0;			/* String buffer index */
	static char buf[5][80];		/* String buffers */
	char *bp, *cp;

	cp = bp = buf[si++];
	si %= 5;				/* Rotate through buffers */

	if (flags & icEmbeddedProfileTrue) {
		sprintf(cp,"Embedded Profile");
	} else {
		sprintf(cp,"Not Embedded Profile");
	}
	cp = cp + strlen(cp);
	if (flags & icUseWithEmbeddedDataOnly) {
		sprintf(cp,", Use with embedded data only");
	} else {
		sprintf(cp,", Use anywhere");
	}
	cp = cp + strlen(cp);

	return bp;
}


static char *string_AsciiOrBinaryData(unsigned long flags) {
	static int si = 0;			/* String buffer index */
	static char buf[5][80];		/* String buffers */
	char *bp, *cp;

	cp = bp = buf[si++];
	si %= 5;				/* Rotate through buffers */

	if (flags & icBinaryData) {
		sprintf(cp,"Binary");
	} else {
		sprintf(cp,"Ascii");
	}
	cp = cp + strlen(cp);

	return bp;
}

/* ------------------------------------------------------------ */
/* Enumeration dump functions */
/* Note - returned buffers are static, can only be used once */
/* before buffers get reused if type is unknown. */

/* public tags and sizes */
static const char *string_TagSignature(icTagSignature sig) {
	static char buf[80];
	switch(sig) {
		case icSigAToB0Tag:
			return "AToB0 Multidimentional Transform";
		case icSigAToB1Tag:
			return "AToB1 Multidimentional Transform";
		case icSigAToB2Tag:
			return "AToB2 Multidimentional Transform";
		case icSigBlueColorantTag:
			return "Blue Colorant";
		case icSigBlueTRCTag:
			return "Blue Tone Reproduction Curve";
		case icSigBToA0Tag:
			return "BToA0 Multidimentional Transform";
		case icSigBToA1Tag:
			return "BToA1 Multidimentional Transform";
		case icSigBToA2Tag:
			return "BToA2 Multidimentional Transform";
		case icSigCalibrationDateTimeTag:
			return "Calibration Date & Time";
		case icSigCharTargetTag:
			return "Characterization Target";
		case icSigCopyrightTag:
			return "Copyright";
		case icSigCrdInfoTag:
			return "CRD Info";
		case icSigDeviceMfgDescTag:
			return "Device Manufacturer Description";
		case icSigDeviceModelDescTag:
			return "Device Model Description";
		case icSigGamutTag:
			return "Gamut";
		case icSigGrayTRCTag:
			return "Gray Tone Reproduction Curve";
		case icSigGreenColorantTag:
			return "Green Colorant";
		case icSigGreenTRCTag:
			return "Green Tone Reproduction Curve";
		case icSigLuminanceTag:
			return "Luminance";
		case icSigMeasurementTag:
			return "Measurement";
		case icSigMediaBlackPointTag:
			return "Media Black Point";
		case icSigMediaWhitePointTag:
			return "Media White Point";
		case icSigNamedColorTag:
			return "Named Color";
		case icSigNamedColor2Tag:
			return "Named Color 2";
		case icSigPreview0Tag:
			return "Preview0";
		case icSigPreview1Tag:
			return "Preview1";
		case icSigPreview2Tag:
			return "Preview2";
		case icSigProfileDescriptionTag:
			return "Profile Description";
		case icSigProfileSequenceDescTag:
			return "Profile Sequence";
		case icSigPs2CRD0Tag:
			return "PS Level 2 CRD perceptual";
		case icSigPs2CRD1Tag:
			return "PS Level 2 CRD colorimetric";
		case icSigPs2CRD2Tag:
			return "PS Level 2 CRD saturation";
		case icSigPs2CRD3Tag:
			return "PS Level 2 CRD absolute";
		case icSigPs2CSATag:
			return "PS Level 2 color space array";
		case icSigPs2RenderingIntentTag:
			return "PS Level 2 Rendering Intent";
		case icSigRedColorantTag:
			return "Red Colorant";
		case icSigRedTRCTag:
			return "Red Tone Reproduction Curve";
		case icSigScreeningDescTag:
			return "Screening Description";
		case icSigScreeningTag:
			return "Screening Attributes";
		case icSigTechnologyTag:
			return "Device Technology";
		case icSigUcrBgTag:
			return "Under Color Removal & Black Generation";
		case icSigVideoCardGammaTag:
			return "Video Card Gamma Curve";
		case icSigViewingCondDescTag:
			return "Viewing Condition Description";
		case icSigViewingConditionsTag:
			return "Viewing Condition Paramaters";
		default:
			sprintf(buf,"Unrecognized - %s",tag2str(sig));
			return buf;
	}
}

/* technology signature descriptions */
static const char *string_TechnologySignature(icTechnologySignature sig) {
	static char buf[80];
	switch(sig) {
		case icSigDigitalCamera:
			return "Digital Camera";
		case icSigFilmScanner:
			return "Film Scanner";
		case icSigReflectiveScanner:
			return "Reflective Scanner";
		case icSigInkJetPrinter:
			return "InkJet Printer";
		case icSigThermalWaxPrinter:
			return "Thermal WaxPrinter";
		case icSigElectrophotographicPrinter:
			return "Electrophotographic Printer";
		case icSigElectrostaticPrinter:
			return "Electrostatic Printer";
		case icSigDyeSublimationPrinter:
			return "DyeSublimation Printer";
		case icSigPhotographicPaperPrinter:
			return "Photographic Paper Printer";
		case icSigFilmWriter:
			return "Film Writer";
		case icSigVideoMonitor:
			return "Video Monitor";
		case icSigVideoCamera:
			return "Video Camera";
		case icSigProjectionTelevision:
			return "Projection Television";
		case icSigCRTDisplay:
			return "Cathode Ray Tube Display";
		case icSigPMDisplay:
			return "Passive Matrix Display";
		case icSigAMDisplay:
			return "Active Matrix Display";
		case icSigPhotoCD:
			return "Photo CD";
		case icSigPhotoImageSetter:
			return "Photo ImageSetter";
		case icSigGravure:
			return "Gravure";
		case icSigOffsetLithography:
			return "Offset Lithography";
		case icSigSilkscreen:
			return "Silkscreen";
		case icSigFlexography:
			return "Flexography";
		default:
			sprintf(buf,"Unrecognized - %s",tag2str(sig));
			return buf;
	}
}

/* type signatures */
static const char *string_TypeSignature(icTagTypeSignature sig) {
	static char buf[80];
	switch(sig) {
		case icSigCurveType:
			return "Curve";
		case icSigDataType:
			return "Data";
		case icSigDateTimeType:
			return "DateTime";
		case icSigLut16Type:
			return "Lut16";
		case icSigLut8Type:
			return "Lut8";
		case icSigMeasurementType:
			return "Measurement";
		case icSigNamedColorType:
			return "Named Color";
		case icSigProfileSequenceDescType:
			return "Profile Sequence Desc";
		case icSigS15Fixed16ArrayType:
			return "S15Fixed16 Array";
		case icSigScreeningType:
			return "Screening";
		case icSigSignatureType:
			return "Signature";
		case icSigTextType:
			return "Text";
		case icSigTextDescriptionType:
			return "Text Description";
		case icSigU16Fixed16ArrayType:
			return "U16Fixed16 Array";
		case icSigUcrBgType:
			return "Under Color Removal & Black Generation";
		case icSigUInt16ArrayType:
			return "UInt16 Array";
		case icSigUInt32ArrayType:
			return "UInt32 Array";
		case icSigUInt64ArrayType:
			return "UInt64 Array";
		case icSigUInt8ArrayType:
			return "UInt8 Array";
		case icSigVideoCardGammaType:
			return "Video Card Gamma";
		case icSigViewingConditionsType:
			return "Viewing Conditions";
		case icSigXYZType:
			return "XYZ (Array?)";
		case icSigNamedColor2Type:
			return "Named Color 2";
		case icSigCrdInfoType:
			return "CRD Info";
		default:
			sprintf(buf,"Unrecognized - %s",tag2str(sig));
			return buf;
	}
}

/* Color Space Signatures */
static const char *string_ColorSpaceSignature(icColorSpaceSignature sig) {
	static char buf[80];
	switch(sig) {
		case icSigXYZData:
			return "XYZ";
		case icSigLabData:
			return "Lab";
		case icSigLuvData:
			return "Luv";
		case icSigYCbCrData:
			return "YCbCr";
		case icSigYxyData:
			return "Yxy";
		case icSigRgbData:
			return "RGB";
		case icSigGrayData:
			return "Gray";
		case icSigHsvData:
			return "HSV";
		case icSigHlsData:
			return "HLS";
		case icSigCmykData:
			return "CMYK";
		case icSigCmyData:
			return "CMY";
		case icSig2colorData:
			return "2 Color";
		case icSig3colorData:
			return "3 Color";
		case icSig4colorData:
			return "4 Color";
		case icSig5colorData:
		case icSigMch5Data:
			return "5 Color";
		case icSig6colorData:
		case icSigMch6Data:
			return "6 Color";
		case icSig7colorData:
		case icSigMch7Data:
			return "7 Color";
		case icSig8colorData:
		case icSigMch8Data:
			return "8 Color";
		case icSig9colorData:
			return "9 Color";
		case icSig10colorData:
			return "10 Color";
		case icSig11colorData:
			return "11 Color";
		case icSig12colorData:
			return "12 Color";
		case icSig13colorData:
			return "13 Color";
		case icSig14colorData:
			return "14 Color";
		case icSig15colorData:
			return "15 Color";
		default:
			sprintf(buf,"Unrecognized - %s",tag2str(sig));
			return buf;
	}
}

#ifdef NEVER
/* Public version of above */
char *ColorSpaceSignature2str(icColorSpaceSignature sig) {
	return string_ColorSpaceSignature(sig);
}
#endif


/* profileClass enumerations */
static const char *string_ProfileClassSignature(icProfileClassSignature sig) {
	static char buf[80];
	switch(sig) {
		case icSigInputClass:
			return "Input";
		case icSigDisplayClass:
			return "Display";
		case icSigOutputClass:
			return "Output";
		case icSigLinkClass:
			return "Link";
		case icSigAbstractClass:
			return "Abstract";
		case icSigColorSpaceClass:
			return "Color Space";
		case icSigNamedColorClass:
			return "Named Color";
		default:
			sprintf(buf,"Unrecognized - %s",tag2str(sig));
			return buf;
	}
}

/* Platform Signatures */
static const char *string_PlatformSignature(icPlatformSignature sig) {
	static char buf[80];
	switch(sig) {
		case icSigMacintosh:
			return "Macintosh";
		case icSigMicrosoft:
			return "Microsoft";
		case icSigSolaris:
			return "Solaris";
		case icSigSGI:
			return "SGI";
		case icSigTaligent:
			return "Taligent";
		default:
			sprintf(buf,"Unrecognized - %s",tag2str(sig));
			return buf;
	}
}

/* Measurement Geometry, used in the measurmentType tag */
static const char *string_MeasurementGeometry(icMeasurementGeometry sig) {
	static char buf[30];
	switch(sig) {
		case icGeometryUnknown:
			return "Unknown";
		case icGeometry045or450:
			return "0/45 or 45/0";
		case icGeometry0dord0:
			return "0/d or d/0";
		default:
			sprintf(buf,"Unrecognized - 0x%x",sig);
			return buf;
	}
}

/* Rendering Intents, used in the profile header */
static const char *string_RenderingIntent(icRenderingIntent sig) {
	static char buf[30];
	switch(sig) {
		case icPerceptual:
			return "Perceptual";
		case icRelativeColorimetric:
	    		return "Relative Colorimetric";
		case icSaturation:
	    		return "Saturation";
		case icAbsoluteColorimetric:
	    		return "Absolute Colorimetric";
		default:
			sprintf(buf,"Unrecognized - 0x%x",sig);
			return buf;
	}
}

/* Different Spot Shapes currently defined, used for screeningType */
static const char *string_SpotShape(icSpotShape sig) {
	static char buf[30];
	switch(sig) {
		case icSpotShapeUnknown:
			return "Unknown";
		case icSpotShapePrinterDefault:
			return "Printer Default";
		case icSpotShapeRound:
			return "Round";
		case icSpotShapeDiamond:
			return "Diamond";
		case icSpotShapeEllipse:
			return "Ellipse";
		case icSpotShapeLine:
			return "Line";
		case icSpotShapeSquare:
			return "Square";
		case icSpotShapeCross:
			return "Cross";
		default:
			sprintf(buf,"Unrecognized - 0x%x",sig);
			return buf;
	}
}

/* Standard Observer, used in the measurmentType tag */
static const char *string_StandardObserver(icStandardObserver sig) {
	static char buf[30];
	switch(sig) {
		case icStdObsUnknown:
			return "Unknown";
		case icStdObs1931TwoDegrees:
			return "1931 Two Degrees";
		case icStdObs1964TenDegrees:
			return "1964 Ten Degrees";
		default:
			sprintf(buf,"Unrecognized - 0x%x",sig);
			return buf;
	}
}

/* Pre-defined illuminants, used in measurement and viewing conditions type */
static const char *string_Illuminant(icIlluminant sig) {
	static char buf[30];
	switch(sig) {
		case icIlluminantUnknown:
			return "Unknown";
		case icIlluminantD50:
			return "D50";
		case icIlluminantD65:
			return "D65";
		case icIlluminantD93:
			return "D93";
		case icIlluminantF2:
			return "F2";
		case icIlluminantD55:
			return "D55";
		case icIlluminantA:
			return "A";
		case icIlluminantEquiPowerE:
			return "Equi-Power(E)";
		case icIlluminantF8:
			return "F8";
		default:
			sprintf(buf,"Unrecognized - 0x%x",sig);
			return buf;
	}
}

/* Return a text abreviation of a color lookup algorithm */
static const char *string_LuAlg(icmLuAlgType alg) {
	static char buf[80];

	switch(alg) {
    	case icmMonoFwdType:
			return "MonoFwd";
    	case icmMonoBwdType:
			return "MonoBwd";
    	case icmMatrixFwdType:
			return "MatrixFwd";
    	case icmMatrixBwdType:
			return "MatrixBwd";
    	case icmLutType:
			return "Lut";
	default:
		sprintf(buf,"Unrecognized - %d",alg);
		return buf;
	}
}

/* Return a string description of the given enumeration value */
/* Public: */
const char *icm2str(icmEnumType etype, int enumval) {

	switch(etype) {
	    case icmScreenEncodings:
			return string_ScreenEncodings((unsigned long) enumval);
	    case icmDeviceAttributes:
			return string_DeviceAttributes((unsigned long) enumval);
		case icmProfileHeaderFlags:
			return string_ProfileHeaderFlags((unsigned long) enumval);
		case icmAsciiOrBinaryData:
			return string_AsciiOrBinaryData((unsigned long) enumval);
		case icmTagSignature:
			return string_TagSignature((icTagSignature) enumval);
		case icmTechnologySignature:
			return string_TechnologySignature((icTechnologySignature) enumval);
		case icmTypeSignature:
			return string_TypeSignature((icTagTypeSignature) enumval);
		case icmColorSpaceSignature:
			return string_ColorSpaceSignature((icColorSpaceSignature) enumval);
		case icmProfileClassSignaure:
			return string_ProfileClassSignature((icProfileClassSignature) enumval);
		case icmPlatformSignature:
			return string_PlatformSignature((icPlatformSignature) enumval);
		case icmMeasurementGeometry:
			return string_MeasurementGeometry((icMeasurementGeometry) enumval);
		case icmRenderingIntent:
			return string_RenderingIntent((icRenderingIntent) enumval);
		case icmSpotShape:
			return string_SpotShape((icSpotShape) enumval);
		case icmStandardObserver:
			return string_StandardObserver((icStandardObserver) enumval);
		case icmIlluminant:
			return string_Illuminant((icIlluminant) enumval);
		case icmLuAlg:
			return string_LuAlg((icmLuAlgType) enumval);
		default:
			return "enum2str got unknown type";
	}
}

/* ========================================================== */
/* Object I/O routines                                        */
/* ========================================================== */
/* icmUInt8Array object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmUInt8Array_get_size(
	icmBase *pp
) {
	icmUInt8Array *p = (icmUInt8Array *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += p->size * 1;	/* 1 byte for each UInt8 */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmUInt8Array_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmUInt8Array *p = (icmUInt8Array *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, size;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmUInt8Array_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt8Array_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmUInt8Array_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 8)/1;		/* Number of elements in the array */

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		icp->al->free(icp->al, buf);
		sprintf(icp->err,"icmUInt8Array_read: Wrong tag type for icmUInt8Array");
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read all the data from the buffer */
	for (i = 0; i < size; i++, bp += 1) {
		p->data[i] = read_UInt8Number(bp);
	}
	icp->al->free(p->icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmUInt8Array_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmUInt8Array *p = (icmUInt8Array *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt8Array_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmUInt8Array_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */
	bp += 8;	/* Skip padding */

	/* Write all the data to the buffer */
	for (i = 0; i < p->size; i++, bp += 1) {
		if ((rv = write_UInt8Number(p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmUInt8Array_write: write_UInt8umber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmUInt8Array_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmUInt8Array_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmUInt8Array *p = (icmUInt8Array *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"UInt8Array:\n");
	fprintf(op,"  No. elements = %lu\n",p->size);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->size; i++)
			fprintf(op,"    %lu:  %u\n",i,p->data[i]);
	}
}

/* Allocate variable sized data elements */
static int icmUInt8Array_allocate(
	icmBase *pp
) {
	icmUInt8Array *p = (icmUInt8Array *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (unsigned int *) icp->al->malloc(icp->al, p->size * sizeof(unsigned int))) == NULL) {
			sprintf(icp->err,"icmUInt8Array_alloc: malloc() of icmUInt8Array data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmUInt8Array_delete(
	icmBase *pp
) {
	icmUInt8Array *p = (icmUInt8Array *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmUInt8Array(
	icc *icp
) {
	icmUInt8Array *p;
	if ((p = (icmUInt8Array *) icp->al->calloc(icp->al,1,sizeof(icmUInt8Array))) == NULL)
		return NULL;
	p->ttype    = icSigUInt8ArrayType;
	p->refcount = 1;
	p->get_size = icmUInt8Array_get_size;
	p->read     = icmUInt8Array_read;
	p->write    = icmUInt8Array_write;
	p->dump     = icmUInt8Array_dump;
	p->allocate = icmUInt8Array_allocate;
	p->del      = icmUInt8Array_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmUInt16Array object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmUInt16Array_get_size(
	icmBase *pp
) {
	icmUInt16Array *p = (icmUInt16Array *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += p->size * 2;	/* 2 bytes for each UInt16 */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmUInt16Array_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmUInt16Array *p = (icmUInt16Array *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, size;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmUInt16Array_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt16Array_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmUInt16Array_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 8)/2;		/* Number of elements in the array */

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmUInt16Array_read: Wrong tag type for icmUInt16Array");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read all the data from the buffer */
	for (i = 0; i < size; i++, bp += 2) {
		p->data[i] = read_UInt16Number(bp);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmUInt16Array_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmUInt16Array *p = (icmUInt16Array *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt16Array_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmUInt16Array_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write all the data to the buffer */
	bp += 8;	/* Skip padding */
	for (i = 0; i < p->size; i++, bp += 2) {
		if ((rv = write_UInt16Number(p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmUInt16Array_write: write_UInt16umber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmUInt16Array_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmUInt16Array_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmUInt16Array *p = (icmUInt16Array *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"UInt16Array:\n");
	fprintf(op,"  No. elements = %lu\n",p->size);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->size; i++)
			fprintf(op,"    %lu:  %u\n",i,p->data[i]);
	}
}

/* Allocate variable sized data elements */
static int icmUInt16Array_allocate(
	icmBase *pp
) {
	icmUInt16Array *p = (icmUInt16Array *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (unsigned int *) icp->al->malloc(icp->al, p->size * sizeof(unsigned int))) == NULL) {
			sprintf(icp->err,"icmUInt16Array_alloc: malloc() of icmUInt16Array data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmUInt16Array_delete(
	icmBase *pp
) {
	icmUInt16Array *p = (icmUInt16Array *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmUInt16Array(
	icc *icp
) {
	icmUInt16Array *p;
	if ((p = (icmUInt16Array *) icp->al->calloc(icp->al,1,sizeof(icmUInt16Array))) == NULL)
		return NULL;
	p->ttype    = icSigUInt16ArrayType;
	p->refcount = 1;
	p->get_size = icmUInt16Array_get_size;
	p->read     = icmUInt16Array_read;
	p->write    = icmUInt16Array_write;
	p->dump     = icmUInt16Array_dump;
	p->allocate = icmUInt16Array_allocate;
	p->del      = icmUInt16Array_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmUInt32Array object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmUInt32Array_get_size(
	icmBase *pp
) {
	icmUInt32Array *p = (icmUInt32Array *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += p->size * 4;	/* 4 bytes for each UInt32 */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmUInt32Array_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmUInt32Array *p = (icmUInt32Array *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, size;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmUInt32Array_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt32Array_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmUInt32Array_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 8)/4;		/* Number of elements in the array */

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmUInt32Array_read: Wrong tag type for icmUInt32Array");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read all the data from the buffer */
	for (i = 0; i < size; i++, bp += 4) {
		p->data[i] = read_UInt32Number(bp);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmUInt32Array_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmUInt32Array *p = (icmUInt32Array *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt32Array_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmUInt32Array_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write all the data to the buffer */
	bp += 8;	/* Skip padding */
	for (i = 0; i < p->size; i++, bp += 4) {
		if ((rv = write_UInt32Number(p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmUInt32Array_write: write_UInt32umber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmUInt32Array_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmUInt32Array_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmUInt32Array *p = (icmUInt32Array *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"UInt32Array:\n");
	fprintf(op,"  No. elements = %lu\n",p->size);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->size; i++)
			fprintf(op,"    %lu:  %u\n",i,p->data[i]);
	}
}

/* Allocate variable sized data elements */
static int icmUInt32Array_allocate(
	icmBase *pp
) {
	icmUInt32Array *p = (icmUInt32Array *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (unsigned int *) icp->al->malloc(icp->al, p->size * sizeof(unsigned int))) == NULL) {
			sprintf(icp->err,"icmUInt32Array_alloc: malloc() of icmUInt32Array data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmUInt32Array_delete(
	icmBase *pp
) {
	icmUInt32Array *p = (icmUInt32Array *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmUInt32Array(
	icc *icp
) {
	icmUInt32Array *p;
	if ((p = (icmUInt32Array *) icp->al->calloc(icp->al,1,sizeof(icmUInt32Array))) == NULL)
		return NULL;
	p->ttype    = icSigUInt32ArrayType;
	p->refcount = 1;
	p->get_size = icmUInt32Array_get_size;
	p->read     = icmUInt32Array_read;
	p->write    = icmUInt32Array_write;
	p->dump     = icmUInt32Array_dump;
	p->allocate = icmUInt32Array_allocate;
	p->del      = icmUInt32Array_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmUInt64Array object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmUInt64Array_get_size(
	icmBase *pp
) {
	icmUInt64Array *p = (icmUInt64Array *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += p->size * 8;	/* 8 bytes for each UInt64 */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmUInt64Array_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmUInt64Array *p = (icmUInt64Array *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, size;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmUInt64Array_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt64Array_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmUInt64Array_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 8)/8;		/* Number of elements in the array */

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmUInt64Array_read: Wrong tag type for icmUInt64Array");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read all the data from the buffer */
	for (i = 0; i < size; i++, bp += 8) {
		read_UInt64Number(&p->data[i], bp);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmUInt64Array_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmUInt64Array *p = (icmUInt64Array *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUInt64Array_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmUInt64Array_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write all the data to the buffer */
	bp += 8;	/* Skip padding */
	for (i = 0; i < p->size; i++, bp += 8) {
		if ((rv = write_UInt64Number(&p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmUInt64Array_write: write_UInt64umber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmUInt64Array_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmUInt64Array_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmUInt64Array *p = (icmUInt64Array *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"UInt64Array:\n");
	fprintf(op,"  No. elements = %lu\n",p->size);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->size; i++)
			fprintf(op,"    %lu:  h=%lu, l=%lu\n",i,p->data[i].h,p->data[i].l);
	}
}

/* Allocate variable sized data elements */
static int icmUInt64Array_allocate(
	icmBase *pp
) {
	icmUInt64Array *p = (icmUInt64Array *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (icmUint64 *) icp->al->malloc(icp->al, p->size * sizeof(icmUint64))) == NULL) {
			sprintf(icp->err,"icmUInt64Array_alloc: malloc() of icmUInt64Array data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmUInt64Array_delete(
	icmBase *pp
) {
	icmUInt64Array *p = (icmUInt64Array *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmUInt64Array(
	icc *icp
) {
	icmUInt64Array *p;
	if ((p = (icmUInt64Array *) icp->al->calloc(icp->al,1,sizeof(icmUInt64Array))) == NULL)
		return NULL;
	p->ttype    = icSigUInt64ArrayType;
	p->refcount = 1;
	p->get_size = icmUInt64Array_get_size;
	p->read     = icmUInt64Array_read;
	p->write    = icmUInt64Array_write;
	p->dump     = icmUInt64Array_dump;
	p->allocate = icmUInt64Array_allocate;
	p->del      = icmUInt64Array_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmU16Fixed16Array object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmU16Fixed16Array_get_size(
	icmBase *pp
) {
	icmU16Fixed16Array *p = (icmU16Fixed16Array *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += p->size * 4;	/* 4 byte for each U16Fixed16 */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmU16Fixed16Array_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmU16Fixed16Array *p = (icmU16Fixed16Array *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, size;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmU16Fixed16Array_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmU16Fixed16Array_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmU16Fixed16Array_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 8)/4;		/* Number of elements in the array */

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmU16Fixed16Array_read: Wrong tag type for icmU16Fixed16Array");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read all the data from the buffer */
	for (i = 0; i < size; i++, bp += 4) {
		p->data[i] = read_U16Fixed16Number(bp);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmU16Fixed16Array_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmU16Fixed16Array *p = (icmU16Fixed16Array *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmU16Fixed16Array_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmU16Fixed16Array_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write all the data to the buffer */
	bp += 8;	/* Skip padding */
	for (i = 0; i < p->size; i++, bp += 4) {
		if ((rv = write_U16Fixed16Number(p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmU16Fixed16Array_write: write_U16Fixed16umber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmU16Fixed16Array_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmU16Fixed16Array_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmU16Fixed16Array *p = (icmU16Fixed16Array *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"U16Fixed16Array:\n");
	fprintf(op,"  No. elements = %lu\n",p->size);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->size; i++)
			fprintf(op,"    %lu:  %f\n",i,p->data[i]);
	}
}

/* Allocate variable sized data elements */
static int icmU16Fixed16Array_allocate(
	icmBase *pp
) {
	icmU16Fixed16Array *p = (icmU16Fixed16Array *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (double *) icp->al->malloc(icp->al, p->size * sizeof(double))) == NULL) {
			sprintf(icp->err,"icmU16Fixed16Array_alloc: malloc() of icmU16Fixed16Array data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmU16Fixed16Array_delete(
	icmBase *pp
) {
	icmU16Fixed16Array *p = (icmU16Fixed16Array *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmU16Fixed16Array(
	icc *icp
) {
	icmU16Fixed16Array *p;
	if ((p = (icmU16Fixed16Array *) icp->al->calloc(icp->al,1,sizeof(icmU16Fixed16Array))) == NULL)
		return NULL;
	p->ttype    = icSigU16Fixed16ArrayType;
	p->refcount = 1;
	p->get_size = icmU16Fixed16Array_get_size;
	p->read     = icmU16Fixed16Array_read;
	p->write    = icmU16Fixed16Array_write;
	p->dump     = icmU16Fixed16Array_dump;
	p->allocate = icmU16Fixed16Array_allocate;
	p->del      = icmU16Fixed16Array_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmS15Fixed16Array object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmS15Fixed16Array_get_size(
	icmBase *pp
) {
	icmS15Fixed16Array *p = (icmS15Fixed16Array *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += p->size * 4;	/* 4 byte for each S15Fixed16 */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmS15Fixed16Array_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmS15Fixed16Array *p = (icmS15Fixed16Array *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, size;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmS15Fixed16Array_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmS15Fixed16Array_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmS15Fixed16Array_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 8)/4;		/* Number of elements in the array */

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmS15Fixed16Array_read: Wrong tag type for icmS15Fixed16Array");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read all the data from the buffer */
	for (i = 0; i < size; i++, bp += 4) {
		p->data[i] = read_S15Fixed16Number(bp);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmS15Fixed16Array_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmS15Fixed16Array *p = (icmS15Fixed16Array *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmS15Fixed16Array_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmS15Fixed16Array_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write all the data to the buffer */
	bp += 8;	/* Skip padding */
	for (i = 0; i < p->size; i++, bp += 4) {
		if ((rv = write_S15Fixed16Number(p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmS15Fixed16Array_write: write_S15Fixed16umber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmS15Fixed16Array_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmS15Fixed16Array_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmS15Fixed16Array *p = (icmS15Fixed16Array *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"S15Fixed16Array:\n");
	fprintf(op,"  No. elements = %lu\n",p->size);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->size; i++)
			fprintf(op,"    %lu:  %f\n",i,p->data[i]);
	}
}

/* Allocate variable sized data elements */
static int icmS15Fixed16Array_allocate(
	icmBase *pp
) {
	icmS15Fixed16Array *p = (icmS15Fixed16Array *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (double *) icp->al->malloc(icp->al, p->size * sizeof(double))) == NULL) {
			sprintf(icp->err,"icmS15Fixed16Array_alloc: malloc() of icmS15Fixed16Array data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmS15Fixed16Array_delete(
	icmBase *pp
) {
	icmS15Fixed16Array *p = (icmS15Fixed16Array *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmS15Fixed16Array(
	icc *icp
) {
	icmS15Fixed16Array *p;
	if ((p = (icmS15Fixed16Array *) icp->al->calloc(icp->al,1,sizeof(icmS15Fixed16Array))) == NULL)
		return NULL;
	p->ttype    = icSigS15Fixed16ArrayType;
	p->refcount = 1;
	p->get_size = icmS15Fixed16Array_get_size;
	p->read     = icmS15Fixed16Array_read;
	p->write    = icmS15Fixed16Array_write;
	p->dump     = icmS15Fixed16Array_dump;
	p->allocate = icmS15Fixed16Array_allocate;
	p->del      = icmS15Fixed16Array_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */

/* Data conversion support functions */
static int write_XYZNumber(icmXYZNumber *p, char *d) {
	int rv;
	if ((rv = write_S15Fixed16Number(p->X, d + 0)) != 0)
		return rv;
	if ((rv = write_S15Fixed16Number(p->Y, d + 4)) != 0)
		return rv;
	if ((rv = write_S15Fixed16Number(p->Z, d + 8)) != 0)
		return rv;
	return 0;
}

static int read_XYZNumber(icmXYZNumber *p, char *d) {
	p->X = read_S15Fixed16Number(d + 0);
	p->Y = read_S15Fixed16Number(d + 4);
	p->Z = read_S15Fixed16Number(d + 8);
	return 0;
}


/* Helper: Return a string that shows the XYZ number value */
static char *string_XYZNumber(icmXYZNumber *p) {
	static char buf[40];

	sprintf(buf,"%f, %f, %f", p->X, p->Y, p->Z);
	return buf;
}

/* Helper: Return a string that shows the XYZ number value, */
/* and the Lab D50 number in paren. */
static char *string_XYZNumber_and_Lab(icmXYZNumber *p) {
	static char buf[50];
	double lab[3];
	lab[0] = p->X;
	lab[1] = p->Y;
	lab[2] = p->Z;
	icmXYZ2Lab(&icmD50, lab, lab);
	sprintf(buf,"%f, %f, %f    [Lab %f, %f, %f]", p->X, p->Y, p->Z, lab[0], lab[1], lab[2]);
	return buf;
}
			
/* icmXYZArray object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmXYZArray_get_size(
	icmBase *pp
) {
	icmXYZArray *p = (icmXYZArray *)pp;
	unsigned int len = 0;
	len += 8;				/* 8 bytes for tag and padding */
	len += p->size * 12;	/* 12 bytes for each XYZ */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmXYZArray_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmXYZArray *p = (icmXYZArray *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, size;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmXYZArray_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmXYZArray_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmXYZArray_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 8)/12;		/* Number of elements in the array */

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmXYZArray_read: Wrong tag type for icmXYZArray");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read all the data from the buffer */
	for (i = 0; i < size; i++, bp += 12) {
		read_XYZNumber(&p->data[i], bp);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmXYZArray_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmXYZArray *p = (icmXYZArray *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmXYZArray_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmXYZArray_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write all the data to the buffer */
	bp += 8;	/* Skip padding */
	for (i = 0; i < p->size; i++, bp += 12) {
		if ((rv = write_XYZNumber(&p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmXYZArray_write: write_XYZumber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmXYZArray_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmXYZArray_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmXYZArray *p = (icmXYZArray *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"XYZArray:\n");
	fprintf(op,"  No. elements = %lu\n",p->size);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->size; i++) {
			fprintf(op,"    %lu:  %s\n",i,string_XYZNumber_and_Lab(&p->data[i]));
			
		}
	}
}


/* Allocate variable sized data elements */
static int icmXYZArray_allocate(
	icmBase *pp
) {
	icmXYZArray *p = (icmXYZArray *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (icmXYZNumber *) icp->al->malloc(icp->al, p->size * sizeof(icmXYZNumber))) == NULL) {
			sprintf(icp->err,"icmXYZArray_alloc: malloc() of icmXYZArray data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmXYZArray_delete(
	icmBase *pp
) {
	icmXYZArray *p = (icmXYZArray *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmXYZArray(
	icc *icp
) {
	icmXYZArray *p;
	if ((p = (icmXYZArray *) icp->al->calloc(icp->al,1,sizeof(icmXYZArray))) == NULL)
		return NULL;
	p->ttype    = icSigXYZArrayType;
	p->refcount = 1;
	p->get_size = icmXYZArray_get_size;
	p->read     = icmXYZArray_read;
	p->write    = icmXYZArray_write;
	p->dump     = icmXYZArray_dump;
	p->allocate = icmXYZArray_allocate;
	p->del      = icmXYZArray_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmCurve object */

/* Do a forward lookup through the curve */
/* Return 0 on success, 1 if clipping occured, 2 on other error */
static int icmCurve_lookup_fwd(
	icmCurve *p,
	double *out,
	double *in
) {
	int rv = 0;
	if (p->flag == icmCurveLin) {
		*out = *in;
	} else if (p->flag == icmCurveGamma) {
		double val = *in;
		if (val <= 0.0)
			*out = 0.0;
		else
			*out = pow(val, p->data[0]);
	} else { /* Use linear interpolation */
		int ix;
		double val, w;
		double inputEnt_1 = (double)(p->size-1);

		val = *in * inputEnt_1;
		if (val < 0.0) {
			val = 0.0;
			rv |= 1;
		} else if (val > inputEnt_1) {
			val = inputEnt_1;
			rv |= 1;
		}
		ix = (int)floor(val);		/* Coordinate */
		if (ix > (p->size-2))
			ix = (p->size-2);
		w = val - (double)ix;		/* weight */
		val = p->data[ix];
		*out = val + w * (p->data[ix+1] - val);
	}
	return rv;
}

/* - - - - - - - - - - - - */
/* Support for reverse interpolation of 1D lookup tables */

/* Create a reverse curve lookup acceleration table */
/* return non-zero on error, 2 = malloc error. */
static int icmTable_setup_bwd(
	icc          *icp,			/* Base icc object */
	icmRevTable  *rt,			/* Reverse table data to setup */
	unsigned long size,			/* Size of fwd table */
	double       *data			/* Table */
) {
	int i;

	rt->size = size;		/* Stash pointers to these away */
	rt->data = data;
	
	/* Find range of output values */
	rt->rmin = 1e300;
	rt->rmax = -1e300;
	for (i = 0; i < rt->size; i++) {
		if (rt->data[i] > rt->rmax)
			rt->rmax = rt->data[i];
		if (rt->data[i] < rt->rmin)
			rt->rmin = rt->data[i];
	}

	/* Decide on reverse granularity */
	rt->rsize = (rt->size+2)/2;
	rt->qscale = (double)rt->rsize/(rt->rmax - rt->rmin);	/* Scale factor to quantize to */
	
	/* Initialize the reverse lookup structures, and get overall min/max */
	if ((rt->rlists = (int **) icp->al->calloc(icp->al, 1, rt->rsize * sizeof(int *))) == NULL) {
		return 2;
	}

	/* Assign each output value range bucket lists it intersects */
	for (i = 0; i < (rt->size-1); i++) {
		int s, e, j;	/* Start and end indexes (inclusive) */
		s = (int)((rt->data[i] - rt->rmin) * rt->qscale);
		e = (int)((rt->data[i+1] - rt->rmin) * rt->qscale);
		if (s > e) {	/* swap */
			int t;
			t = s; s = e; e = t;
		}
		if (e >= rt->rsize)
			e = rt->rsize-1;

		/* For all buckets that may contain this output range, add index of this output */
		for (j = s; j <= e; j++) {
			int as;			/* Allocation size */
			int nf;			/* Next free slot */
			if (rt->rlists[j] == NULL) {	/* No allocation */
				as = 5;						/* Start with space for 5 */
				if ((rt->rlists[j] = (int *) icp->al->malloc(icp->al, sizeof(int) * as)) == NULL) {
					return 2;
				}
				rt->rlists[j][0] = as;
				nf = rt->rlists[j][1] = 2;
			} else {
				as = rt->rlists[j][0];	/* Allocate space for this list */
				nf = rt->rlists[j][1];	/* Next free location in list */
				if (nf >= as) {			/* need to expand space */
					as *= 2;
					rt->rlists[j] = (int *) icp->al->realloc(icp->al,rt->rlists[j], sizeof(int) * as);
					if (rt->rlists[j] == NULL) {
						return 2;
					}
					rt->rlists[j][0] = as;
				}
			}
			rt->rlists[j][nf++] = i;
			rt->rlists[j][1] = nf;
		}
	}
	rt->inited = 1;
	return 0;
}

/* Free up any data */
static void icmTable_delete_bwd(
	icc          *icp,			/* Base icc */
	icmRevTable  *rt			/* Reverse table data to setup */
) {
	if (rt->inited != 0) {
		while (rt->rsize > 0)
			icp->al->free(icp->al, rt->rlists[--rt->rsize]);
		icp->al->free(icp->al, rt->rlists);
		rt->size = 0;			/* Don't keep these */
		rt->data = NULL;
	}
}

/* Do a reverse lookup through the curve */
/* Return 0 on success, 1 if clipping occured, 2 on other error */
static int icmTable_lookup_bwd(
	icmRevTable *rt,
	double *out,
	double *in
) {
	int rv = 0;
	int ix, i, k;
	double oval, ival = *in, val;
	double rsize_1;

	/* Find appropriate reverse list */
	rsize_1 = (double)(rt->rsize-1);
	val = ((ival - rt->rmin) * rt->qscale);
	if (val < 0.0)
		val = 0.0;
	else if (val > rsize_1)
		val = rsize_1;
	ix = (int)floor(val);		/* Coordinate */

	if (ix > (rt->size-2))
		ix = (rt->size-2);
	if (rt->rlists[ix] != NULL)  {		/* There is a list of fwd candidates */
		/* For each candidate forward range */
		for (i = 2; i < rt->rlists[ix][1]; i++)  {	/* For all fwd indexes */
			double lv,hv;
			k = rt->rlists[ix][i];					/* Base index */
			lv = rt->data[k];
			hv = rt->data[k+1];
			if ((ival >= lv && ival <= hv)	/* If this slot contains output value */
			 || (ival >= hv && ival <= lv)) {
				/* Reverse linear interpolation */
				if (hv == lv) {	/* Technically non-monotonic - due to quantization ? */
					oval = (k + 0.5)/(rt->size-1.0);
				} else
					oval = (k + ((ival - lv)/(hv - lv)))/(rt->size-1.0);
				/* If we kept looking, we would find multiple */
				/* solution for non-monotonic curve */
				*out = oval;
				return rv;
			}
		}
	}

	/* We have failed to find an exact value, so return the nearest value */
	/* (This is slow !) */
	val = fabs(ival - rt->data[0]);
	for (k = 0, i = 1; i < rt->size; i++) {
		double er;
		er = fabs(ival - rt->data[i]);
		if (er < val) {	/* new best */
			val = er;
			k = i;
		}
	}
	*out = k/(rt->size-1.0);
	rv |= 1;
	return rv;
}


/* - - - - - - - - - - - - */

/* Do a reverse lookup through the curve */
/* Return 0 on success, 1 if clipping occured, 2 on other error */
static int icmCurve_lookup_bwd(
	icmCurve *p,
	double *out,
	double *in
) {
	icc *icp = p->icp;
	int rv = 0;
	if (p->flag == icmCurveLin) {
		*out = *in;
	} else if (p->flag == icmCurveGamma) {
		double val = *in;
		if (val <= 0.0)
			*out = 0.0;
		else
			*out = pow(val, 1.0/p->data[0]);
	} else { /* Use linear interpolation */
		if (p->rt.inited == 0) {	
			rv = icmTable_setup_bwd(icp, &p->rt, p->size, p->data);
			if (rv != 0) {
				sprintf(icp->err,"icmCurve_lookup: Malloc failure in reverse lookup init.");
				return icp->errc = rv;
			}
		}
		rv = icmTable_lookup_bwd(&p->rt, out, in);
	}
	return rv;
}

/* Return the number of bytes needed to write this tag */
static unsigned int icmCurve_get_size(
	icmBase *pp
) {
	icmCurve *p = (icmCurve *)pp;
	unsigned int len = 0;
	len += 12;			/* 12 bytes for tag, padding and count */
	len += p->size * 2;	/* 2 bytes for each UInt16 */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmCurve_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmCurve *p = (icmCurve *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i;
	char *bp, *buf, *end;

	if (len < 12) {
		sprintf(icp->err,"icmCurve_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmCurve_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;
	end = buf + len;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmCurve_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmCurve_read: Wrong tag type for icmCurve");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	p->size = read_UInt32Number(bp+8);
	bp = bp + 12;

	/* Set flag up before allocating */
	if (p->size == 0) {		/* Linear curve */
		p->flag = icmCurveLin;
	} else if (p->size == 1) {	/* Gamma curve */
		p->flag = icmCurveGamma;
	} else {
		p->flag = icmCurveSpec;
	}

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	if (p->flag == icmCurveGamma) {	/* Gamma curve */
		if ((bp + 1) > end) {
			sprintf(icp->err,"icmCurve_read: Data too short to curve gamma");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		p->data[0] = read_U8Fixed8Number(bp);
	} else if (p->flag == icmCurveSpec) {
		/* Read all the data from the buffer */
		for (i = 0; i < p->size; i++, bp += 2) {
			if ((bp + 2) > end) {
				sprintf(icp->err,"icmData_read: Data too short to curve value");
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
			p->data[i] = read_DCS16Number(bp);
		}
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmCurve_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmCurve *p = (icmCurve *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmCurve_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmCurve_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */
	/* Write count */
	if ((rv = write_UInt32Number(p->size,bp+8)) != 0) {
		sprintf(icp->err,"icmCurve_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write all the data to the buffer */
	bp += 12;	/* Skip padding */
	if (p->flag == icmCurveLin) {
		if (p->size != 0) {
			sprintf(icp->err,"icmCurve_write: Must be exactly 0 entry for Linear");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
	} else if (p->flag == icmCurveGamma) {
		if (p->size != 1) {
			sprintf(icp->err,"icmCurve_write: Must be exactly 1 entry for Gamma");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		if ((rv = write_U8Fixed8Number(p->data[0],bp)) != 0) {
			sprintf(icp->err,"icmCurve_write: write_U8Fixed8umber(%f) failed",p->data[0]);
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	} else if (p->flag == icmCurveSpec) {
		if (p->size < 2) {
			sprintf(icp->err,"icmCurve_write: Must be 2 or more entries for Specified curve");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		for (i = 0; i < p->size; i++, bp += 2) {
			if ((rv = write_DCS16Number(p->data[i],bp)) != 0) {
				sprintf(icp->err,"icmCurve_write: write_UInt16umber(%f) failed",p->data[i]);
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmCurve_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmCurve_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmCurve *p = (icmCurve *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"Curve:\n");

	if (p->flag == icmCurveLin) {
		fprintf(op,"  Curve is linear\n");
	} else if (p->flag == icmCurveGamma) {
		fprintf(op,"  Curve is gamma of %f\n",p->data[0]);
	} else {
		fprintf(op,"  No. elements = %lu\n",p->size);
		if (verb >= 2) {
			unsigned long i;
			for (i = 0; i < p->size; i++)
				fprintf(op,"    %3lu:  %f\n",i,p->data[i]);
		}
	}
}

/* Allocate variable sized data elements */
static int icmCurve_allocate(
	icmBase *pp
) {
	icmCurve *p = (icmCurve *)pp;
	icc *icp = p->icp;

	if (p->flag == icmCurveUndef) {
		sprintf(icp->err,"icmCurve_alloc: flag not set");
		return icp->errc = 1;
	} else if (p->flag == icmCurveLin) {
		p->size = 0;
	} else if (p->flag == icmCurveGamma) {
		p->size = 1;
	}
	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (double *) icp->al->malloc(icp->al, p->size * sizeof(double))) == NULL) {
			sprintf(icp->err,"icmCurve_alloc: malloc() of icmCurve data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmCurve_delete(
	icmBase *pp
) {
	icmCurve *p = (icmCurve *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icmTable_delete_bwd(icp, &p->rt);	/* Free reverse table info */
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmCurve(
	icc *icp
) {
	icmCurve *p;
	if ((p = (icmCurve *) icp->al->calloc(icp->al,1,sizeof(icmCurve))) == NULL)
		return NULL;
	p->ttype    = icSigCurveType;
	p->refcount = 1;
	p->get_size = icmCurve_get_size;
	p->read     = icmCurve_read;
	p->write    = icmCurve_write;
	p->dump     = icmCurve_dump;
	p->allocate = icmCurve_allocate;
	p->del      = icmCurve_delete;
	p->icp      = icp;

	p->lookup_fwd = icmCurve_lookup_fwd;
	p->lookup_bwd = icmCurve_lookup_bwd;

	p->rt.inited = 0;

	p->flag = icmCurveUndef;
	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmData object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmData_get_size(
	icmBase *pp
) {
	icmData *p = (icmData *)pp;
	unsigned int len = 0;
	len += 12;			/* 12 bytes for tag and padding */
	len += p->size * 1;	/* 1 byte for each data element */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmData_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmData *p = (icmData *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned size, f;
	char *bp, *buf;

	if (len < 12) {
		sprintf(icp->err,"icmData_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmData_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmData_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = size = (len - 12)/1;		/* Number of elements in the array */

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmData_read: Wrong tag type for icmData");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	/* Read the data type flag */
	f = read_UInt32Number(bp+8);
	if (f == 0) {
		p->flag = icmDataASCII;
	} else if (f == 1) {
		p->flag = icmDataBin;
	} else {
		sprintf(icp->err,"icmData_read: Unknown flag value 0x%x",f);
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 12;	/* Skip padding and flag */

	if (p->size > 0) {
		if (p->flag == icmDataASCII) {
			if (check_null_string(bp,p->size) != 0) {
				sprintf(icp->err,"icmData_read: ACSII is not null terminated");
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
		}
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}

		memcpy((void *)p->data, (void *)bp, p->size);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmData_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmData *p = (icmData *)pp;
	icc *icp = p->icp;
	unsigned int len, f;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmData_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmData_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */
	switch(p->flag) {
		case icmDataASCII:
			f = 0;
			break;
		case icmDataBin:
			f = 1;
			break;
		default:
			sprintf(icp->err,"icmData_write: Unknown Data Flag value");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
	}
	/* Write data flag descriptor to the buffer */
	if ((rv = write_UInt32Number(f,bp+8)) != 0) {
		sprintf(icp->err,"icmData_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	bp += 12;	/* Skip padding */

	if (p->data != NULL) {
		if (p->flag == icmDataASCII) {
			if ((rv = check_null_string((char *)p->data, p->size)) != 0) {
				sprintf(icp->err,"icmData_write: ASCII is not null terminated");
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
		}
		memcpy((void *)bp, (void *)p->data, p->size);
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmData_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmData_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmData *p = (icmData *)pp;
	unsigned long i, r, c, size = 0;

	if (verb <= 0)
		return;

	fprintf(op,"Data:\n");
	switch(p->flag) {
		case icmDataASCII:
			fprintf(op,"  ASCII data\n");
			size = p->size > 0 ? p->size-1 : 0;
			break;
		case icmDataBin:
			fprintf(op,"  Binary data\n");
			size = p->size;
			break;
		case icmDataUndef:
			fprintf(op,"  Undefined data\n");
			size = p->size;
			break;
	}
	fprintf(op,"  No. elements = %lu\n",p->size);

	i = 0;
	for (r = 1;; r++) {		/* count rows */
		if (i >= size) {
			fprintf(op,"\n");
			break;
		}
		if (r > 1 && verb < 2) {
			fprintf(op,"...\n");
			break;			/* Print 1 row if not verbose */
		}
		c = 1;
		fprintf(op,"    0x%04lx: ",i);
		c += 10;
		while (i < size && c < 75) {
			if (p->flag == icmDataASCII) {
				if (isprint(p->data[i])) {
					fprintf(op,"%c",p->data[i]);
					c++;
				} else {
					fprintf(op,"\\%03o",p->data[i]);
					c += 4;
				}
			} else {
				fprintf(op,"%02x ",p->data[i]);
				c += 3;
			}
			i++;
		}
		if (i < size)
			fprintf(op,"\n");
	}
}

/* Allocate variable sized data elements */
static int icmData_allocate(
	icmBase *pp
) {
	icmData *p = (icmData *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (unsigned char *) icp->al->malloc(icp->al, p->size * sizeof(unsigned char))) == NULL) {
			sprintf(icp->err,"icmData_alloc: malloc() of icmData data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmData_delete(
	icmBase *pp
) {
	icmData *p = (icmData *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmData(
	icc *icp
) {
	icmData *p;
	if ((p = (icmData *) icp->al->calloc(icp->al,1,sizeof(icmData))) == NULL)
		return NULL;
	p->ttype    = icSigDataType;
	p->refcount = 1;
	p->get_size = icmData_get_size;
	p->read     = icmData_read;
	p->write    = icmData_write;
	p->dump     = icmData_dump;
	p->allocate = icmData_allocate;
	p->del      = icmData_delete;
	p->icp      = icp;

	p->flag = icmDataUndef;
	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmText object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmText_get_size(
	icmBase *pp
) {
	icmText *p = (icmText *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += p->size;		/* 1 byte for each character element (inc. null) */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmText_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmText *p = (icmText *)pp;
	icc *icp = p->icp;
	int rv = 0;
	char *bp, *buf;

	if (len < 8) {
		sprintf(icp->err,"icmText_read: Tag too short to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmText_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmText_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->size = (len - 8)/1;		/* Number of elements in the array */

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmText_read: Wrong tag type for icmText");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp = bp + 8;

	if (p->size > 0) {
		if (check_null_string(bp,p->size) != 0) {
			sprintf(icp->err,"icmText_read: text is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
		memcpy((void *)p->data, (void *)bp, p->size);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmText_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmText *p = (icmText *)pp;
	icc *icp = p->icp;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmText_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmText_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */
	bp = bp + 8;

	if (p->data != NULL) {
		if ((rv = check_null_string(p->data, p->size)) != 0) {
			sprintf(icp->err,"icmText_write: text is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		memcpy((void *)bp, (void *)p->data, p->size);
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmText_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmText_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmText *p = (icmText *)pp;
	unsigned long i, r, c, size;

	if (verb <= 0)
		return;

	fprintf(op,"Text:\n");
	fprintf(op,"  No. chars = %lu\n",p->size);

	size = p->size > 0 ? p->size-1 : 0;
	i = 0;
	for (r = 1;; r++) {		/* count rows */
		if (i >= size) {
			fprintf(op,"\n");
			break;
		}
		if (r > 1 && verb < 2) {
			fprintf(op,"...\n");
			break;			/* Print 1 row if not verbose */
		}
		c = 1;
		fprintf(op,"    0x%04lx: ",i);
		c += 10;
		while (i < size && c < 75) {
			if (isprint(p->data[i])) {
				fprintf(op,"%c",p->data[i]);
				c++;
			} else {
				fprintf(op,"\\%03o",p->data[i]);
				c += 4;
			}
			i++;
		}
		if (i < size)
			fprintf(op,"\n");
	}
}

/* Allocate variable sized data elements */
static int icmText_allocate(
	icmBase *pp
) {
	icmText *p = (icmText *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (char *) icp->al->malloc(icp->al, p->size * sizeof(char))) == NULL) {
			sprintf(icp->err,"icmText_alloc: malloc() of icmText data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmText_delete(
	icmBase *pp
) {
	icmText *p = (icmText *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmText(
	icc *icp
) {
	icmText *p;
	if ((p = (icmText *) icp->al->calloc(icp->al,1,sizeof(icmText))) == NULL)
		return NULL;
	p->ttype    = icSigTextType;
	p->refcount = 1;
	p->get_size = icmText_get_size;
	p->read     = icmText_read;
	p->write    = icmText_write;
	p->dump     = icmText_dump;
	p->allocate = icmText_allocate;
	p->del      = icmText_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */

/* Data conversion support functions */
static int write_DateTimeNumber(icmDateTimeNumber *p, char *d) {
	int rv;
	if (p->year < 1900 || p->year > 3000
	 || p->month == 0 || p->month > 12
	 || p->day == 0 || p->day > 31
	 || p->hours > 23
	 || p->minutes > 59
	 || p->seconds > 59)
		return 1;

	if ((rv = write_UInt16Number(p->year,    d + 0)) != 0)
		return rv;
	if ((rv = write_UInt16Number(p->month,   d + 2)) != 0)
		return rv;
	if ((rv = write_UInt16Number(p->day,     d + 4)) != 0)
		return rv;
	if ((rv = write_UInt16Number(p->hours,   d + 6)) != 0)
		return rv;
	if ((rv = write_UInt16Number(p->minutes, d + 8)) != 0)
		return rv;
	if ((rv = write_UInt16Number(p->seconds, d + 10)) != 0)
		return rv;
	return 0;
}

static int read_DateTimeNumber(icmDateTimeNumber *p, char *d) {
	p->year    = read_UInt16Number(d + 0);
	p->month   = read_UInt16Number(d + 2);
	p->day     = read_UInt16Number(d + 4);
	p->hours   = read_UInt16Number(d + 6);
	p->minutes = read_UInt16Number(d + 8);
	p->seconds = read_UInt16Number(d + 10);

	if (p->year < 1900 || p->year > 3000
	 || p->month == 0 || p->month > 12
	 || p->day == 0 || p->day > 31
	 || p->hours > 23
	 || p->minutes > 59
	 || p->seconds > 59) {
		unsigned int tt; 

		/* Check for Adobe problem */
		if (p->month < 1900 || p->month > 3000
		 || p->year == 0 || p->year > 12
		 || p->hours == 0 || p->hours > 31
		 || p->day > 23
		 || p->seconds > 59
		 || p->minutes > 59)
			return 1;			/* Nope */

		/* Correct Adobe's faulty profile */
		tt = p->month; p->month = p->year; p->year = tt;
		tt = p->hours; p->hours = p->day; p->day = tt;
		tt = p->seconds; p->seconds = p->minutes; p->minutes = tt;
		return 0;
	}
	return 0;
}

/* Return a string that shows the given date and time */
static char *string_DateTimeNumber(icmDateTimeNumber *p) {
	static const char *mstring[13] = {"Bad", "Jan","Feb","Mar","Apr","May","Jun",
					  "Jul","Aug","Sep","Oct","Nov","Dec"};
	static char buf[80];

	sprintf(buf,"%d %s %4d, %d:%02d:%02d", 
	                p->day, mstring[p->month > 12 ? 0 : p->month], p->year,
	                p->hours, p->minutes, p->seconds);
	return buf;
}

/* Set the DateTime structure to the current date and time */
static void setcur_DateTimeNumber(icmDateTimeNumber *p) {
	time_t cclk;
	struct tm *ctm;
	
	cclk = time(NULL);
	ctm = localtime(&cclk);
	
	p->year    = ctm->tm_year + 1900;
	p->month   = ctm->tm_mon + 1;
	p->day     = ctm->tm_mday;
	p->hours   = ctm->tm_hour;
	p->minutes = ctm->tm_min;
	p->seconds = ctm->tm_sec;
}

/* Return the number of bytes needed to write this tag */
static unsigned int icmDateTimeNumber_get_size(
	icmBase *pp
) {
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += 12;			/* 12 bytes for Date & Time */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmDateTimeNumber_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmDateTimeNumber *p = (icmDateTimeNumber *)pp;
	icc *icp = p->icp;
	int rv;
	char *bp, *buf;

	if (len < 20) {
		sprintf(icp->err,"icmDateTimeNumber_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmDateTimeNumber_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmDateTimeNumber_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmDateTimeNumber_read: Wrong tag type for icmDateTimeNumber");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	/* Read the time and date from buffer */
	if((rv = read_DateTimeNumber(p, bp)) != 0) {
		sprintf(icp->err,"icmDateTimeNumber_read: Corrupted DateTime");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmDateTimeNumber_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmDateTimeNumber *p = (icmDateTimeNumber *)pp;
	icc *icp = p->icp;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmDateTimeNumber_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmDateTimeNumber_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write all the data to the buffer */
	bp += 8;	/* Skip padding */
	if ((rv = write_DateTimeNumber(p, bp)) != 0) {
		sprintf(icp->err,"icmDateTimeNumber_write: write_DateTimeNumber() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmDateTimeNumber_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmDateTimeNumber_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmDateTimeNumber *p = (icmDateTimeNumber *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"DateTimeNumber:\n");
	fprintf(op,"  Date = %s\n", string_DateTimeNumber(p));
}

/* Allocate variable sized data elements */
static int icmDateTimeNumber_allocate(
	icmBase *pp
) {
	/* Nothing to do */
	return 0;
}

/* Free all storage in the object */
static void icmDateTimeNumber_delete(
	icmBase *pp
) {
	icmDateTimeNumber *p = (icmDateTimeNumber *)pp;
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmDateTimeNumber(
	icc *icp
) {
	icmDateTimeNumber *p;
	if ((p = (icmDateTimeNumber *) icp->al->calloc(icp->al,1,sizeof(icmDateTimeNumber))) == NULL)
		return NULL;
	p->ttype    = icSigDateTimeType;
	p->refcount = 1;
	p->get_size = icmDateTimeNumber_get_size;
	p->read     = icmDateTimeNumber_read;
	p->write    = icmDateTimeNumber_write;
	p->dump     = icmDateTimeNumber_dump;
	p->allocate = icmDateTimeNumber_allocate;
	p->del      = icmDateTimeNumber_delete;
	p->icp      = icp;

	setcur_DateTimeNumber(p);	/* Default to current date and time */
	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmLut object */

/* Utility function - raise one integer to an integer power */
static unsigned int uipow(unsigned int a, unsigned int b) {
	unsigned int rv = 1;
	for (; b > 0; b--)
		rv *= a;
	return rv;
}

/* - - - - - - - - - - - - - - - - */
/* Check if the matrix is non-zero */
static int icmLut_nu_matrix(
	icmLut *p		/* Pointer to Lut object */
) {
	int i, j;
	
	for (j = 0; j < 3; j++) {		/* Rows */
		for (i = 0; i < 3; i++) {	/* Columns */
			if (   (i == j && p->e[j][i] != 1.0)
			    || (i != j && p->e[j][i] != 0.0))
				return 1;
		}
	}
	return 0;
}

/* return the locations of the minimum and */
/* maximum values of the given channel, in the clut */
static void icmLut_min_max(
	icmLut *p,		/* Pointer to Lut object */
	double *minp,	/* Return position of min/max */
	double *maxp,
	int chan		/* Channel, -1 for average of all */
) {
	double *tp;
	double minv, maxv;	/* Values */
	int e, ee, f;
	double gc[MAX_CHAN];	/* Grid coordinate */

	minv = 1e6;
	maxv = -1e6;

	for (e = 0; e < p->inputChan; e++)
		gc[e] = 0;	/* init coords */

	/* Search the whole table */
	for (tp = p->clutTable, e = 0; e < p->inputChan; tp += p->outputChan) {
		double v;
		if (chan == -1) {
			for (v = 0.0, f = 0; f < p->outputChan; f++)
				v += tp[f];
		} else {
			v = tp[chan];
		}
		if (v < minv) {
			minv = v;
			for (ee = 0; ee < p->inputChan; ee++)
				minp[ee] = gc[ee]/(p->clutPoints-1.0);
		}
		if (v > maxv) {
			maxv = v;
			for (ee = 0; ee < p->inputChan; ee++)
				maxp[ee] = gc[ee]/(p->clutPoints-1.0);
		}

		/* Increment coord */
		for (e = 0; e < p->inputChan; e++) {
			gc[e]++;
			if (gc[e] < p->clutPoints)
				break;	/* No carry */
			gc[e] = 0;
		}
	}
}

/* Convert XYZ throught Luts matrix */
/* Return 0 on success, 1 if clipping occured, 2 on other error */
static int icmLut_lookup_matrix(
icmLut *p,		/* Pointer to Lut object */
double *out,	/* Output array[outputChan] in ICC order - see Table 39 in 6.5.5 */
double *in		/* Input array[inputChan] */
) {
	double t0,t1;	/* Take care if out == in */
	t0     = p->e[0][0] * in[0] + p->e[0][1] * in[1] + p->e[0][2] * in[2];
	t1     = p->e[1][0] * in[0] + p->e[1][1] * in[1] + p->e[1][2] * in[2];
	out[2] = p->e[2][0] * in[0] + p->e[2][1] * in[1] + p->e[2][2] * in[2];
	out[0] = t0;
	out[1] = t1;

	return 0;
}

/* Convert normalized numbers though this Luts input tables. */
/* Return 0 on success, 1 if clipping occured, 2 on other error */
static int icmLut_lookup_input(
icmLut *p,		/* Pointer to Lut object */
double *out,	/* Output array[inputChan] */
double *in		/* Input array[inputChan] */
) {
	int rv = 0;
	int ix,n;
	double inputEnt_1 = (double)(p->inputEnt-1);
	double *table = p->inputTable;

	/* Use linear interpolation */
	for (n = 0; n < p->inputChan; n++, table += p->inputEnt) {
		double val, w;
		val = in[n] * inputEnt_1;
		if (val < 0.0) {
			val = 0.0;
			rv |= 1;
		} else if (val > inputEnt_1) {
			val = inputEnt_1;
			rv |= 1;
		}
		ix = (int)floor(val);		/* Grid coordinate */
		if (ix > (p->inputEnt-2))
			ix = (p->inputEnt-2);
		w = val - (double)ix;		/* weight */
		val = table[ix];
		out[n] = val + w * (table[ix+1] - val);
	}
	return rv;
}

/* Convert normalized numbers though this Luts multi-dimensional table. */
/* using n-linear interpolation. */
static int icmLut_lookup_clut_nl(
/* Return 0 on success, 1 if clipping occured, 2 on other error */
icmLut *p,		/* Pointer to Lut object */
double *out,	/* Output array[inputChan] */
double *in		/* Input array[outputChan] */
) {
	icc *icp = p->icp;
	int rv = 0;
	double *gp;					/* Pointer to grid cube base */
	double co[MAX_CHAN];		/* Coordinate offset with the grid cell */
	double *gw, GW[1 << 8];		/* weight for each grid cube corner */

	if (p->inputChan <= 8) {
		gw = GW;				/* Use stack allocation */
	} else {
		if ((gw = (double *) icp->al->malloc(icp->al, (1 << p->inputChan) * sizeof(double))) == NULL) {
			sprintf(icp->err,"icmLut_lookup_clut: malloc() failed");
			return icp->errc = 2;
		}
	}

	/* We are using an n-linear (ie. Trilinear for 3D input) interpolation. */
	/* The implementation here uses more multiplies that some other schemes, */
	/* (for instance, see "Tri-Linear Interpolation" by Steve Hill, */
	/* Graphics Gems IV, page 521), but has less involved bookeeping, */
	/* needs less local storage for intermediate output values, does fewer */
	/* output and intermediate value reads, and fp multiplies are fast on */
	/* todays processors! */

	/* Compute base index into grid and coordinate offsets */
	{
		int e;
		double clutPoints_1 = (double)(p->clutPoints-1);
		int    clutPoints_2 = p->clutPoints-2;
		gp = p->clutTable;		/* Base of grid array */

		for (e = 0; e < p->inputChan; e++) {
			int x;
			double val;
			val = in[e] * clutPoints_1;
			if (val < 0.0) {
				val = 0.0;
				rv |= 1;
			} else if (val > clutPoints_1) {
				val = clutPoints_1;
				rv |= 1;
			}
			x = (int)floor(val);		/* Grid coordinate */
			if (x > clutPoints_2)
				x = clutPoints_2;
			co[e] = val - (double)x;	/* 1.0 - weight */
			gp += x * p->dinc[e];		/* Add index offset for base of cube */
		}
	}
	/* Compute corner weights needed for interpolation */
	{
		int e, i, g = 1;
		gw[0] = 1.0;
		for (e = 0; e < p->inputChan; e++) {
			for (i = 0; i < g; i++) {
				gw[g+i] = gw[i] * co[e];
				gw[i] *= (1.0 - co[e]);
			}
			g *= 2;
		}
	}
	/* Now compute the output values */
	{
		int i, f;
		double w = gw[0];
		double *d = gp + p->dcube[0];
		for (f = 0; f < p->outputChan; f++)			/* Base of cube */
			out[f] = w * d[f];
		for (i = 1; i < (1 << p->inputChan); i++) {	/* For all other corners of cube */
			w = gw[i];				/* Strength reduce */
			d = gp + p->dcube[i];
			for (f = 0; f < p->outputChan; f++) {
				out[f] += w * d[f];
			}
		}
	}
	if (gw != GW)
		icp->al->free(icp->al, (void *)gw);
	return rv;
}

/* Convert normalized numbers though this Luts multi-dimensional table */
/* using simplex interpolation. */
static int icmLut_lookup_clut_sx(
/* Return 0 on success, 1 if clipping occured, 2 on other error */
icmLut *p,		/* Pointer to Lut object */
double *out,	/* Output array[inputChan] */
double *in		/* Input array[outputChan] */
) {
	int rv = 0;
	double *gp;					/* Pointer to grid cube base */
	double co[MAX_CHAN];		/* Coordinate offset with the grid cell */
	int    si[MAX_CHAN];		/* co[] Sort index, [0] = smalest */

	/* We are using a simplex (ie. tetrahedral for 3D input) interpolation. */
	/* This method is more appropriate for XYZ/RGB/CMYK input spaces, */

	/* Compute base index into grid and coordinate offsets */
	{
		int e;
		double clutPoints_1 = (double)(p->clutPoints-1);
		int    clutPoints_2 = p->clutPoints-2;
		gp = p->clutTable;		/* Base of grid array */

		for (e = 0; e < p->inputChan; e++) {
			int x;
			double val;
			val = in[e] * clutPoints_1;
			if (val < 0.0) {
				val = 0.0;
				rv |= 1;
			} else if (val > clutPoints_1) {
				val = clutPoints_1;
				rv |= 1;
			}
			x = (int)floor(val);		/* Grid coordinate */
			if (x > clutPoints_2)
				x = clutPoints_2;
			co[e] = val - (double)x;	/* 1.0 - weight */
			gp += x * p->dinc[e];		/* Add index offset for base of cube */
		}
	}
	/* Do selection sort on coordinates */
	{
		int e, f;
		for (e = 0; e < p->inputChan; e++)
			si[e] = e;						/* Initial unsorted indexes */
		for (e = 0; e < (p->inputChan-1); e++) {
			double cosn;
			cosn = co[si[e]];				/* Current smallest value */
			for (f = e+1; f < p->inputChan; f++) {	/* Check against rest */
				int tt;
				tt = si[f];
				if (cosn > co[tt]) {
					si[f] = si[e]; 			/* Exchange */
					si[e] = tt;
					cosn = co[tt];
				}
			}
		}
	}
	/* Now compute the weightings, simplex vertices and output values */
	{
		int e, f;
		double w;		/* Current vertex weight */

		w = 1.0 - co[si[p->inputChan-1]];		/* Vertex at base of cell */
		for (f = 0; f < p->outputChan; f++)
			out[f] = w * gp[f];

		for (e = p->inputChan-1; e > 0; e--) {	/* Middle verticies */
			w = co[si[e]] - co[si[e-1]];
			gp += p->dinc[si[e]];				/* Move to top of cell in next largest dimension */
			for (f = 0; f < p->outputChan; f++)
				out[f] += w * gp[f];
		}

		w = co[si[0]];
		gp += p->dinc[si[0]];		/* Far corner from base of cell */
		for (f = 0; f < p->outputChan; f++)
			out[f] += w * gp[f];
	}
	return rv;
}

/* Convert normalized numbers though this Luts output tables. */
/* Return 0 on success, 1 if clipping occured, 2 on other error */
static int icmLut_lookup_output(
icmLut *p,		/* Pointer to Lut object */
double *out,	/* Output array[outputChan] */
double *in		/* Input array[outputChan] */
) {
	int rv = 0;
	int ix,n;
	double outputEnt_1 = (double)(p->outputEnt-1);
	double *table = p->outputTable;

	/* Use linear interpolation */
	for (n = 0; n < p->outputChan; n++, table += p->outputEnt) {
		double val, w;
		val = in[n] * outputEnt_1;
		if (val < 0.0) {
			val = 0.0;
			rv |= 1;
		} else if (val > outputEnt_1) {
			val = outputEnt_1;
			rv |= 1;
		}
		ix = (int)floor(val);		/* Grid coordinate */
		if (ix > (p->outputEnt-2))
			ix = (p->outputEnt-2);
		w = val - (double)ix;		/* weight */
		val = table[ix];
		out[n] = val + w * (table[ix+1] - val);
	}
	return rv;
}

/* ----------------------------------------------- */
/* Pseudo - Hilbert count sequencer */

/* This multi-dimensional count sequence is a distributed */
/* Gray code sequence, with direction reversal on every */
/* alternate power of 2 scale. */
/* It is intended to aid cache coherence in multi-dimensional */
/* regular sampling. It approximates the Hilbert curve sequence. */

/* Initialise, returns total usable count */
unsigned
psh_init(
psh *p,	/* Pointer to structure to initialise */
int      di,	/* Dimensionality */
unsigned res,	/* Size per coordinate */
int co[]		/* Coordinates to initialise (May be NULL) */
) {
	int e;

	p->di = di;
	p->res = res;

	/* Compute bits */
	for (p->bits = 0; (1 << p->bits) < res; p->bits++)
		;

	/* Compute the total count mask */
	p->tmask = ((((unsigned)1) << (p->bits * di))-1);

	/* Compute usable count */
	p->count = 1;
	for (e = 0; e < di; e++)
		p->count *= res;

	p->ix = 0;

	if (co != NULL) {
		for (e = 0; e < di; e++)
			co[e] = 0;
	}

	return p->count;
}

/* Reset the counter */
void
psh_reset(
psh *p	/* Pointer to structure */
) {
	p->ix = 0;
}

/* Increment pseudo-hilbert coordinates */
/* Return non-zero if count rolls over to 0 */
int
psh_inc(
psh *p,	/* Pointer to structure */
int co[]		/* Coordinates to return */
) {
	int di = p->di;
	int res = p->res;
	int bits = p->bits;
	int e;

	do {
		int b;
		int gix;	/* Gray code index */
		
		p->ix = (p->ix + 1) & p->tmask;

		gix = p->ix ^ (p->ix >> 1);		/* Convert to gray code index */
	
		for (e = 0; e < di; e++) 
			co[e] = 0;
		
		for (b = 0; b < bits; b++) {	/* Distribute bits */
			if (b & 1) {
				for (e = di-1; e >= 0; e--)  {		/* In reverse order */
					co[e] |= (gix & 1) << b;
					gix >>= 1;
				}
			} else {
				for (e = 0; e < di; e++)  {			/* In normal order */
					co[e] |= (gix & 1) << b;
					gix >>= 1;
				}
			}
		}
	
		/* Convert from Gray to binary coordinates */
		for (e = 0; e < di; e++)  {
			unsigned sh, tv;

			for(sh = 1, tv = co[e];; sh <<= 1) {
				unsigned ptv = tv;
				tv ^= (tv >> sh);
				if (ptv <= 1 || sh == 16)
					break;
			}
			if (tv >= res)	/* Dumbo filter - increment again if outside cube range */
				break;
			co[e] = tv;
		}

	} while (e < di);
	
	return (p->ix == 0);
}

/* ------------------------------------------------------- */
/* Parameter to getNormFunc function */
typedef enum {
    icmFromLuti   = 0,  /* return "fromo Lut normalized index" conversion function */
    icmToLuti     = 1,  /* return "to Lut normalized index" conversion function */
    icmFromLutv   = 2,  /* return "from Lut normalized value" conversion function */
    icmToLutv     = 3   /* return "to Lut normalized value" conversion function */
} icmNormFlag;

/* Return an appropriate color space normalization function, */
/* given the color space and Lut type */
/* Return 0 on success, 1 on match failure */
static int getNormFunc(
	icColorSpaceSignature csig, 
	icTagTypeSignature    tagSig,
	icmNormFlag           flag,
	void               (**nfunc)(double *out, double *in)
);

/* Helper function to initialize the three tables contents */
/* from supplied transfer functions. */
/* Set errc and return error number */
static int icmLut_set_tables (
icmLut *p,									/* Pointer to Lut object */
void   *cbctx,								/* Opaque callback context pointer value */
icColorSpaceSignature insig, 				/* Input color space */
icColorSpaceSignature outsig, 				/* Output color space */
void (*infunc)(void *cbcntx, double *out, double *in),
								/* Input transfer function, inspace->inspace' (NULL = default) */
double *inmin, double *inmax,				/* Maximum range of inspace' values (NULL = default) */
void (*clutfunc)(void *cbctx, double *out, double *in),
								/* inspace' -> outspace' transfer function */
double *clutmin, double *clutmax,			/* Maximum range of outspace' values (NULL = default) */
void (*outfunc)(void *cbctx, double *out, double *in)
								/* Output transfer function, outspace'->outspace (NULL = deflt) */
) {
	icc *icp = p->icp;
	int n, e;
	int ii[MAX_CHAN];		/* Index value */
	psh counter;			/* Pseudo-Hilbert counter */
	double _iv[2 * MAX_CHAN], *iv = &_iv[MAX_CHAN];	/* Real index value/table value */
	double imin[MAX_CHAN], imax[MAX_CHAN];
	double omin[MAX_CHAN], omax[MAX_CHAN];
	void (*ifromindex)(double *out, double *in);	/* Index to input color space function */
	void (*itoentry)(double *out, double *in);		/* Input color space to entry function */
	void (*ifromentry)(double *out, double *in);	/* Entry to input color space function */
	void (*otoentry)(double *out, double *in);		/* Output colorspace to table value function */
	void (*ofromentry)(double *out, double *in);	/* Table value to output color space function */

	if (getNormFunc(insig, p->ttype, icmFromLuti, &ifromindex) != 0) {
		sprintf(icp->err,"icmLut_set_tables index to input colorspace function lookup failed");
		return icp->errc = 1;
	}
	if (getNormFunc(insig, p->ttype, icmToLutv, &itoentry) != 0) {
		sprintf(icp->err,"icmLut_set_tables input colorspace to table entry function lookup failed");
		return icp->errc = 1;
	}
	if (getNormFunc(insig, p->ttype, icmFromLutv, &ifromentry) != 0) {
		sprintf(icp->err,"icmLut_set_tables table entry to input colorspace function lookup failed");
		return icp->errc = 1;
	}

	if (getNormFunc(outsig, p->ttype, icmToLutv, &otoentry) != 0) {
		sprintf(icp->err,"icmLut_set_tables output colorspace to table entry function lookup failed");
		return icp->errc = 1;
	}
	if (getNormFunc(outsig, p->ttype, icmFromLutv, &ofromentry) != 0) {
		sprintf(icp->err,"icmLut_set_tables table entry to output colorspace function lookup failed");
		return icp->errc = 1;
	}

	/* Setup input table value min-max */
	if (inmin == NULL) {
#ifdef SYMETRICAL_DEFAULT_LAB_RANGE	/* Try symetrical range */
		if (insig == icSigLabData) { /* Special case Lab */
			double mn[3], mx[3];
			/* Because the symetric range will cause slight clipping, */
			/* only do it if the input table has sufficient resolution */
			/* to represent the clipping faithfuly. */
			if (p->inputEnt >= 64) {
				mn[0] =   0.0, mn[1] = -127.0, mn[2] = -127.0;
				mx[0] = 100.0, mx[1] =  127.0, mx[2] =  127.0;
				itoentry(imin, mn);	/* Convert from input color space to table representation */
				itoentry(imax, mx);
			} else {
				for (e = 0; e < p->inputChan; e++) {
					imin[e] = 0.0;
					imax[e] = 1.0;
				}
			}
		} else
#endif
		{
			for (e = 0; e < p->inputChan; e++) {
				imin[e] = 0.0;		/* We are assuming this is true for all other color spaces. */
				imax[e] = 1.0;
			}
		}
	} else {
		itoentry(imin, inmin);	/* Convert from input color space to table representation */
		itoentry(imax, inmax);
	}

	/* Setup output table value min-max */
	if (clutmin == NULL) {
#ifdef SYMETRICAL_DEFAULT_LAB_RANGE	/* Try symetrical range */
		if (outsig == icSigLabData) { /* Special case Lab */
			double mn[3], mx[3];
			/* Because the symetric range will cause slight clipping, */
			/* only do it if the output table has sufficient resolution */
			/* to represent the clipping faithfuly. */
			if (p->outputEnt >= 64) {
				mn[0] =   0.0, mn[1] = -127.0, mn[2] = -127.0;
				mx[0] = 100.0, mx[1] =  127.0, mx[2] =  127.0;
				otoentry(omin, mn);/* Convert from output color space to table representation */
				otoentry(omax, mx);
			} else {
				for (e = 0; e < p->inputChan; e++) {
					omin[e] = 0.0;
					omax[e] = 1.0;
				}
			}
		} else
#endif
		{
			for (e = 0; e < p->outputChan; e++) {
				omin[e] = 0.0;		/* We are assuming this is true for all other color spaces. */
				omax[e] = 1.0;
			}
		}
	} else {
		otoentry(omin, clutmin);/* Convert from output color space to table representation */
		otoentry(omax, clutmax);
	}

	/* Create the input table entry values */
	for (n = 0; n < p->inputEnt; n++) {
		double fv;
		fv = n/(p->inputEnt-1.0);
		for (e = 0; e < p->inputChan; e++)
			iv[e] = fv;

		ifromindex(iv,iv);			/* Convert from index value to input color space value */

		if (infunc != NULL)
			infunc(cbctx, iv, iv);	/* In colorspace -> input table -> In colorspace. */

		itoentry(iv,iv);			/* Convert from input color space value to table value */

		/* Expand used range to 0.0 - 1.0, and clip to legal values */
		/* Note that if the range is reduced, and clipping occurs, */
		/* then there should be enough resolution within the input */
		/* table, to represent the sharp edges of the clipping. */
		for (e = 0; e < p->inputChan; e++) {
			double tt;
			tt = (iv[e] - imin[e])/(imax[e] - imin[e]);
			if (tt < 0.0)
				tt = 0.0;
			else if (tt > 1.0)
				tt = 1.0;
			iv[e] = tt;
		}

		for (e = 0; e < p->inputChan; e++) 		/* Input tables */
			p->inputTable[e * p->inputEnt + n] = iv[e];
	}

	/* Create the multi-dimensional lookup table values */

	/* To make this clut function cache friendly, we use the pseudo-hilbert */
	/* count sequence. This keeps each point close to the last in the */
	/* multi-dimensional space. */ 

	psh_init(&counter, p->inputChan, p->clutPoints, ii);	/* Initialise counter */

	/* Itterate through all verticies in the grid */
	for (;;) {
		int ti;			/* Table index */
	
		for (ti = e = 0; e < p->inputChan; e++) { 	/* Input tables */
			ti += ii[e] * p->dinc[e];				/* Clut index */
			iv[e] = ii[e]/(p->clutPoints-1.0);		/* Vertex coordinates */
			iv[e] = iv[e] * (imax[e] - imin[e]) + imin[e]; /* Undo expansion to 0.0 - 1.0 */
			*((int *)&iv[-e-1]) = ii[e];			/* Trick to supply grid index in iv[] */
		}
	
		ifromentry(iv,iv);			/* Convert from table value to input color space */
	
		/* Apply incolor -> outcolor function we want to represent */
		clutfunc(cbctx, iv, iv);
	
		otoentry(iv,iv);			/* Convert from output color space value to table value */
	
		/* Expand used range to 0.0 - 1.0, and clip to legal values */
		for (e = 0; e < p->outputChan; e++) {
			double tt;
			tt = (iv[e] - omin[e])/(omax[e] - omin[e]);
			if (tt < 0.0)
				tt = 0.0;
			else if (tt > 1.0)
				tt = 1.0;
			iv[e] = tt;
		}
	
		for (e = 0; e < p->outputChan; e++) 	/* Output chans */
			p->clutTable[ti++] = iv[e];
	
		/* Increment index within block (Reverse index significancd) */
		if (psh_inc(&counter, ii))
			break;
	}

	/* Create the output table entry values */
	for (n = 0; n < p->outputEnt; n++) {
		double fv;
		fv = n/(p->outputEnt-1.0);
		for (e = 0; e < p->outputChan; e++)
			iv[e] = fv;

		/* Undo expansion to 0.0 - 1.0 */
		for (e = 0; e < p->outputChan; e++) 		/* Output tables */
			iv[e] = iv[e] * (omax[e] - omin[e]) + omin[e];

		ofromentry(iv,iv);			/* Convert from table value to output color space value */

		if (outfunc != NULL)
			outfunc(cbctx, iv, iv);	/* Out colorspace -> output table -> out colorspace. */

		otoentry(iv,iv);			/* Convert from output color space value to table value */

		/* Clip to legal values */
		for (e = 0; e < p->outputChan; e++) {
			double tt;
			tt = iv[e];
			if (tt < 0.0)
				tt = 0.0;
			else if (tt > 1.0)
				tt = 1.0;
			iv[e] = tt;
		}

		for (e = 0; e < p->outputChan; e++) 		/* Input tables */
			p->outputTable[e * p->outputEnt + n] = iv[e];
	}
	return 0;
}

/* - - - - - - - - - - - - - - - - */
/* Return the number of bytes needed to write this tag */
static unsigned int icmLut_get_size(
	icmBase *pp
) {
	icmLut *p = (icmLut *)pp;
	unsigned int len = 0;

	if (p->ttype == icSigLut8Type) {
		len += 48;			/* tag and header */
		len += 1 * (p->inputChan * p->inputEnt);
		len += 1 * (p->outputChan * uipow(p->clutPoints,p->inputChan));
		len += 1 * (p->outputChan * p->outputEnt);
	} else {
		len += 52;			/* tag and header */
		len += 2 * (p->inputChan * p->inputEnt);
		len += 2 * (p->outputChan * uipow(p->clutPoints,p->inputChan));
		len += 2 * (p->outputChan * p->outputEnt);
	}
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmLut_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmLut *p = (icmLut *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i, j, g, size;
	char *bp, *buf;

	if (len < 4) {
		sprintf(icp->err,"icmLut_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmLut_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmLut_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	p->ttype = (icTagTypeSignature)read_SInt32Number(bp);
	if (p->ttype != icSigLut8Type && p->ttype != icSigLut16Type) {
		sprintf(icp->err,"icmLut_read: Wrong tag type for icmLut");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	if (p->ttype == icSigLut8Type) {
		if (len < 48) {
			sprintf(icp->err,"icmLut_read: Tag too small to be legal");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
	} else {
		if (len < 52) {
			sprintf(icp->err,"icmLut_read: Tag too small to be legal");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
	}

	/* Read in the info common to 8 and 16 bit Lut */
	p->inputChan = read_UInt8Number(bp+8);
	p->outputChan = read_UInt8Number(bp+9);
	p->clutPoints = read_UInt8Number(bp+10);

	/* Sanity check */
	if (p->inputChan > MAX_CHAN) {
		sprintf(icp->err,"icmLut_read: Can't handle > %d input channels\n",MAX_CHAN);
		return icp->errc = 1;
	}

	if (p->outputChan > MAX_CHAN) {
		sprintf(icp->err,"icmLut_read: Can't handle > %d output channels\n",MAX_CHAN);
		return icp->errc = 1;
	}

	/* Read 3x3 transform matrix */
	for (j = 0; j < 3; j++) {		/* Rows */
		for (i = 0; i < 3; i++) {	/* Columns */
			p->e[j][i] = read_S15Fixed16Number(bp + 12 + ((j * 3 + i) * 4));
		}
	}
	/* Read 16 bit specific stuff */
	if (p->ttype == icSigLut8Type) {
		p->inputEnt  = 256;	/* By definition */
		p->outputEnt = 256;	/* By definition */
		bp = buf+48;
	} else {
		p->inputEnt  = read_UInt16Number(bp+48);
		p->outputEnt = read_UInt16Number(bp+50);
		bp = buf+52;
	}

	if (len < icmLut_get_size((icmBase *)p)) {
		sprintf(icp->err,"icmLut_read: Tag too small for contents");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read the input tables */
	size = (p->inputChan * p->inputEnt);
	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}
	if (p->ttype == icSigLut8Type) {
		for (i = 0; i < size; i++, bp += 1)
			p->inputTable[i] = read_DCS8Number(bp);
	} else {
		for (i = 0; i < size; i++, bp += 2)
			p->inputTable[i] = read_DCS16Number(bp);
	}

	/* Read the clut table */
	size = (p->outputChan * uipow(p->clutPoints,p->inputChan));
	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}
	if (p->ttype == icSigLut8Type) {
		for (i = 0; i < size; i++, bp += 1)
			p->clutTable[i] = read_DCS8Number(bp);
	} else {
		for (i = 0; i < size; i++, bp += 2)
			p->clutTable[i] = read_DCS16Number(bp);
	}

	/* Read the output tables */
	size = (p->outputChan * p->outputEnt);
	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}
	if (p->ttype == icSigLut8Type) {
		for (i = 0; i < size; i++, bp += 1)
			p->outputTable[i] = read_DCS8Number(bp);
	} else {
		for (i = 0; i < size; i++, bp += 2)
			p->outputTable[i] = read_DCS16Number(bp);
	}

	/* Private: compute dimensional increment though clut */
	/* Note that first channel varies least rapidly. */
	i = p->inputChan-1;
	p->dinc[i--] = p->outputChan;
	for (; i < p->inputChan; i--)
		p->dinc[i] = p->dinc[i+1] * p->clutPoints;

	/* Private: compute offsets from base of cube to other corners */
	for (p->dcube[0] = 0, g = 1, j = 0; j < p->inputChan; j++) {
		for (i = 0; i < g; i++)
			p->dcube[g+i] = p->dcube[i] + p->dinc[j];
		g *= 2;
	}

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmLut_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmLut *p = (icmLut *)pp;
	icc *icp = p->icp;
	unsigned long i,j;
	unsigned int len, size;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmLut_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmLut_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write the info common to 8 and 16 bit Lut */
	if ((rv = write_UInt8Number(p->inputChan, bp+8)) != 0) {
		sprintf(icp->err,"icmLut_write: write_UInt8Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_UInt8Number(p->outputChan, bp+9)) != 0) {
		sprintf(icp->err,"icmLut_write: write_UInt8Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_UInt8Number(p->clutPoints, bp+10)) != 0) {
		sprintf(icp->err,"icmLut_write: write_UInt8Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write 3x3 transform matrix */
	for (j = 0; j < 3; j++) {		/* Rows */
		for (i = 0; i < 3; i++) {	/* Columns */
			if ((rv = write_S15Fixed16Number(p->e[j][i],bp + 12 + ((j * 3 + i) * 4))) != 0) {
				sprintf(icp->err,"icmLut_write: write_S15Fixed16Number() failed");
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	}

	/* Write 16 bit specific stuff */
	if (p->ttype == icSigLut8Type) {
		if (p->inputEnt != 256 || p->outputEnt != 256) {
			sprintf(icp->err,"icmLut_write: 8 bit Input and Output tables must be 256 entries");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		bp = buf+48;
	} else {
		if ((rv = write_UInt16Number(p->inputEnt, bp+48)) != 0) {
			sprintf(icp->err,"icmLut_write: write_UInt16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_UInt16Number(p->outputEnt, bp+50)) != 0) {
			sprintf(icp->err,"icmLut_write: write_UInt16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		bp = buf+52;
	}

	/* Write the input tables */
	size = (p->inputChan * p->inputEnt);
	if (p->ttype == icSigLut8Type) {
		for (i = 0; i < size; i++, bp += 1) {
			if ((rv = write_DCS8Number(p->inputTable[i], bp)) != 0) {
				sprintf(icp->err,"icmLut_write: inputTable write_DCS8Number() failed");
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	} else {
		for (i = 0; i < size; i++, bp += 2) {
			if ((rv = write_DCS16Number(p->inputTable[i], bp)) != 0) {
				sprintf(icp->err,"icmLut_write: inputTable write_DCS16Number(%f) failed",p->inputTable[i]);
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	}

	/* Write the clut table */
	size = (p->outputChan * uipow(p->clutPoints,p->inputChan));
	if (p->ttype == icSigLut8Type) {
		for (i = 0; i < size; i++, bp += 1) {
			if ((rv = write_DCS8Number(p->clutTable[i], bp)) != 0) {
				sprintf(icp->err,"icmLut_write: clutTable write_DCS8Number() failed");
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	} else {
		for (i = 0; i < size; i++, bp += 2) {
			if ((rv = write_DCS16Number(p->clutTable[i], bp)) != 0) {
				sprintf(icp->err,"icmLut_write: clutTable write_DCS16Number(%f) failed",p->clutTable[i]);
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	}

	/* Write the output tables */
	size = (p->outputChan * p->outputEnt);
	if (p->ttype == icSigLut8Type) {
		for (i = 0; i < size; i++, bp += 1) {
			if ((rv = write_DCS8Number(p->outputTable[i], bp)) != 0) {
				sprintf(icp->err,"icmLut_write: outputTable write_DCS8Number() failed");
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	} else {
		for (i = 0; i < size; i++, bp += 2) {
			if ((rv = write_DCS16Number(p->outputTable[i], bp)) != 0) {
				sprintf(icp->err,"icmLut_write: outputTable write_DCS16Number(%f) failed",p->outputTable[i]);
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	}

	/* Write buffer to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmLut_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmLut_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmLut *p = (icmLut *)pp;
	if (verb <= 0)
		return;

	if (p->ttype == icSigLut8Type) {
		fprintf(op,"Lut8:\n");
	} else {
		fprintf(op,"Lut16:\n");
	}
	fprintf(op,"  Input Channels = %u\n",p->inputChan);
	fprintf(op,"  Output Channels = %u\n",p->outputChan);
	fprintf(op,"  CLUT resolution = %u\n",p->clutPoints);
	fprintf(op,"  Input Table entries = %u\n",p->inputEnt);
	fprintf(op,"  Output Table entries = %u\n",p->outputEnt);
	fprintf(op,"  XYZ matrix =  %f, %f, %f\n",p->e[0][0],p->e[0][1],p->e[0][2]);
	fprintf(op,"                %f, %f, %f\n",p->e[1][0],p->e[1][1],p->e[1][2]);
	fprintf(op,"                %f, %f, %f\n",p->e[2][0],p->e[2][1],p->e[2][2]);

	if (verb >= 2) {
		unsigned int i,size;
		int j;
		unsigned int ii[MAX_CHAN];	/* maximum no of input channels */

		fprintf(op,"  Input table:\n");
		for (i = 0; i < p->inputEnt; i++) {
			fprintf(op,"    %3u: ",i);
			for (j = 0; j < p->inputChan; j++)
				fprintf(op," %1.10f",p->inputTable[j * p->inputEnt + i]);
			fprintf(op,"\n");
		}

		fprintf(op,"\n  CLUT table:\n");
		if (p->inputChan > MAX_CHAN) {
			fprintf(op,"  !!Can't dump > %d input channel CLUT table!!\n",MAX_CHAN);
		} else {
			size = (p->outputChan * uipow(p->clutPoints,p->inputChan));
			for (j = 0; j < p->inputChan; j++)
				ii[j] = 0;
			for (i = 0; i < size;) {
				int k;
				/* Print table entry index */
				fprintf(op,"   ");
				for (j = p->inputChan-1; j >= 0; j--)
					fprintf(op," %2u",ii[j]);
				fprintf(op,":");
				/* Print table entry contents */
				for (k = 0; k < p->outputChan; k++, i++)
					fprintf(op," %1.10f",p->clutTable[i]);
				fprintf(op,"\n");
			
				for (j = 0; j < p->inputChan; j++) { /* Increment index */
					ii[j]++;
					if (ii[j] < p->clutPoints)
						break;	/* No carry */
					ii[j] = 0;
				}
			}
		}

		fprintf(op,"\n  Output table:\n");
		for (i = 0; i < p->outputEnt; i++) {
			fprintf(op,"    %3u: ",i);
			for (j = 0; j < p->outputChan; j++)
				fprintf(op," %1.10f",p->outputTable[j * p->outputEnt + i]);
			fprintf(op,"\n");
		}

	}
}

/* Allocate variable sized data elements */
static int icmLut_allocate(
	icmBase *pp
) {
	unsigned int i, j, g, size;
	icmLut *p = (icmLut *)pp;
	icc *icp = p->icp;

	/* Sanity check */
	if (p->inputChan > MAX_CHAN) {
		sprintf(icp->err,"icmLut_alloc: Can't handle > %d input channels\n",MAX_CHAN);
		return icp->errc = 1;
	}

	if (p->outputChan > MAX_CHAN) {
		sprintf(icp->err,"icmLut_alloc: Can't handle > %d output channels\n",MAX_CHAN);
		return icp->errc = 1;
	}

	size = (p->inputChan * p->inputEnt);
	if (size != p->inputTable_size) {
		if (p->inputTable != NULL)
			icp->al->free(icp->al, p->inputTable);
		if ((p->inputTable = (double *) icp->al->calloc(icp->al,sizeof(double), size)) == NULL) {
			sprintf(icp->err,"icmLut_alloc: calloc() of Lut inputTable data failed");
			return icp->errc = 2;
		}
		p->inputTable_size = size;
	}
	size = (p->outputChan * uipow(p->clutPoints,p->inputChan));
	if (size != p->clutTable_size) {
		if (p->clutTable != NULL)
			icp->al->free(icp->al, p->clutTable);
		if ((p->clutTable = (double *) icp->al->calloc(icp->al,sizeof(double), size)) == NULL) {
			sprintf(icp->err,"icmLut_alloc: calloc() of Lut clutTable data failed");
			return icp->errc = 2;
		}
		p->clutTable_size = size;
	}
	size = (p->outputChan * p->outputEnt);
	if (size != p->outputTable_size) {
		if (p->outputTable != NULL)
			icp->al->free(icp->al, p->outputTable);
		if ((p->outputTable = (double *) icp->al->calloc(icp->al,sizeof(double), size)) == NULL) {
			sprintf(icp->err,"icmLut_alloc: calloc() of Lut outputTable data failed");
			return icp->errc = 2;
		}
		p->outputTable_size = size;
	}

	/* Private: compute dimensional increment though clut */
	/* Note that first channel varies least rapidly. */
	i = p->inputChan-1;
	p->dinc[i--] = p->outputChan;
	for (; i < p->inputChan; i--)
		p->dinc[i] = p->dinc[i+1] * p->clutPoints;

	/* Private: compute offsets from base of cube to other corners */
	for (p->dcube[0] = 0, g = 1, j = 0; j < p->inputChan; j++) {
		for (i = 0; i < g; i++)
			p->dcube[g+i] = p->dcube[i] + p->dinc[j];
		g *= 2;
	}
	
	return 0;
}

/* Free all storage in the object */
static void icmLut_delete(
	icmBase *pp
) {
	icmLut *p = (icmLut *)pp;
	icc *icp = p->icp;

	if (p->inputTable != NULL)
		icp->al->free(icp->al, p->inputTable);
	if (p->clutTable != NULL)
		icp->al->free(icp->al, p->clutTable);
	if (p->outputTable != NULL)
		icp->al->free(icp->al, p->outputTable);
	icmTable_delete_bwd(icp, &p->rit);
	icmTable_delete_bwd(icp, &p->rot);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmLut(
	icc *icp
) {
	int i,j;
	icmLut *p;
	if ((p = (icmLut *) icp->al->calloc(icp->al,1,sizeof(icmLut))) == NULL)
		return NULL;
	p->ttype    = icSigLut16Type;
	p->refcount = 1;
	p->get_size = icmLut_get_size;
	p->read     = icmLut_read;
	p->write    = icmLut_write;
	p->dump     = icmLut_dump;
	p->allocate = icmLut_allocate;
	p->del      = icmLut_delete;

	/* Lookup methods */
	p->nu_matrix      = icmLut_nu_matrix;
	p->min_max        = icmLut_min_max;
	p->lookup_matrix  = icmLut_lookup_matrix;
	p->lookup_input   = icmLut_lookup_input;
	p->lookup_clut_nl = icmLut_lookup_clut_nl;
	p->lookup_clut_sx = icmLut_lookup_clut_sx;
	p->lookup_output  = icmLut_lookup_output;

	/* Set method */
	p->set_tables = icmLut_set_tables;

	p->icp      = icp;

	/* Set matrix to reasonable default */
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++) {
			if (i == j)
				p->e[i][j] = 1.0;
			else
				p->e[i][j] = 0.0;
		}

	/* Init lookups to non-dangerous values */
	for (i = 0; i < MAX_CHAN; i++)
		p->dinc[i] = 0;

	for (i = 0; i < (1 << MAX_CHAN); i++)
		p->dcube[i] = 0;

	p->rit.inited = 0;
	p->rot.inited = 0;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* Measurement */

/* Return the number of bytes needed to write this tag */
static unsigned int icmMeasurement_get_size(
	icmBase *pp
) {
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += 4;			/* 4 for standard observer */
	len += 12;			/* 12 for XYZ of measurement backing */
	len += 4;			/* 4 for measurement geometry */
	len += 4;			/* 4 for measurement flare */
	len += 4;			/* 4 for standard illuminant */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmMeasurement_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmMeasurement *p = (icmMeasurement *)pp;
	icc *icp = p->icp;
	int rv;
	char *bp, *buf;

	if (len < 36) {
		sprintf(icp->err,"icmMeasurement_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmMeasurement_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmMeasurement_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmMeasurement_read: Wrong tag type for icmMeasurement");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read the encoded standard observer */
	p->observer = (icStandardObserver)read_SInt32Number(bp + 8);

	/* Read the XYZ values for measurement backing */
	if ((rv = read_XYZNumber(&p->backing, bp+12)) != 0) {
		sprintf(icp->err,"icmMeasurement: read_XYZNumber error");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Read the encoded measurement geometry */
	p->geometry = (icMeasurementGeometry)read_SInt32Number(bp + 24);

	/* Read the proportion of flare  */
	p->flare = read_U16Fixed16Number(bp + 28);

	/* Read the encoded standard illuminant */
	p->illuminant = (icIlluminant)read_SInt32Number(bp + 32);

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmMeasurement_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmMeasurement *p = (icmMeasurement *)pp;
	icc *icp = p->icp;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmMeasurement_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmMeasurement_write, type: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write the encoded standard observer */
	if ((rv = write_SInt32Number((int)p->observer, bp + 8)) != 0) {
		sprintf(icp->err,"icmMeasurementa_write, observer: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write the XYZ values for measurement backing */
	if ((rv = write_XYZNumber(&p->backing, bp+12)) != 0) {
		sprintf(icp->err,"icmMeasurement, backing: write_XYZNumber error");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write the encoded measurement geometry */
	if ((rv = write_SInt32Number((int)p->geometry, bp + 24)) != 0) {
		sprintf(icp->err,"icmMeasurementa_write, geometry: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write the proportion of flare */
	if ((rv = write_U16Fixed16Number(p->flare, bp + 28)) != 0) {
		sprintf(icp->err,"icmMeasurementa_write, flare: write_U16Fixed16Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write the encoded standard illuminant */
	if ((rv = write_SInt32Number((int)p->illuminant, bp + 32)) != 0) {
		sprintf(icp->err,"icmMeasurementa_write, illuminant: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmMeasurement_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmMeasurement_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmMeasurement *p = (icmMeasurement *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"Measurement:\n");
	fprintf(op,"  Standard Observer = %s\n", string_StandardObserver(p->observer));
	fprintf(op,"  XYZ for Measurement Backing = %s\n", string_XYZNumber_and_Lab(&p->backing));
	fprintf(op,"  Measurement Geometry = %s\n", string_MeasurementGeometry(p->geometry));
	fprintf(op,"  Measurement Flare = %5.1f%%\n", p->flare * 100.0);
	fprintf(op,"  Standard Illuminant = %s\n", string_Illuminant(p->illuminant));
}

/* Allocate variable sized data elements */
static int icmMeasurement_allocate(
	icmBase *pp
) {
	/* Nothing to do */
	return 0;
}

/* Free all storage in the object */
static void icmMeasurement_delete(
	icmBase *pp
) {
	icmMeasurement *p = (icmMeasurement *)pp;
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmMeasurement(
	icc *icp
) {
	icmMeasurement *p;
	if ((p = (icmMeasurement *) icp->al->calloc(icp->al,1,sizeof(icmMeasurement))) == NULL)
		return NULL;
	p->ttype    = icSigMeasurementType;
	p->refcount = 1;
	p->get_size = icmMeasurement_get_size;
	p->read     = icmMeasurement_read;
	p->write    = icmMeasurement_write;
	p->dump     = icmMeasurement_dump;
	p->allocate = icmMeasurement_allocate;
	p->del      = icmMeasurement_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */

/* Named color structure read/write support */
static int read_NamedColorVal(
	icmNamedColorVal *p,
	char *bp,
	char *end,
	icColorSpaceSignature pcs,		/* Header Profile Connection Space */
	unsigned int ndc				/* Number of device corrds */
) {
	icc *icp = p->icp;
	int i;
	unsigned int mxl;	/* Max possible string length */

	mxl = (end - bp) < 32 ? (end - bp) : 32;
	if (check_null_string(bp,mxl)) {
		sprintf(icp->err,"icmNamedColorVal_read: Root name string not terminated");
		return icp->errc = 1;
	}
	strcpy((void *)p->root, (void *)bp);
	bp += strlen(p->root) + 1;
	if ((bp + ndc) > end) {
		sprintf(icp->err,"icmNamedColorVal_read: Data too short to read device coords");
		return icp->errc = 1;
	}
	for (i = 0; i < ndc; i++) {
		p->deviceCoords[i] = read_DCS8Number(bp);
		bp += 1;
	}
	return 0;
}

static int read_NamedColorVal2(
	icmNamedColorVal *p,
	char *bp,
	char *end,
	icColorSpaceSignature pcs,		/* Header Profile Connection Space */
	unsigned int ndc				/* Number of device corrds */
) {
	icc *icp = p->icp;
	int i;
	if ((bp + 32 + 6 + ndc * 2) > end) {
		sprintf(icp->err,"icmNamedColorVal2_read: Data too short to read");
		return icp->errc = 1;
	}
	if (check_null_string(bp,32)) {
		sprintf(icp->err,"icmNamedColorVal2_read: Root name string not terminated");
		return icp->errc = 1;
	}
	memcpy((void *)p->root,(void *)(bp + 0),32);
	switch(pcs) {
		case icSigXYZData:
				p->pcsCoords[0] = read_PCSXYZ16Number(bp+32);
				p->pcsCoords[1] = read_PCSXYZ16Number(bp+34);
				p->pcsCoords[2] = read_PCSXYZ16Number(bp+36);
			break;
	   	case icSigLabData:
				p->pcsCoords[0] =  read_PCSL16Number(bp+32);
				p->pcsCoords[1] = read_PCSab16Number(bp+34);
				p->pcsCoords[2] = read_PCSab16Number(bp+36);
			break;
		default:
			return 1;		/* Unknown PCS */
	}
	for (i = 0; i < ndc; i++)
		p->deviceCoords[i] = read_DCS16Number(bp + 32 + 6 + 2 * i);
	return 0;
}

static int write_NamedColorVal(
	icmNamedColorVal *p,
	char *d,
	icColorSpaceSignature pcs,		/* Header Profile Connection Space */
	unsigned int ndc				/* Number of device corrds */
) {
	icc *icp = p->icp;
	int i, rv = 0;
	if (check_null_string(p->root,32) != 0) {
		sprintf(icp->err,"icmNamedColorVal_write: Root string names is unterminated");
		return icp->errc = 1;
	}
	strcpy((void *)d,(void *)p->root);
	d += strlen(p->root) + 1;
	for (i = 0; i < ndc; i++) {
		if ((rv = write_DCS8Number(p->deviceCoords[i], d)) != 0) {
			sprintf(icp->err,"icmNamedColorVal_write: write of device coord failed");
			return icp->errc = 1;
		}
		d += 1;
	}
	return 0;
}

static int write_NamedColorVal2(
	icmNamedColorVal *p,
	char *bp,
	icColorSpaceSignature pcs,		/* Header Profile Connection Space */
	unsigned int ndc				/* Number of device corrds */
) {
	icc *icp = p->icp;
	int i, rv = 0;
	if (check_null_string(p->root,32)) {
		sprintf(icp->err,"icmNamedColorVal2_write: Root string names is unterminated");
		return icp->errc = 1;
	}
	memcpy((void *)(bp + 0),(void *)p->root,32);
	switch(pcs) {
		case icSigXYZData:
				rv |= write_PCSXYZ16Number(p->pcsCoords[0], bp+32);
				rv |= write_PCSXYZ16Number(p->pcsCoords[1], bp+34);
				rv |= write_PCSXYZ16Number(p->pcsCoords[2], bp+36);
			break;
    	case icSigLabData:
				rv |=  write_PCSL16Number(p->pcsCoords[0], bp+32);
				rv |= write_PCSab16Number(p->pcsCoords[1], bp+34);
				rv |= write_PCSab16Number(p->pcsCoords[2], bp+36);
			break;
		default:
			sprintf(icp->err,"icmNamedColorVal2_write: Unknown PCS");
			return icp->errc = 1;
	}
	if (rv) {
		sprintf(icp->err,"icmNamedColorVal2_write: write of PCS coord failed");
		return icp->errc = 1;
	}
	for (i = 0; i < ndc; i++) {
		if ((rv = write_DCS16Number(p->deviceCoords[i], bp + 32 + 6 + 2 * i)) != 0) {
			sprintf(icp->err,"icmNamedColorVal2_write: write of device coord failed");
			return icp->errc = 1;
		}
	}
	return 0;
}

/* - - - - - - - - - - - */
/* icmNamedColor object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmNamedColor_get_size(
	icmBase *pp
) {
	icmNamedColor *p = (icmNamedColor *)pp;
	unsigned int len = 0;
	if (p->ttype == icSigNamedColorType) {
		unsigned int i;
		len += 8;			/* 8 bytes for tag and padding */
		len += 4;			/* 4 for vendor specific flags */
		len += 4;			/* 4 for count of named colors */
		len += strlen(p->prefix) + 1; /* prefix of color names */
		len += strlen(p->suffix) + 1; /* suffix of color names */
		for (i = 0; i < p->count; i++) {
			len += strlen(p->data[i].root) + 1; /* color names */
			len += p->nDeviceCoords * 1;	/* bytes for each named color */
		}
	} else {	/* Named Color 2 */
		len += 8;			/* 8 bytes for tag and padding */
		len += 4;			/* 4 for vendor specific flags */
		len += 4;			/* 4 for count of named colors */
		len += 4;			/* 4 for number of device coords */
		len += 32;			/* 32 for prefix of color names */
		len += 32;			/* 32 for suffix of color names */
		len += p->count * (32 + 6 + p->nDeviceCoords * 2);	/* bytes for each named color */
	}
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmNamedColor_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmNamedColor *p = (icmNamedColor *)pp;
	icc *icp = p->icp;
	unsigned long i;
	char *bp, *buf, *end;
	int rv = 0;

	if (len < 4) {
		sprintf(icp->err,"icmNamedColor_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmNamedColor_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;
	end = buf + len;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmNamedColor_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	p->ttype = (icTagTypeSignature)read_SInt32Number(bp);
	if (p->ttype != icSigNamedColorType && p->ttype != icSigNamedColor2Type) {
		sprintf(icp->err,"icmNamedColor_read: Wrong tag type for icmNamedColor");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	if (p->ttype == icSigNamedColorType) {
		if (len < 16) {
			sprintf(icp->err,"icmNamedColor_read: Tag too small to be legal");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		/* Make sure that the number of device coords in known */
		p->nDeviceCoords = number_ColorSpaceSignature(icp->header->colorSpace);
		if (p->nDeviceCoords > MAX_CHAN) {
			sprintf(icp->err,"icmNamedColor_read: Can't handle more than %d device channels",MAX_CHAN);
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}

	} else {	/* icmNC2 */
		if (len < 84) {
			sprintf(icp->err,"icmNamedColor_read: Tag too small to be legal");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
	}

	/* Read vendor specific flag */
	p->vendorFlag = read_UInt32Number(bp+8);

	/* Read count of named colors */
	p->count = read_UInt32Number(bp+12);

	if (p->ttype == icSigNamedColorType) {
		unsigned int mxl;	/* Max possible string length */
		bp = bp + 16;

		/* Prefix for each color name */
		mxl = (end - bp) < 32 ? (end - bp) : 32;
		if (check_null_string(bp,mxl) != 0) {
			sprintf(icp->err,"icmNamedColor_read: Color prefix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		strcpy((void *)p->prefix, (void *)bp);
		bp += strlen(p->prefix) + 1;
	
		/* Suffix for each color name */
		mxl = (end - bp) < 32 ? (end - bp) : 32;
		if (check_null_string(bp,mxl) != 0) {
			sprintf(icp->err,"icmNamedColor_read: Color suffix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		strcpy((void *)p->suffix, (void *)bp);
		bp += strlen(p->suffix) + 1;
	
		if ((rv = p->allocate((void *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
	
		/* Read all the data from the buffer */
		for (i = 0; i < p->count; i++) {
			if ((rv = read_NamedColorVal(p->data+i, bp, end, icp->header->pcs, p->nDeviceCoords)) != 0) {
				icp->al->free(icp->al, buf);
				return rv;
			}
			bp += strlen(p->data[i].root) + 1;
			bp += p->nDeviceCoords * 1;
		}
	} else {  /* icmNC2 */
		/* Number of device coords per color */
		p->nDeviceCoords = read_UInt32Number(bp+16);
	
		/* Prefix for each color name */
		memcpy((void *)p->prefix, (void *)(bp + 20), 32);
		if (check_null_string(p->prefix,32) != 0) {
			sprintf(icp->err,"icmNamedColor_read: Color prefix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
	
		/* Suffix for each color name */
		memcpy((void *)p->suffix, (void *)(bp + 52), 32);
		if (check_null_string(p->suffix,32) != 0) {
			sprintf(icp->err,"icmNamedColor_read: Color suffix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
	
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
	
		/* Read all the data from the buffer */
		bp = bp + 84;
		for (i = 0; i < p->count; i++, bp += (32 + 6 + p->nDeviceCoords * 2)) {
			if ((rv = read_NamedColorVal2(p->data+i, bp, end, icp->header->pcs, p->nDeviceCoords)) != 0) {
				icp->al->free(icp->al, buf);
				return rv;
			}
		}
	}
	icp->al->free(icp->al, buf);
	return rv;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmNamedColor_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmNamedColor *p = (icmNamedColor *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmNamedColor_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmNamedColor_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write vendor specific flag */
	if ((rv = write_UInt32Number(p->vendorFlag, bp+8)) != 0) {
		sprintf(icp->err,"icmNamedColor_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write count of named colors */
	if ((rv = write_UInt32Number(p->count, bp+12)) != 0) {
		sprintf(icp->err,"icmNamedColor_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	if (p->ttype == icSigNamedColorType) {
		bp = bp + 16;
	
		/* Prefix for each color name */
		if ((rv = check_null_string(p->prefix,32)) != 0) {
			sprintf(icp->err,"icmNamedColor_write: Color prefix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		strcpy((void *)bp, (void *)p->prefix);
		bp += strlen(p->prefix) + 1;
	
		/* Suffix for each color name */
		if (check_null_string(p->suffix,32)) {
			sprintf(icp->err,"icmNamedColor_write: Color sufix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		strcpy((void *)bp, (void *)p->suffix);
		bp += strlen(p->suffix) + 1;
	
		/* Write all the data to the buffer */

		for (i = 0; i < p->count; i++) {
			if ((rv = write_NamedColorVal(p->data+i, bp, icp->header->pcs, p->nDeviceCoords)) != 0) {
				icp->al->free(icp->al, buf);
				return rv;
			}
			bp += strlen(p->data[i].root) + 1;
			bp += p->nDeviceCoords * 1;
		}
	} else {	/* icmNC2 */
		/* Number of device coords per color */
		if ((rv = write_UInt32Number(p->nDeviceCoords, bp+16)) != 0) {
			sprintf(icp->err,"icmNamedColor_write: write_UInt32Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	
		/* Prefix for each color name */
		if ((rv = check_null_string(p->prefix,32)) != 0) {
			sprintf(icp->err,"icmNamedColor_write: Color prefix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		memcpy((void *)(bp + 20), (void *)p->prefix, 32);
	
		/* Suffix for each color name */
		if (check_null_string(p->suffix,32)) {
			sprintf(icp->err,"icmNamedColor_write: Color sufix is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		memcpy((void *)(bp + 52), (void *)p->suffix, 32);
	
		/* Write all the data to the buffer */
		bp = bp + 84;
		for (i = 0; i < p->count; i++, bp += (32 + 6 + p->nDeviceCoords * 2)) {
			if ((rv = write_NamedColorVal2(p->data+i, bp, icp->header->pcs, p->nDeviceCoords)) != 0) {
				icp->al->free(icp->al, buf);
				return rv;
			}
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmNamedColor_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmNamedColor_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmNamedColor *p = (icmNamedColor *)pp;
	icc *icp = p->icp;
	if (verb <= 0)
		return;

	if (p->ttype == icSigNamedColorType)
		fprintf(op,"NamedColor:\n");
	else
		fprintf(op,"NamedColor2:\n");
	fprintf(op,"  Vendor Flag = 0x%x\n",p->vendorFlag);
	fprintf(op,"  No. colors  = %u\n",p->count);
	fprintf(op,"  No. dev. coords = %u\n",p->nDeviceCoords);
	fprintf(op,"  Name prefix = '%s'\n",p->prefix);
	fprintf(op,"  Name suffix = '%s'\n",p->suffix);
	if (verb >= 2) {
		unsigned long i, n;
		icmNamedColorVal *vp;
		for (i = 0; i < p->count; i++) {
			vp = p->data + i;
			fprintf(op,"    Color %lu:\n",i);
			fprintf(op,"      Name root = '%s'\n",vp->root);

			if (p->ttype == icSigNamedColor2Type) {
				switch(icp->header->pcs) {
					case icSigXYZData:
							fprintf(op,"      XYZ = %f, %f, %f'\n",
							        vp->pcsCoords[0],vp->pcsCoords[1],vp->pcsCoords[2]);
						break;
			    	case icSigLabData:
							fprintf(op,"      Lab = %f, %f, %f'\n",
							        vp->pcsCoords[0],vp->pcsCoords[1],vp->pcsCoords[2]);
						break;
					default:
							fprintf(op,"      Unexpected PCS\n");
						break;
				}
			}
			if (p->nDeviceCoords > 0) {
				fprintf(op,"      Device Coords = ");
				for (n = 0; n < p->nDeviceCoords; n++) {
					if (n > 0)
						printf(", ");
					printf("%f",vp->deviceCoords[n]);
				}
				printf("\n");
			}
		}
	}
}

/* Allocate variable sized data elements */
static int icmNamedColor_allocate(
	icmBase *pp
) {
	icmNamedColor *p = (icmNamedColor *)pp;
	icc *icp = p->icp;

	if (p->count != p->_count) {
		unsigned int i;
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (icmNamedColorVal *) icp->al->calloc(icp->al,p->count, sizeof(icmNamedColorVal))) == NULL) {
			sprintf(icp->err,"icmNamedColor_alloc: malloc() of icmNamedColor data failed");
			return icp->errc = 2;
		}
		for (i = 0; i < p->count; i++) {
			p->data[i].icp = icp;	/* Do init */
		}
		p->_count = p->count;
	}
	return 0;
}

/* Free all storage in the object */
static void icmNamedColor_delete(
	icmBase *pp
) {
	icmNamedColor *p = (icmNamedColor *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmNamedColor(
	icc *icp
) {
	icmNamedColor *p;
	if ((p = (icmNamedColor *) icp->al->calloc(icp->al,1,sizeof(icmNamedColor))) == NULL)
		return NULL;
	p->ttype    = icSigNamedColor2Type;
	p->refcount = 1;
	p->get_size = icmNamedColor_get_size;
	p->read     = icmNamedColor_read;
	p->write    = icmNamedColor_write;
	p->dump     = icmNamedColor_dump;
	p->allocate = icmNamedColor_allocate;
	p->del      = icmNamedColor_delete;
	p->icp      = icp;

	/* Default the the number of device coords appropriately for NamedColorType */
	p->nDeviceCoords = number_ColorSpaceSignature(icp->header->colorSpace);

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* textDescription */

/* Return the number of bytes needed to write this tag */
static unsigned int icmTextDescription_get_size(
	icmBase *pp
) {
	icmTextDescription *p = (icmTextDescription *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += 4 + p->size;	/* Ascii string length + ascii string */
	len += 8 + 2 * p->ucSize;	/* Unicode language code + length + string */
	len += 3 + 67;		/* ScriptCode code, length string */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmTextDescription_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmTextDescription *p = (icmTextDescription *)pp;
	icc *icp = p->icp;
	int rv;
	char *bp, *buf, *end;

#ifdef ICM_STRICT
	if (len < (8 + 4 + 8 + 3 /* + 67 */)) {
#else
	if (len < (8 + 4 + 8 + 3)) {
#endif
		sprintf(icp->err,"icmTextDescription_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmTextDescription_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;
	end = buf + len;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmTextDescription_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read from the buffer into the structure */
	if ((rv = p->core_read(p, &bp, end)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	icp->al->free(icp->al, buf);
	return 0;
}

/* core read the object, return 0 on success, error code on fail */
static int icmTextDescription_core_read(
	icmTextDescription *p,
	char **bpp,				/* Pointer to buffer pointer, returns next after read */
	char *end				/* Pointer to past end of read buffer */
) {
	icc *icp = p->icp;
	int rv = 0;
	char *bp = *bpp;

	if ((bp + 8) > end) {
		sprintf(icp->err,"icmTextDescription_read: Data too short to type descriptor");
		*bpp = bp;
		return icp->errc = 1;
	}

	p->size = read_UInt32Number(bp);
	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		*bpp = bp;
		sprintf(icp->err,"icmTextDescription_read: Wrong tag type ('%s') for icmTextDescription",
		        tag2str((icTagTypeSignature)read_SInt32Number(bp)));
		return icp->errc = 1;
	}
	bp = bp + 8;

	/* Read the Ascii string */
	if ((bp + 4) > end) {
		*bpp = bp;
		sprintf(icp->err,"icmTextDescription_read: Data too short to read Ascii header");
		return icp->errc = 1;
	}
	p->size = read_UInt32Number(bp);
	bp += 4;
	if (p->size > 0) {
		if ((bp + p->size) > end) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_read: Data to short to read Ascii string");
			return icp->errc = 1;
		}
		if (check_null_string(bp,p->size)) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_read: ascii string is not terminated");
			return icp->errc = 1;
		}
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			return rv;
		}
		strcpy((void *)p->desc, (void *)bp);
		bp += p->size;
	}
	
	/* Read the Unicode string */
	if ((bp + 8) > end) {
		*bpp = bp;
		sprintf(icp->err,"icmTextDescription_read: Data too short to read Unicode string");
		return icp->errc = 1;
	}
	p->ucLangCode = read_UInt32Number(bp);
	bp += 4;
	p->ucSize = read_UInt32Number(bp);
	bp += 4;
	if (p->ucSize > 0) {
		ORD16 *up;
		if ((bp + 2 * p->ucSize) > end) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_read: Data too short to read Unicode string");
			return icp->errc = 1;
		}
		if (check_null_string16(bp,p->ucSize)) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_read: Unicode string is not terminated");
			return icp->errc = 1;
		}
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			return rv;
		}
		for(up = p->ucDesc; bp[0] != 0 || bp[1] != 0; up++, bp += 2)
			*up = read_UInt16Number(bp);
		*up = 0;	/* Unicode null */
		bp += 2;
	}
	
	/* Read the ScriptCode string */
	if ((bp + 3) > end) {
		*bpp = bp;
		sprintf(icp->err,"icmTextDescription_read: Data too short to read ScriptCode header");
		return icp->errc = 1;
	}
	p->scCode = read_UInt16Number(bp);
	bp += 2;
	p->scSize = read_UInt8Number(bp);
	bp += 1;
	if (p->scSize > 0) {
		if (p->scSize > 67) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_read: ScriptCode string too long");
			return icp->errc = 1;
		}
		if ((bp + p->scSize) > end) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_read: Data too short to read ScriptCode string");
			return icp->errc = 1;
		}
		if (check_null_string(bp,p->scSize)) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_read: ScriptCode string is not terminated");
			return icp->errc = 1;
		}
		memcpy((void *)p->scDesc, (void *)bp, p->scSize);
	} else {
		memset((void *)p->scDesc, 0, 67);
	}
	bp += 67;
	
	*bpp = bp;
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmTextDescription_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmTextDescription *p = (icmTextDescription *)pp;
	icc *icp = p->icp;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmTextDescription_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write to the buffer from the structure */
	if ((rv = p->core_write(p, &bp)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmTextDescription_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Core write the contents of the object. Return 0 on sucess, error code on failure */
static int icmTextDescription_core_write(
	icmTextDescription *p,
	char **bpp				/* Pointer to buffer pointer, returns next after read */
) {
	icc *icp = p->icp;
	char *bp = *bpp;
	int rv = 0;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmTextDescription_write: write_SInt32Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */
	bp = bp + 8;

	/* Write the Ascii string */
	if ((rv = write_UInt32Number(p->size,bp)) != 0) {
		sprintf(icp->err,"icmTextDescription_write: write_UInt32Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	bp += 4;
	if (p->size > 0) {
		if (check_null_string(p->desc,p->size)) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_write: ascii string is not terminated");
			return icp->errc = 1;
		}
		strcpy((void *)bp, (void *)p->desc);
		bp += p->size;
	}
	
	/* Write the Unicode string */
	if ((rv = write_UInt32Number(p->ucLangCode, bp)) != 0) {
		sprintf(icp->err,"icmTextDescription_write: write_UInt32Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	bp += 4;
	if ((rv = write_UInt32Number(p->ucSize, bp)) != 0) {
		sprintf(icp->err,"icmTextDescription_write: write_UInt32Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	bp += 4;
	if (p->ucSize > 0) {
		ORD16 *up;
		if (check_null_string16((char *)p->ucDesc,p->ucSize)) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_write: Unicode string is not terminated");
			return icp->errc = 1;
		}
		for(up = p->ucDesc; *up != 0; up++, bp += 2) {
			if ((rv = write_UInt16Number(((unsigned int)*up), bp)) != 0) {
				sprintf(icp->err,"icmTextDescription_write: write_UInt16Number() failed");
				*bpp = bp;
				return icp->errc = rv;
			}
		}
		bp[0] = 0;	/* null */
		bp[1] = 0;
		bp += 2;
	}
	
	/* Write the ScriptCode string */
	if ((rv = write_UInt16Number(p->scCode, bp)) != 0) {
		sprintf(icp->err,"icmTextDescription_write: write_UInt16Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	bp += 2;
	if ((rv = write_UInt8Number(p->scSize, bp)) != 0) {
		sprintf(icp->err,"icmTextDescription_write: write_UInt8Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	bp += 1;
	if (p->scSize > 0) {
		if (p->scSize > 67) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_write: ScriptCode string too long");
			return icp->errc = 1;
		}
		if (check_null_string((char *)p->scDesc,p->scSize)) {
			*bpp = bp;
			sprintf(icp->err,"icmTextDescription_write: ScriptCode string is not terminated");
			return icp->errc = 1;
		}
		memcpy((void *)bp, (void *)p->scDesc, 67);
	} else {
		memset((void *)bp, 0, 67);
	}
	bp += 67;

	*bpp = bp;
	return 0;
}

/* Dump a text description of the object */
static void icmTextDescription_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmTextDescription *p = (icmTextDescription *)pp;
	unsigned long i, r, c;

	if (verb <= 0)
		return;

	fprintf(op,"TextDescription:\n");

	if (p->size > 0) {
		unsigned long size = p->size > 0 ? p->size-1 : 0;
		fprintf(op,"  ASCII data, length %lu chars:\n",p->size);

		i = 0;
		for (r = 1;; r++) {		/* count rows */
			if (i >= size) {
				fprintf(op,"\n");
				break;
			}
			if (r > 1 && verb < 2) {
				fprintf(op,"...\n");
				break;			/* Print 1 row if not verbose */
			}
			c = 1;
			fprintf(op,"    0x%04lx: ",i);
			c += 10;
			while (i < size && c < 75) {
				if (isprint(p->desc[i])) {
					fprintf(op,"%c",p->desc[i]);
					c++;
				} else {
					fprintf(op,"\\%03o",p->desc[i]);
					c += 4;
				}
				i++;
			}
			if (i < size)
				fprintf(op,"\n");
		}
	} else {
		fprintf(op,"  No ASCII data\n");
	}

	/* Can't dump Unicode or ScriptCode as text with portable code */
	if (p->ucSize > 0) {
		unsigned long size = p->ucSize;
		fprintf(op,"  Unicode Data, Language code 0x%x, length %lu chars\n",
		        p->ucLangCode, p->ucSize);
		i = 0;
		for (r = 1;; r++) {		/* count rows */
			if (i >= size) {
				fprintf(op,"\n");
				break;
			}
			if (r > 1 && verb < 2) {
				fprintf(op,"...\n");
				break;			/* Print 1 row if not verbose */
			}
			c = 1;
			fprintf(op,"    0x%04lx: ",i);
			c += 10;
			while (i < size && c < 75) {
				fprintf(op,"%04x ",p->ucDesc[i]);
				c += 5;
				i++;
			}
			if (i < size)
				fprintf(op,"\n");
		}
	} else {
		fprintf(op,"  No Unicode data\n");
	}
	if (p->scSize > 0) {
		unsigned long size = p->scSize;
		fprintf(op,"  ScriptCode Data, Code 0x%x, length %lu chars\n",
		        p->scCode, p->scSize);
		i = 0;
		for (r = 1;; r++) {		/* count rows */
			if (i >= size) {
				fprintf(op,"\n");
				break;
			}
			if (r > 1 && verb < 2) {
				fprintf(op,"...\n");
				break;			/* Print 1 row if not verbose */
			}
			c = 1;
			fprintf(op,"    0x%04lx: ",i);
			c += 10;
			while (i < size && c < 75) {
				fprintf(op,"%02x ",p->scDesc[i]);
				c += 3;
				i++;
			}
			if (i < size)
				fprintf(op,"\n");
		}
	} else {
		fprintf(op,"  No ScriptCode data\n");
	}
}

/* Allocate variable sized data elements */
static int icmTextDescription_allocate(
	icmBase *pp
) {
	icmTextDescription *p = (icmTextDescription *)pp;
	icc *icp = p->icp;

	if (p->size != p->_size) {
		if (p->desc != NULL)
			icp->al->free(icp->al, p->desc);
		if ((p->desc = (char *) icp->al->malloc(icp->al, p->size * sizeof(char))) == NULL) {
			sprintf(icp->err,"icmTextDescription_alloc: malloc() of Ascii description failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	if (p->ucSize != p->uc_size) {
		if (p->ucDesc != NULL)
			icp->al->free(icp->al, p->ucDesc);
		if ((p->ucDesc = (ORD16 *) icp->al->malloc(icp->al, p->ucSize * sizeof(ORD16))) == NULL) {
			sprintf(icp->err,"icmTextDescription_alloc: malloc() of Unicode description failed");
			return icp->errc = 2;
		}
		p->uc_size = p->ucSize;
	}
	return 0;
}

/* Free all variable sized elements */
static void icmTextDescription_unallocate(
	icmTextDescription *p
) {
	icc *icp = p->icp;

	if (p->desc != NULL)
		icp->al->free(icp->al, p->desc);
	if (p->ucDesc != NULL)
		icp->al->free(icp->al, p->ucDesc);
}

/* Free all storage in the object */
static void icmTextDescription_delete(
	icmBase *pp
) {
	icmTextDescription *p = (icmTextDescription *)pp;
	icc *icp = p->icp;

	icmTextDescription_unallocate(p);
	icp->al->free(icp->al, p);
}

/* Initialze a named object */
static void icmTextDescription_init(
	icmTextDescription *p,
	icc *icp
) {
	memset((void *)p, 0, sizeof(icmTextDescription));	/* Imitate calloc */

	p->ttype    = icSigTextDescriptionType;
	p->refcount = 1;
	p->get_size = icmTextDescription_get_size;
	p->read     = icmTextDescription_read;
	p->write    = icmTextDescription_write;
	p->dump     = icmTextDescription_dump;
	p->allocate = icmTextDescription_allocate;
	p->del      = icmTextDescription_delete;
	p->icp      = icp;

	p->core_read  = icmTextDescription_core_read;
	p->core_write = icmTextDescription_core_write;
}

/* Create an empty object. Return null on error */
static icmBase *new_icmTextDescription(
	icc *icp
) {
	icmTextDescription *p;
	if ((p = (icmTextDescription *) icp->al->calloc(icp->al,1,sizeof(icmTextDescription))) == NULL)
		return NULL;

	icmTextDescription_init(p,icp);
	return (icmBase *)p;
}

/* ---------------------------------------------------------- */

/* Support for icmDescStruct */

/* Return the number of bytes needed to write this tag */
static unsigned int icmDescStruct_get_size(
	icmDescStruct *p
) {
	unsigned int len = 0;
	len += 20;				/* 20 bytes for header info */
	len += p->device.get_size((icmBase *)&p->device);
	len += p->model.get_size((icmBase *)&p->model);
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmDescStruct_read(
	icmDescStruct *p,
	char **bpp,				/* Pointer to buffer pointer, returns next after read */
	char *end				/* Pointer to past end of read buffer */
) {
	icc *icp = p->icp;
	char *bp = *bpp;
	int rv = 0;

	if ((bp + 20) > end) {
		sprintf(icp->err,"icmDescStruct_read: Data too short read header");
		*bpp = bp;
		return icp->errc = 1;
	}

    p->deviceMfg = read_SInt32Number(bp + 0);
    p->deviceModel = read_UInt32Number(bp + 4);
    read_UInt64Number(&p->attributes, bp + 8);
	p->technology = read_UInt32Number(bp + 16);
	*bpp = bp += 20;

	/* Read the device text description */
	if ((rv = p->device.core_read(&p->device, bpp, end)) != 0) {
		return rv;
	}

	/* Read the model text description */
	if ((rv = p->model.core_read(&p->model, bpp, end)) != 0) {
		return rv;
	}
	
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmDescStruct_write(
	icmDescStruct *p,
	char **bpp				/* Pointer to buffer pointer, returns next after read */
) {
	icc *icp = p->icp;
	char *bp = *bpp;
	int rv = 0;

    if ((rv = write_SInt32Number(p->deviceMfg, bp + 0)) != 0) {
		sprintf(icp->err,"icmDescStruct_write: write_SInt32Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
    if ((rv = write_UInt32Number(p->deviceModel, bp + 4)) != 0) {
		sprintf(icp->err,"icmDescStruct_write: write_UInt32Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
    if ((rv = write_UInt64Number(&p->attributes, bp + 8)) != 0) {
		sprintf(icp->err,"icmDescStruct_write: write_UInt64Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	if ((rv = write_UInt32Number(p->technology, bp + 16)) != 0) {
		sprintf(icp->err,"icmDescStruct_write: write_UInt32Number() failed");
		*bpp = bp;
		return icp->errc = rv;
	}
	*bpp = bp += 20;

	/* Write the device text description */
	if ((rv = p->device.core_write(&p->device, bpp)) != 0) {
		return rv;
	}

	/* Write the model text description */
	if ((rv = p->model.core_write(&p->model, bpp)) != 0) {
		return rv;
	}

	return 0;
}

/* Dump a text description of the object */
static void icmDescStruct_dump(
	icmDescStruct *p,
	FILE *op,		/* Output to dump to */
	int   verb,		/* Verbosity level */
	int   index		/* Description index */
) {
	if (verb <= 0)
		return;

	fprintf(op,"DescStruct %u:\n",index);
	if (verb >= 1) {
		fprintf(op,"  Dev. Mnfctr.    = %s\n",tag2str(p->deviceMfg));	/* ~~~ */
		fprintf(op,"  Dev. Model      = %s\n",tag2str(p->deviceModel));	/* ~~~ */
		fprintf(op,"  Dev. Attrbts    = %s\n", string_DeviceAttributes(p->attributes.l));
		fprintf(op,"  Dev. Technology = %s\n", string_TechnologySignature(p->technology));
		p->device.dump((icmBase *)&p->device, op,verb);
		p->model.dump((icmBase *)&p->model, op,verb);
		fprintf(op,"\n");
	}
}

/* Allocate variable sized data elements (ie. descriptions) */
static int icmDescStruct_allocate(
	icmDescStruct *p
) {
	int rv;

	if ((rv = p->device.allocate((icmBase *)&p->device)) != 0) {
		return rv;
	}
	if ((rv = p->model.allocate((icmBase *)&p->model)) != 0) {
		return rv;
	}
	return 0;
}

/* Free all storage in the object */
static void icmDescStruct_delete(
	icmDescStruct *p
) {
	icmTextDescription_unallocate(&p->device);
	icmTextDescription_unallocate(&p->model);
}

/* Init a DescStruct object */
static void icmDescStruct_init(
	icmDescStruct *p,
	icc *icp
) {

	p->allocate = icmDescStruct_allocate;
	p->icp = icp;

	icmTextDescription_init(&p->device, icp);
	icmTextDescription_init(&p->model, icp);
}

/* - - - - - - - - - - - - - - - */
/* icmProfileSequenceDesc object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmProfileSequenceDesc_get_size(
	icmBase *pp
) {
	icmProfileSequenceDesc *p = (icmProfileSequenceDesc *)pp;
	unsigned int len = 0;
	unsigned int i;
	len += 12;				/* 8 bytes for tag, padding and count */
	for (i = 0; i < p->count; i++) {	/* All the description structures */
		len += icmDescStruct_get_size(&p->data[i]);
	}
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmProfileSequenceDesc_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmProfileSequenceDesc *p = (icmProfileSequenceDesc *)pp;
	icc *icp = p->icp;
	unsigned long i;
	char *bp, *buf, *end;
	int rv = 0;

	if (len < 12) {
		sprintf(icp->err,"icmProfileSequenceDesc_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmProfileSequenceDesc_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;
	end = buf + len;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmProfileSequenceDesc_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmProfileSequenceDesc_read: Wrong tag type for icmProfileSequenceDesc");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp += 8;	/* Skip padding */

	p->count = read_UInt32Number(bp);	/* Number of sequence descriptions */
	bp += 4;

	/* Read all the sequence descriptions */
	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}
	for (i = 0; i < p->count; i++) {
		if ((rv = icmDescStruct_read(&p->data[i], &bp, end)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
	}

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmProfileSequenceDesc_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmProfileSequenceDesc *p = (icmProfileSequenceDesc *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmProfileSequenceDesc_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmProfileSequenceDesc_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	if ((rv = write_UInt32Number(p->count,bp+8)) != 0) {
		sprintf(icp->err,"icmProfileSequenceDesc_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	bp = bp + 12;

	/* Write all the description structures */
	for (i = 0; i < p->count; i++) {
		if ((rv = icmDescStruct_write(&p->data[i], &bp)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmProfileSequenceDesc_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmProfileSequenceDesc_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmProfileSequenceDesc *p = (icmProfileSequenceDesc *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"ProfileSequenceDesc:\n");
	fprintf(op,"  No. elements = %u\n",p->count);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->count; i++)
			icmDescStruct_dump(&p->data[i], op, verb-1, i);
	}
}

/* Allocate variable sized data elements (ie. count of profile descriptions) */
static int icmProfileSequenceDesc_allocate(
	icmBase *pp
) {
	icmProfileSequenceDesc *p = (icmProfileSequenceDesc *)pp;
	icc *icp = p->icp;
	unsigned int i;

	if (p->count != p->_count) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (icmDescStruct *) icp->al->malloc(icp->al, p->count * sizeof(icmDescStruct))) == NULL) {
			sprintf(icp->err,"icmProfileSequenceDesc_allocate Allocation of DescStruct array failed");
			return icp->errc = 2;
		}
		/* Now init the DescStructs */
		for (i = 0; i < p->count; i++) {
			icmDescStruct_init(&p->data[i], icp);
		}
		p->_count = p->count;
	}
	return 0;
}

/* Free all storage in the object */
static void icmProfileSequenceDesc_delete(
	icmBase *pp
) {
	icmProfileSequenceDesc *p = (icmProfileSequenceDesc *)pp;
	icc *icp = p->icp;
	unsigned int i;

	for (i = 0; i < p->count; i++) {
		icmDescStruct_delete(&p->data[i]);	/* Free allocated contents */
	}
	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmProfileSequenceDesc(
	icc *icp
) {
	icmProfileSequenceDesc *p;
	if ((p = (icmProfileSequenceDesc *) icp->al->calloc(icp->al,1,sizeof(icmProfileSequenceDesc))) == NULL)
		return NULL;
	p->ttype    = icSigProfileSequenceDescType;
	p->refcount = 1;
	p->get_size = icmProfileSequenceDesc_get_size;
	p->read     = icmProfileSequenceDesc_read;
	p->write    = icmProfileSequenceDesc_write;
	p->dump     = icmProfileSequenceDesc_dump;
	p->allocate = icmProfileSequenceDesc_allocate;
	p->del      = icmProfileSequenceDesc_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* Signature */

/* Return the number of bytes needed to write this tag */
static unsigned int icmSignature_get_size(
	icmBase *pp
) {
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += 4;			/* 4 for signature */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmSignature_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmSignature *p = (icmSignature *)pp;
	icc *icp = p->icp;
	char *bp, *buf;

	if (len < 12) {
		sprintf(icp->err,"icmSignature_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmSignature_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmSignature_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmSignaturSignatureng tag type for icmSignature");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read the encoded measurement geometry */
	p->sig = (icTechnologySignature)read_SInt32Number(bp + 8);

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmSignature_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmSignature *p = (icmSignature *)pp;
	icc *icp = p->icp;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmSignature_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmSignature_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write the signature */
	if ((rv = write_SInt32Number((int)p->sig, bp + 8)) != 0) {
		sprintf(icp->err,"icmSignaturea_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmSignature_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmSignature_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmSignature *p = (icmSignature *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"Signature\n");
	fprintf(op,"  Technology = %s\n", string_TechnologySignature(p->sig));
}

/* Allocate variable sized data elements */
static int icmSignature_allocate(
	icmBase *pp
) {
	/* Nothing to do */
	return 0;
}

/* Free all storage in the object */
static void icmSignature_delete(
	icmBase *pp
) {
	icmSignature *p = (icmSignature *)pp;
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmSignature(
	icc *icp
) {
	icmSignature *p;
	if ((p = (icmSignature *) icp->al->calloc(icp->al,1,sizeof(icmSignature))) == NULL)
		return NULL;
	p->ttype    = icSigSignatureType;
	p->refcount = 1;
	p->get_size = icmSignature_get_size;
	p->read     = icmSignature_read;
	p->write    = icmSignature_write;
	p->dump     = icmSignature_dump;
	p->allocate = icmSignature_allocate;
	p->del      = icmSignature_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */

/* Data conversion support functions */
static int read_ScreeningData(icmScreeningData *p, char *d) {
	p->frequency = read_S15Fixed16Number(d + 0);
	p->angle     = read_S15Fixed16Number(d + 4);
	p->spotShape = (icSpotShape)read_SInt32Number(d + 8);
	return 0;
}

static int write_ScreeningData(icmScreeningData *p, char *d) {
	int rv;
	if ((rv = write_S15Fixed16Number(p->frequency, d + 0)) != 0)
		return rv;
	if ((rv = write_S15Fixed16Number(p->angle, d + 4)) != 0)
		return rv;
	if ((rv = write_SInt32Number((int)p->spotShape, d + 8)) != 0)
		return rv;
	return 0;
}


/* icmScreening object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmScreening_get_size(
	icmBase *pp
) {
	icmScreening *p = (icmScreening *)pp;
	unsigned int len = 0;
	len += 16;				/* 16 bytes for tag, padding, flag & channeles */
	len += p->channels * 12;	/* 12 bytes for each channel */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmScreening_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmScreening *p = (icmScreening *)pp;
	icc *icp = p->icp;
	int rv = 0;
	unsigned long i;
	char *bp, *buf, *end;

	if (len < 12) {
		sprintf(icp->err,"icmScreening_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmScreening_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;
	end = buf + len;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmScreening_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmScreening_read: Wrong tag type for icmScreening");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->screeningFlag = read_UInt32Number(bp+8);		/* Flags */
	p->channels      = read_UInt32Number(bp+12);	/* Number of channels */
	bp = bp + 16;

	if ((rv = p->allocate((icmBase *)p)) != 0) {
		icp->al->free(icp->al, buf);
		return rv;
	}

	/* Read all the data from the buffer */
	for (i = 0; i < p->channels; i++, bp += 12) {
		if ((bp + 12) > end) {
			sprintf(icp->err,"icmScreening_read: Data too short to read Screening Data");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		read_ScreeningData(&p->data[i], bp);
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmScreening_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmScreening *p = (icmScreening *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmScreening_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmScreening_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	if ((rv = write_UInt32Number(p->screeningFlag,bp+8)) != 0) {
			sprintf(icp->err,"icmScreening_write: write_UInt32Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	if ((rv = write_UInt32Number(p->channels,bp+12)) != 0) {
			sprintf(icp->err,"icmScreening_write: write_UInt32NumberXYZumber() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	bp = bp + 16;

	/* Write all the data to the buffer */
	for (i = 0; i < p->channels; i++, bp += 12) {
		if ((rv = write_ScreeningData(&p->data[i],bp)) != 0) {
			sprintf(icp->err,"icmScreening_write: write_ScreeningData() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmScreening_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmScreening_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmScreening *p = (icmScreening *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"Screening:\n");
	fprintf(op,"  Flags = %s\n", string_ScreenEncodings(p->screeningFlag));
	fprintf(op,"  No. channels = %u\n",p->channels);
	if (verb >= 2) {
		unsigned long i;
		for (i = 0; i < p->channels; i++) {
			fprintf(op,"    %lu:\n",i);
			fprintf(op,"      Frequency:  %f\n",p->data[i].frequency);
			fprintf(op,"      Angle:      %f\n",p->data[i].angle);
			fprintf(op,"      Spot shape: %s\n", string_SpotShape(p->data[i].spotShape));
		}
	}
}

/* Allocate variable sized data elements */
static int icmScreening_allocate(
	icmBase *pp
) {
	icmScreening *p = (icmScreening *)pp;
	icc *icp = p->icp;

	if (p->channels != p->_channels) {
		if (p->data != NULL)
			icp->al->free(icp->al, p->data);
		if ((p->data = (icmScreeningData *) icp->al->malloc(icp->al, p->channels * sizeof(icmScreeningData))) == NULL) {
			sprintf(icp->err,"icmScreening_alloc: malloc() of icmScreening data failed");
			return icp->errc = 2;
		}
		p->_channels = p->channels;
	}
	return 0;
}

/* Free all storage in the object */
static void icmScreening_delete(
	icmBase *pp
) {
	icmScreening *p = (icmScreening *)pp;
	icc *icp = p->icp;

	if (p->data != NULL)
		icp->al->free(icp->al, p->data);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmScreening(
	icc *icp
) {
	icmScreening *p;
	if ((p = (icmScreening *) icp->al->calloc(icp->al,1,sizeof(icmScreening))) == NULL)
		return NULL;
	p->ttype    = icSigScreeningType;
	p->refcount = 1;
	p->get_size = icmScreening_get_size;
	p->read     = icmScreening_read;
	p->write    = icmScreening_write;
	p->dump     = icmScreening_dump;
	p->allocate = icmScreening_allocate;
	p->del      = icmScreening_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmUcrBg object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmUcrBg_get_size(
	icmBase *pp
) {
	icmUcrBg *p = (icmUcrBg *)pp;
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += 4 + p->UCRcount * 2;	/* Undercolor Removal */
	len += 4 + p->BGcount * 2;	/* Black Generation */
	len += p->size;				/* Description string */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmUcrBg_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmUcrBg *p = (icmUcrBg *)pp;
	icc *icp = p->icp;
	unsigned long i;
	int rv = 0;
	char *bp, *buf, *end;

	if (len < 16) {
		sprintf(icp->err,"icmUcrBg_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUcrBg_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;
	end = buf + len;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmUcrBg_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmUcrBg_read: Wrong tag type for icmUcrBg");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->UCRcount = read_UInt32Number(bp+8);	/* First curve count */
	bp = bp + 12;

	if (p->UCRcount > 0) {
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
		for (i = 0; i < p->UCRcount; i++, bp += 2) {
			if ((bp + 2) > end) {
				sprintf(icp->err,"icmUcrBg_read: Data too short to read UCR Data");
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
			if (p->UCRcount == 1)	/* % */
				p->UCRcurve[i] = (double)read_UInt16Number(bp);
			else					/* 0.0 - 1.0 */
				p->UCRcurve[i] = read_DCS16Number(bp);
		}
	} else {
		p->UCRcurve = NULL;
	}

	if ((bp + 4) > end) {
		sprintf(icp->err,"icmData_read: Data too short to read Black Gen count");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->BGcount = read_UInt32Number(bp);	/* First curve count */
	bp += 4;

	if (p->BGcount > 0) {
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
		for (i = 0; i < p->BGcount; i++, bp += 2) {
			if ((bp + 2) > end) {
				sprintf(icp->err,"icmUcrBg_read: Data too short to read BG Data");
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
			if (p->BGcount == 1)	/* % */
				p->BGcurve[i] = (double)read_UInt16Number(bp);
			else					/* 0.0 - 1.0 */
				p->BGcurve[i] = read_DCS16Number(bp);
		}
	} else {
		p->BGcurve = NULL;
	}

	p->size = end - bp;		/* Nominal string length */
	if (p->size > 0) {
		if (check_null_string(bp, p->size) != 0) {
			sprintf(icp->err,"icmUcrBg_read: string is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		p->size = strlen(bp) + 1;
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
		memcpy((void *)p->string, (void *)bp, p->size);
		bp += p->size;
	} else {
		p->string = NULL;
	}

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmUcrBg_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmUcrBg *p = (icmUcrBg *)pp;
	icc *icp = p->icp;
	unsigned long i;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmUcrBg_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmUcrBg_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */
	bp = bp + 8;

	/* Write UCR curve */
	if ((rv = write_UInt32Number(p->UCRcount,bp)) != 0) {
		sprintf(icp->err,"icmUcrBg_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	bp += 4;

	for (i = 0; i < p->UCRcount; i++, bp += 2) {
		if (p->UCRcount == 1) { /* % */
			if ((rv = write_UInt16Number((unsigned int)(p->UCRcurve[i]+0.5),bp)) != 0) {
				sprintf(icp->err,"icmUcrBg_write: write_UInt16umber() failed");
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		} else {
			if ((rv = write_DCS16Number(p->UCRcurve[i],bp)) != 0) {
				sprintf(icp->err,"icmUcrBg_write: write_DCS16umber(%f) failed",p->UCRcurve[i]);
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	}

	/* Write BG curve */
	if ((rv = write_UInt32Number(p->BGcount,bp)) != 0) {
		sprintf(icp->err,"icmUcrBg_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	bp += 4;

	for (i = 0; i < p->BGcount; i++, bp += 2) {
		if (p->BGcount == 1) { /* % */
			if ((rv = write_UInt16Number((unsigned int)(p->BGcurve[i]+0.5),bp)) != 0) {
				sprintf(icp->err,"icmUcrBg_write: write_UInt16umber() failed");
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		} else {
			if ((rv = write_DCS16Number(p->BGcurve[i],bp)) != 0) {
				sprintf(icp->err,"icmUcrBg_write: write_DCS16umber(%f) failed",p->BGcurve[i]);
				icp->al->free(icp->al, buf);
				return icp->errc = rv;
			}
		}
	}

	if (p->string != NULL) {
		if ((rv = check_null_string(p->string,p->size)) != 0) {
			sprintf(icp->err,"icmUcrBg_write: text is not null terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		memcpy((void *)bp, (void *)p->string, p->size);
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmUcrBg_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmUcrBg_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmUcrBg *p = (icmUcrBg *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"Undercolor Removal Curve & Black Generation:\n");

	if (p->UCRcount == 0) {
		fprintf(op,"  UCR: Not specified\n");
	} else if (p->UCRcount == 1) {
		fprintf(op,"  UCR: %f%%\n",p->UCRcurve[0]);
	} else {
		fprintf(op,"  UCR curve no. elements = %u\n",p->UCRcount);
		if (verb >= 2) {
			unsigned long i;
			for (i = 0; i < p->UCRcount; i++)
				fprintf(op,"  %3lu:  %f\n",i,p->UCRcurve[i]);
		}
	}
	if (p->BGcount == 0) {
		fprintf(op,"  BG: Not specified\n");
	} else if (p->BGcount == 1) {
		fprintf(op,"  BG: %f%%\n",p->BGcurve[0]);
	} else {
		fprintf(op,"  BG curve no. elements = %u\n",p->BGcount);
		if (verb >= 2) {
			unsigned long i;
			for (i = 0; i < p->BGcount; i++)
				fprintf(op,"  %3lu:  %f\n",i,p->BGcurve[i]);
		}
	}

	{
		unsigned long i, r, c, size;
		fprintf(op,"  Description:\n");
		fprintf(op,"    No. chars = %lu\n",p->size);
	
		size = p->size > 0 ? p->size-1 : 0;
		i = 0;
		for (r = 1;; r++) {		/* count rows */
			if (i >= size) {
				fprintf(op,"\n");
				break;
			}
			if (r > 1 && verb < 2) {
				fprintf(op,"...\n");
				break;			/* Print 1 row if not verbose */
			}
			c = 1;
			fprintf(op,"      0x%04lx: ",i);
			c += 10;
			while (i < size && c < 73) {
				if (isprint(p->string[i])) {
					fprintf(op,"%c",p->string[i]);
					c++;
				} else {
					fprintf(op,"\\%03o",p->string[i]);
					c += 4;
				}
				i++;
			}
			if (i < size)
				fprintf(op,"\n");
		}
	}
}

/* Allocate variable sized data elements */
static int icmUcrBg_allocate(
	icmBase *pp
) {
	icmUcrBg *p = (icmUcrBg *)pp;
	icc *icp = p->icp;

	if (p->UCRcount != p->UCR_count) {
		if (p->UCRcurve != NULL)
			icp->al->free(icp->al, p->UCRcurve);
		if ((p->UCRcurve = (double *) icp->al->malloc(icp->al, p->UCRcount * sizeof(double))) == NULL) {
			sprintf(icp->err,"icmUcrBg_allocate: malloc() of UCR curve data failed");
			return icp->errc = 2;
		}
		p->UCR_count = p->UCRcount;
	}
	if (p->BGcount != p->BG_count) {
		if (p->BGcurve != NULL)
			icp->al->free(icp->al, p->BGcurve);
		if ((p->BGcurve = (double *) icp->al->malloc(icp->al, p->BGcount * sizeof(double))) == NULL) {
			sprintf(icp->err,"icmUcrBg_allocate: malloc() of BG curve data failed");
			return icp->errc = 2;
		}
		p->BG_count = p->BGcount;
	}
	if (p->size != p->_size) {
		if (p->string != NULL)
			icp->al->free(icp->al, p->string);
		if ((p->string = (char *) icp->al->malloc(icp->al, p->size * sizeof(char))) == NULL) {
			sprintf(icp->err,"icmUcrBg_allocate: malloc() of string data failed");
			return icp->errc = 2;
		}
		p->_size = p->size;
	}
	return 0;
}

/* Free all storage in the object */
static void icmUcrBg_delete(
	icmBase *pp
) {
	icmUcrBg *p = (icmUcrBg *)pp;
	icc *icp = p->icp;

	if (p->UCRcurve != NULL)
		icp->al->free(icp->al, p->UCRcurve);
	if (p->BGcurve != NULL)
		icp->al->free(icp->al, p->BGcurve);
	if (p->string != NULL)
		icp->al->free(icp->al, p->string);
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmUcrBg(
	icc *icp
) {
	icmUcrBg *p;
	if ((p = (icmUcrBg *) icp->al->calloc(icp->al,1,sizeof(icmUcrBg))) == NULL)
		return NULL;
	p->ttype    = icSigUcrBgType;
	p->refcount = 1;
	p->get_size = icmUcrBg_get_size;
	p->read     = icmUcrBg_read;
	p->write    = icmUcrBg_write;
	p->dump     = icmUcrBg_dump;
	p->allocate = icmUcrBg_allocate;
	p->del      = icmUcrBg_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* VideoCardGamma (ColorSync 2.5 specific - c/o Neil Okamoto) */

static unsigned int icmVideoCardGamma_get_size(
	icmBase *pp
) {
	icmVideoCardGamma *p = (icmVideoCardGamma *)pp;
	unsigned int len = 0;

	len += 8;			/* 8 bytes for tag and padding */
	len += 4;			/* 4 for gamma type */

	/* compute size of remainder */
	if (p->tagType == icmVideoCardGammaTableType) {
		len += 2;       /* 2 bytes for channels */
		len += 2;       /* 2 for entry count */
		len += 2;       /* 2 for entry size */
		len += ( p->u.table.channels *     /* compute table size */
				 p->u.table.entryCount *
				 p->u.table.entrySize );
	}
	else if (p->tagType == icmVideoCardGammaFormulaType) {
		len += 12;		/* 4 bytes each for red gamma, min, & max */
		len += 12;		/* 4 bytes each for green gamma, min & max */
		len += 12;		/* 4 bytes each for blue gamma, min & max */
	}
	return len;
}
/* read the object, return 0 on success, error code on fail */
static int icmVideoCardGamma_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmVideoCardGamma *p = (icmVideoCardGamma *)pp;
	icc *icp = p->icp;
	int rv, c;
	char *bp, *buf;
	unsigned char *pchar;
	unsigned short *pshort;

	if (len < 18) {
		sprintf(icp->err,"icmVideoCardGamma_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmVideoCardGamma_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmVideoCardGamma_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmVideoCardGamma_read: Wrong tag type for icmVideoCardGamma");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read gamma format (eg. table or formula) from the buffer */
	p->tagType = read_UInt32Number(bp+8);

	/* Read remaining gamma data based on format */
	switch ((int)p->tagType) {
	case icmVideoCardGammaTableType:
		p->u.table.channels   = read_UInt16Number(bp+12);
		p->u.table.entryCount = read_UInt16Number(bp+14);
		p->u.table.entrySize  = read_UInt16Number(bp+16);
		if (len-18 < p->u.table.channels*p->u.table.entryCount*p->u.table.entrySize) {
			sprintf(icp->err,"icmVideoCardGamma_read: Tag too small to be legal");
			return icp->errc = 1;
		}
		if ((rv = pp->allocate(pp)) != 0) {  /* make space for table */
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		pchar = (unsigned char*)p->u.table.data;
		pshort = (unsigned short*)p->u.table.data;
		for (c=0, bp=bp+18; c<p->u.table.channels*p->u.table.entryCount; c++) {
			switch (p->u.table.entrySize) {
			case 1:
				*pchar++ = read_UInt8Number(bp);
				bp++;
				break;
			case 2:
				*pshort++ = read_UInt16Number(bp);
				bp+=2;
				break;
			default:
				sprintf(icp->err,"icmVideoCardGamma_read: unsupported table entry size");
				pp->del(pp);
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
		}
		break;
	case icmVideoCardGammaFormulaType:
		if (len < 48) {
			sprintf(icp->err,"icmVideoCardGamma_read: Tag too small to be legal");
			return icp->errc = 1;
		}
		p->u.formula.redGamma   = read_S15Fixed16Number(bp+12);
		p->u.formula.redMin     = read_S15Fixed16Number(bp+16);
		p->u.formula.redMax     = read_S15Fixed16Number(bp+20);
		p->u.formula.greenGamma = read_S15Fixed16Number(bp+24);
		p->u.formula.greenMin   = read_S15Fixed16Number(bp+28);
		p->u.formula.greenMax   = read_S15Fixed16Number(bp+32);
		p->u.formula.blueGamma  = read_S15Fixed16Number(bp+36);
		p->u.formula.blueMin    = read_S15Fixed16Number(bp+40);
		p->u.formula.blueMax    = read_S15Fixed16Number(bp+44);
		break;
	default:
		sprintf(icp->err,"icmVideoCardGammaTable_read: Unknown gamma format for icmVideoCardGamma");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmVideoCardGamma_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmVideoCardGamma *p = (icmVideoCardGamma *)pp;
	icc *icp = p->icp;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0, c;
	unsigned char *pchar;
	unsigned short *pshort;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmViewingConditions_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmVideoCardGamma_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write gamma format (eg. table of formula) */
	if ((rv = write_UInt32Number(p->tagType,bp+8)) != 0) {
		sprintf(icp->err,"icmVideoCardGamma_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write remaining gamma data based on format */
	switch ((int)p->tagType) {
	case icmVideoCardGammaTableType:
		if ((rv = write_UInt16Number(p->u.table.channels,bp+12)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_UInt16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_UInt16Number(p->u.table.entryCount,bp+14)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_UInt16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_UInt16Number(p->u.table.entrySize,bp+16)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_UInt16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		pchar = (unsigned char*)p->u.table.data;
		pshort = (unsigned short*)p->u.table.data;
		for (c=0, bp=bp+18; c<p->u.table.channels*p->u.table.entryCount; c++) {
			switch (p->u.table.entrySize) {
			case 1:
				write_UInt8Number(*pchar++,bp);
				bp++;
				break;
			case 2:
				write_UInt16Number(*pshort++,bp);
				bp+=2;
				break;
			default:
				sprintf(icp->err,"icmVideoCardGamma_write: unsupported table entry size");
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
		}
		break;
	case icmVideoCardGammaFormulaType:
		if ((rv = write_S15Fixed16Number(p->u.formula.redGamma,bp+12)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.redMin,bp+16)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.redMax,bp+20)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.greenGamma,bp+24)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.greenMin,bp+28)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.greenMax,bp+32)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.blueGamma,bp+36)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.blueMin,bp+40)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		if ((rv = write_S15Fixed16Number(p->u.formula.blueMax,bp+44)) != 0) {
			sprintf(icp->err,"icmVideoCardGamma_write: write_S15Fixed16Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		break;
	default:
		sprintf(icp->err,"icmVideoCardGammaTable_write: Unknown gamma format for icmVideoCardGamma");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmViewingConditions_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmVideoCardGamma_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmVideoCardGamma *p = (icmVideoCardGamma *)pp;
	int c,i;

	if (verb <= 0)
		return;

	switch ((int)p->tagType) {
	case icmVideoCardGammaTableType:
		fprintf(op,"VideoCardGammaTable:\n");
		fprintf(op,"  channels  = %d\n", p->u.table.channels);
		fprintf(op,"  entries   = %d\n", p->u.table.entryCount);
		fprintf(op,"  entrysize = %d\n", p->u.table.entrySize);
		if (verb >= 2) {
			/* dump array contents also */
			for (c=0; c<p->u.table.channels; c++) {
				fprintf(op,"  channel #%d\n",c);
				for (i=0; i<p->u.table.entryCount; i++) {
					if (p->u.table.entrySize == 1) {
						fprintf(op,"    %d: %d\n",i,((unsigned char*)p->u.table.data)[c*p->u.table.entryCount+i]);
					}
					else if (p->u.table.entrySize == 2) {
						fprintf(op,"    %d: %d\n",i,((unsigned short*)p->u.table.data)[c*p->u.table.entryCount+i]);
					}
				}
			}
		}
		break;
	case icmVideoCardGammaFormulaType:
		fprintf(op,"VideoCardGammaFormula:\n");
		fprintf(op,"  red gamma   = %f\n", p->u.formula.redGamma);
		fprintf(op,"  red min     = %f\n", p->u.formula.redMin);
		fprintf(op,"  red max     = %f\n", p->u.formula.redMax);
		fprintf(op,"  green gamma = %f\n", p->u.formula.greenGamma);
		fprintf(op,"  green min   = %f\n", p->u.formula.greenMin);
		fprintf(op,"  green max   = %f\n", p->u.formula.greenMax);
		fprintf(op,"  blue gamma  = %f\n", p->u.formula.blueGamma);
		fprintf(op,"  blue min    = %f\n", p->u.formula.blueMin);
		fprintf(op,"  blue max    = %f\n", p->u.formula.blueMax);
		break;
	default:
		fprintf(op,"  Unknown tag format\n");
	}
}

/* Allocate variable sized data elements */
static int icmVideoCardGamma_allocate(
	icmBase *pp
) {
	icmVideoCardGamma *p = (icmVideoCardGamma *)pp;
	icc *icp = p->icp;
	int size;

	/* note: allocation is only relevant for table type
	 * and in that case the channels, entryCount, and entrySize
	 * fields must all be set prior to getting here
	 */

	if (p->tagType == icmVideoCardGammaTableType) {
		if (p->u.table.data != NULL)
			icp->al->free(icp->al, p->u.table.data);
		size = (p->u.table.channels *
				p->u.table.entryCount);
		switch (p->u.table.entrySize) {
		case 1:
			size *= sizeof(unsigned char);
			break;
		case 2:
			size *= sizeof(unsigned short);
			break;
		default:
			sprintf(icp->err,"icmVideoCardGamma_alloc: unsupported table entry size");
			return icp->errc = 1;
		}
		if ((p->u.table.data = (void*) icp->al->malloc(icp->al, size)) == NULL) {
			sprintf(icp->err,"icmVideoCardGamma_alloc: malloc() of table data failed");
			return icp->errc = 2;
		}
	}

	return 0;
}

/* Free all storage in the object */
static void icmVideoCardGamma_delete(
	icmBase *pp
) {
	icmVideoCardGamma *p = (icmVideoCardGamma *)pp;
	icc *icp = p->icp;

	if (p->tagType == icmVideoCardGammaTableType && p->u.table.data != NULL)
		icp->al->free(icp->al, p->u.table.data);

	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmVideoCardGamma(
	icc *icp
) {
	icmVideoCardGamma *p;
	if ((p = (icmVideoCardGamma *) icp->al->calloc(icp->al,1,sizeof(icmVideoCardGamma))) == NULL)
		return NULL;
	p->ttype    = icSigVideoCardGammaType;
	p->refcount = 1;
	p->get_size = icmVideoCardGamma_get_size;
	p->read     = icmVideoCardGamma_read;
	p->write    = icmVideoCardGamma_write;
	p->dump     = icmVideoCardGamma_dump;
	p->allocate = icmVideoCardGamma_allocate;
	p->del      = icmVideoCardGamma_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* ViewingConditions */

/* Return the number of bytes needed to write this tag */
static unsigned int icmViewingConditions_get_size(
	icmBase *pp
) {
	unsigned int len = 0;
	len += 8;			/* 8 bytes for tag and padding */
	len += 12;			/* 12 for XYZ of illuminant */
	len += 12;			/* 12 for XYZ of surround */
	len += 4;			/* 4 for illuminant type */
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmViewingConditions_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmViewingConditions *p = (icmViewingConditions *)pp;
	icc *icp = p->icp;
	int rv;
	char *bp, *buf;

	if (len < 36) {
		sprintf(icp->err,"icmViewingConditions_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmViewingConditions_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmViewingConditions_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmViewingConditions_read: Wrong tag type for icmViewingConditions");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read the XYZ values for the illuminant */
	if ((rv = read_XYZNumber(&p->illuminant, bp+8)) != 0) {
		sprintf(icp->err,"icmViewingConditions: read_XYZNumber error");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Read the XYZ values for the surround */
	if ((rv = read_XYZNumber(&p->surround, bp+20)) != 0) {
		sprintf(icp->err,"icmViewingConditions: read_XYZNumber error");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Read the encoded standard illuminant */
	p->stdIlluminant = (icIlluminant)read_SInt32Number(bp + 32);

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmViewingConditions_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmViewingConditions *p = (icmViewingConditions *)pp;
	icc *icp = p->icp;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmViewingConditions_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmViewingConditions_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */

	/* Write the XYZ values for the illuminant */
	if ((rv = write_XYZNumber(&p->illuminant, bp+8)) != 0) {
		sprintf(icp->err,"icmViewingConditions: write_XYZNumber error");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write the XYZ values for the surround */
	if ((rv = write_XYZNumber(&p->surround, bp+20)) != 0) {
		sprintf(icp->err,"icmViewingConditions: write_XYZNumber error");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write the encoded standard illuminant */
	if ((rv = write_SInt32Number((int)p->stdIlluminant, bp + 32)) != 0) {
		sprintf(icp->err,"icmViewingConditionsa_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmViewingConditions_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmViewingConditions_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmViewingConditions *p = (icmViewingConditions *)pp;
	if (verb <= 0)
		return;

	fprintf(op,"Viewing Conditions:\n");
	fprintf(op,"  XYZ value of illuminant in cd/m^2 = %s\n", string_XYZNumber(&p->illuminant));
	fprintf(op,"  XYZ value of surround in cd/m^2   = %s\n", string_XYZNumber(&p->surround));
	fprintf(op,"  Illuminant type = %s\n", string_Illuminant(p->stdIlluminant));
}

/* Allocate variable sized data elements */
static int icmViewingConditions_allocate(
	icmBase *pp
) {
	/* Nothing to do */
	return 0;
}

/* Free all storage in the object */
static void icmViewingConditions_delete(
	icmBase *pp
) {
	icmViewingConditions *p = (icmViewingConditions *)pp;
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmViewingConditions(
	icc *icp
) {
	icmViewingConditions *p;
	if ((p = (icmViewingConditions *) icp->al->calloc(icp->al,1,sizeof(icmViewingConditions))) == NULL)
		return NULL;
	p->ttype    = icSigViewingConditionsType;
	p->refcount = 1;
	p->get_size = icmViewingConditions_get_size;
	p->read     = icmViewingConditions_read;
	p->write    = icmViewingConditions_write;
	p->dump     = icmViewingConditions_dump;
	p->allocate = icmViewingConditions_allocate;
	p->del      = icmViewingConditions_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ---------------------------------------------------------- */
/* icmCrdInfo object */

/* Return the number of bytes needed to write this tag */
static unsigned int icmCrdInfo_get_size(
	icmBase *pp
) {
	icmCrdInfo *p = (icmCrdInfo *)pp;
	unsigned int len = 0, t;
	len += 8;				/* 8 bytes for tag and padding */
	len += 4 + p->ppsize;	/* Postscript product name */
	for (t = 0; t < 4; t++) {	/* For all 4 intents */
		len += 4 + p->crdsize[t];	/* crd names */ 
	}
	return len;
}

/* read the object, return 0 on success, error code on fail */
static int icmCrdInfo_read(
	icmBase *pp,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icmCrdInfo *p = (icmCrdInfo *)pp;
	icc *icp = p->icp;
	unsigned long t;
	int rv = 0;
	char *bp, *buf, *end;

	if (len < 28) {
		sprintf(icp->err,"icmCrdInfo_read: Tag too small to be legal");
		return icp->errc = 1;
	}

	/* Allocate a file read buffer */
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmCrdInfo_read: malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;
	end = buf + len;

	/* Read portion of file into buffer */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, bp, 1, len) != len) {
		sprintf(icp->err,"icmCrdInfo_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Read type descriptor from the buffer */
	if (((icTagTypeSignature)read_SInt32Number(bp)) != p->ttype) {
		sprintf(icp->err,"icmCrdInfo_read: Wrong tag type for icmCrdInfo");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	bp = bp + 8;

	/* Postscript product name */
	if ((bp + 4) > end) {
		sprintf(icp->err,"icmCrdInfo_read: Data too short to read Postscript product name");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	p->ppsize = read_UInt32Number(bp);
	bp += 4;
	if (p->ppsize > 0) {
		if ((bp + p->ppsize) > end) {
			sprintf(icp->err,"icmCrdInfo_read: Data to short to read Postscript product string");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		if (check_null_string(bp,p->ppsize)) {
			sprintf(icp->err,"icmCrdInfo_read: Postscript product name is not terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		if ((rv = p->allocate((icmBase *)p)) != 0) {
			icp->al->free(icp->al, buf);
			return rv;
		}
		memcpy((void *)p->ppname, (void *)bp, p->ppsize);
		bp += p->ppsize;
	}
	
	/* CRD names for the four rendering intents */
	for (t = 0; t < 4; t++) {	/* For all 4 intents */
		if ((bp + 4) > end) {
			sprintf(icp->err,"icmCrdInfo_read: Data too short to read CRD%ld name",t);
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		p->crdsize[t] = read_UInt32Number(bp);
		bp += 4;
		if (p->crdsize[t] > 0) {
			if ((bp + p->crdsize[t]) > end) {
				sprintf(icp->err,"icmCrdInfo_read: Data to short to read CRD%ld string",t);
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
			if (check_null_string(bp,p->crdsize[t])) {
				sprintf(icp->err,"icmCrdInfo_read: CRD%ld name is not terminated",t);
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
			if ((rv = p->allocate((icmBase *)p)) != 0) { 
				icp->al->free(icp->al, buf);
				return rv;
			}
			memcpy((void *)p->crdname[t], (void *)bp, p->crdsize[t]);
			bp += p->crdsize[t];
		}
	}

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmCrdInfo_write(
	icmBase *pp,
	unsigned long of			/* File offset to write from */
) {
	icmCrdInfo *p = (icmCrdInfo *)pp;
	icc *icp = p->icp;
	unsigned long t;
	unsigned int len;
	char *bp, *buf;		/* Buffer to write from */
	int rv = 0;

	/* Allocate a file write buffer */
	len = p->get_size((icmBase *)p);
	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmCrdInfo_write malloc() failed");
		return icp->errc = 2;
	}
	bp = buf;

	/* Write type descriptor to the buffer */
	if ((rv = write_SInt32Number((int)p->ttype,bp)) != 0) {
		sprintf(icp->err,"icmCrdInfo_write: write_SInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	write_SInt32Number(0,bp+4);			/* Set padding to 0 */
	bp = bp + 8;

	/* Postscript product name */
	if ((rv = write_UInt32Number(p->ppsize,bp)) != 0) {
		sprintf(icp->err,"icmCrdInfo_write: write_UInt32Number() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	bp += 4;
	if (p->ppsize > 0) {
		if ((rv = check_null_string(p->ppname,p->ppsize)) != 0) {
			sprintf(icp->err,"icmCrdInfo_write: Postscript product name is not terminated");
			icp->al->free(icp->al, buf);
			return icp->errc = 1;
		}
		memcpy((void *)bp, (void *)p->ppname, p->ppsize);
		bp += p->ppsize;
	}

	/* CRD names for the four rendering intents */
	for (t = 0; t < 4; t++) {	/* For all 4 intents */
		if ((rv = write_UInt32Number(p->crdsize[t],bp)) != 0) {
			sprintf(icp->err,"icmCrdInfo_write: write_UInt32Number() failed");
			icp->al->free(icp->al, buf);
			return icp->errc = rv;
		}
		bp += 4;
		if (p->ppsize > 0) {
			if ((rv = check_null_string(p->crdname[t],p->crdsize[t])) != 0) {
				sprintf(icp->err,"icmCrdInfo_write: CRD%ld name is not terminated",t);
				icp->al->free(icp->al, buf);
				return icp->errc = 1;
			}
			memcpy((void *)bp, (void *)p->crdname[t], p->crdsize[t]);
			bp += p->crdsize[t];
		}
	}

	/* Write to the file */
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmCrdInfo_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}
	icp->al->free(icp->al, buf);
	return 0;
}

/* Dump a text description of the object */
static void icmCrdInfo_dump(
	icmBase *pp,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	icmCrdInfo *p = (icmCrdInfo *)pp;
	unsigned long i, r, c, size, t;

	if (verb <= 0)
		return;

	fprintf(op,"PostScript Product name and CRD names:\n");

	fprintf(op,"  Product name:\n");
	fprintf(op,"    No. chars = %lu\n",p->ppsize);
	
	size = p->ppsize > 0 ? p->ppsize-1 : 0;
	i = 0;
	for (r = 1;; r++) {		/* count rows */
		if (i >= size) {
			fprintf(op,"\n");
			break;
		}
		if (r > 1 && verb < 2) {
			fprintf(op,"...\n");
			break;			/* Print 1 row if not verbose */
		}
		c = 1;
		fprintf(op,"      0x%04lx: ",i);
		c += 10;
		while (i < size && c < 73) {
			if (isprint(p->ppname[i])) {
				fprintf(op,"%c",p->ppname[i]);
				c++;
			} else {
				fprintf(op,"\\%03o",p->ppname[i]);
				c += 4;
			}
			i++;
		}
		if (i < size)
			fprintf(op,"\n");
	}

	for (t = 0; t < 4; t++) {	/* For all 4 intents */
		fprintf(op,"  CRD%ld name:\n",t);
		fprintf(op,"    No. chars = %lu\n",p->crdsize[t]);
		
		size = p->crdsize[t] > 0 ? p->crdsize[t]-1 : 0;
		i = 0;
		for (r = 1;; r++) {		/* count rows */
			if (i >= size) {
				fprintf(op,"\n");
				break;
			}
			if (r > 1 && verb < 2) {
				fprintf(op,"...\n");
				break;			/* Print 1 row if not verbose */
			}
			c = 1;
			fprintf(op,"      0x%04lx: ",i);
			c += 10;
			while (i < size && c < 73) {
				if (isprint(p->crdname[t][i])) {
					fprintf(op,"%c",p->crdname[t][i]);
					c++;
				} else {
					fprintf(op,"\\%03o",p->crdname[t][i]);
					c += 4;
				}
				i++;
			}
			if (i < size)
				fprintf(op,"\n");
		}
	}
}

/* Allocate variable sized data elements */
static int icmCrdInfo_allocate(
	icmBase *pp
) {
	icmCrdInfo *p = (icmCrdInfo *)pp;
	icc *icp = p->icp;
	unsigned int t;

	if (p->ppsize != p->_ppsize) {
		if (p->ppname != NULL)
			icp->al->free(icp->al, p->ppname);
		if ((p->ppname = (char *) icp->al->malloc(icp->al, p->ppsize * sizeof(char))) == NULL) {
			sprintf(icp->err,"icmCrdInfo_alloc: malloc() of string data failed");
			return icp->errc = 2;
		}
		p->_ppsize = p->ppsize;
	}
	for (t = 0; t < 4; t++) {	/* For all 4 intents */
		if (p->crdsize[t] != p->_crdsize[t]) {
			if (p->crdname[t] != NULL)
				icp->al->free(icp->al, p->crdname[t]);
			if ((p->crdname[t] = (char *) icp->al->malloc(icp->al, p->crdsize[t] * sizeof(char))) == NULL) {
				sprintf(icp->err,"icmCrdInfo_alloc: malloc() of CRD%d name string failed",t);
				return icp->errc = 2;
			}
			p->_crdsize[t] = p->crdsize[t];
		}
	}
	return 0;
}

/* Free all storage in the object */
static void icmCrdInfo_delete(
	icmBase *pp
) {
	icmCrdInfo *p = (icmCrdInfo *)pp;
	icc *icp = p->icp;
	unsigned int t;

	if (p->ppname != NULL)
		icp->al->free(icp->al, p->ppname);
	for (t = 0; t < 4; t++) {	/* For all 4 intents */
		if (p->crdname[t] != NULL)
			icp->al->free(icp->al, p->crdname[t]);
	}
	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmBase *new_icmCrdInfo(
	icc *icp
) {
	icmCrdInfo *p;
	if ((p = (icmCrdInfo *) icp->al->calloc(icp->al,1,sizeof(icmCrdInfo))) == NULL)
		return NULL;
	p->ttype    = icSigCrdInfoType;
	p->refcount = 1;
	p->get_size = icmCrdInfo_get_size;
	p->read     = icmCrdInfo_read;
	p->write    = icmCrdInfo_write;
	p->dump     = icmCrdInfo_dump;
	p->allocate = icmCrdInfo_allocate;
	p->del      = icmCrdInfo_delete;
	p->icp      = icp;

	return (icmBase *)p;
}

/* ========================================================== */
/* icmHeader object */
/* ========================================================== */

/* Return the number of bytes needed to write this tag */
static unsigned int icmHeader_get_size(
	icmHeader *p
) {
	return 128;		/* By definition */
}

/* read the object, return 0 on success, error code on fail */
static int icmHeader_read(
	icmHeader *p,
	unsigned long len,		/* tag length */
	unsigned long of		/* start offset within file */
) {
	icc *icp = p->icp;
	char *buf;
	unsigned int tt;
	int rv = 0;
	
	if (len != 128) {
		sprintf(icp->err,"icmHeader_read: Length expected to be 128");
		return icp->errc = 1;
	}

	if ((buf = (char *) icp->al->malloc(icp->al, len)) == NULL) {
		sprintf(icp->err,"icmHeader_read: malloc() failed");
		return icp->errc = 2;
	}
	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->read(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmHeader_read: fseek() or fread() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}

	/* Fill in the in-memory structure */
	p->size  = read_UInt32Number(buf + 0);	/* Profile size in bytes */
    p->cmmId = read_SInt32Number(buf + 4);	/* CMM for profile */
	tt       = read_UInt8Number(buf + 8);	/* Raw major version number */
    p->majv  = (tt >> 4) * 10 + (tt & 0xf);	/* Integer major version number */
	tt       = read_UInt8Number(buf + 9);	/* Raw minor/bug fix version numbers */
    p->minv  = (tt >> 4);					/* Integer minor version number */
    p->bfv   = (tt & 0xf);					/* Integer bug fix version number */
	p->deviceClass = (icProfileClassSignature)
	           read_SInt32Number(buf + 12);	/* Type of profile */
    p->colorSpace = (icColorSpaceSignature)
	           read_SInt32Number(buf + 16);	/* Color space of data */
    p->pcs = (icColorSpaceSignature)
	           read_SInt32Number(buf + 20);	/* PCS: XYZ or Lab */
	if ((rv = read_DateTimeNumber(&p->date, buf + 24)) != 0) {	/* Creation Date */
		sprintf(icp->err,"icmHeader_read: read_DateTimeNumber corrupted");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	tt = read_SInt32Number(buf+36);
	if (tt != icMagicNumber) {				/* Check magic number */
		sprintf(icp->err,"icmHeader_read: wrong magic number 0x%x",tt);
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
    p->platform = (icPlatformSignature)
	           read_SInt32Number(buf + 40);			/* Primary platform */
    p->flags = read_UInt32Number(buf + 44);			/* Various bits */
    p->manufacturer = read_SInt32Number(buf + 48); /* Dev manufacturer */
    p->model = read_SInt32Number(buf + 52);			/* Dev model */
    read_UInt64Number(&p->attributes, buf + 56);	/* Device attributes */
	p->renderingIntent = (icRenderingIntent)
	           read_SInt32Number(buf + 64);	/* Rendering intent */
	if ((rv = read_XYZNumber(&p->illuminant, buf + 68)) != 0) {	/* Profile illuminant */
		sprintf(icp->err,"icmHeader_read: read_XYZNumber error");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
    p->creator = read_SInt32Number(buf + 80);	/* Profile creator */

	icp->al->free(icp->al, buf);
	return 0;
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icmHeader_write(
	icmHeader *p,
	unsigned long of			/* File offset to write from */
) {
	icc *icp = p->icp;
	char *buf;		/* Buffer to write from */
	unsigned int len;
	unsigned int tt;
	int rv = 0;

	len = p->get_size(p);
	if ((buf = (char *) icp->al->calloc(icp->al,1,len)) == NULL) {			/* Zero it - some CMS are fussy */
		sprintf(icp->err,"icmHeader_write calloc() failed");
		return icp->errc = 2;
	}

	/* Fill in the write buffer */
	if ((rv = write_UInt32Number(p->size, buf + 0)) != 0) {	/* Profile size in bytes */
		sprintf(icp->err,"icmHeader_write: profile size");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

    if ((rv = write_SInt32Number(p->cmmId, buf + 4)) != 0) {	/* CMM for profile */
		sprintf(icp->err,"icmHeader_write: cmmId");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if (p->majv < 0 || p->majv > 99			/* Sanity check version numbers */
	 || p->minv < 0 || p->minv > 9
	 || p->bfv  < 0 || p->bfv  > 9) {
		sprintf(icp->err,"icmHeader_write: version number");
		icp->al->free(icp->al, buf);
		return icp->errc = 1;
	}
	tt = ((p->majv/10) << 4) + (p->majv % 10);
	if ((rv = write_UInt8Number(tt, buf + 8)) != 0) {	/* Raw major version number */
		sprintf(icp->err,"icmHeader_write: Uint8Number major version");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
    tt = (p->minv << 4) + p->bfv;
	if ((rv = write_UInt8Number(tt, buf + 9)) != 0) {	/* Raw minor/bug fix version numbers */
		sprintf(icp->err,"icmHeader_write: Uint8Number minor/bug fix");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_SInt32Number((int)p->deviceClass, buf + 12)) != 0) {	/* Type of profile */
		sprintf(icp->err,"icmHeader_write: SInt32Number deviceClass");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_SInt32Number((int)p->colorSpace, buf + 16)) != 0) {	/* Color space of data */
		sprintf(icp->err,"icmHeader_write: SInt32Number data color space");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_SInt32Number((int)p->pcs, buf + 20)) != 0) {		/* PCS: XYZ or Lab */
		sprintf(icp->err,"icmHeader_write: SInt32Number PCS");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_DateTimeNumber(&p->date, buf + 24)) != 0) {		/* Creation Date */
		sprintf(icp->err,"icmHeader_write: DateTimeNumber creation");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_SInt32Number(icMagicNumber, buf+36)) != 0) {		/* Magic number */
		sprintf(icp->err,"icmHeader_write: SInt32Number magic");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_SInt32Number((int)p->platform, buf + 40)) != 0) {	/* Primary platform */
		sprintf(icp->err,"icmHeader_write: SInt32Number platform");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
    if ((rv = write_UInt32Number(p->flags, buf + 44)) != 0) {			/* Various bits */
		sprintf(icp->err,"icmHeader_write: UInt32Number flags");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
    if ((rv = write_SInt32Number(p->manufacturer, buf + 48)) != 0) { /* Dev manufacturer */
		sprintf(icp->err,"icmHeader_write: SInt32Number manufaturer");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
    if ((write_SInt32Number(p->model, buf + 52)) != 0) {				/* Dev model */
		sprintf(icp->err,"icmHeader_write: SInt32Number model");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
    if ((rv = write_UInt64Number(&p->attributes, buf + 56)) != 0) {	/* Device attributes */
		sprintf(icp->err,"icmHeader_write: UInt64Number attributes");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_SInt32Number((int)p->renderingIntent, buf + 64)) != 0) { /* Rendering intent */
		sprintf(icp->err,"icmHeader_write: SInt32Number rendering intent");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
	if ((rv = write_XYZNumber(&p->illuminant, buf + 68)) != 0) {		/* Profile illuminant */
		sprintf(icp->err,"icmHeader_write: XYZNumber illuminant");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}
    if ((rv = write_SInt32Number(p->creator, buf + 80)) != 0) {		/* Profile creator */
		sprintf(icp->err,"icmHeader_write: SInt32Number creator");
		icp->al->free(icp->al, buf);
		return icp->errc = rv;
	}

	if (   icp->fp->seek(icp->fp, of) != 0
	    || icp->fp->write(icp->fp, buf, 1, len) != len) {
		sprintf(icp->err,"icmHeader_write fseek() or fwrite() failed");
		icp->al->free(icp->al, buf);
		return icp->errc = 2;
	}

	icp->al->free(icp->al, buf);
	return rv;
}

static void icmHeader_dump(
	icmHeader *p,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	if (verb <= 0)
		return;

	fprintf(op,"Header:\n");
	fprintf(op,"  size         = %d bytes\n",p->size);
	fprintf(op,"  CMM          = %s\n",tag2str(p->cmmId));
	fprintf(op,"  Version      = %d.%d.%d\n",p->majv, p->minv, p->bfv);
	fprintf(op,"  Device Class = %s\n", string_ProfileClassSignature(p->deviceClass));
	fprintf(op,"  Color Space  = %s\n", string_ColorSpaceSignature(p->colorSpace));
	fprintf(op,"  Conn. Space  = %s\n", string_ColorSpaceSignature(p->pcs));
	fprintf(op,"  Date, Time   = %s\n", string_DateTimeNumber(&p->date));
	fprintf(op,"  Platform     = %s\n", string_PlatformSignature(p->platform));
	fprintf(op,"  Flags        = %s\n", string_ProfileHeaderFlags(p->flags));
	fprintf(op,"  Dev. Mnfctr. = %s\n",tag2str(p->manufacturer));	/* ~~~ */
	fprintf(op,"  Dev. Model   = %s\n",tag2str(p->model));	/* ~~~ */
	fprintf(op,"  Dev. Attrbts = %s\n", string_DeviceAttributes(p->attributes.l));
	fprintf(op,"  Rndrng Intnt = %s\n", string_RenderingIntent(p->renderingIntent));
	fprintf(op,"  Illuminant   = %s\n", string_XYZNumber_and_Lab(&p->illuminant));
	fprintf(op,"  Creator      = %s\n",tag2str(p->creator));	/* ~~~ */
	fprintf(op,"\n");
}

static void icmHeader_delete(
	icmHeader *p
) {
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

/* Create an empty object. Return null on error */
static icmHeader *new_icmHeader(
	icc *icp
) {
	icmHeader *p;
	if ((p = (icmHeader *) icp->al->calloc(icp->al,1,sizeof(icmHeader))) == NULL)
		return NULL;
	p->icp      = icp;
	p->get_size = icmHeader_get_size;
	p->read     = icmHeader_read;
	p->write    = icmHeader_write;
	p->dump     = icmHeader_dump;
	p->del      = icmHeader_delete;

	return p;
}

/* ---------------------------------------------------------- */
/* Type vector table. Match the Tag type against the object creator */
static struct {
	icTagTypeSignature  ttype;			/* The tag type signature */
	icmBase *              (*new_obj)(icc *icp);
} typetable[] = {
	{icSigCrdInfoType,             new_icmCrdInfo},
	{icSigCurveType,               new_icmCurve},
	{icSigDataType,                new_icmData},
	{icSigDateTimeType,            new_icmDateTimeNumber},
	{icSigLut16Type,               new_icmLut},
	{icSigLut8Type,                new_icmLut},
	{icSigMeasurementType,         new_icmMeasurement},
	{icSigNamedColorType,          new_icmNamedColor},
	{icSigNamedColor2Type,         new_icmNamedColor},
	{icSigProfileSequenceDescType, new_icmProfileSequenceDesc},
	{icSigS15Fixed16ArrayType,     new_icmS15Fixed16Array},
	{icSigScreeningType,           new_icmScreening},
	{icSigSignatureType,           new_icmSignature},
	{icSigTextDescriptionType,     new_icmTextDescription},
	{icSigTextType,                new_icmText},
	{icSigU16Fixed16ArrayType,     new_icmU16Fixed16Array},
	{icSigUcrBgType,               new_icmUcrBg},
	{icSigVideoCardGammaType,      new_icmVideoCardGamma},
	{icSigUInt16ArrayType,         new_icmUInt16Array},
	{icSigUInt32ArrayType,         new_icmUInt32Array},
	{icSigUInt64ArrayType,         new_icmUInt64Array},
	{icSigUInt8ArrayType,          new_icmUInt8Array},
	{icSigViewingConditionsType,   new_icmViewingConditions},
	{icSigXYZArrayType,            new_icmXYZArray},
	{icMaxEnumType,                NULL}
}; 

/* Table that lists the legal Types for each Tag Signature */
static struct {
	icTagSignature      sig;
	icTagTypeSignature  ttypes[4];			/* Arbitrary max of 4 */
} sigtypetable[] = {
	{icSigAToB0Tag,					{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigAToB1Tag,					{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigAToB2Tag,					{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigBlueColorantTag,			{icSigXYZType,icMaxEnumType}},
	{icSigBlueTRCTag,				{icSigCurveType,icMaxEnumType}},
	{icSigBToA0Tag,					{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigBToA1Tag,					{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigBToA2Tag,					{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigCalibrationDateTimeTag,	{icSigDateTimeType,icMaxEnumType}},
	{icSigCharTargetTag,			{icSigTextType,icMaxEnumType}},
	{icSigCopyrightTag,				{icSigTextType,icMaxEnumType}},
	{icSigCrdInfoTag,				{icSigCrdInfoType,icMaxEnumType}},
	{icSigDeviceMfgDescTag,			{icSigTextDescriptionType,icMaxEnumType}},
	{icSigDeviceModelDescTag,		{icSigTextDescriptionType,icMaxEnumType}},
	{icSigGamutTag,					{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigGrayTRCTag,				{icSigCurveType,icMaxEnumType}},
	{icSigGreenColorantTag,			{icSigXYZType,icMaxEnumType}},
	{icSigGreenTRCTag,				{icSigCurveType,icMaxEnumType}},
	{icSigLuminanceTag,				{icSigXYZType,icMaxEnumType}},
	{icSigMeasurementTag,			{icSigMeasurementType,icMaxEnumType}},
	{icSigMediaBlackPointTag,		{icSigXYZType,icMaxEnumType}},
	{icSigMediaWhitePointTag,		{icSigXYZType,icMaxEnumType}},
	{icSigNamedColorTag,			{icSigNamedColorType,icMaxEnumType}},
	{icSigNamedColor2Tag,			{icSigNamedColor2Type,icMaxEnumType}},
	{icSigPreview0Tag,				{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigPreview1Tag,				{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigPreview2Tag,				{icSigLut8Type,icSigLut16Type,icMaxEnumType}},
	{icSigProfileDescriptionTag,	{icSigTextDescriptionType,icMaxEnumType}},
	{icSigProfileSequenceDescTag,	{icSigProfileSequenceDescType,icMaxEnumType}},
	{icSigPs2CRD0Tag,				{icSigDataType,icMaxEnumType}},
	{icSigPs2CRD1Tag,				{icSigDataType,icMaxEnumType}},
	{icSigPs2CRD2Tag,				{icSigDataType,icMaxEnumType}},
	{icSigPs2CRD3Tag,				{icSigDataType,icMaxEnumType}},
	{icSigPs2CSATag,				{icSigDataType,icMaxEnumType}},
	{icSigPs2RenderingIntentTag,	{icSigDataType,icMaxEnumType}},
	{icSigRedColorantTag,			{icSigXYZType,icMaxEnumType}},
	{icSigRedTRCTag,				{icSigCurveType,icMaxEnumType}},
	{icSigScreeningDescTag,			{icSigTextDescriptionType,icMaxEnumType}},
	{icSigScreeningTag,				{icSigScreeningType,icMaxEnumType}},
	{icSigTechnologyTag,			{icSigSignatureType,icMaxEnumType}},
	{icSigUcrBgTag,					{icSigUcrBgType,icMaxEnumType}},
	{icSigVideoCardGammaTag,		{icSigVideoCardGammaType,icMaxEnumType}},
	{icSigViewingCondDescTag,		{icSigTextDescriptionType,icMaxEnumType}},
	{icSigViewingConditionsTag,		{icSigViewingConditionsType,icMaxEnumType}},
	{icMaxEnumType,					{icMaxEnumType}}
}; 

/* Fake color tag for specifying PCS */
#define icSigPCSData  ((icColorSpaceSignature) 0x50435320L)

/* Table that lists the required tags for various profiles */
static struct {
	icProfileClassSignature sig;		/* Profile signature */
	int      			    chans;		/* Data Color channels, -ve for match but try next, */
										/*          -100 for ignore, -200 for ignore and try next */
	icColorSpaceSignature   colsig;		/* Data Color space signature, icMaxEnumData for ignore, */
										/*                           icSigPCSData for XYZ of Lab */
	icColorSpaceSignature   pcssig;		/* PCS Color space signature, icMaxEnumData for ignore, */
										/*                          icSigPCSData for XYZ or Lab */
	icTagSignature          tags[12];	/* Arbitrary max of 12 */
} tagchecktable[] = {
    {icSigInputClass,      -1, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigGrayTRCTag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigInputClass,      -3, icMaxEnumData, icSigXYZData,
	 	{icSigProfileDescriptionTag,
		 icSigRedColorantTag,
		 icSigGreenColorantTag,
		 icSigBlueColorantTag,
		 icSigRedTRCTag,
		 icSigGreenTRCTag,
		 icSigBlueTRCTag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigInputClass,     -100, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigDisplayClass,     -1, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigGrayTRCTag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigDisplayClass,     -3, icSigRgbData, icSigXYZData,	/* Rgb or any 3 component space ?? */
	 	{icSigProfileDescriptionTag,
		 icSigRedColorantTag,
		 icSigGreenColorantTag,
		 icSigBlueColorantTag,
		 icSigRedTRCTag,
		 icSigGreenTRCTag,
		 icSigBlueTRCTag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

	/* Non-3 component Display device */
    {icSigDisplayClass,     -100, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,					/* BToA doesn't seem to be required, which is strange... */
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigOutputClass,     -1, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigGrayTRCTag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigOutputClass,     -1, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,
		 icSigBToA0Tag,
		 icSigGamutTag,
		 icSigAToB1Tag,
		 icSigBToA1Tag,
		 icSigAToB2Tag,
		 icSigBToA2Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigOutputClass,     -2, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,
		 icSigBToA0Tag,
		 icSigGamutTag,
		 icSigAToB1Tag,
		 icSigBToA1Tag,
		 icSigAToB2Tag,
		 icSigBToA2Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigOutputClass,     -3, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,
		 icSigBToA0Tag,
		 icSigGamutTag,
		 icSigAToB1Tag,
		 icSigBToA1Tag,
		 icSigAToB2Tag,
		 icSigBToA2Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigOutputClass,     -4, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,
		 icSigBToA0Tag,
		 icSigGamutTag,
		 icSigAToB1Tag,
		 icSigBToA1Tag,
		 icSigAToB2Tag,
		 icSigBToA2Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigOutputClass,     -100, icMaxEnumData, icSigPCSData,	/* Assumed from Hexachrome examples */
	 	{icSigProfileDescriptionTag,
		 icSigBToA0Tag,
		 icSigBToA1Tag,
		 icSigBToA2Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigLinkClass,      -100, icMaxEnumData, icMaxEnumData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,
		 icSigProfileSequenceDescTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigColorSpaceClass,       -100, icMaxEnumData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigBToA0Tag,
		 icSigAToB0Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigAbstractClass,      -100, icSigPCSData, icSigPCSData,
	 	{icSigProfileDescriptionTag,
		 icSigAToB0Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigNamedColorClass,        -200, icMaxEnumData, icMaxEnumData,
	 	{icSigProfileDescriptionTag,
		 icSigNamedColor2Tag,
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

    {icSigNamedColorClass,        -100, icMaxEnumData, icMaxEnumData,
	 	{icSigProfileDescriptionTag,
		 icSigNamedColorTag,				/* Not strictly V3.4 */
		 icSigMediaWhitePointTag,
		 icSigCopyrightTag, icMaxEnumType}},

	{icMaxEnumType,-1,icMaxEnumData, icMaxEnumData,		{icMaxEnumType}}
}; 

/* ------------------------------------------------------------- */
/* Check that the ICC profile looks like it will be legal. */
/* Return non-zero and set error string if not */
static int check_icc_legal(
	icc *p
) {
	int i, j;
	icProfileClassSignature  sig;
	icColorSpaceSignature colsig;
	icColorSpaceSignature pcssig;
	int      	          dchans;

	if (p->header == NULL) {
		sprintf(p->err,"icc_check_legal: Header is missing");
		return p->errc = 1;
	}

	sig = p->header->deviceClass;
	colsig = p->header->colorSpace;
	dchans = number_ColorSpaceSignature(colsig);
	pcssig = p->header->pcs;

	/* Find a matching table entry */
	for (i = 0; tagchecktable[i].sig != icMaxEnumType; i++) {
		if (    tagchecktable[i].sig   == sig
		 && (   tagchecktable[i].chans == dchans	/* Exactly matches */
		     || tagchecktable[i].chans == -dchans	/* Exactly matches, but can try next table */
		     || tagchecktable[i].chans < -99)		/* Doesn't have to match or try next table */
		 && (   tagchecktable[i].colsig == colsig
		     || (tagchecktable[i].colsig == icSigPCSData
		         && (colsig == icSigXYZData || colsig == icSigLabData))
		     || tagchecktable[i].colsig == icMaxEnumData)
		 && (   tagchecktable[i].pcssig == pcssig
		     || (tagchecktable[i].pcssig == icSigPCSData
		         && (pcssig == icSigXYZData || pcssig == icSigLabData))
		     || tagchecktable[i].pcssig == icMaxEnumData)) {

			/* Found entry, so now check that all the required tags are present. */
			for (j = 0; tagchecktable[i].tags[j] != icMaxEnumType; j++) {
				if (p->find_tag(p, tagchecktable[i].tags[j]) != 0) {	/* Not present! */
					if (tagchecktable[i].chans == -200
					 || tagchecktable[i].chans == -dchans) {	/* But can try next table */
						break;
					}
					sprintf(p->err,"icc_check_legal: deviceClass %s is missing required tag %s",
					               tag2str(sig), tag2str(tagchecktable[i].tags[j]));
					return p->errc = 1;
				}
			}
			if (tagchecktable[i].tags[j] == icMaxEnumType) {
				break;		/* Fount all required tags */
			}
		}
	}

	/* According to the spec. if the deviceClass is:
		an Abstract Class: both in and out color spaces should be PCS
		an Link Class: both in and out color spaces can be any, and should
		    be the input space of the first profile in the link, and the
		    input space of the last profile in the link respectively.
		a Named Class: in and out color spaces are not defined in the spec.
		Input, Display, Output and ColorSpace Classes, input color
		    space can be any, and the output space must be PCS.
		~~ should check this here ???
	*/
	
	return 0;	/* Assume anything is ok */
}


/* read the object, return 0 on success, error code on fail */
/* NOTE: this doesn't read the tag types, they should be read on demand. */
static int icc_read(
	icc *p,
	icmFile *fp,			/* File to read from */
	unsigned long of		/* File offset to read from */
) {
	char tcbuf[4];			/* Tag count read buffer */
	int i;
	unsigned int len;
	int er = 0;				/* Error code */
	
	p->fp = fp;
	p->of = of;
	if (p->header == NULL) {
		sprintf(p->err,"icc_read: No header defined");
		return p->errc = 1;
	}

	/* Read the header */
	if (p->header->read(p->header, 128, of)) {
		return 1;
	}

	/* Read the tag count */
	if (   p->fp->seek(p->fp, of + 128) != 0
	    || p->fp->read(p->fp, tcbuf, 1, 4) != 4) {
		sprintf(p->err,"icc_read: fseek() or fread() failed on tag count");
		return p->errc = 1;
	}

	p->count = read_UInt32Number(tcbuf);		/* Tag count */
	if (p->count > 0) {
		char *bp, *buf;
		if ((p->data = (icmTag *) p->al->malloc(p->al, p->count * sizeof(icmTag))) == NULL) {
			sprintf(p->err,"icc_read: Tag table malloc() failed");
			return p->errc = 2;
		}
	
		len = 4 + p->count * 12;
		if ((buf = (char *) p->al->malloc(p->al, len)) == NULL) {
			sprintf(p->err,"icc_read: Tag table read buffer malloc() failed");
			p->al->free(p->al, p->data);
			p->data = NULL;
			return p->errc = 2;
		}
		if (   p->fp->seek(p->fp, of + 128) != 0
		    || p->fp->read(p->fp, buf, 1, len) != len) {
			sprintf(p->err,"icc_read: fseek() or fread() failed on tag table");
			p->al->free(p->al, p->data);
			p->data = NULL;
			p->al->free(p->al, buf);
			return p->errc = 1;
		}

		/* Fill in the tag table structure */
		bp = buf+4;
		for (i = 0; i < p->count; i++, bp += 12) {
	    	p->data[i].sig = (icTagSignature)read_SInt32Number(bp + 0);	
	    	p->data[i].offset = read_UInt32Number(bp + 4);
	    	p->data[i].size = read_UInt32Number(bp + 8);	
			if (   p->fp->seek(p->fp, of + p->data[i].offset) != 0
			    || p->fp->read(p->fp, tcbuf, 1, 4) != 4) {
				sprintf(p->err,"icc_read: fseek() or fread() failed on tag headers");
				p->al->free(p->al, p->data);
				p->data = NULL;
				p->al->free(p->al, buf);
				return p->errc = 1;
			}
	    	p->data[i].ttype = read_SInt32Number(tcbuf);		/* Tag type */
	    	p->data[i].objp = NULL;							/* Read on demand */
		}
		p->al->free(p->al, buf);
	}	/* p->count > 0 */

	return er;
}

#define DO_ALIGN(val) (((val) + 3) & ~3)

/* Return the total size needed for the profile */
/* Return 0 on error. */
static unsigned int icc_get_size(
	icc *p
) {
	unsigned int size = 0;
	int i;

	/* Check that the right tags etc. are present for a legal ICC profile */
	if (check_icc_legal(p) != 0) {
		return 0;
	}

	/* Compute the total size and tag element data offsets */
	if (p->header == NULL) {
		sprintf(p->err,"icc_get_size: No header defined");
		p->errc = 1;
		return 0;
	}

	size += p->header->get_size(p->header);

	size = DO_ALIGN(size);
	size += 4 + p->count * 12;	/* Tag table length */
	
	/* Reset touched flag for each tag type */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].objp == NULL) {
			sprintf(p->err,"icc_get_size: Internal error - NULL tag element");
			p->errc = 1;
			return 0;
		}
		p->data[i].objp->touched = 0;
	}
	/* Get size for each tag type, skipping links */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].objp->touched == 0) { /* Not alllowed for previously */
			size = DO_ALIGN(size);
			size += p->data[i].objp->get_size(p->data[i].objp);
			p->data[i].objp->touched = 1;	/* Don't account for this again */
		}
	}

	return size;	/* Total size needed */
}

/* Write the contents of the object. Return 0 on sucess, error code on failure */
static int icc_write(
	icc *p,
	icmFile *fp,		/* File to write to */
	unsigned long of	/* File offset to write to */
) {
	char *bp, *buf;		/* Buffer to write to */
	unsigned int len;
	int rv = 0;
	int i;
	unsigned int size = 0;

	/* Check that the right tags etc. are present for a legal ICC profile */
	if ((rv = check_icc_legal(p)) != 0) {
		return rv;
	}

	p->fp = fp;			/* Open file pointer */
	p->of = of;			/* Offset of ICC profile */

	/* Compute the total size and tag element data offsets */
	if (p->header == NULL) {
		sprintf(p->err,"icc_write: No header defined");
		return p->errc = 1;
	}

	size += p->header->get_size(p->header);

	len = 4 + p->count * 12;	/* Tag table length */
	size = DO_ALIGN(size);
	size += len;
	
	/* Allocate memory buffer for tag table */
	if ((buf = (char *) p->al->malloc(p->al, len)) == NULL) {
		sprintf(p->err,"icc_write malloc() failed");
		return p->errc = 2;
	}
	bp = buf;

    if ((rv = write_UInt32Number(p->count, bp)) != 0) {		/* Tag count */
		sprintf(p->err,"icc_write: write_UInt32Number() failed on tag count");
		p->al->free(p->al, buf);
		return p->errc = rv;
	}
	bp += 4;
	/* Reset touched flag for each tag type */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].objp == NULL) {
			sprintf(p->err,"icc_write: Internal error - NULL tag element");
			p->al->free(p->al, buf);
			return p->errc = 1;
		}
		p->data[i].objp->touched = 0;
	}
	/* Set the offset and size for each tag type */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].objp->touched == 0) {	/* Allocate space for tag type */
			size = DO_ALIGN(size);
			p->data[i].offset = size;			/* Profile relative target */
			p->data[i].size = p->data[i].objp->get_size(p->data[i].objp);
			size += p->data[i].size;
			p->data[i].objp->touched = 1;	/* Allocated space for it */
		} else { /* must be linked - copy allocation */
			int k;
			for (k = 0; k < p->count; k++) {	/* Find linked tag */
				if (p->data[k].objp == p->data[i].objp)
					break;
			}
			if (k == p->count) {
				sprintf(p->err,"icc_write: corrupted link"); 
				return p->errc = 2;
			}
		    p->data[i].offset = p->data[k].offset;
		    p->data[i].size   = p->data[k].size;
		}
		/* Write tag table entry for this tag */
		if ((rv = write_SInt32Number((int)p->data[i].sig,bp + 0)) != 0) {
			sprintf(p->err,"icc_write: write_SInt32Number() failed on tag signature");
			p->al->free(p->al, buf);
			return p->errc = rv;
		}
		if ((rv = write_UInt32Number(p->data[i].offset, bp + 4)) != 0) {
			sprintf(p->err,"icc_write: write_UInt32Number() failed on tag offset");
			p->al->free(p->al, buf);
			return p->errc = rv;
		}
		if ((rv = write_UInt32Number(p->data[i].size, bp + 8)) != 0) {
			sprintf(p->err,"icc_write: write_UInt32Number() failed on tag size");
			p->al->free(p->al, buf);
			return p->errc = rv;
		}
		bp += 12;
	}
	p->header->size = size;		/* Record total icc size */
	
	/* Write the header */
	if ((rv = p->header->write(p->header, of)) != 0) {
		p->al->free(p->al, buf);
		return rv;
	}

	/* Write the tag table */
	if (   p->fp->seek(p->fp, of + 128) != 0
	    || p->fp->write(p->fp, buf, 1, len) != len) {
		sprintf(p->err,"icc_write: fseek() or fwrite() failed");
		p->al->free(p->al, buf);
		return p->errc = 1;
	}
	p->al->free(p->al, buf);

	/* Write all the tag element data */
	for (i = 0; i < p->count; i++) {	/* For all the tag element data */
		if (p->data[i].objp->touched == 0)
			continue;		/* Must be linked, and we've already written it */
		if ((rv = p->data[i].objp->write(p->data[i].objp, of + p->data[i].offset)) != 0) {
			return rv;
		}
		p->data[i].objp->touched = 0;	/* Written it */
	}

	if (p->fp->flush(p->fp) != 0) {
		sprintf(p->err,"icc_write flush() failed");
		p->al->free(p->al, buf);
		return p->errc = 2;
	}
	return rv;
}
#undef DO_ALIGN

/* Create and add a tag with the given signature. */
/* Returns a pointer to the element object */
/* Returns NULL if error - icc->errc will contain */
/* 2 on system error, */
/* 3 if unknown tag */
/* NOTE: that we prevent tag duplication */
static icmBase *icc_add_tag(
	icc *p,
    icTagSignature sig,			/* Tag signature - may be unknown */
	icTagTypeSignature  ttype	/* Tag type */
) {
	icmBase *tp;
	icmBase *nob;
	int i, j, ok = 1;

	/* Check that a known signature has an acceptable type */
	for (i = 0; sigtypetable[i].sig != icMaxEnumType; i++) {
		if (sigtypetable[i].sig == sig) {	/* recognized signature */
			ok = 0;
			for (j = 0; sigtypetable[i].ttypes[j] != icMaxEnumType; j++) {
				if (sigtypetable[i].ttypes[j] == ttype)	/* recognized type */
					ok = 1;
			}
			break;
		}
	}
	if (!ok) {
		sprintf(p->err,"icc_add_tag: wrong tag type for signature");
		p->errc = 1;
		return NULL;
	}

	/* Check that we know how to handle this type */
	for (i = 0; typetable[i].ttype != icMaxEnumType; i++) {
		if (typetable[i].ttype == ttype)
			break;
	}
	if (typetable[i].ttype == icMaxEnumType) {
		sprintf(p->err,"icc_add_tag: unsupported tag type");
		p->errc = 1;
		return NULL;
	}

	/* Check that this tag doesn't already exits */
	/* (Perhaps we should simply replace it, rather than erroring ?) */
	for (j = 0; j < p->count; j++) {
		if (p->data[j].sig == sig) {
			sprintf(p->err,"icc_add_tag: Already have tag '%s' in profile",tag2str(p->data[j].sig)); 
			p->errc = 1;
			return NULL;
		}
	}

	/* Make space in tag table for new tag item */
	if (p->data == NULL)
		tp = p->al->malloc(p->al, (p->count+1) * sizeof(icmTag));
	else
		tp = p->al->realloc(p->al, (void *)p->data, (p->count+1) * sizeof(icmTag));
	if (tp == NULL) {
		sprintf(p->err,"icc_add_tag: Tag table realloc() failed");
		p->errc = 2;
		return NULL;
	}
	p->data = (icmTag *)tp;

	/* Allocate the empty object */
	if ((nob = typetable[i].new_obj(p)) == NULL)
		return NULL;

	/* Fill out our tag table entry */
    p->data[p->count].sig = sig;		/* The tag signature */
	p->data[p->count].ttype = nob->ttype = ttype;	/* The tag type signature */
    p->data[p->count].offset = 0;		/* Unknown offset yet */
    p->data[p->count].size = 0;			/* Unknown size yet */
    p->data[p->count].objp = nob;		/* Empty object */
	p->count++;

	return nob;
}

/* Create and add a tag which is a link to an existing tag. */
/* Returns a pointer to the element object */
/* Returns NULL if error - icc->errc will contain */
/* 3 if incompatible tag */
/* NOTE: that we prevent tag duplication */
static icmBase *icc_link_tag(
	icc *p,
    icTagSignature sig,			/* Tag signature - may be unknown */
    icTagSignature ex_sig		/* Tag signature of tag to link to */
) {
	icmBase *tp;
	int i, j, exi, ok = 1;

	/* Search for existing signature */
	for (exi = 0; exi < p->count; exi++) {
		if (p->data[exi].sig == ex_sig)		/* Found it */
			break;
	}
	if (exi == p->count) {
		sprintf(p->err,"icc_link_tag: Can't find existing tag '%s'",tag2str(ex_sig)); 
		p->errc = 1;
		return NULL;
	}

    if (p->data[exi].objp == NULL) {
		sprintf(p->err,"icc_link_tag: Existing tag '%s' isn't loaded",tag2str(ex_sig)); 
		p->errc = 1;
		return NULL;
	}

	/* Check that a known signature has an acceptable type */
	for (i = 0; sigtypetable[i].sig != icMaxEnumType; i++) {
		if (sigtypetable[i].sig == sig) {	/* recognized signature */
			ok = 0;
			for (j = 0; sigtypetable[i].ttypes[j] != icMaxEnumType; j++) {
				if (sigtypetable[i].ttypes[j] == p->data[exi].ttype)	/* recognized type */
					ok = 1;
			}
			break;
		}
	}
	if (!ok) {
		sprintf(p->err,"icc_link_tag: wrong tag type for signature");
		p->errc = 1;
		return NULL;
	}

	/* Check that this tag doesn't already exits */
	for (j = 0; j < p->count; j++) {
		if (p->data[j].sig == sig) {
			sprintf(p->err,"icc_link_tag: Already have tag '%s' in profile",tag2str(p->data[j].sig)); 
			p->errc = 1;
			return NULL;
		}
	}

	/* Make space in tag table for new tag item */
	if (p->data == NULL)
		tp = p->al->malloc(p->al, (p->count+1) * sizeof(icmTag));
	else
		tp = p->al->realloc(p->al, (void *)p->data, (p->count+1) * sizeof(icmTag));
	if (tp == NULL) {
		sprintf(p->err,"icc_link_tag: Tag table realloc() failed");
		p->errc = 2;
		return NULL;
	}
	p->data = (icmTag *)tp;

	/* Fill out our tag table entry */
    p->data[p->count].sig  = sig;		/* The tag signature */
	p->data[p->count].ttype  = p->data[exi].ttype;	/* The tag type signature */
    p->data[p->count].offset = p->data[exi].offset;	/* Same offset (may not be allocated yet) */
    p->data[p->count].size = p->data[exi].size;		/* Same size (may not be allocated yet) */
    p->data[p->count].objp = p->data[exi].objp;		/* Shared object */
	p->data[exi].objp->refcount++;					/* Bump reference count on tag type */
	p->count++;

	return p->data[exi].objp;
}

/* Search for tag signature */
/* return: */
/* 0 if found */
/* 1 if found but not handled type */
/* 2 if not found */
/* NOTE: doesn't set icc->errc or icc->err[] */
/* NOTE: we don't handle tag duplication - you'll always get the first in the file. */
static int icc_find_tag(
	icc *p,
    icTagSignature sig			/* Tag signature - may be unknown */
) {
	int i,j;

	/* Search for signature */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].sig == sig)		/* Found it */
			break;
	}
	if (i == p->count)
		return 2;

	/* See if we can handle this type */
	for (j = 0; typetable[j].ttype != icMaxEnumType; j++) {
		if (typetable[j].ttype == p->data[i].ttype)
			break;
	}
	if (typetable[j].ttype == icMaxEnumType)
		return 1;

	return 0;
}

/* Read the tag element data, and return a pointer to the object */
/**
 * Returns NULL if error - icc->errc will contain:
 * 1 if found but not handled type
 * 2 if not found
 **/
/* NOTE: we don't handle tag duplication - you'll always get the first in the file */
static icmBase *icc_read_tag(
	icc *p,
    icTagSignature sig			/* Tag signature - may be unknown */
) {
	icmBase *nob;
	int i,j,k;

	/* Search for signature */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].sig == sig)		/* Found it */
			break;
	}
	if (i >= p->count) {
		sprintf(p->err,"icc_read_tag: Tag '%s' not found",string_TagSignature(sig));
		p->errc = 2;
		return NULL;
	}

	/* See if it's already been read */
    if (p->data[i].objp != NULL) {
		return p->data[i].objp;		/* Just return it */
	}
	
	/* See if this should be a link */
	for (k = 0; k < p->count; k++) {
		if (i == k)
			continue;
	    if (p->data[i].ttype  == p->data[k].ttype	/* Exact match and already read */
	     && p->data[i].offset == p->data[k].offset
	     && p->data[i].size   == p->data[k].size
	     && p->data[k].objp != NULL)
			break;
	}
	if (k < p->count) {		/* Make this a link */
		p->data[i].objp = p->data[k].objp;
		p->data[k].objp->refcount++;	/* Bump reference count */
		return p->data[k].objp;			/* Done */
	}

	/* See if we can handle this type */
	for (j = 0; typetable[j].ttype != icMaxEnumType; j++) {
		if (typetable[j].ttype == p->data[i].ttype)
			break;
	}
	if (typetable[j].ttype == icMaxEnumType) {
		sprintf(p->err,"icc_read_tag: Unhandled tag type '%s'",string_TypeSignature(p->data[i].ttype));
		p->errc = 1;
		return NULL;
	}
	
	/* Creat and read in the object */
	if ((nob = typetable[j].new_obj(p)) == NULL)
		return NULL;
	if ((nob->read(nob, p->data[i].size, p->of + p->data[i].offset)) != 0) {
		nob->del(nob);		/* Failed, so destroy it */
		return NULL;
	}
    p->data[i].objp = nob;
	return nob;
}

/* Rename a tag signature */
static int icc_rename_tag(
	icc *p,
    icTagSignature sig,			/* Existing Tag signature - may be unknown */
    icTagSignature sigNew		/* New Tag signature - may be unknown */
) {
	int i, j, k, ok = 1;

	/* Search for signature */
	for (k = 0; k < p->count; k++) {
		if (p->data[k].sig == sig)		/* Found it */
			break;
	}
	if (k >= p->count) {
		sprintf(p->err,"icc_rename_tag: Tag '%s' not found",string_TagSignature(sig));
		return p->errc = 2;
	}

	/* Check that a known new signature has an acceptable type */
	for (i = 0; sigtypetable[i].sig != icMaxEnumType; i++) {
		if (sigtypetable[i].sig == sigNew) {	/* recognized signature */
			ok = 0;
			for (j = 0; sigtypetable[i].ttypes[j] != icMaxEnumType; j++) {
				if (sigtypetable[i].ttypes[j] == p->data[k].ttype)	/* recognized type */
					ok = 1;
			}
			break;
		}
	}

	if (!ok) {
		sprintf(p->err,"icc_rename_tag: wrong signature for tag type");
		p->errc = 1;
		return p->errc;
	}

	/* change its signature */
	p->data[k].sig = sigNew;

	return 0;
}

/* Unread the tag, and free the underlying tag type */
/* if this was the last reference to it. */
/* Returns non-zero on error: */
/* tag not found - icc->errc will contain 2 */
/* tag not read - icc->errc will contain 2 */
static int icc_unread_tag(
	icc *p,
    icTagSignature sig		/* Tag signature - may be unknown */
) {
	int i;

	/* Search for signature */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].sig == sig)		/* Found it */
			break;
	}
	if (i >= p->count) {
		sprintf(p->err,"icc_unread_tag: Tag '%s' not found",string_TagSignature(sig));
		return p->errc = 2;
	}

	/* See if it's been read */
    if (p->data[i].objp == NULL) {
		sprintf(p->err,"icc_unread_tag: Tag '%s' not currently loaded",string_TagSignature(sig));
		return p->errc = 2;
	}
	
	if (--(p->data[i].objp->refcount) == 0)			/* decrement reference count */
			(p->data[i].objp->del)(p->data[i].objp);	/* Last reference */
  	p->data[i].objp = NULL;

	return 0;
}

/* Delete the tag, and free the underlying tag type */
/* if this was the last reference to it. */
/* Returns non-zero on error: */
/* tag not found - icc->errc will contain 2 */
static int icc_delete_tag(
	icc *p,
    icTagSignature sig		/* Tag signature - may be unknown */
) {
	int i;

	/* Search for signature */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].sig == sig)		/* Found it */
			break;
	}
	if (i >= p->count) {
		sprintf(p->err,"icc_delete_tag: Tag '%s' not found",string_TagSignature(sig));
		return p->errc = 2;
	}

	/* If it's been read into memory, decrement the reference count */
    if (p->data[i].objp != NULL) {
		if (--(p->data[i].objp->refcount) == 0)			/* decrement reference count */
			(p->data[i].objp->del)(p->data[i].objp);	/* Last reference */
  		p->data[i].objp = NULL;
	}
	
	/* Now remove it from the tag list */
	for (; i < (p->count-1); i++)
		p->data[i] = p->data[i+1];	/* Copy the structure down */

	p->count--;		/* One less tag in list */

	return 0;
}


/* Read all the tags into memory. */
/* Returns non-zero on error. */
static int icc_read_all_tags(
	icc *p
) {
	int i;

	for (i = 0; i < p->count; i++) {	/* For all the tag element data */
		icmBase *ob;
		if ((ob = p->read_tag(p, p->data[i].sig)) == NULL) {
			return p->errc;
		}
	}
	return 0;
}


static void icc_dump(
	icc *p,
	FILE *op,		/* Output to dump to */
	int   verb		/* Verbosity level */
) {
	int i;
	if (verb <= 0)
		return;

	fprintf(op,"icc:\n");

	/* Dump the header */
	if (p->header != NULL)
		p->header->dump(p->header,op,verb);

	/* Dump all the tag elements */
	for (i = 0; i < p->count; i++) {	/* For all the tag element data */
		icmBase *ob;
		int tr;
		fprintf(op,"tag %d:\n",i);
		fprintf(op,"  sig      %s\n",tag2str(p->data[i].sig)); 
		fprintf(op,"  type     %s\n",tag2str(p->data[i].ttype)); 
		fprintf(op,"  offset   %d\n", p->data[i].offset);
		fprintf(op,"  size     %d\n", p->data[i].size);
		tr = 0;
		if ((ob = p->data[i].objp) == NULL) {
			/* The object is not loaded, so load it then free it */
			if ((ob = p->read_tag(p, p->data[i].sig)) == NULL) {
				fprintf(op,"Unable to read: %d, %s\n",p->errc,p->err);
			}
			tr = 1;
		}
		if (ob != NULL) {
			/* fprintf(op,"  refcount %d\n", ob->refcount); */
			ob->dump(ob,op,verb-1);

			if (tr != 0) {		/* Cleanup if temporary */
				ob->refcount--;
				(ob->del)(ob);
				p->data[i].objp = NULL;
			}
		}
		fprintf(op,"\n");
	}
}

static void icc_delete(
	icc *p
) {
	int i;
	icmAlloc *al = p->al;
	int del_al   = p->del_al;

	/* Free up the header */
	if (p->header != NULL)
		(p->header->del)(p->header);

	/* Free up the tag data objects */
	for (i = 0; i < p->count; i++) {
		if (p->data[i].objp != NULL) {
			if (--(p->data[i].objp->refcount) == 0)	/* decrement reference count */
				(p->data[i].objp->del)(p->data[i].objp);	/* Last reference */
	  	  	p->data[i].objp = NULL;
		}
	}

	/* Free tag table */
	al->free(al, p->data);

	/* This object */
	al->free(al, p);

	if (del_al)			/* We are responsible for deleting allocator */
		al->del(al);
}

/* ================================================== */
/* Lut Color normalizing and de-normalizing functions */

/* As a rule, I am representing Lut in memory as values in machine form as real */
/* numbers in the range 0.0 - 1.0. For many color spaces (ie. RGB, Gray, */
/* hsv, hls, cmyk and other device coords), this is entirely appropriate. */
/* For the PCS though, this is not correct, since (I assume!) the binary */
/* representation will be consistent with the encoding in Annex A, page 74 */
/* of the standard. Note that the standard doesn't specify the encoding of */
/* many color spaces (ie. Yuv, Yxy etc.), and is unclear about PCS. */

/* The following functions convert to and from the PCS spaces (XYZ or Lab) */
/* and the real Lut input/output values. These are used to convert real color */
/* space values into/out of the raw lut 0.0-1.0 representation. */

/* This is used internally to support the Lut->lookup() function, */
/* and can also be used by someone writing a Lut based profile to determine */
/* the colorspace range that the input lut indexes cover, as well */
/* as processing the output luts values into normalized form ready */
/* for writing. */

/* These functions should be accessed by calling icc.getNormFuncs() */ 

/* - - - - - - - - - - - - - - - - */
/* According to 6.5.5 and 6.5.6 of the spec., */
/* XYZ index values are represented the same as their table */
/* values, ie. as a u1.15 representation, with a value */
/* range from 0.0 ->  1.999969482422 */

/* Convert Lut index/value to XYZ coord. */ 
static void Lut_Lut2XYZ(double *out, double *in) {
	out[0] = in[0] * (1.0 + 32767.0/32768); /* X */
	out[1] = in[1] * (1.0 + 32767.0/32768); /* Y */
	out[2] = in[2] * (1.0 + 32767.0/32768); /* Z */
}

/* Convert XYZ coord to Lut index/value. */ 
static void Lut_XYZ2Lut(double *out, double *in) {
	out[0] = in[0] * (1.0/(1.0 + 32767.0/32768));
	out[1] = in[1] * (1.0/(1.0 + 32767.0/32768));
	out[2] = in[2] * (1.0/(1.0 + 32767.0/32768));
}

/* - - - - - - - - - - - - - - - - */
/* Convert Lab to Lut numbers */
/* Annex A specifies 8 and 16 bit encoding, but is */
/* silent on the Lut index normalization. */
/* Following Michael Bourgoin's 1998 SIGGRAPH course comment on this, */
/* we assume here that the index encoding is the same as the */
/* value encoding. */

/* Convert Lut16 table index/value to Lab */
static void Lut_Lut2Lab16(double *out, double *in) {
	out[0] = in[0] * (100.0 * 65535.0)/65280.0;			/* L */
	out[1] = (in[1] * (255.0 * 65535.0)/65280) - 128.0;	/* a */
	out[2] = (in[2] * (255.0 * 65535.0)/65280) - 128.0;	/* b */
}

/* Convert Lab to Lut16 table index/value */
static void Lut_Lab2Lut16(double *out, double *in) {
	out[0] = in[0] * 65280.0/(100.0 * 65535.0);				/* L */
	out[1] = (in[1] + 128.0) * 65280.0/(255.0 * 65535.0);	/* a */
	out[2] = (in[2] + 128.0) * 65280.0/(255.0 * 65535.0);	/* b */
}

/* Convert Lut8 table index/value to Lab */
static void Lut_Lut2Lab8(double *out, double *in) {
	out[0] = in[0] * 100.0;				/* L */
	out[1] = (in[1] * 255.0) - 128.0;	/* a */
	out[2] = (in[2] * 255.0) - 128.0;	/* b */
}

/* Convert Lab to Lut8 table index/value */
static void Lut_Lab2Lut8(double *out, double *in) {
	out[0] = in[0] * 1.0/100.0;				/* L */
	out[1] = (in[1] + 128.0) * 1.0/255.0;	/* a */
	out[2] = (in[2] + 128.0) * 1.0/255.0;	/* b */
}

/* - - - - - - - - - - - - - - - - */
/* Convert Luv to Lut number */
/* This data normalization is taken from Apples */
/* Colorsync specification. */
/* As per other color spaces, we assume that the index */
/* normalization is the same as the data normalization. */

/* Convert Lut table index/value to Luv */
static void Lut_Lut2Luv(double *out, double *in) {
	out[0] = in[0] * 100.0;						/* L */
	out[1] = (in[1] * 65535.0/256.0) - 128.0;	/* u */
	out[2] = (in[2] * 65535.0/256.0) - 128.0;	/* v */
}

/* Convert Luv to Lut table index/value */
static void Lut_Luv2Lut(double *out, double *in) {
	out[0] = in[0] * 1.0/100.0;					/* L */
	out[1] = (in[1] + 128.0) * 256.0/65535.0;	/* u */
	out[2] = (in[2] + 128.0) * 256.0/65535.0;	/* v */
}

/* - - - - - - - - - - - - - - - - */
/* Default N component conversions */
static void Lut_N(double *out, double *in, int nc) {
	for (--nc; nc >= 0; nc--)
		out[nc] = in[nc];
}

/* 1 */
static void Lut_1(double *out, double *in) {
	out[0] = in[0];
}

/* 2 */
static void Lut_2(double *out, double *in) {
	out[0] = in[0];
	out[1] = in[1];
}

/* 3 */
static void Lut_3(double *out, double *in) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/* 4 */
static void Lut_4(double *out, double *in) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
}

/* 5 */
static void Lut_5(double *out, double *in) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
	out[4] = in[4];
}

/* 6 */
static void Lut_6(double *out, double *in) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
	out[4] = in[4];
	out[5] = in[5];
}

/* 7 */
static void Lut_7(double *out, double *in) {
	Lut_N(out, in, 7);
}

/* 8 */
static void Lut_8(double *out, double *in) {
	Lut_N(out, in, 8);
}

/* 9 */
static void Lut_9(double *out, double *in) {
	Lut_N(out, in, 9);
}

/* 10 */
static void Lut_10(double *out, double *in) {
	Lut_N(out, in, 10);
}

/* 11 */
static void Lut_11(double *out, double *in) {
	Lut_N(out, in, 11);
}

/* 12 */
static void Lut_12(double *out, double *in) {
	Lut_N(out, in, 12);
}

/* 13 */
static void Lut_13(double *out, double *in) {
	Lut_N(out, in, 13);
}

/* 14 */
static void Lut_14(double *out, double *in) {
	Lut_N(out, in, 14);
}

/* 15 */
static void Lut_15(double *out, double *in) {
	Lut_N(out, in, 15);
}

/* Function table - match conversions to color spaces. */
/* Anything not here, we don't know how to convert. */
/* (ie. YCbCr) */
static struct {
	icColorSpaceSignature csig;
	void (*fromLut8)(double *out, double *in);		/* 8  bit from Lut index/entry */
	void (*fromLut16)(double *out, double *in);		/* 16 bit from Lut index/entry */
	void (*toLut8)(double *out, double *in);		/* 8  bit to Lut index/entry */
	void (*toLut16)(double *out, double *in);		/* 16 bit to Lut index/entry */
} colnormtable[] = {
	{icSigXYZData,     NULL,         Lut_Lut2XYZ,   NULL,         Lut_XYZ2Lut },
	{icSigLabData,     Lut_Lut2Lab8, Lut_Lut2Lab16, Lut_Lab2Lut8, Lut_Lab2Lut16 },
	{icSigLuvData,     Lut_Lut2Luv,  Lut_Lut2Luv,   Lut_Luv2Lut,  Lut_Luv2Lut },
	{icSigYxyData,     Lut_3,        Lut_3,         Lut_3,        Lut_3 },
	{icSigRgbData,     Lut_3,        Lut_3,         Lut_3,        Lut_3 },
	{icSigGrayData,    Lut_1,        Lut_1,         Lut_1,        Lut_1 },
	{icSigHsvData,     Lut_3,        Lut_3,         Lut_3,        Lut_3 },
	{icSigHlsData,     Lut_3,        Lut_3,         Lut_3,        Lut_3 },
	{icSigCmykData,    Lut_4,        Lut_4,         Lut_4,        Lut_4 },
	{icSigCmyData,     Lut_3,        Lut_3,         Lut_3,        Lut_3 },
	{icSigMch6Data,    Lut_6,        Lut_6,         Lut_6,        Lut_6 },
	{icSig2colorData,  Lut_2,        Lut_2,         Lut_2,        Lut_2 },
	{icSig3colorData,  Lut_3,        Lut_3,         Lut_3,        Lut_3 },
	{icSig4colorData,  Lut_4,        Lut_4,         Lut_4,        Lut_4 },
	{icSig5colorData,  Lut_5,        Lut_5,         Lut_5,        Lut_5 },
	{icSig6colorData,  Lut_6,        Lut_6,         Lut_6,        Lut_6 },
	{icSig7colorData,  Lut_7,        Lut_7,         Lut_7,        Lut_7 },
	{icSig8colorData,  Lut_8,        Lut_8,         Lut_8,        Lut_8 },
	{icSig9colorData,  Lut_9,        Lut_9,         Lut_9,        Lut_9 },
	{icSig10colorData, Lut_10,       Lut_10,        Lut_10,       Lut_10 },
	{icSig11colorData, Lut_11,       Lut_11,        Lut_11,       Lut_11 },
	{icSig12colorData, Lut_12,       Lut_12,        Lut_12,       Lut_12 },
	{icSig13colorData, Lut_13,       Lut_13,        Lut_13,       Lut_13 },
	{icSig14colorData, Lut_14,       Lut_14,        Lut_14,       Lut_14 },
	{icSig15colorData, Lut_15,       Lut_15,        Lut_15,       Lut_15 },
	{icMaxEnumData,    NULL,         NULL,          NULL,         NULL   }
};
	
/* Find appropriate conversion functions */
/* given the color space and Lut type */
/* Return 0 on success, 1 on match failure */
/* NOTE: doesn't set error value, message etc.! */
static int getNormFunc(
	icColorSpaceSignature csig, 
	icTagTypeSignature    tagSig,
	icmNormFlag           flag,
	void               (**nfunc)(double *out, double *in)
) {
	int i;
	for (i = 0; colnormtable[i].csig != icMaxEnumData; i++) {
		if (colnormtable[i].csig == csig)
			break;	/* Found it */
	}
	if (colnormtable[i].csig == icMaxEnumData) {	/* Oops */
		*nfunc   = NULL;
		return 1;
	}

	if (flag == icmFromLuti || flag == icmFromLutv) {	/* Table index/value decoding functions */
		if (tagSig == icSigLut8Type) {
			*nfunc = colnormtable[i].fromLut8;
			return 0;
		} else if (tagSig == icSigLut16Type) {
			*nfunc = colnormtable[i].fromLut16;
			return 0;
		} else {
			*nfunc   = NULL;
			return 1;
		}
	} else if (flag == icmToLuti || flag == icmToLutv) {	/* Table index/value encoding functions */
		if (tagSig == icSigLut8Type) {
			*nfunc = colnormtable[i].toLut8;
			return 0;
		} else if (tagSig == icSigLut16Type) {
			*nfunc = colnormtable[i].toLut16;
			return 0;
		} else {
			*nfunc   = NULL;
			return 1;
		}
	} else {
		*nfunc   = NULL;
		return 1;
	}
	return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */
/* Colorspace ranges - used instead of norm/denorm by Mono & Matrix */

/* Function table - match ranges to color spaces. */
/* Anything not here, we don't know how to convert. */
/* (ie. YCbCr) */
static struct {
	icColorSpaceSignature csig;
	int same;				/* Non zero if first entry applies to all channels */
	double min[15];			/* Minimum value for this colorspace */
	double max[15];			/* Maximum value for this colorspace */
} colorrangetable[] = {
	{icSigXYZData,     1, { 0.0 } , { 1.0 + 32767.0/32768.0 } },
	{icSigLabData,     0, { 0.0, -128.0, -128.0 },
	                      { 100.0 + 25500.0/65280.0, 127.0 + 255.0/256.0, 127.0 + 255.0/256.0 } }, 
	{icSigLuvData,     0, { 0.0, -128.0, -128.0 },
	                      { 100.0, 127.0 + 255.0/256.0, 127.0 + 255.0/256.0 } }, 
	{icSigYxyData,     1, { 0.0 }, { 1.0 } },
	{icSigRgbData,     1, { 0.0 }, { 1.0 } },
	{icSigGrayData,    1, { 0.0 }, { 1.0 } },
	{icSigHsvData,     1, { 0.0 }, { 1.0 } },
	{icSigHlsData,     1, { 0.0 }, { 1.0 } },
	{icSigCmykData,    1, { 0.0 }, { 1.0 } },
	{icSigCmyData,     1, { 0.0 }, { 1.0 } },
	{icSigMch6Data,    1, { 0.0 }, { 1.0 } },
	{icSig2colorData,  1, { 0.0 }, { 1.0 } },
	{icSig3colorData,  1, { 0.0 }, { 1.0 } },
	{icSig4colorData,  1, { 0.0 }, { 1.0 } },
	{icSig5colorData,  1, { 0.0 }, { 1.0 } },
	{icSig6colorData,  1, { 0.0 }, { 1.0 } },
	{icSig7colorData,  1, { 0.0 }, { 1.0 } },
	{icSig8colorData,  1, { 0.0 }, { 1.0 } },
	{icSig9colorData,  1, { 0.0 }, { 1.0 } },
	{icSig10colorData, 1, { 0.0 }, { 1.0 } },
	{icSig11colorData, 1, { 0.0 }, { 1.0 } },
	{icSig12colorData, 1, { 0.0 }, { 1.0 } },
	{icSig13colorData, 1, { 0.0 }, { 1.0 } },
	{icSig14colorData, 1, { 0.0 }, { 1.0 } },
	{icSig15colorData, 1, { 0.0 }, { 1.0 } },
	{icMaxEnumData     }
};
	
/* Find appropriate typical encoding ranges for a */
/* colorspace given the color space. */
/* Return 0 on success, 1 on match failure */
static int getRange(
	icColorSpaceSignature csig, 
	double *min, double *max
) {
	int i, e, ee;
	for (i = 0; colorrangetable[i].csig != icMaxEnumData; i++) {
		if (colorrangetable[i].csig == csig)
			break;	/* Found it */
	}
	if (colorrangetable[i].csig == icMaxEnumData) {	/* Oops */
		return 1;
	}
	ee = number_ColorSpaceSignature(csig);		/* Get number of components */

	if (colorrangetable[i].same) {		/* All channels are the same */
		for (e = 0; e < ee; e++) {
			if (min != NULL)
				min[e] = colorrangetable[i].min[0];
			if (max != NULL)
				max[e] = colorrangetable[i].max[0];
		}
	} else {
		for (e = 0; e < ee; e++) {
			if (min != NULL)
				min[e] = colorrangetable[i].min[e];
			if (max != NULL)
				max[e] = colorrangetable[i].max[e];
		}
	}
	return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */

/* 
	Matrix Inversion
	by Richard Carling
	from "Graphics Gems", Academic Press, 1990
*/

/* 
 *   adjoint( original_matrix, inverse_matrix )
 * 
 *     calculate the adjoint of a 3x3 matrix
 *
 *      Let  a   denote the minor determinant of matrix A obtained by
 *           ij
 *
 *      deleting the ith row and jth column from A.
 *
 *                    i+j
 *     Let  b   = (-1)    a
 *          ij            ji
 *
 *    The matrix B = (b  ) is the adjoint of A
 *                     ij
 */

#define det2x2(a, b, c, d) (a * d - b * c)

static void adjoint(
double out[3][3],
double in[3][3]
) {
    double a1, a2, a3, b1, b2, b3, c1, c2, c3;

    /* assign to individual variable names to aid  */
    /* selecting correct values  */

	a1 = in[0][0]; b1 = in[0][1]; c1 = in[0][2];
	a2 = in[1][0]; b2 = in[1][1]; c2 = in[1][2];
	a3 = in[2][0]; b3 = in[2][1]; c3 = in[2][2];

    /* row column labeling reversed since we transpose rows & columns */

    out[0][0]  =   det2x2(b2, b3, c2, c3);
    out[1][0]  = - det2x2(a2, a3, c2, c3);
    out[2][0]  =   det2x2(a2, a3, b2, b3);
        
    out[0][1]  = - det2x2(b1, b3, c1, c3);
    out[1][1]  =   det2x2(a1, a3, c1, c3);
    out[2][1]  = - det2x2(a1, a3, b1, b3);
        
    out[0][2]  =   det2x2(b1, b2, c1, c2);
    out[1][2]  = - det2x2(a1, a2, c1, c2);
    out[2][2]  =   det2x2(a1, a2, b1, b2);
}

/*
 * double = det3x3(  a1, a2, a3, b1, b2, b3, c1, c2, c3 )
 * 
 * calculate the determinant of a 3x3 matrix
 * in the form
 *
 *     | a1,  b1,  c1 |
 *     | a2,  b2,  c2 |
 *     | a3,  b3,  c3 |
 */

static double det3x3(double in[3][3]) {
    double a1, a2, a3, b1, b2, b3, c1, c2, c3;
    double ans;

	a1 = in[0][0]; b1 = in[0][1]; c1 = in[0][2];
	a2 = in[1][0]; b2 = in[1][1]; c2 = in[1][2];
	a3 = in[2][0]; b3 = in[2][1]; c3 = in[2][2];

    ans = a1 * det2x2(b2, b3, c2, c3)
        - b1 * det2x2(a2, a3, c2, c3)
        + c1 * det2x2(a2, a3, b2, b3);
    return ans;
}

#define SMALL_NUMBER	1.e-8
/* 
 *   inverse( original_matrix, inverse_matrix )
 * 
 *    calculate the inverse of a 4x4 matrix
 *
 *     -1     
 *     A  = ___1__ adjoint A
 *         det A
 */

/* Return non-zero if not invertable */
static int inverse3x3(
double out[3][3],
double in[3][3]
) {
    int i, j;
    double det;

    /*  calculate the 3x3 determinant
     *  if the determinant is zero, 
     *  then the inverse matrix is not unique.
     */
    det = det3x3(in);

    if ( fabs(det) < SMALL_NUMBER)
        return 1;

    /* calculate the adjoint matrix */
    adjoint(out, in);

    /* scale the adjoint matrix to get the inverse */
    for (i = 0; i < 3; i++)
        for(j = 0; j < 3; j++)
		    out[i][j] /= det;
	return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */

/* Multuply XYZ array by 3x3 transform matrix */
static void icmMulBy3x3(double out[3], double mat[3][3], double in[3]) {
	double tt[3];

	tt[0] = mat[0][0] * in[0] + mat[0][1] * in[1] + mat[0][2] * in[2];
	tt[1] = mat[1][0] * in[0] + mat[1][1] * in[1] + mat[1][2] * in[2];
	tt[2] = mat[2][0] * in[0] + mat[2][1] * in[1] + mat[2][2] * in[2];

	out[0] = tt[0];
	out[1] = tt[1];
	out[2] = tt[2];
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */
/* CIE XYZ to perceptual Lab */
void
icmXYZ2Lab(icmXYZNumber *w, double *out, double *in) {
	double X = in[0], Y = in[1], Z = in[2];
	double x,y,z,fx,fy,fz;
	double L;

	x = X/w->X;
	if (x > 0.008856451586)
		fx = pow(x,1.0/3.0);
	else
		fx = 7.787036979 * x + 16.0/116.0;

	y = Y/w->Y;
	if (y > 0.008856451586) {
		fy = pow(y,1.0/3.0);
		L = 116.0 * fy - 16.0;
	} else {
		fy = 7.787036979 * y + 16.0/116.0;
		L = 903.2963058 * y;
	}

	z = Z/w->Z;
	if (z > 0.008856451586)
		fz = pow(z,1.0/3.0);
	else
		fz = 7.787036979 * z + 16.0/116.0;

	out[0] = L;
	out[1] = 500.0 * (fx - fy);
	out[2] = 200.0 * (fy - fz);
}

/* Perceptual Lab to CIE XYZ */
void
icmLab2XYZ(icmXYZNumber *w, double *out, double *in) {
	double L = in[0], a = in[1], b = in[2];
	double x,y,z,fx,fy,fz;

	if (L > 8.0) {
		fy = (L + 16.0)/116.0;
		y = pow(fy,3.0);
	} else {
		y = L/903.2963058;
		fy = 7.787036979 * y + 16.0/116.0;
	}

	fx = a/500.0 + fy;
	if (fx > 24.0/116.0)
		x = pow(fx,3.0);
	else
		x = (fx - 16.0/116.0)/7.787036979;

	fz = fy - b/200.0;
	if (fz > 24.0/116.0)
		z = pow(fz,3.0);
	else
		z = (fz - 16.0/116.0)/7.787036979;

	out[0] = x * w->X;
	out[1] = y * w->Y;
	out[2] = z * w->Z;
}

/* available D50 Illuminant */
icmXYZNumber icmD50 = { 		/* Profile illuminant - D50 */
    0.9642, 1.0000, 0.8249
};

/* available D65 Illuminant */
icmXYZNumber icmD65 = { 		/* Profile illuminant - D65 */
	0.9505, 1.0000, 1.0890
};

/* Default black point */
icmXYZNumber icmBlack = {
    0.0000, 0.0000, 0.0000
};

/* Return the normal Delta E given two Lab values */
double icmLabDE(double *Lab1, double *Lab2) {
	double rv = 0.0, tt;

	tt = Lab1[0] - Lab2[0];
	rv += tt * tt;
	tt = Lab1[1] - Lab2[1];
	rv += tt * tt;
	tt = Lab1[2] - Lab2[2];
	rv += tt * tt;
	
	return sqrt(rv);
}

/* Return the normal Delta E squared, given two Lab values */
double icmLabDEsq(double *Lab1, double *Lab2) {
	double rv = 0.0, tt;

	tt = Lab1[0] - Lab2[0];
	rv += tt * tt;
	tt = Lab1[1] - Lab2[1];
	rv += tt * tt;
	tt = Lab1[2] - Lab2[2];
	rv += tt * tt;
	
	return rv;
}

/* Return the CIE94 Delta E color difference measure */
double icmCIE94(double Lab1[3], double Lab2[3]) {
	double desq, dhsq;
	double dlsq, dcsq;
	double c12;

	{
		double dl, da, db;
		dl = Lab1[0] - Lab2[0];
		dlsq = dl * dl;		/* dl squared */
		da = Lab1[1] - Lab2[1];
		db = Lab1[2] - Lab2[2];

		/* Compute normal Lab delta E squared */
		desq = dlsq + da * da + db * db;
	}

	{
		double c1, c2, dc;

		/* Compute chromanance for the two colors */
		c1 = sqrt(Lab1[1] * Lab1[1] + Lab1[2] * Lab1[2]);
		c2 = sqrt(Lab2[1] * Lab2[1] + Lab2[2] * Lab2[2]);
		c12 = sqrt(c1 * c2);	/* Symetric chromanance */

		/* delta chromanance squared */
		dc = c2 - c1;
		dcsq = dc * dc;
	}

	/* Compute delta hue squared */
	if ((dhsq = desq - dlsq - dcsq) < 0.0)
		dhsq = 0.0;

	{
		double sc, sh;

		/* Weighting factors for delta chromanance & delta hue */
		sc = 1.0 + 0.048 * c12;
		sh = 1.0 + 0.014 * c12;
	
		return sqrt(dlsq + dcsq/(sc * sc) + dhsq/(sh * sh));
	}
}

/* Return the CIE94 Delta E color difference measure, squared */
double icmCIE94sq(double Lab1[3], double Lab2[3]) {
	double desq, dhsq;
	double dlsq, dcsq;
	double c12;

	{
		double dl, da, db;
		dl = Lab1[0] - Lab2[0];
		dlsq = dl * dl;		/* dl squared */
		da = Lab1[1] - Lab2[1];
		db = Lab1[2] - Lab2[2];

		/* Compute normal Lab delta E squared */
		desq = dlsq + da * da + db * db;
	}

	{
		double c1, c2, dc;

		/* Compute chromanance for the two colors */
		c1 = sqrt(Lab1[1] * Lab1[1] + Lab1[2] * Lab1[2]);
		c2 = sqrt(Lab2[1] * Lab2[1] + Lab2[2] * Lab2[2]);
		c12 = sqrt(c1 * c2);	/* Symetric chromanance */

		/* delta chromanance squared */
		dc = c2 - c1;
		dcsq = dc * dc;
	}

	/* Compute delta hue squared */
	if ((dhsq = desq - dlsq - dcsq) < 0.0)
		dhsq = 0.0;

	{
		double sc, sh;

		/* Weighting factors for delta chromanance & delta hue */
		sc = 1.0 + 0.048 * c12;
		sh = 1.0 + 0.014 * c12;
	
		return dlsq + dcsq/(sc * sc) + dhsq/(sh * sh);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */

/* Multiply one 3x3 with another */
static void mul3x3(double dst[3][3], double src[3][3]) {
	int i, j, k;
	double td[3][3];		/* Temporary dest */

	for (j = 0; j < 3; j++) {
		for (i = 0; i < 3; i++) {
			double tt = 0.0;
			for (k = 0; k < 3; k++)
				tt += src[j][k] * dst[k][i];
			td[j][i] = tt;
		}
	}

	/* Copy result out */
	for (j = 0; j < 3; j++)
		for (i = 0; i < 3; i++)
			dst[j][i] = td[j][i];
}

/* Chrmatic Adaption transform utility */
/* Return a 3x3 chromatic adaption matrix */
void icmChromAdaptMatrix(
	int flags,
	icmXYZNumber d_wp,		/* Destination white point */
	icmXYZNumber s_wp,		/* Source white point */
	double mat[3][3]		/* Destination matrix */
) {
	double dst[3], src[3];			/* Source & destination white points */
	double vkmat[3][3];				/* Von Kries matrix */
	double bradford[3][3] = {		/* Bradford cone space matrix */
		{  0.8951,  0.2664, -0.1614 },
		{ -0.7502,  1.7135,  0.0367 },
		{  0.0389, -0.0685,  1.0296 }
	};
	double ibradford[3][3];			/* Inverse Bradford */

	/* Set initial matrix to unity */
	if (!(flags & ICM_CAM_MULMATRIX)) {
		mat[0][0] = mat[1][1] = mat[2][2] = 1.0;
		mat[0][1] = mat[0][2] = 0.0;
		mat[1][0] = mat[1][2] = 0.0;
		mat[2][0] = mat[2][1] = 0.0;
	}

	icmXYZ2Ary(src, s_wp);
	icmXYZ2Ary(dst, d_wp);

	if (flags & ICM_CAM_BRADFORD) {
		icmMulBy3x3(src, bradford, src);
		icmMulBy3x3(dst, bradford, dst);
	}

	/* Setup the Von Kries white point adaption matrix */
	vkmat[0][0] = dst[0]/src[0];
	vkmat[1][1] = dst[1]/src[1];
	vkmat[2][2] = dst[2]/src[2];
	vkmat[0][1] = vkmat[0][2] = 0.0;
	vkmat[1][0] = vkmat[1][2] = 0.0;
	vkmat[2][0] = vkmat[2][1] = 0.0;

	/* Transform to Bradford space if requested */
	if (flags & ICM_CAM_BRADFORD) {
		mul3x3(mat, bradford);
	}

	/* Apply chromatic adaption */
	mul3x3(mat, vkmat);

	/* Transform from Bradford space */
	if (flags & ICM_CAM_BRADFORD) {
		inverse3x3(ibradford, bradford);
		mul3x3(mat, ibradford);
	}

	/* We're done */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */

/* Return information about the native lut in/out colorspaces. */
/* Any pointer may be NULL if value is not to be returned */
static void
icmLutSpaces(
	struct _icmLuBase *p,			/* This */
	icColorSpaceSignature *ins,		/* Return Native input color space */
	int *inn,						/* Return number of input components */
	icColorSpaceSignature *outs,	/* Return Native output color space */
	int *outn						/* Return number of output components */
) {
	if (ins != NULL)
		*ins = p->inSpace;
	if (inn != NULL)
		*inn = (int)number_ColorSpaceSignature(p->inSpace);

	if (outs != NULL)
		*outs = p->outSpace;
	if (outn != NULL)
		*outn = (int)number_ColorSpaceSignature(p->outSpace);
}

/* Return information about the effective lookup in/out colorspaces, */
/* including allowance for PCS overide. */
/* Any pointer may be NULL if value is not to be returned */
static void
icmLuSpaces(
	struct _icmLuBase *p,			/* This */
	icColorSpaceSignature *ins,		/* Return effective input color space */
	int *inn,						/* Return number of input components */
	icColorSpaceSignature *outs,	/* Return effective output color space */
	int *outn,						/* Return number of output components */
	icmLuAlgType *alg,				/* Return type of lookup algorithm used */
    icRenderingIntent *intt,		/* Return the intent being implented */
    icmLookupFunc *fnc,				/* Return the profile function being implemented */
	icColorSpaceSignature *pcs		/* Return the profile effective PCS */
) {
	if (ins != NULL)
		*ins = p->e_inSpace;
	if (inn != NULL)
		*inn = (int)number_ColorSpaceSignature(p->e_inSpace);

	if (outs != NULL)
		*outs = p->e_outSpace;
	if (outn != NULL)
		*outn = (int)number_ColorSpaceSignature(p->e_outSpace);

	if (alg != NULL)
		*alg = p->ttype;

    if (intt != NULL)
		*intt = p->intent;

	if (fnc != NULL)
		*fnc = p->function;

	if (pcs != NULL)
		*pcs = p->e_pcs;
}

/* Return the media white and black points in XYZ space. */
/* Note that if not in the icc, the black point will be returned as 0, 0, 0 */
/* Any pointer may be NULL if value is not to be returned */
static void icmLuWh_bk_points(
struct _icmLuBase *p,
icmXYZNumber *wht,
icmXYZNumber *blk
) {
	if (wht != NULL)
		*wht = p->whitePoint;	/* Structure copy */

	if (blk != NULL)
		*blk = p->blackPoint;	/* Structure copy */
}

/* Get the effective (externally visible) ranges for the Monochrome or Matrix profile */
/* Arguments may be NULL */
static void
icmLu_get_ranges (
	struct _icmLuBase *p,
	double *inmin, double *inmax,		/* Return maximum range of inspace values */
	double *outmin, double *outmax		/* Return maximum range of outspace values */
) {
	/* Hmm. we have no way of handlin an error from getRange. */
	/* It shouldn't ever return one unless there is a mismatch between */
	/* getRange and Lu creation... */
	getRange(p->e_inSpace, inmin, inmax);
	getRange(p->e_outSpace, outmin, outmax);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */
/* Forward and Backward Monochrome type conversion */
/* Return 0 on success, 1 if clipping occured, 2 on other error */

/* Individual components of Fwd conversion: */

/* Actual device to linearised device */
static int
icmLuMonoFwd_curve (
icmLuMono *p,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	icc *icp = p->icp;
	int rv = 0;

	/* Translate from device to PCS scale */
	if ((rv |= p->grayCurve->lookup_fwd(p->grayCurve,&out[0],&in[0])) > 1) {
		sprintf(icp->err,"icc_lookup: Curve->lookup_fwd() failed");
		icp->errc = rv;
		return 2;
	}

	return rv;
}

/* Linearised device to relative PCS */
static int
icmLuMonoFwd_map (
icmLuMono *p,		/* This */
double *out,		/* Vector of output values (native space) */
double *in			/* Vector of input values (native space) */
) {
	int rv = 0;
	double Y = in[0];		/* In case out == in */

	out[0] = p->pcswht.X;
	out[1] = p->pcswht.Y;
	out[2] = p->pcswht.Z;
	if (p->pcs == icSigLabData)
		icmXYZ2Lab(&p->pcswht, out, out);	/* in Lab */

	/* Scale linearized device level to PCS white */
	out[0] *= Y;
	out[1] *= Y;
	out[2] *= Y;

	return rv;
}

/* relative PCS to absolute PCS (if required) */
static int
icmLuMonoFwd_abs (	/* Abs comes last in Fwd conversion */
icmLuMono *p,		/* This */
double *out,		/* Vector of output values in Effective PCS */
double *in			/* Vector of input values in Native PCS */
) {
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < 3; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* Do absolute conversion */
	if (p->intent == icAbsoluteColorimetric) {

		if (p->pcs == icSigLabData) 	/* Convert L to Y */
			icmLab2XYZ(&p->pcswht, out, out);
		
		/* Convert from Relative to Absolute colorometric */
		icmMulBy3x3(out, p->toAbs, out);
		
		if (p->e_pcs == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);

	} else {

		/* Convert from Native to Effective output space */
		if (p->pcs == icSigLabData && p->e_pcs == icSigXYZData)
			icmLab2XYZ(&p->pcswht, out, out);
		else if (p->pcs == icSigXYZData && p->e_pcs == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	}

	return rv;
}


/* Overall Fwd conversion routine */
static int
icmLuMonoFwd_lookup (
icmLuBase *pp,		/* This */
double *out,		/* Vector of output values */
double *in			/* Input value */
) {
	int rv = 0;
	icmLuMono *p = (icmLuMono *)pp;
	rv |= icmLuMonoFwd_curve(p, out, in);
	rv |= icmLuMonoFwd_map(p, out, out);
	rv |= icmLuMonoFwd_abs(p, out, out);
	return rv;
}


/* Individual components of Bwd conversion: */

/* Convert from relative PCS to absolute PCS (if required) */
static int
icmLuMonoBwd_abs (	/* Abs comes first in Bwd conversion */
icmLuMono *p,		/* This */
double *out,		/* Vector of output values in Native PCS */
double *in			/* Vector of input values in Effective PCS */
) {
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < 3; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* Force to monochrome locus in correct space */
	if (p->e_pcs == icSigLabData) {
		double wp[3];

		if (p->intent == icAbsoluteColorimetric) {
			wp[0] = p->whitePoint.X;
			wp[1] = p->whitePoint.Y;
			wp[2] = p->whitePoint.Z;
		} else {
			wp[0] = p->pcswht.X;
			wp[1] = p->pcswht.Y;
			wp[2] = p->pcswht.Z;
		}
		icmXYZ2Lab(&p->pcswht, wp, wp);	/* Convert to Lab white point */
		out[1] = out[0]/wp[0] * wp[1];
		out[2] = out[0]/wp[0] * wp[2];

	} else {
		if (p->intent == icAbsoluteColorimetric) {
			out[0] = out[1]/p->whitePoint.Y * p->whitePoint.X;
			out[2] = out[1]/p->whitePoint.Y * p->whitePoint.Z;
		} else {
			out[0] = out[1]/p->pcswht.Y * p->pcswht.X;
			out[2] = out[1]/p->pcswht.Y * p->pcswht.Z;
		}
	}

	/* Do absolute conversion to */
	if (p->intent == icAbsoluteColorimetric) {

		if (p->e_pcs == icSigLabData)
			icmLab2XYZ(&p->pcswht, out, out);

		icmMulBy3x3(out, p->fromAbs, out);

		/* Convert from Effective to Native input space */
		if (p->pcs == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);

	} else {

		/* Convert from Effective to Native input space */
		if (p->e_pcs == icSigLabData && p->pcs == icSigXYZData)
			icmLab2XYZ(&p->pcswht, out, out);
		else if (p->e_pcs == icSigXYZData && p->pcs == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	}

	return rv;
}

/* Map from relative PCS to linearised device */
static int
icmLuMonoBwd_map (
icmLuMono *p,		/* This */
double *out,		/* Output value */
double *in			/* Vector of input values (native space) */
) {
	int rv = 0;
	double pcsw[3];

	pcsw[0] = p->pcswht.X;
	pcsw[1] = p->pcswht.Y;
	pcsw[2] = p->pcswht.Z;
	if (p->pcs == icSigLabData)
		icmXYZ2Lab(&p->pcswht, pcsw, pcsw);	/* in Lab (should be 100.0!) */

	/* Divide linearized device level into PCS white luminence */
	if (p->pcs == icSigLabData)
		out[0] = in[0]/pcsw[0];
	else
		out[0] = in[1]/pcsw[1];

	return rv;
}

/* Map from linearised device to actual device */
static int
icmLuMonoBwd_curve (
icmLuMono *p,		/* This */
double *out,		/* Output value */
double *in			/* Input value */
) {
	icc *icp = p->icp;
	int rv = 0;

	/* Convert to device value through curve */
	if ((rv = p->grayCurve->lookup_bwd(p->grayCurve,&out[0],&in[0])) > 1) {
		sprintf(icp->err,"icc_lookup: Curve->lookup_bwd() failed");
		icp->errc = rv;
		return 2;
	}

	return rv;
}

/* Overall Bwd conversion routine */
static int
icmLuMonoBwd_lookup (
icmLuBase *pp,		/* This */
double *out,		/* Output value */
double *in			/* Vector of input values */
) {
	double temp[3];
	int rv = 0;
	icmLuMono *p = (icmLuMono *)pp;
	rv |= icmLuMonoBwd_abs(p, temp, in);
	rv |= icmLuMonoBwd_map(p, out, temp);
	rv |= icmLuMonoBwd_curve(p, out, out);
	return rv;
}

static void
icmLuMono_delete(
icmLuBase *p
) {
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

static icmLuBase *
new_icmLuMono(
	struct _icc          *icp,
    icColorSpaceSignature inSpace,		/* Native Input color space */
    icColorSpaceSignature outSpace,		/* Native Output color space */
    icColorSpaceSignature pcs,			/* Native PCS */
    icColorSpaceSignature e_inSpace,	/* Effective Input color space */
    icColorSpaceSignature e_outSpace,	/* Effective Output color space */
    icColorSpaceSignature e_pcs,		/* Effective PCS */
	icmXYZNumber          whitePoint,	/* Profile absolute white point */
	icmXYZNumber          blackPoint,	/* Profile absolute black point */
	icRenderingIntent     intent,		/* Rendering intent */
	icmLookupFunc         func,			/* Functionality requested */
	int dir								/* 0 = fwd, 1 = bwd */
) {
	icmLuMono *p;

	if ((p = (icmLuMono *) icp->al->calloc(icp->al,1,sizeof(icmLuMono))) == NULL)
		return NULL;
	p->icp      = icp;
	p->del      = icmLuMono_delete;
	p->lutspaces= icmLutSpaces;
	p->spaces   = icmLuSpaces;
	p->get_ranges = icmLu_get_ranges;
	p->wh_bk_points = icmLuWh_bk_points;
	p->fwd_lookup = icmLuMonoFwd_lookup;
	p->fwd_curve  = icmLuMonoFwd_curve;
	p->fwd_map    = icmLuMonoFwd_map;
	p->fwd_abs    = icmLuMonoFwd_abs;
	p->bwd_lookup = icmLuMonoBwd_lookup;
	p->bwd_abs    = icmLuMonoFwd_abs;
	p->bwd_map    = icmLuMonoFwd_map;
	p->bwd_curve  = icmLuMonoFwd_curve;
	if (dir) {
		p->ttype      = icmMonoBwdType;
		p->lookup     = icmLuMonoBwd_lookup;
	} else {
		p->ttype      = icmMonoFwdType;
		p->lookup     = icmLuMonoFwd_lookup;
	}

	/* See if the color spaces are appropriate for the mono type */
	if (number_ColorSpaceSignature(icp->header->colorSpace) != 1
	  || ( icp->header->pcs != icSigXYZData && icp->header->pcs != icSigLabData)) {
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Find the appropriate tags */
	if ((p->grayCurve = (icmCurve *)icp->read_tag(icp, icSigGrayTRCTag)) == NULL
         || p->grayCurve->ttype != icSigCurveType) {
		p->del((icmLuBase *)p);
		return NULL;
	}

	p->pcswht = icp->header->illuminant;
	p->whitePoint = whitePoint;
	p->blackPoint = blackPoint;
	p->intent   = intent;
	p->function = func;
	p->inSpace  = inSpace;
	p->outSpace = outSpace;
	p->pcs      = pcs;
	p->e_inSpace  = e_inSpace;
	p->e_outSpace = e_outSpace;
	p->e_pcs      = e_pcs;

	/* Create absolute <-> relative conversion matricies */
	icmChromAdaptMatrix(ICM_CAM_BRADFORD, whitePoint, icmD50, p->toAbs);
	icmChromAdaptMatrix(ICM_CAM_BRADFORD, icmD50, whitePoint,  p->fromAbs);

	return (icmLuBase *)p;
}

static icmLuBase *
new_icmLuMonoFwd(
	struct _icc          *icp,
    icColorSpaceSignature inSpace,		/* Native Input color space */
    icColorSpaceSignature outSpace,		/* Native Output color space */
    icColorSpaceSignature pcs,			/* Native PCS */
    icColorSpaceSignature e_inSpace,	/* Effective Input color space */
    icColorSpaceSignature e_outSpace,	/* Effective Output color space */
    icColorSpaceSignature e_pcs,		/* Effective PCS */
	icmXYZNumber          whitePoint,	/* Profile absolute white point */
	icmXYZNumber          blackPoint,	/* Profile absolute black point */
	icRenderingIntent     intent,		/* Rendering intent */
	icmLookupFunc         func			/* Functionality requested */
) {
	return new_icmLuMono(icp, inSpace, outSpace, pcs, e_inSpace, e_outSpace, e_pcs,
	                     whitePoint, blackPoint, intent, func, 0);
}


static icmLuBase *
new_icmLuMonoBwd(
	struct _icc          *icp,
    icColorSpaceSignature inSpace,		/* Native Input color space */
    icColorSpaceSignature outSpace,		/* Native Output color space */
    icColorSpaceSignature pcs,			/* Native PCS */
    icColorSpaceSignature e_inSpace,	/* Effective Input color space */
    icColorSpaceSignature e_outSpace,	/* Effective Output color space */
    icColorSpaceSignature e_pcs,		/* Effective PCS */
	icmXYZNumber          whitePoint,	/* Profile absolute white point */
	icmXYZNumber          blackPoint,	/* Profile absolute black point */
	icRenderingIntent     intent,		/* Rendering intent */
	icmLookupFunc         func			/* Functionality requested */
) {
	return new_icmLuMono(icp, inSpace, outSpace, pcs, e_inSpace, e_outSpace, e_pcs,
	                     whitePoint, blackPoint, intent, func, 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - */
/* Forward and Backward Matrix type conversion */
/* Return 0 on success, 1 if clipping occured, 2 on other error */

/* Individual components of Fwd conversion: */
static int
icmLuMatrixFwd_curve (
icmLuMatrix *p,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	icc *icp = p->icp;
	int rv = 0;

	/* Curve lookups */
	if ((rv |= p->redCurve->lookup_fwd(  p->redCurve,  &out[0],&in[0])) > 1
	 || (rv |= p->greenCurve->lookup_fwd(p->greenCurve,&out[1],&in[1])) > 1
	 || (rv |= p->blueCurve->lookup_fwd( p->blueCurve, &out[2],&in[2])) > 1) {
		sprintf(icp->err,"icc_lookup: Curve->lookup_fwd() failed");
		icp->errc = rv;
		return 2;
	}

	return rv;
}

static int
icmLuMatrixFwd_matrix (
icmLuMatrix *p,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int rv = 0;
	double tt[3];

	/* Matrix */
	tt[0] = p->mx[0][0] * in[0] + p->mx[0][1] * in[1] + p->mx[0][2] * in[2];
	tt[1] = p->mx[1][0] * in[0] + p->mx[1][1] * in[1] + p->mx[1][2] * in[2];
	tt[2] = p->mx[2][0] * in[0] + p->mx[2][1] * in[1] + p->mx[2][2] * in[2];

	out[0] = tt[0];
	out[1] = tt[1];
	out[2] = tt[2];

	return rv;
}

static int
icmLuMatrixFwd_abs (/* Abs comes last in Fwd conversion */
icmLuMatrix *p,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < 3; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* If required, convert from Relative to Absolute colorometric */
	if (p->intent == icAbsoluteColorimetric) {
		icmMulBy3x3(out, p->toAbs, out);
	}

	/* If e_pcs is Lab, then convert XYZ to Lab */
	if (p->e_pcs == icSigLabData)
		icmXYZ2Lab(&p->pcswht, out, out);

	return rv;
}


/* Overall Fwd conversion */
static int
icmLuMatrixFwd_lookup (
icmLuBase *pp,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int rv = 0;
	icmLuMatrix *p = (icmLuMatrix *)pp;
	rv |= icmLuMatrixFwd_curve(p, out, in);
	rv |= icmLuMatrixFwd_matrix(p, out, out);
	rv |= icmLuMatrixFwd_abs(p, out, out);
	return rv;
}

/* Individual components of Bwd conversion: */

static int
icmLuMatrixBwd_abs (/* Abs comes first in Bwd conversion */
icmLuMatrix *p,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < 3; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* If e_pcs is Lab, then convert Lab to XYZ */
	if (p->e_pcs == icSigLabData)
		icmLab2XYZ(&p->pcswht, out, out);

	/* If required, convert from Absolute to Relative colorometric */
	if (p->intent == icAbsoluteColorimetric) {
		icmMulBy3x3(out, p->fromAbs, out);
	}

	return rv;
}

static int
icmLuMatrixBwd_matrix (
icmLuMatrix *p,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int rv = 0;
	double tt[3];

	/* Matrix */
	tt[0] = p->bmx[0][0] * in[0] + p->bmx[0][1] * in[1] + p->bmx[0][2] * in[2];
	tt[1] = p->bmx[1][0] * in[0] + p->bmx[1][1] * in[1] + p->bmx[1][2] * in[2];
	tt[2] = p->bmx[2][0] * in[0] + p->bmx[2][1] * in[1] + p->bmx[2][2] * in[2];

	out[0] = tt[0];
	out[1] = tt[1];
	out[2] = tt[2];

	return rv;
}

static int
icmLuMatrixBwd_curve (
icmLuMatrix *p,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	icc *icp = p->icp;
	int rv = 0;

	/* Curves */
	if ((rv |= p->redCurve->lookup_bwd(p->redCurve,&out[0],&out[0])) > 1
	 ||	(rv |= p->greenCurve->lookup_bwd(p->greenCurve,&out[1],&out[1])) > 1
	 || (rv |= p->blueCurve->lookup_bwd(p->blueCurve,&out[2],&out[2])) > 1) {
		sprintf(icp->err,"icc_lookup: Curve->lookup_bwd() failed");
		icp->errc = rv;
		return 2;
	}
	return rv;
}

/* Overall Bwd conversion */
static int
icmLuMatrixBwd_lookup (
icmLuBase *pp,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int rv = 0;
	icmLuMatrix *p = (icmLuMatrix *)pp;
	rv |= icmLuMatrixBwd_abs(p, out, in);
	rv |= icmLuMatrixBwd_matrix(p, out, out);
	rv |= icmLuMatrixBwd_curve(p, out, out);
	return rv;
}

static void
icmLuMatrix_delete(
icmLuBase *p
) {
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

/* We setup valid fwd and bwd component conversions, */
/* but setup only the asked for overal conversion. */
static icmLuBase *
new_icmLuMatrix(
	struct _icc          *icp,
    icColorSpaceSignature inSpace,		/* Native Input color space */
    icColorSpaceSignature outSpace,		/* Native Output color space */
    icColorSpaceSignature pcs,			/* Native PCS */
    icColorSpaceSignature e_inSpace,	/* Effective Input color space */
    icColorSpaceSignature e_outSpace,	/* Effective Output color space */
    icColorSpaceSignature e_pcs,		/* Effective PCS */
	icmXYZNumber          whitePoint,	/* Profile absolute white point */
	icmXYZNumber          blackPoint,	/* Profile absolute black point */
	icRenderingIntent     intent,		/* Rendering intent */
	icmLookupFunc         func,			/* Functionality requested */
	int dir								/* 0 = fwd, 1 = bwd */
) {
	icmLuMatrix *p;

	if ((p = (icmLuMatrix *) icp->al->calloc(icp->al,1,sizeof(icmLuMatrix))) == NULL)
		return NULL;
	p->icp      = icp;
	p->del      = icmLuMatrix_delete;
	p->lutspaces= icmLutSpaces;
	p->spaces   = icmLuSpaces;
	p->get_ranges = icmLu_get_ranges;
	p->wh_bk_points = icmLuWh_bk_points;
	p->fwd_lookup = icmLuMatrixFwd_lookup;
	p->fwd_curve  = icmLuMatrixFwd_curve;
	p->fwd_matrix = icmLuMatrixFwd_matrix;
	p->fwd_abs    = icmLuMatrixFwd_abs;
	p->bwd_lookup = icmLuMatrixBwd_lookup;
	p->bwd_abs    = icmLuMatrixBwd_abs;
	p->bwd_matrix = icmLuMatrixBwd_matrix;
	p->bwd_curve  = icmLuMatrixBwd_curve;
	if (dir) {
		p->ttype      = icmMatrixBwdType;
		p->lookup     = icmLuMatrixBwd_lookup;
	} else {
		p->ttype      = icmMatrixFwdType;
		p->lookup     = icmLuMatrixFwd_lookup;
	}

	/* Note that we can use matrix type even if PCS is Lab, */
	/* by simply converting it. */

	/* Find the appropriate tags */
	if ((p->redCurve = (icmCurve *)icp->read_tag(icp, icSigRedTRCTag)) == NULL
     || p->redCurve->ttype != icSigCurveType
	 || (p->greenCurve = (icmCurve *)icp->read_tag(icp, icSigGreenTRCTag)) == NULL
     || p->greenCurve->ttype != icSigCurveType
	 || (p->blueCurve = (icmCurve *)icp->read_tag(icp, icSigBlueTRCTag)) == NULL
     || p->blueCurve->ttype != icSigCurveType
	 || (p->redColrnt = (icmXYZArray *)icp->read_tag(icp, icSigRedColorantTag)) == NULL
     || p->redColrnt->ttype != icSigXYZType || p->redColrnt->size < 1
	 || (p->greenColrnt = (icmXYZArray *)icp->read_tag(icp, icSigGreenColorantTag)) == NULL
     || p->greenColrnt->ttype != icSigXYZType || p->greenColrnt->size < 1
	 || (p->blueColrnt = (icmXYZArray *)icp->read_tag(icp, icSigBlueColorantTag)) == NULL
     || p->blueColrnt->ttype != icSigXYZType || p->blueColrnt->size < 1) {
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Setup the matrix */
	p->mx[0][0] = p->redColrnt->data[0].X;
	p->mx[0][1] = p->greenColrnt->data[0].X;
	p->mx[0][2] = p->blueColrnt->data[0].X;
	p->mx[1][1] = p->greenColrnt->data[0].Y;
	p->mx[1][0] = p->redColrnt->data[0].Y;
	p->mx[1][2] = p->blueColrnt->data[0].Y;
	p->mx[2][1] = p->greenColrnt->data[0].Z;
	p->mx[2][0] = p->redColrnt->data[0].Z;
	p->mx[2][2] = p->blueColrnt->data[0].Z;

	if (inverse3x3(p->bmx, p->mx) != 0) {	/* Compute inverse */
		sprintf(icp->err,"icc_new_iccLuMatrix: Matrix wasn't invertable");
		icp->errc = 2;
		p->del((icmLuBase *)p);
		return NULL;
	}

	p->pcswht = icp->header->illuminant;
	p->whitePoint = whitePoint;
	p->blackPoint = blackPoint;
	p->intent   = intent;
	p->function = func;
	p->inSpace  = inSpace;
	p->outSpace = outSpace;
	p->pcs      = pcs;
	p->e_inSpace  = e_inSpace;
	p->e_outSpace = e_outSpace;
	p->e_pcs      = e_pcs;

	/* Create absolute <-> relative conversion matricies */
	icmChromAdaptMatrix(ICM_CAM_BRADFORD, whitePoint, icmD50, p->toAbs);
	icmChromAdaptMatrix(ICM_CAM_BRADFORD, icmD50, whitePoint,  p->fromAbs);

	return (icmLuBase *)p;
}

static icmLuBase *
new_icmLuMatrixFwd(
	struct _icc          *icp,
    icColorSpaceSignature inSpace,		/* Native Input color space */
    icColorSpaceSignature outSpace,		/* Native Output color space */
    icColorSpaceSignature pcs,			/* Native PCS */
    icColorSpaceSignature e_inSpace,	/* Effective Input color space */
    icColorSpaceSignature e_outSpace,	/* Effective Output color space */
    icColorSpaceSignature e_pcs,		/* Effective PCS */
	icmXYZNumber          whitePoint,	/* Profile absolute white point */
	icmXYZNumber          blackPoint,	/* Profile absolute black point */
	icRenderingIntent     intent,		/* Rendering intent */
	icmLookupFunc         func			/* Functionality requested */
) {
	return new_icmLuMatrix(icp, inSpace, outSpace, pcs, e_inSpace, e_outSpace, e_pcs,
	                       whitePoint, blackPoint, intent, func, 0);
}


static icmLuBase *
new_icmLuMatrixBwd(
	struct _icc          *icp,
    icColorSpaceSignature inSpace,		/* Native Input color space */
    icColorSpaceSignature outSpace,		/* Native Output color space */
    icColorSpaceSignature pcs,			/* Native PCS */
    icColorSpaceSignature e_inSpace,	/* Effective Input color space */
    icColorSpaceSignature e_outSpace,	/* Effective Output color space */
    icColorSpaceSignature e_pcs,		/* Effective PCS */
	icmXYZNumber          whitePoint,	/* Profile absolute white point */
	icmXYZNumber          blackPoint,	/* Profile absolute black point */
	icRenderingIntent     intent,		/* Rendering intent */
	icmLookupFunc         func			/* Functionality requested */
) {
	return new_icmLuMatrix(icp, inSpace, outSpace, pcs, e_inSpace, e_outSpace, e_pcs,
	                       whitePoint, blackPoint, intent, func, 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - */
/* Forward and Backward Multi-Dimensional Interpolation type conversion */
/* Return 0 on success, 1 if clipping occured, 2 on other error */

/* Components of overall lookup, in order */
static int icmLuLut_in_abs(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < lut->inputChan; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* If Bwd Lut, take care of Absolute color space and effective input space */
	if ((p->function == icmBwd || p->function == icmGamut || p->function == icmPreview)
		&& p->intent == icAbsoluteColorimetric) {
	
		if (p->e_inSpace == icSigLabData)
			icmLab2XYZ(&p->pcswht, out, out);

		/* Convert from Absolute to Relative colorometric */
		icmMulBy3x3(out, p->fromAbs, out);
		
		if (p->inSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);

	} else {

		/* Convert from Effective to Native input space */
		if (p->e_inSpace == icSigLabData && p->inSpace == icSigXYZData)
			icmLab2XYZ(&p->pcswht, out, out);
		else if (p->e_inSpace == icSigXYZData && p->inSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	}

	return rv;
}

/* Possible matrix lookup */
static int icmLuLut_matrix(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	int rv = 0;

	if (p->usematrix)		
		rv |= lut->lookup_matrix(lut,out,in);
	else if (out != in) {
		int i;
		for (i = 0; i < lut->inputChan; i++)
			out[i] = in[i];
	}
	return rv;
}

/* Do input -> input' lookup */
static int icmLuLut_input(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	int rv = 0;

	p->in_normf(out, in); 						/* Normalize from input color space */
	rv |= lut->lookup_input(lut,out,out);		/* Lookup though input tables */
	p->in_denormf(out,out);						/* De-normalize to input color space */
	return rv;
}

/* Do input'->output' lookup */
static int icmLuLut_clut(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	double temp[MAX_CHAN];
	int rv = 0;

	p->in_normf(temp, in); 						/* Normalize from input color space */
	rv |= p->lookup_clut(lut,out,temp);			/* Lookup though clut tables */
	p->out_denormf(out,out);					/* De-normalize to output color space */
	return rv;
}

/* Do output'->output lookup */
static int icmLuLut_output(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	int rv = 0;

	p->out_normf(out,in);						/* Normalize from output color space */
	rv |= lut->lookup_output(lut,out,out);		/* Lookup though output tables */
	p->out_denormf(out, out);					/* De-normalize to output color space */
	return rv;
}

static int icmLuLut_out_abs(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < lut->inputChan; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* If Fwd Lut, take care of Absolute color space */
	/* and convert from native to effective out PCS */
	if ((p->function == icmFwd || p->function == icmPreview)
		&& p->intent == icAbsoluteColorimetric) {

		if (p->outSpace == icSigLabData)
			icmLab2XYZ(&p->pcswht, out, out);
		
		/* Convert from Relative to Absolute colorometric XYZ */
		icmMulBy3x3(out, p->toAbs, out);

		if (p->e_outSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	} else {

		/* Convert from Native to Effective output space */
		if (p->outSpace == icSigLabData && p->e_outSpace == icSigXYZData)
			icmLab2XYZ(&p->pcswht, out, out);
		else if (p->outSpace == icSigXYZData && p->e_outSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	}
	return rv;
}


/* Overall lookup */
static int
icmLuLut_lookup (
icmLuBase *pp,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int rv = 0;
	icmLuLut *p = (icmLuLut *)pp;
	icmLut *lut = p->lut;
	double temp[MAX_CHAN];

	rv |= p->in_abs(p,temp,in);						/* Possible absolute conversion */
	if (p->usematrix)		
		rv |= lut->lookup_matrix(lut,temp,temp);/* If XYZ, multiply by non-unity matrix */
	p->in_normf(temp, temp);					/* Normalize for input color space */
	rv |= lut->lookup_input(lut,temp,temp);		/* Lookup though input tables */
	rv |= p->lookup_clut(lut,out,temp);			/* Lookup though clut tables */
	rv |= lut->lookup_output(lut,out,out);		/* Lookup though output tables */
	p->out_denormf(out,out);					/* Normalize for output color space */
	rv |= p->out_abs(p,out,out);				/* Possible absolute conversion */

	return rv;
}

#ifdef NEVER	/* The following should be identical in effect to the above. */

/* Overall lookup */
static int
icmLuLut_lookup (
icmLuBase *pp,		/* This */
double *out,		/* Vector of output values */
double *in			/* Vector of input values */
) {
	int i, rv = 0;
	icmLuLut *p = (icmLuLut *)pp;
	icmLut *lut = p->lut;
	double temp[MAX_CHAN];

	rv |= p->in_abs(p,temp,in);
	rv |= p->matrix(p,temp,temp);
	rv |= p->input(p,temp,temp);
	rv |= p->clut(p,out,temp);
	rv |= p->output(p,out,out);
	rv |= p->out_abs(p,out,out);

	return rv;
}
#endif	/* NEVER */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Some components of inverse lookup, in order */
/* ~~ should these be in icmLut (like all the fwd transforms)? */

static int icmLuLut_inv_out_abs(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < lut->inputChan; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* If Fwd Lut, take care of Absolute color space */
	/* and convert from effective to native inverse output PCS */
	/* OutSpace must be PCS: XYZ or Lab */
	if ((p->function == icmFwd || p->function == icmPreview)
		&& p->intent == icAbsoluteColorimetric) {

		if (p->e_outSpace == icSigLabData)
			icmLab2XYZ(&p->pcswht, out, out);
	
		/* Convert from Absolute to Relative colorometric */
		icmMulBy3x3(out, p->fromAbs, out);
		
		if (p->outSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);

	} else {

		/* Convert from Effective to Native output space */
		if (p->e_outSpace == icSigLabData && p->outSpace == icSigXYZData)
			icmLab2XYZ(&p->pcswht, out, out);
		else if (p->e_outSpace == icSigXYZData && p->outSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	}
	return rv;
}

/* Do output->output' inverse lookup */
static int icmLuLut_inv_output(icmLuLut *p, double *out, double *in) {
	icc *icp = p->icp;
	icmLut *lut = p->lut;
	int rv = 0;

	if (lut->rot.inited == 0) {	
		rv = icmTable_setup_bwd(icp, &lut->rot, lut->outputEnt, lut->outputTable);
		if (rv != 0) {
			sprintf(icp->err,"icc_Lut_inv_input: Malloc failure in inverse lookup init.");
			return icp->errc = rv;
		}
	}

	p->out_normf(out,in);						/* Normalize from output color space */
	rv |= icmTable_lookup_bwd(&lut->rot, out, out); /* Reverse lookup though output tables */
	p->out_denormf(out, out);					/* De-normalize to output color space */
	return rv;
}

/* No output' -> input inverse lookup. */
/* This is non-trivial ! */

/* Do input' -> input inverse lookup */
static int icmLuLut_inv_input(icmLuLut *p, double *out, double *in) {
	icc *icp = p->icp;
	icmLut *lut = p->lut;
	int rv = 0;

	if (lut->rit.inited == 0) {	
		rv = icmTable_setup_bwd(icp, &lut->rit, lut->inputEnt, lut->inputTable);
		if (rv != 0) {
			sprintf(icp->err,"icc_Lut_inv_input: Malloc failure in inverse lookup init.");
			return icp->errc = rv;
		}
	}

	p->in_normf(out, in); 						/* Normalize from input color space */
	rv |= icmTable_lookup_bwd(&lut->rit, out, out); /* Reverse lookup though input tables */
	p->in_denormf(out,out);						/* De-normalize to input color space */
	return rv;
}

/* Possible inverse matrix lookup */
static int icmLuLut_inv_matrix(icmLuLut *p, double *out, double *in) {
	icc *icp = p->icp;
	icmLut *lut = p->lut;
	int rv = 0;

	if (p->usematrix) {
		double tt[3];
		if (p->imx_valid == 0) {
			if (inverse3x3(p->imx, lut->e) != 0) {	/* Compute inverse */
				sprintf(icp->err,"icc_new_iccLuMatrix: Matrix wasn't invertable");
				icp->errc = 2;
				return 2;
			}
			p->imx_valid = 1;
		}
		/* Matrix multiply */
		tt[0] = p->imx[0][0] * in[0] + p->imx[0][1] * in[1] + p->imx[0][2] * in[2];
		tt[1] = p->imx[1][0] * in[0] + p->imx[1][1] * in[1] + p->imx[1][2] * in[2];
		tt[2] = p->imx[2][0] * in[0] + p->imx[2][1] * in[1] + p->imx[2][2] * in[2];
		out[0] = tt[0], out[1] = tt[1], out[2] = tt[2];
	} else if (out != in) {
		int i;
		for (i = 0; i < lut->inputChan; i++)
			out[i] = in[i];
	}
	return rv;
}

static int icmLuLut_inv_in_abs(icmLuLut *p, double *out, double *in) {
	icmLut *lut = p->lut;
	int rv = 0;

	if (out != in) {
		int i;
		for (i = 0; i < lut->inputChan; i++)		/* Don't alter input values */
			out[i] = in[i];
	}

	/* If Bwd Lut, take care of Absolute color space, and */
	/* convert from native to effective input space */
	if ((p->function == icmBwd || p->function == icmGamut || p->function == icmPreview)
		&& p->intent == icAbsoluteColorimetric) {

		if (p->inSpace == icSigLabData)
			icmLab2XYZ(&p->pcswht, out, out);

		/* Convert from Relative to Absolute colorometric XYZ */
		icmMulBy3x3(out, p->toAbs, out);
		
		if (p->e_inSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	} else {

		/* Convert from Native to Effective input space */
		if (p->inSpace == icSigLabData && p->e_inSpace == icSigXYZData)
			icmLab2XYZ(&p->pcswht, out, out);
		else if (p->inSpace == icSigXYZData && p->e_inSpace == icSigLabData)
			icmXYZ2Lab(&p->pcswht, out, out);
	}
	return rv;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Return LuLut information */
static void icmLuLut_get_info(
	icmLuLut     *p,		/* this */
	icmLut       **lutp,	/* Pointer to icc lut type */
	icmXYZNumber *pcswhtp,	/* Pointer to profile PCS white point */
	icmXYZNumber *whitep,	/* Pointer to profile absolute white point */
	icmXYZNumber *blackp	/* Pointer to profile absolute black point */
) {
	if (lutp != NULL)
		*lutp = p->lut;
	if (pcswhtp != NULL)
		*pcswhtp = p->pcswht;
	if (whitep != NULL)
		*whitep = p->whitePoint;
	if (blackp != NULL)
		*blackp = p->blackPoint;
}

/* Get the native ranges for the LuLut */
static void
icmLuLut_get_lutranges (
	struct _icmLuLut *p,
	double *inmin, double *inmax,		/* Return maximum range of inspace values */
	double *outmin, double *outmax		/* Return maximum range of outspace values */
) {
	int i;

	for (i = 0; i < p->lut->inputChan; i++) {
		inmin[i] = 0.0;	/* Normalized range of input space values */
		inmax[i] = 1.0;
	}
	p->in_denormf(inmin,inmin);	/* Convert to real colorspace range */
	p->in_denormf(inmax,inmax);

	/* Make sure min and max are so. */
	for (i = 0; i < p->lut->inputChan; i++) {
		if (inmin[i] > inmax[i]) {
			double tt;
			tt = inmin[i];
			inmin[i] = inmax[i];
			inmax[i] = tt;
		}
	}

	for (i = 0; i < p->lut->outputChan; i++) {
		outmin[i] = 0.0;	/* Normalized range of output space values */
		outmax[i] = 1.0;
	}
	p->out_denormf(outmin,outmin);	/* Convert to real colorspace range */
	p->out_denormf(outmax,outmax);

	/* Make sure min and max are so. */
	for (i = 0; i < p->lut->outputChan; i++) {
		if (outmin[i] > outmax[i]) {
			double tt;
			tt = outmin[i];
			outmin[i] = outmax[i];
			outmax[i] = tt;
		}
	}
}

/* Get the effective (externally visible) ranges for the LuLut */
/* Arguments may be NULL */
static void
icmLuLut_get_ranges (
	struct _icmLuBase *pp,
	double *inmin, double *inmax,		/* Return maximum range of inspace values */
	double *outmin, double *outmax		/* Return maximum range of outspace values */
) {
	icmLuLut *p = (icmLuLut *)pp;
	double tinmin[MAX_CHAN], tinmax[MAX_CHAN], toutmin[MAX_CHAN], toutmax[MAX_CHAN];
	int i;

	/* fudge NULL arguments so that they don't bomb */
	if (inmin == NULL)
		inmin = tinmin;
	if (inmax == NULL)
		inmax = tinmax;
	if (outmin == NULL)
		outmin = toutmin;
	if (outmax == NULL)
		outmax = toutmax;

	for (i = 0; i < p->lut->inputChan; i++) {
		inmin[i] = 0.0;	/* Normalized range of input space values */
		inmax[i] = 1.0;
	}
	p->e_in_denormf(inmin,inmin);	/* Convert to real colorspace range */
	p->e_in_denormf(inmax,inmax);

	/* Make sure min and max are so. */
	for (i = 0; i < p->lut->inputChan; i++) {
		if (inmin[i] > inmax[i]) {
			double tt;
			tt = inmin[i];
			inmin[i] = inmax[i];
			inmax[i] = tt;
		}
	}

	for (i = 0; i < p->lut->outputChan; i++) {
		outmin[i] = 0.0;	/* Normalized range of output space values */
		outmax[i] = 1.0;
	}
	p->e_out_denormf(outmin,outmin);	/* Convert to real colorspace range */
	p->e_out_denormf(outmax,outmax);

	/* Make sure min and max are so. */
	for (i = 0; i < p->lut->outputChan; i++) {
		if (outmin[i] > outmax[i]) {
			double tt;
			tt = outmin[i];
			outmin[i] = outmax[i];
			outmax[i] = tt;
		}
	}
}

/* Return the underlying Lut matrix */
static void
icmLuLut_get_matrix (
	struct _icmLuLut *p,
	double m[3][3]
) {
	int i, j;
	icmLut *lut = p->lut;

	if (p->usematrix) {
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				m[i][j] = lut->e[i][j];	/* Copy from Lut */

	} else {							/* return unity matrix */
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				if (i == j)
					m[i][j] = 1.0;
				else
					m[i][j] = 0.0;
			}
		}
	}
}


static void
icmLuLut_delete(
icmLuBase *p
) {
	icc *icp = p->icp;

	icp->al->free(icp->al, p);
}

static icmLuBase *
new_icmLuLut(
	icc                   *icp,
	icTagSignature        ttag,			/* Target Lut tag */
    icColorSpaceSignature inSpace,		/* Native Input color space */
    icColorSpaceSignature outSpace,		/* Native Output color space */
    icColorSpaceSignature pcs,			/* Native PCS */
    icColorSpaceSignature e_inSpace,	/* Effective Input color space */
    icColorSpaceSignature e_outSpace,	/* Effective Output color space */
    icColorSpaceSignature e_pcs,		/* Effective PCS */
	icmXYZNumber          whitePoint,	/* Profile absolute white point */
	icmXYZNumber          blackPoint,	/* Profile absolute black point */
	icRenderingIntent     intent,		/* Rendering intent */
	icmLookupFunc         func			/* Functionality requested */
) {
	icmLuLut *p;

	if ((p = (icmLuLut *) icp->al->calloc(icp->al,1,sizeof(icmLuLut))) == NULL)
		return NULL;
	p->ttype    = icmLutType;
	p->icp      = icp;
	p->del      = icmLuLut_delete;
	p->lutspaces= icmLutSpaces;
	p->spaces   = icmLuSpaces;
	p->wh_bk_points = icmLuWh_bk_points;

	p->lookup   = icmLuLut_lookup;
	p->in_abs   = icmLuLut_in_abs;
	p->matrix   = icmLuLut_matrix;
	p->input    = icmLuLut_input;
	p->clut     = icmLuLut_clut;
	p->output   = icmLuLut_output;
	p->out_abs  = icmLuLut_out_abs;

	p->inv_in_abs   = icmLuLut_inv_in_abs;
	p->inv_matrix   = icmLuLut_inv_matrix;
	p->inv_input    = icmLuLut_inv_input;
	p->inv_output   = icmLuLut_inv_output;
	p->inv_out_abs  = icmLuLut_inv_out_abs;

	p->pcswht   = icp->header->illuminant;
	p->whitePoint = whitePoint;
	p->blackPoint = blackPoint;
	p->intent   = intent;
	p->function = func;
	p->inSpace  = inSpace;
	p->outSpace = outSpace;
	p->pcs      = pcs;
	p->e_inSpace  = e_inSpace;
	p->e_outSpace = e_outSpace;
	p->e_pcs      = e_pcs;
	p->get_info = icmLuLut_get_info;
	p->get_lutranges = icmLuLut_get_lutranges;
	p->get_ranges = icmLuLut_get_ranges;
	p->get_matrix = icmLuLut_get_matrix;

	/* Create absolute <-> relative conversion matricies */
	icmChromAdaptMatrix(ICM_CAM_BRADFORD, whitePoint, icmD50, p->toAbs);
	icmChromAdaptMatrix(ICM_CAM_BRADFORD, icmD50, whitePoint,  p->fromAbs);

	/* Get the Lut tag, & check that it is expected type */
	if ((p->lut = (icmLut *)icp->read_tag(icp, ttag)) == NULL
	 || (p->lut->ttype != icSigLut8Type && p->lut->ttype != icSigLut16Type)) {
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Check if matrix should be used */
	if (inSpace == icSigXYZData && p->lut->nu_matrix(p->lut))
		p->usematrix = 1;
	else
		p->usematrix = 0;

	/* Lookup input color space to normalized index function */
	if (getNormFunc(inSpace, p->lut->ttype, icmToLuti, &p->in_normf)) {
		sprintf(icp->err,"icc_get_luobj: Unknown colorspace");
		icp->errc = 1;
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Lookup normalized index to input color space function */
	if (getNormFunc(inSpace, p->lut->ttype, icmFromLuti, &p->in_denormf)) {
		sprintf(icp->err,"icc_get_luobj: Unknown colorspace");
		icp->errc = 1;
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Lookup output color space to normalized Lut entry value function */
	if (getNormFunc(outSpace, p->lut->ttype, icmToLutv, &p->out_normf)) {
		sprintf(icp->err,"icc_get_luobj: Unknown colorspace");
		icp->errc = 1;
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Lookup normalized Lut entry value to output color space function */
	if (getNormFunc(outSpace, p->lut->ttype, icmFromLutv, &p->out_denormf)) {
		sprintf(icp->err,"icc_get_luobj: Unknown colorspace");
		icp->errc = 1;
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Lookup normalized index to effective input color space function */
	if (getNormFunc(e_inSpace, p->lut->ttype, icmFromLuti, &p->e_in_denormf)) {
		sprintf(icp->err,"icc_get_luobj: Unknown effective colorspace");
		icp->errc = 1;
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Lookup normalized Lut entry value to effective output color space function */
	if (getNormFunc(e_outSpace, p->lut->ttype, icmFromLutv, &p->e_out_denormf)) {
		sprintf(icp->err,"icc_get_luobj: Unknown effective colorspace");
		icp->errc = 1;
		p->del((icmLuBase *)p);
		return NULL;
	}

	/* Determine appropriate clut lookup algorithm */
	{
		int use_sx;				/* -1 = undecided, 0 = N-linear, 1 = Simplex lookup */
		icColorSpaceSignature ins, outs;	/* In and out Lut color spaces */
		int inn, outn;		/* in and out number of Lut components */

		p->lutspaces((icmLuBase *)p, &ins, &inn, &outs, &outn);

		/* Determine if the input space is "Device" like, */
		/* ie. luminance will be expected to vary most strongly */
		/* with the diagonal change in input coordinates. */
		switch(ins) {

			/* Luminence is carried by the sum of all the output channels, */
			/* so output luminence will dominantly be in diagonal direction. */
			case icSigRgbData:
			case icSigGrayData:
			case icSigCmykData:
			case icSigCmyData:
			case icSigMch6Data:
				use_sx = 1;		/* Simplex interpolation is appropriate */
				break;

			/* A single channel carries the luminence information */
			case icSigLabData:
			case icSigLuvData:
			case icSigYCbCrData:
			case icSigYxyData:
			case icSigXYZData:
			case icSigHlsData:
			case icSigHsvData:
				use_sx = 0;		/* N-linear interpolation is appropriate */
				break;
			default:
				use_sx = -1;		/* undecided */
			    	break;
		}

		/* If we couldn't figure it out from the input space, */
		/* check output luminance variation with a diagonal input */
		/* change. */
		if (use_sx == -1) {
			int lc;		/* Luminance channel */

			/* Determine where the luminence is carried in the output */
			switch(outs) {

				/* Luminence is carried by the sum of all the output channels */
				case icSigRgbData:
				case icSigGrayData:
				case icSigCmykData:
				case icSigCmyData:
				case icSigMch6Data:
					lc = -1;		/* Average all channels */
					break;

				/* A single channel carries the luminence information */
				case icSigLabData:
				case icSigLuvData:
				case icSigYCbCrData:
				case icSigYxyData:
					lc = 0;
					break;

				case icSigXYZData:
				case icSigHlsData:
					lc = 1;
					break;

				case icSigHsvData:
					lc = 2;
					break;
				
				/* default means give up and use N-linear type lookup */
				default:
					lc = -2;
					break;
			}

			/* If we know how luminance is represented in output space */
			if (lc != -2) {
				double tout1[MAX_CHAN];		/* Test output values */
				double tout2[MAX_CHAN];
				double tt, diag;
				int n;

				/* Determine input space location of min and max of */
				/* given output channel (chan = -1 means average of all) */
				p->lut->min_max(p->lut, tout1, tout2, lc);
				
				/* Convert to vector and then calculate normalized */
				/* dot product with diagonal vector (1,1,1...) */
				for (tt = 0.0, n = 0; n < inn; n++) {
					tout1[n] = tout2[n] - tout1[n];
					tt += tout1[n] * tout1[n];
				}
				if (tt > 0.0)
					tt = sqrt(tt);			/* normalizing factor for maximum delta */
				else
					tt = 1.0;				/* Hmm. */
				tt *= sqrt((double)inn);	/* Normalizing factor for diagonal vector */
				for (diag = 0.0, n = 0; n < outn; n++)
					diag += tout1[n] / tt;
				diag = fabs(diag);

				/* I'm not really convinced that this is a reliable */
				/* indicator of whether simplex interpolation should be used ... */
				/* It does seem to do the right thing with YCC space though. */
				if (diag > 0.8)	/* Diagonal is dominant ? */
					use_sx = 1;

				/* If we couldn't figure it out, use N-linear interpolation */
				if (use_sx == -1)
					use_sx = 0;
			}
		}

		if (use_sx) {
			p->lookup_clut = p->lut->lookup_clut_sx;
		} else
			p->lookup_clut = p->lut->lookup_clut_nl;
	}
	return (icmLuBase *)p;
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

/* Return an appropriate lookup object */
/* Return NULL on error, and detailed error in icc */
static icmLuBase* icc_get_luobj (
	icc *p,						/* ICC */
	icmLookupFunc func,			/* Conversion functionality */
	icRenderingIntent intent,	/* Rendering intent, including icmAbsoluteColorimetricXYZ */
	icColorSpaceSignature pcsor,/* PCS overide (0 = def) */
	icmLookupOrder order		/* Conversion representation search Order */
) {
	int rv;
	icmLuBase *luobj = NULL;	/* Lookup object to return */
	icmXYZNumber whitePoint, blackPoint;
	icColorSpaceSignature pcs, e_pcs;	/* PCS and effective PCS */
	
	/* Check that the profile is legal, since we depend on it */
	if ((rv = check_icc_legal(p)) != 0)
		return NULL;
	
	/* Figure out the native and effective PCS */
	e_pcs = pcs = p->header->pcs;
	if (pcsor != icmSigDefaultData)
		e_pcs = pcsor;			/* Overide */

	/* Get White and Black points from the profile */
	{
		icmXYZArray *whitePointTag, *blackPointTag;

		if ((whitePointTag = (icmXYZArray *)p->read_tag(p, icSigMediaWhitePointTag)) == NULL
         || whitePointTag->ttype != icSigXYZType || whitePointTag->size < 1) {
			if (intent == icAbsoluteColorimetric) {
				sprintf(p->err,"icc_lookup: Profile is missing Media White Point Tag");
				p->errc = 1;
				return NULL;
			}
			whitePoint = icmD50;						/* safe value */
		} else
			whitePoint = whitePointTag->data[0];	/* Copy structure */

		if ((blackPointTag = (icmXYZArray *)p->read_tag(p, icSigMediaBlackPointTag)) == NULL
         || blackPointTag->ttype != icSigXYZType || blackPointTag->size < 1) {
			blackPoint = icmBlack;						/* default */
		} else 
			blackPoint = blackPointTag->data[0];	/* Copy structure */
	}

	/* How we expect to execute the request depends firstly on the type of profile */
	switch (p->header->deviceClass) {
    	case icSigInputClass:
    	case icSigDisplayClass:
			/* Look for AToB0 based profile + optional BToA0 reverse */
			/* or three component matrix profile (reversable) */
			/* or momochrome table profile (reversable) */ 
			/* No intent */
			/* Device <-> PCS */
			/* Determine the algorithm and set its parameters */

			if (intent != icmDefaultIntent
			 && intent != icAbsoluteColorimetric) {
				sprintf(p->err,"icc_get_luobj: Intent is inappropriate for Input/Display profile");
				p->errc = 1;
				return NULL;
			}

			switch (func) {
		    	case icmFwd:	/* Device to PCS */
					if (order != icmLuOrdRev) {
						/* Try Lut type lookup first */
						if ((luobj = new_icmLuLut(p, icSigAToB0Tag,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
					} else {
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;

						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* Try Lut type lookup last */
						if ((luobj = new_icmLuLut(p, icSigAToB0Tag,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
					}
					break;

		    	case icmBwd:	/* PCS to Device */
					if (order != icmLuOrdRev) {
						/* Try Lut type lookup first */
						if ((luobj = new_icmLuLut(p, icSigBToA0Tag,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
					} else {
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* Try Lut type lookup last */
						if ((luobj = new_icmLuLut(p, icSigBToA0Tag,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
					}
					break;

				default:
					sprintf(p->err,"icc_get_luobj: Inaproptiate function requested");
					p->errc = 1;
					return NULL;
				}
			break;
			
    	case icSigOutputClass:
			/* Expect BToA Lut and optional AToB Lut, All intents, expect gamut */
			/* or momochrome table profile (reversable) */ 
			/* Device <-> PCS */
			/* Gamut Lut - no intent */
			/* Optional preview links PCS <-> PCS */
			
			/* Determine the algorithm and set its parameters */
			switch (func) {
				icTagSignature ttag;
				
		    	case icmFwd:	/* Device to PCS */

					if (intent == icmDefaultIntent)
						intent = icRelativeColorimetric;	/* Make this the default */

					switch (intent) {
		    			case icRelativeColorimetric:
		    			case icAbsoluteColorimetric:
								ttag = icSigAToB1Tag;
							break;
		    			case icPerceptual:
								ttag = icSigAToB0Tag;
							break;
		    			case icSaturation:
								ttag = icSigAToB2Tag;
							break;
						default:
							sprintf(p->err,"icc_get_luobj: Unknown intent");
							p->errc = 1;
							return NULL;
					}

					if (order != icmLuOrdRev) {
						/* Try Lut type lookup first */
						if ((luobj = new_icmLuLut(p, ttag,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
					} else {
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;

						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixFwd(p,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* Try Lut type lookup last */
						if ((luobj = new_icmLuLut(p, ttag,
						     p->header->colorSpace, pcs, pcs,
						     p->header->colorSpace, e_pcs, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
					}
					break;

		    	case icmBwd:	/* PCS to Device */

					if (intent == icmDefaultIntent)
						intent = icRelativeColorimetric;	/* Make this the default */

					switch (intent) {
		    			case icRelativeColorimetric:
		    			case icAbsoluteColorimetric:
								ttag = icSigBToA1Tag;
							break;
		    			case icPerceptual:
								ttag = icSigBToA0Tag;
							break;
		    			case icSaturation:
								ttag = icSigBToA2Tag;
							break;
						default:
							sprintf(p->err,"icc_get_luobj: Unknown intent");
							p->errc = 1;
							return NULL;
					}

					if (order != icmLuOrdRev) {
						/* Try Lut type lookup first */
						if ((luobj = new_icmLuLut(p, ttag,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
					} else {
						/* See if it could be a monochrome lookup */
						if ((luobj = new_icmLuMonoBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;

						/* See if it could be a matrix lookup */
						if ((luobj = new_icmLuMatrixBwd(p,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
	
						/* Try Lut type lookup last */
						if ((luobj = new_icmLuLut(p, ttag,
						     pcs, p->header->colorSpace, pcs,
						     e_pcs, p->header->colorSpace, e_pcs,
						     whitePoint, blackPoint, intent, func)) != NULL)
							break;
					}
					break;

		    	case icmGamut:	/* PCS to 1D */

					if (intent != icmDefaultIntent) {
						sprintf(p->err,"icc_get_luobj: Intent is inappropriate for type of function");
						p->errc = 1;
						return NULL;
					}

					/* If the target tag exists, and it is a Lut */
					luobj = new_icmLuLut(p, icSigGamutTag,
					     pcs, icSigGrayData, pcs,
					     e_pcs, icSigGrayData, e_pcs,
					     whitePoint, blackPoint, intent, func);
					break;

		    	case icmPreview:	/* PCS to PCS */

					switch (intent)  {
		    			case icRelativeColorimetric:
								ttag = icSigPreview1Tag;
							break;
		    			case icPerceptual:
								ttag = icSigPreview0Tag;
							break;
		    			case icSaturation:
								ttag = icSigPreview2Tag;
							break;
		    			case icAbsoluteColorimetric:
							sprintf(p->err,"icc_get_luobj: Intent is inappropriate for type of function");
							p->errc = 1;
							return NULL;
						default:
							sprintf(p->err,"icc_get_luobj: Unknown intent");
							p->errc = 1;
							return NULL;
					}

					/* If the target tag exists, and it is a Lut */
					luobj = new_icmLuLut(p, ttag,
					     pcs, pcs, pcs,
					     e_pcs, e_pcs, e_pcs,
					     whitePoint, blackPoint, intent, func);
					break;

				default:
					sprintf(p->err,"icc_get_luobj: Inaproptiate function requested");
					p->errc = 1;
					return NULL;
			}
			break;

    	case icSigLinkClass:
			/* Expect AToB0 Lut and optional BToA0 Lut, One intent in header */
			/* Device <-> Device */

			if (intent != p->header->renderingIntent
			 && intent != icmDefaultIntent) {
				sprintf(p->err,"icc_get_luobj: Intent is inappropriate for Link profile");
				p->errc = 1;
				return NULL;
			}
			intent = p->header->renderingIntent;

			/* Determine the algorithm and set its parameters */
			switch (func) {
		    	case icmFwd:	/* Device to PCS (== Device) */

					luobj = new_icmLuLut(p, icSigAToB0Tag,
					     p->header->colorSpace, pcs, pcs,
					     p->header->colorSpace, pcs, pcs,
					     whitePoint, blackPoint, intent, func);
					break;

		    	case icmBwd:	/* PCS (== Device) to Device */

					luobj = new_icmLuLut(p, icSigBToA0Tag,
					     pcs, p->header->colorSpace, pcs,
					     pcs, p->header->colorSpace, pcs,
					     whitePoint, blackPoint, intent, func);
					break;

				default:
					sprintf(p->err,"icc_get_luobj: Inaproptiate function requested");
					p->errc = 1;
					return NULL;
			}
			break;

    	case icSigAbstractClass:
			/* Expect AToB0 Lut and BToA Lut, no intents */
			/* PCS <-> PCS */
			/* Determine the algorithm and set its parameters */

			if (intent != icmDefaultIntent) {
				sprintf(p->err,"icc_get_luobj: Intent is inappropriate for Abstract profile");
				p->errc = 1;
				return NULL;
			}

			switch (func) {
		    	case icmFwd:	/* PCS (== Device) to PCS */

					luobj = new_icmLuLut(p, icSigAToB0Tag,
					     p->header->colorSpace, pcs, pcs,
					     e_pcs, e_pcs, e_pcs,
					     whitePoint, blackPoint, intent, func);
					break;

		    	case icmBwd:	/* PCS to PCS (== Device) */

					luobj = new_icmLuLut(p, icSigBToA0Tag,
					     pcs, p->header->colorSpace, pcs,
					     e_pcs, e_pcs, e_pcs,
					     whitePoint, blackPoint, intent, func);
					break;

				default:
					sprintf(p->err,"icc_get_luobj: Inaproptiate function requested");
					p->errc = 1;
					return NULL;
			}
			break;

    	case icSigColorSpaceClass:
			/* Expect AToB0 Lut and BToA0 Lut, no intents, */
			/* Device <-> PCS */

			if (intent != icmDefaultIntent) {
				sprintf(p->err,"icc_get_luobj: Intent is inappropriate for Colorspace profile");
				p->errc = 1;
				return NULL;
			}

			/* Determine the algorithm and set its parameters */
			switch (func) {
		    	case icmFwd:	/* Device to PCS */

					luobj = new_icmLuLut(p, icSigAToB0Tag,
					     p->header->colorSpace, pcs, pcs,
					     p->header->colorSpace, e_pcs, e_pcs,
					     whitePoint, blackPoint, intent, func);
					break;

		    	case icmBwd:	/* PCS to Device */

					luobj = new_icmLuLut(p, icSigBToA0Tag,
					     pcs, p->header->colorSpace, pcs,
					     e_pcs, p->header->colorSpace, e_pcs,
					     whitePoint, blackPoint, intent, func);
					break;

				default:
					sprintf(p->err,"icc_get_luobj: Inaproptiate function requested");
					p->errc = 1;
					return NULL;
			}
			break;

    	case icSigNamedColorClass:
			/* Expect Name -> Device, Optional PCS */
			/* and a reverse lookup would be useful */
			/* (ie. PCS or Device coords to closest named color) */
			/* ~~ to be implemented ~~ */

			/* ~~ Absolute intent is valid for processing of */
			/* PCS from named Colors. Also allow for e_pcs */
			if (intent != icmDefaultIntent) {
				sprintf(p->err,"icc_get_luobj: Intent is inappropriate for Named Color profile");
				p->errc = 1;
				return NULL;
			}

			sprintf(p->err,"icc_get_luobj: Named Colors not handled yet");
			p->errc = 1;
			return NULL;

    	default:
			sprintf(p->err,"icc_get_luobj: Unknown profile class");
			p->errc = 1;
			return NULL;
	}

	return luobj;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */

/* Create an empty object. Return null on error */
icc *new_icc_a(
icmAlloc *al			/* Optional memory allocator. NULL for default */
) {
	icc *p;
	int del_al = 0;

	if (al == NULL) {	/* None provided, create default */
		if ((al = new_icmAllocStd()) == NULL)
			return NULL;
		del_al = 1;		/* We need to delete it */
	}

	if ((p = (icc *) al->calloc(al, 1,sizeof(icc))) == NULL)
		return NULL;
	p->al = al;				/* Heap allocator */
	p->del_al = del_al;		/* Flag noting whether we delete it */

	p->get_size      = icc_get_size;
	p->read          = icc_read;
	p->write         = icc_write;
	p->dump          = icc_dump;
	p->del           = icc_delete;
	p->add_tag       = icc_add_tag;
	p->link_tag      = icc_link_tag;
	p->find_tag      = icc_find_tag;
	p->read_tag      = icc_read_tag;
	p->rename_tag    = icc_rename_tag;
	p->unread_tag    = icc_unread_tag;
	p->read_all_tags = icc_read_all_tags;
	p->delete_tag    = icc_delete_tag;

	p->get_luobj  = icc_get_luobj;

#if defined(__IBMC__) && defined(_M_IX86)
	_control87(EM_UNDERFLOW, EM_UNDERFLOW);
#endif

	/* Allocate a header object */
	if ((p->header = new_icmHeader(p)) == NULL) {
		al->free(al, p);
		if (del_al)
			al->del(al);
		return NULL;
	}

	/* Values that must be set before writing */
	p->header->deviceClass = icMaxEnumClass;/* Type of profile - must be set! */
    p->header->colorSpace = icMaxEnumData;	/* Clr space of data - must be set! */
    p->header->pcs = icMaxEnumData;			/* PCS: XYZ or Lab - must be set! */
    p->header->renderingIntent = icMaxEnumIntent;	/* Rendering intent - must be set ! */

	/* Values that should be set before writing */
	p->header->manufacturer = -1;			/* Dev manufacturer - should be set ! */
    p->header->model = -1;					/* Dev model number - should be set ! */
    p->header->attributes.l = 0;			/* ICC Device attributes - should set ! */
    p->header->flags = 0;					/* Embedding flags - should be set ! */
	
	/* Values that may be set before writing */
    p->header->attributes.h = 0;			/* Dev Device attributes - may be set ! */
    p->header->creator = str2tag("argl");	/* Profile creator - Argyll - may be set ! */

	/* Init default values in header */
	p->header->cmmId = str2tag("argl");		/* CMM for profile - Argyll CMM */
    p->header->majv = 2;					/* Current version 2.1.0 */
	p->header->minv = 1;
	p->header->bfv  = 0;
	setcur_DateTimeNumber(&p->header->date);/* Creation Date */
    p->header->platform = icSigMicrosoft;	/* Primary Platform */
    p->header->illuminant = icmD50;			/* Profile illuminant - D50 */

	return p;
}


/* For backwards compatibility - a NULL allocator version */
icc *new_icc(void) {
	return new_icc_a(NULL);
}


/* ---------------------------------------------------------- */
