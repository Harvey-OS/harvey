/***********************************************************************
This file provides alternative functions for several VMS VMS  C  library
routines which either unacceptable, or incorrect, implementations.  They
have  been developed and  tested under VMS Version  4.4, but indications
are  that they apply  to  earlier versions, back to 3.2  at least.  They
should be retested with each new release of VMS C.

Contents:
	EXIT
	FSEEK
	FTELL
	GETCHAR
	GETENV
	READ
	UNGETC
	getlogin
	qsort
	system
	tell
	unlink

The VAX VMS  file system record  structure has  unfortunate consequences
for random access files.

By default, text  files written by most system  utilities, and languages
other than C, have a variable  length record format,  in  which a 16-bit
character count is  aligned on an  even-byte boundary in the  disk block
b(always 512 bytes   in VMS, independent  of  record and  file  formats),
followed by  <count> bytes of data.   Binary files, such  as .EXE, .OBJ,
and  TeX .DVI  and font  files, all use a  512-byte  fixed record format
which  has no explicit  length  field.  No  file  byte count  is stored;
instead, the block count,  and the  offset of the  last data byte in the
last block are recorded in the file header  (do ``DUMP/HEADER filespec''
to see it).  For binary files with fixed-length  records, the last block
is normally  assumed to be  full,  and  consequently, file   transfer of
binary data from other machines  via Kermit, FTP, or DCL  COPY from ANSI
tapes, generally fails because  the input file length is  not a multiple
of 512.

This record organization may  be contrasted with  the STREAM, STREAM_LF,
and STREAM_CR organizations supported from Version 4.0; in  these,  disk
blocks contain a continuous byte stream in which nothing, or  LF, or CR,
is recognized as a record terminator.  These formats are similar to  the
Unix  and TOPS-20 file system  formats  which also use continuous   byte
streams.

For C, this  means that a  program operating on a file  in record format
cannot count input characters and expect that count to be the same value
as the  offset parameter passed  to fseek(),  which  numerous C programs
assume to  be the case.  The draft  ANSI C  standard,  and  Harbison and
Steele's ``C Reference Manual'', emphasize that only  values returned by
ftell() should be used as arguments to fseek(),  allowing the program to
return to  a position previously read or  written.  UNFORTUNATELY, VMS C
ftell()  DOES NOT  RETURN   A CORRECT  OFFSET VALUE FOR   RECORD  FILES.
Instead, for record files, it returns the byte  offset  of the start  of
the current record, no matter where in that  record the current position
may  be.   This  misbehavior  is  completely unnecessary,   since    the
replacements below perform correctly, and are written entirely in C.

Another problem is that ungetc(char c,  FILE*  fp) is unreliable.  VMS C
implements  characters  as  signed 8-bit integers  (so  do many other  C
implementations).  fgetc(FILE*  fp) returns an int,  not  a  char, whose
value is EOF (-1) in the event of end-of-file;  however, this value will
also  be returned for  a   character  0xFF, so  it  is essential  to use
feof(FILE  *fp) to test  for a  true end-of-file condition  when  EOF is
returned.   ungetc() checks the sign of  its argument c,  and  if it  is
negative (which it will be for 128 of the 256 signed  bytes), REFUSES TO
PUT IT BACK IN THE INPUT STREAM, on the assumption that c is really EOF.
This  too can  be fixed;   ungetc()  should only  do   nothing if feof()
indicates  a  true  end-of-file  condition.   The   overhead of  this is
trivial, since feof() is   actually implemented  as a macro   which does
nothing more than a logical AND and compare-with-zero.

getchar()  waits for a <CR> to  be typed when stdin is  a terminal;  the
replacement vms_getchar() remedies this.

Undoubtedly  other  deficiencies  in   VMS  C will   reveal  themselves.

VMS read() returns   only  a  single  disk   block on  each call.    Its
replacment, vms_read(), will  return  the  requested number of bytes, if
possible.

There are also a  few Unix standard  functions which are  unimplemented.
qsort() is not provided.  getlogin()  and unlink() have VMS  equivalents
provided below.  tell() is considered obsolete, since its  functionality
is available from lseek(), but it is still seen in a few programs, so is
provided below.   getenv()  fails if  the  name contains  a  colon;  its
replacement allows the colon.

In the interest  of  minimal source perturbation,  replacements  for VMS
functions   are  given   the same  names,    but prefixed  "vms_".   For
readability,   the original names  are  preserved,  but are converted to
upper-case:

	#define FTELL vms_ftell
	#define FSEEK vms_fseek
	#define GETCHAR vms_getchar
	#define GETENV vms_getenv
	#define UNGETC vms_ungetc

These  are  only defined to work   correctly for fixed  length  512-byte
records, and no check is made that the file has that organization (it is
possible, but   not without  expensive calls to    fstat(), or access to
internal library structures).

[02-Apr-87]  --	Nelson   H.F.  Beebe,  University  of Utah  Center  for
		Scientific Computing
***********************************************************************/

