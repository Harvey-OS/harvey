/* Copyright (C) 1991-2004 artofcode LLC.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gp.h,v 1.27 2004/01/15 09:27:10 giles Exp $ */
/* Interface to platform-specific routines */
/* Requires gsmemory.h */

#ifndef gp_INCLUDED
#  define gp_INCLUDED

#include "gstypes.h"
/*
 * This file defines the interface to ***ALL*** platform-specific routines,
 * with the exception of the thread/synchronization interface (gpsync.h)
 * and getenv (gpgetenv.h).  The implementations are in gp_*.c files
 * specific to each platform.  We try very hard to keep this list short!
 */
/*
 * gp_getenv is declared in a separate file because a few places need it
 * and don't want to include any of the other gs definitions.
 */
#include "gpgetenv.h"
/*
 * The prototype for gp_readline is in srdline.h, since it is shared with
 * stream.h.
 */
#include "srdline.h"

/* ------ Initialization/termination ------ */

/*
 * This routine is called early in the initialization.
 * It should do as little as possible.  In particular, it should not
 * do things like open display connections: that is the responsibility
 * of the display device driver.
 */
void gp_init(void);

/*
 * This routine is called just before the program exits (normally or
 * abnormally).  It too should do as little as possible.
 */
void gp_exit(int exit_status, int code);

/*
 * Exit the program.  Normally this just calls the `exit' library procedure,
 * but it does something different on a few platforms.
 */
void gp_do_exit(int exit_status);

/* ------ Miscellaneous ------ */

/*
 * Get the string corresponding to an OS error number.
 * If no string is available, return NULL.  The caller may assume
 * the string is allocated statically and permanently.
 */
const char *gp_strerror(int);

/* ------ Date and time ------ */

/*
 * Read the current time (in seconds since an implementation-defined epoch)
 * into ptm[0], and fraction (in nanoseconds) into ptm[1].
 */
void gp_get_realtime(long ptm[2]);

/*
 * Read the current user CPU time (in seconds) into ptm[0],
 * and fraction (in nanoseconds) into ptm[1].
 */
void gp_get_usertime(long ptm[2]);

/* ------ Reading lines from stdin ------ */

/*
 * These routines are intended to provide an abstract interface to GNU
 * readline or to other packages that offer enhanced line-reading
 * capability.
 */

/*
 * Allocate a state structure for line reading.  This is called once at
 * initialization.  *preadline_data is an opaque pointer that is passed
 * back to gp_readline and gp_readline_finit.
 */
int gp_readline_init(void **preadline_data, gs_memory_t *mem);

/*
 * See srdline.h for the definition of sreadline_proc.
 */
int gp_readline(stream *s_in, stream *s_out, void *readline_data,
		gs_const_string *prompt, gs_string *buf,
		gs_memory_t *bufmem, uint *pcount, bool *pin_eol,
		bool (*is_stdin)(const stream *));

/*
 * Free a readline state.
 */
void gp_readline_finit(void *readline_data);

/* ------ Reading from stdin, unbuffered if possible ------ */

/* Read bytes from stdin, using unbuffered if possible.
 * Store up to len bytes in buf.
 * Returns number of bytes read, or 0 if EOF, or -ve if error.
 * If unbuffered is NOT possible, fetch 1 byte if interactive
 * is non-zero, or up to len bytes otherwise.
 * If unbuffered is possible, fetch at least 1 byte (unless error or EOF) 
 * and any additional bytes that are available without blocking.
 */
int gp_stdin_read(char *buf, int len, int interactive, FILE *f);

/* ------ Screen management ------ */

/*
 * The following are only relevant for X Windows.
 */

/* Get the environment variable that specifies the display to use. */
const char *gp_getenv_display(void);

/* ------ File naming and accessing ------ */

/*
 * Define the maximum size of a file name returned by gp_open_scratch_file
 * or gp_open_printer.  (This should really be passed as an additional
 * parameter, but it would break too many clients to make this change now.)
 * Note that this is the size of the buffer, not the maximum number of
 * characters: the latter is one less, because of the terminating \0.
 */
#define gp_file_name_sizeof 260 /* == MAX_PATH on Windows */

/* Define the character used for separating file names in a list. */
extern const char gp_file_name_list_separator;

/* Define the default scratch file name prefix. */
extern const char gp_scratch_file_name_prefix[];

/* Define the name of the null output file. */
extern const char gp_null_file_name[];

/* Define the name that designates the current directory. */
extern const char gp_current_directory_name[];

/* Define the string to be concatenated with the file mode */
/* for opening files without end-of-line conversion. */
/* This is always either "" or "b". */
extern const char gp_fmode_binary_suffix[];

/* Define the file modes for binary reading or writing. */
/* (This is just a convenience: they are "r" or "w" + the suffix.) */
extern const char gp_fmode_rb[];
extern const char gp_fmode_wb[];

/**
 * gp_open_scratch_file: Create a scratch file.
 * @prefix: Name prefix.
 * @fname: Where to store filename of newly created file.
 * @mode: File access mode (in fopen syntax).
 *
 * Creates a scratch (temporary) file in the filesystem. The exact
 * location and name of the file is platform dependent, but in general
 * uses @prefix as a prefix. If @prefix is not absolute, then choose
 * an appropriate system directory, usually as determined from
 * gp_gettmpdir(), followed by a path as returned from a system call.
 *
 * Implementations should make sure that 
 *
 * Return value: Opened file object, or NULL on error.
 **/
FILE *gp_open_scratch_file(const char *prefix,
			   char fname[gp_file_name_sizeof],
			   const char *mode);

/* Open a file with the given name, as a stream of uninterpreted bytes. */
FILE *gp_fopen(const char *fname, const char *mode);

/* Force given file into binary mode (no eol translations, etc) */
/* if 2nd param true, text mode if 2nd param false */
int gp_setmode_binary(FILE * pfile, bool mode);

typedef enum {
    gp_combine_small_buffer = -1,
    gp_combine_cant_handle = 0,
    gp_combine_success = 1
} gp_file_name_combine_result;

/*
 * Combine a file name with a prefix.
 * Concatenates two paths and reduce parten references and current 
 * directory references from the concatenation when possible.
 * The trailing zero byte is being added.
 * Various platforms may share this code.
 */
gp_file_name_combine_result gp_file_name_combine(const char *prefix, uint plen, 
	    const char *fname, uint flen, bool no_sibling, char *buffer, uint *blen);

/* -------------- Helpers for gp_file_name_combine_generic ------------- */
/* Platforms, which do not call gp_file_name_combine_generic, */
/* must stub the helpers against linkage problems. */

/* Return length of root prefix of the file name, or zero. */
/*	unix:   length("/")	    */
/*	Win:    length("c:/") or length("//computername/cd:/")  */
/*	mac:	length("volume:")    */
/*	VMS:	length("device:[root.]["	    */
uint gp_file_name_root(const char *fname, uint len);

/* Check whether a part of file name starts (ends) with a separator. */
/* Must return the length of the separator.*/
/* If the 'len' is negative, must search in backward direction. */
/*	unix:   '/'	    */
/*	Win:    '/' or '\'  */
/*	mac:	':' except "::"	    */
/*	VMS:	smart - see the implementation   */
uint gs_file_name_check_separator(const char *fname, int len, const char *item);

/* Check whether a part of file name is a parent reference. */
/*	unix, Win:  equal to ".."	*/
/*	mac:	equal to ":"		*/
/*	VMS:	equal to "."		*/
bool gp_file_name_is_parent(const char *fname, uint len);

/* Check if a part of file name is a current directory reference. */
/*	unix, Win:  equal to "."	*/
/*	mac:	equal to ""		*/
/*	VMS:	equal to ""		*/
bool gp_file_name_is_current(const char *fname, uint len);

/* Returns a string for referencing the current directory. */
/*	unix, Win:  "."	    */
/*	mac:	":"	    */
/*	VMS:	""          */
const char *gp_file_name_current(void);

/* Returns a string for separating a file name item. */
/*	unix, Win:  "/"	    */
/*	mac:	":"	    */
/*	VMS:	"]"	    */
const char *gp_file_name_separator(void);

/* Returns a string for separating a directory item. */
/*	unix, Win:  "/"	    */
/*	mac:	":"	    */
/*	VMS:	"."	    */
const char *gp_file_name_directory_separator(void);

/* Returns a string for representing a parent directory reference. */
/*	unix, Win:  ".."    */
/*	mac:	":"	    */
/*	VMS:	"."	    */
const char *gp_file_name_parent(void);

/* Answer whether the platform allows parent refenences. */
/*	unix, Win, Mac: yes */
/*	VMS:	no.         */
bool gp_file_name_is_partent_allowed(void);

/* Answer whether an empty item is meanful in file names on the platform. */
/*	unix, Win:  no	    */
/*	mac:	yes	    */
/*	VMS:	yes         */
bool gp_file_name_is_empty_item_meanful(void);

/* Read a 'resource' stored in a special database indexed by a 32 bit  */
/* 'type' and 16 bit 'id' in an extended attribute of a file. The is   */
/* primarily for accessing fonts on MacOS, which classically used this */
/* format. Presumedly a 'nop' on systems that don't support Apple HFS. */
int gp_read_macresource(byte *buf, const char *fname, 
                                     const uint type, const ushort id);


/* ------ persistent cache interface ------ */