#define EXIT	vms_exit
#define FTELL	vms_ftell
#define FSEEK	vms_fseek
#define GETENV	vms_getenv
#define GETCHAR vms_getchar
#define READ	vms_read
#define UNGETC	vms_ungetc

#include <stdio.h>
#include <types.h>
#include <ctype.h>
#include <stat.h>
#include <descrip.h>
#include <iodef.h>		/* need for vms_getchar() */
#include <ssdef.h>

#ifdef __GNUC__
#include <stdlib.h>
#endif

void  EXIT(int code);
long  FTELL(FILE *fp);
long  FSEEK(FILE *fp, long n, long dir);
long  UNGETC(char c, FILE *fp);
int   GETCHAR(void);
int   READ(int file_desc, char *buffer, int nbytes);
char *GETENV(char *name);
char *getlogin(void);
long  tell(int handle);
int   unlink(char *filename);

/**********************************************************************/
/*-->EXIT*/

void
vms_exit(int code)
{
    switch (code)
    {
    case 0:
	exit(1);			/* success */
	break;

    default:
	exit(2);			/* error */
	break;
    }
}


/**********************************************************************/
/*-->FSEEK*/

/* VMS fseek() and ftell() on fixed-length record files work correctly
only at block boundaries.  This replacement code patches in the offset
within	the  block.  Directions	 from	current	  position   and  from
end-of-file are converted to absolute positions, and then the code for
that case is invoked. */

long
FSEEK(FILE *fp, long n, long dir)
{
    long k,m,pos,val,oldpos;
    struct stat buffer;

    for (;;)			/* loops only once or twice */
    {
      switch (dir)
      {
      case 0:			/* from BOF */
	  oldpos = FTELL(fp);	/* get current byte offset in file */
	  k = n & 511;		/* offset in 512-byte block */
	  m = n >> 9;		/* relative block number in file */
	  if (((*fp)->_cnt) && ((oldpos >> 9) == m)) /* still in same block */
	  {
	    val = 0;		/* success */
	    (*fp)->_ptr = ((*fp)->_base) + k; /* reset pointers to requested byte */
	    (*fp)->_cnt = 512 - k;
	  }
	  else
	  {
	    val = fseek(fp,m << 9,0); /* move to start of requested 512-byte block */
	    if (val == 0)	/* success */
	    {
	      (*fp)->_cnt = 0;	/* indicate empty buffer */
	      (void)fgetc(fp);	/* force refill of buffer */
	      (*fp)->_ptr = ((*fp)->_base) + k;	/* reset pointers to requested byte */
	      (*fp)->_cnt = 512 - k;
	    }
	  }
	  return(val);

      case 1:			/* from current pos */
	  pos = FTELL(fp);
	  if (pos == EOF)	/* then error */
	    return (EOF);
	  n += pos;
	  dir = 0;
	  break;		/* go do case 0 */

      case 2:			/* from EOF */
	  val = fstat(fileno(fp),&buffer);
	  if (val == EOF)	/* then error */
	    return (EOF);
	  n += buffer.st_size - 1; /* convert filesize to offset and */
				   /* add to requested offset */
	  dir = 0;
	  break;		/* go do case 0 */

      default:			/* illegal direction parameter */
	  return (EOF);
      }
    }
}

/**********************************************************************/
/*-->FTELL*/

/* With fixed-length record files, ftell() returns the offset of the
start of block.	 To get the true position, this must be biased by
the offset within the block. */

long
FTELL(FILE *fp)
{
    char c;
    long pos;
    long val;
    if ((*fp)->_cnt == 0)	/* buffer empty--force refill */
    {
	c = fgetc(fp);
	val = UNGETC(c,fp);
	if (val != c)
	    return (EOF);	/* should never happen */
    }
    pos = ftell(fp);		/* this returns multiple of 512 (start of block) */
    if (pos >= 0)		/* then success--patch in offset in block */
      pos += ((*fp)->_ptr) - ((*fp)->_base);
    return (pos);
}
  
/**********************************************************************/
/*-->GETCHAR*/

static int tt_channel = -1;	/* terminal channel for image QIO's */

#define FAILED(status) (~(status) & 1) /* failure if LSB is 0 */

int
GETCHAR()
{
    int ret_char;		/* character returned */
    int status;			/* system service status */
    static $DESCRIPTOR(sys_in,"TT:");

    if (tt_channel == -1)	/* then first call--assign channel */
    {
	status = sys$assign(&sys_in,&tt_channel,0,0);
	if (FAILED(status))
	    lib$stop(status);
    }
    ret_char = 0;
    status = sys$qiow(0,tt_channel,IO$_TTYREADALL | IO$M_NOECHO,0,0,0,
	&ret_char,1,0,0,0,0);
    if (FAILED(status))
        lib$stop(status);

    return (ret_char);
}
  