/*
 * This is used for access to data cached between invocations of 
 * Ghostscript. It is generally used for saving reusable data that
 * is expensive to compute. Concurrent access by multiple instances
 * is safe. Because of this care should be taken to use a new data
 * type when the format of the cached data changes.
 *
 * Generic data buffers are stored under a combination of type and
 * key. It is up the to client to interpret the data buffer appropriately.
 * An insert overwrites any previous entry under that type and key.
 * A query if successful uses the passed callback to allocate a buffer
 * and fills it with the retrieved data. The caller is thus responsible
 * for the buffer's memory management.
 * 
 * See zmisc.c for postscript test operators and an example implementation.
 */

/* return 0 on successful insert, non-zero otherwise */
int gp_cache_insert(int type, byte *key, int keylen, void *buffer, int buflen);

/* return the length of the buffer on success, a negative value otherwise */
typedef void *(*gp_cache_alloc)(void *userdata, int bytes);
int gp_cache_query(int type, byte* key, int keylen, void **buffer,
    gp_cache_alloc alloc, void *userdata);

/* cache data types */
#define GP_CACHE_TYPE_TEST 0
#define GP_CACHE_TYPE_FONTMAP 1
#define GP_CACHE_TYPE_WTS 2


/* ------ Printer accessing ------ */

/*
 * Open a connection to a printer.  A null file name means use the standard
 * printer connected to the machine, if any.  Note that this procedure is
 * called only if the original file name (normally the value of the
 * OutputFile device parameter) was an ordinary file (not stdout, a pipe, or
 * other %filedevice%file name): stdout is handled specially, and other
 * values of filedevice are handled by calling the fopen procedure
 * associated with that kind of "file".
 *
 * Note that if the file name is null (0-length) and a default printer is
 * available, the file name may be replaced with the name of a scratch file
 * for spooling.  If the file name is null and no default printer is
 * available, this procedure returns 0.
 */
FILE *gp_open_printer(char fname[gp_file_name_sizeof], int binary_mode);

/*
 * Close the connection to the printer.  Note that this is only called
 * if the original file name was an ordinary file (not stdout, a pipe,
 * or other %filedevice%file name): stdout is handled specially, and other
 * values of filedevice are handled by calling the fclose procedure
 * associated with that kind of "file".
 */
void gp_close_printer(FILE * pfile, const char *fname);

/* ------ File enumeration ------ */

#ifndef file_enum_DEFINED	/* also defined in iodev.h */
#  define file_enum_DEFINED
typedef struct file_enum_s file_enum;
#endif

/*
 * Begin an enumeration.  pat is a C string that may contain *s or ?s.
 * The implementor should copy the string to a safe place.
 * If the operating system doesn't support correct, arbitrarily placed
 * *s and ?s, the implementation should modify the string so that it
 * will return a conservative superset of the request, and then use
 * the string_match procedure to select the desired subset.  E.g., if the
 * OS doesn't implement ? (single-character wild card), any consecutive
 * string of ?s should be interpreted as *.  Note that \ can appear in
 * the pattern also, as a quoting character.
 */
file_enum *gp_enumerate_files_init(const char *pat, uint patlen,
				   gs_memory_t * memory);

/*
 * Return the next file name in the enumeration.  The client passes in
 * a scratch string and a max length.  If the name of the next file fits,
 * the procedure returns the length.  If it doesn't fit, the procedure
 * returns max length +1.  If there are no more files, the procedure
 * returns -1.
 */
uint gp_enumerate_files_next(file_enum * pfen, char *ptr, uint maxlen);

/*
 * Clean up a file enumeration.  This is only called to abandon
 * an enumeration partway through: ...next should do it if there are
 * no more files to enumerate.  This should deallocate the file_enum
 * structure and any subsidiary structures, strings, buffers, etc.
 */
void gp_enumerate_files_close(file_enum * pfen);


/* ------ Font enumeration ------ */

/* This is used to query the native os for a list of font names and 
 * corresponding paths. The general idea is to save the hassle of 
 * building a custom fontmap file
 */

/* allocate and initialize the iterator
   returns a pointer to its local state or NULL on failure */
void *gp_enumerate_fonts_init(gs_memory_t *mem);

/* get the next element in the font enumeration
   Takes a pointer to its local state and pointers in which to
   return C strings. The string 'name' is the font name, 'path'
   is the access path for the font resource. The returned strings
   are only safe to reference until until the next call.
   Returns 0 when no more fonts are available, a positive value 
   on success, or negative value on error. */
int gp_enumerate_fonts_next(void *enum_state, char **fontname, char **path);

/* clean up and deallocate the iterator */
void gp_enumerate_fonts_free(void *enum_state);

#endif /* gp_INCLUDED */