/**********************************************************************/
/*-->READ*/
int
READ(register int file_desc,register char *buffer,register int nbytes)
{
    register int ngot;
    register int left;
    
    for ((left = nbytes, ngot = 0); left > 0; /* NOOP */)
    {
	ngot = read(file_desc,buffer,left);
	if (ngot < 0)
	    return (-1);	/* error occurred */
	buffer += ngot;
	left -= ngot;
    }
    return(nbytes-left);
}

/**********************************************************************/
/*-->UNGETC*/
long
UNGETC(char c, FILE *fp)	/* VMS ungetc() is a no-op if c < 0 */
{				/* (which is half the time!)        */

    if ((c == EOF) && feof(fp))
	return (EOF);		/* do nothing at true end-of-file */
    else if ((*fp)->_cnt >= 512)/* buffer full--no fgetc() done in this block!*/
	return (EOF);		/* must be user error if this happens */
    else			/* put the character back in the buffer */
    {
      (*fp)->_cnt++;		/* increase count of characters left */
      (*fp)->_ptr--;		/* backup pointer to next available char */
      *((*fp)->_ptr) = c;	/* save the character */
      return (c);		/* and return it */
    }
}

/**********************************************************************/
/*-->getenv*/
char*
GETENV(char *name)
{
    char* p;
    char* result;
    char ucname[256];

    p = ucname;
    while (*name)	/* VMS logical names must be upper-case */
    {
      *p++ = islower(*name) ? toupper(*name) : *name;
      ++name;
    }
    *p = '\0';

    p = strchr(ucname,':');		/* colon in name? */
    if (p == (char *)NULL)		/* no colon in name */
        result = getenv(ucname);
    else				/* try with and without colon */
    {
	result = getenv(ucname);
	if (result == (char *)NULL)
	{
	    *p = '\0';
	    result = getenv(ucname);
	    *p = ':';
	}
    }
    return (result);
}

/**********************************************************************/
/*-->getlogin*/
char*
getlogin()
{
    return ((char *)getenv("USER")); /* use equivalent VMS routine */
}

/**********************************************************************/
/*-->qsort*/

/***********************************************************************
TeXindex uses  the standard  Unix  library function  qsort()  for
record sorting.  Unfortunately, qsort()  is not a stable  sorting
algorithm, so input order is not necessarily preserved for  equal
sort  keys.    This  is   important,  because   the  sorting   is
case-independent, while  the  actual  entries may  not  be.   For
example, the input

\entry{i}{22}{{\CODE{i}}}
\entry{i}{42}{{\CODE{i}}}
\entry{I}{41}{{\CODE{I}}}
\entry{I}{42}{{\CODE{I}}}

produces

\initial {I}
\entry {{\CODE{i}}}{22}
\entry {{\CODE{I}}}{41--42}
\entry {{\CODE{i}}}{42}

instead of the correct

\initial {I}
\entry {{\CODE{i}}}{22, 42}
\entry {{\CODE{I}}}{41--42}

We  therefore  provide  this  stable  shellsort  replacement  for
qsort() based  on the  code  given on  p.  116 of  Kernighan  and
Ritchie, ``The  C Programming  Language'', Prentice-Hall  (1978).
This has  order  N**1.5  average performance,  which  is  usually
slower than qsort().  In the interests of simplicity, we make  no
attempt to handle short sequences by alternative methods.

[07-Nov-86]
***********************************************************************/

#if VMS_QSORT
#define BASE(i) &base[(i)*width]

void
qsort(base, nel, width, compar)
    char base[];	/* start of data in memory */
    int nel;		/* number of elements to be sorted */
    int width;		/* size (in bytes) of each element */
    int (*compar)();	/* comparison function */
{
    int gap;
    int i;
    int j;

    register int k;	/* inner exchange loop parameters */
    register char* p;
    register char* q;
    register char  c;

    for (gap = nel/2; gap > 0; gap /= 2)
    {
	for (i = gap; i < nel; i++)
	{
	    for (j = i-gap; j >= 0; j -= gap)
	    {
	        p = BASE(j);
		q = BASE(j+gap);
		if ((*compar)(p,q) <= 0)
		    break;	/* exit j loop */
		else
		{
		    for (k = 0; k < width; (++p, ++q, ++k))
		    {
			c = *q;
			*q = *p;
			*p = c;
		    }
		}
	    }
	}
    }
}
#endif
/**********************************************************************
*-->system*
int
system(char *s)
{
	struct	dsc$descriptor t;

	t.dsc$w_length = strlen(s);
	t.dsc$a_pointer = s;
	t.dsc$b_class = DSC$K_CLASS_S;
	t.dsc$b_dtype = DSC$K_DTYPE_T;
	return (LIB$SPAWN(&t) == SS$_NORMAL) ? 0 : 127;
}


**********************************************************************/
/*-->tell*/
long
tell(int handle)
{
    return (lseek(handle,0L,1));
}

/**********************************************************************/
/*-->unlink*/
int
unlink(char *filename)
{
	return (delete(filename)); /* use equivalent VMS routine */
}
