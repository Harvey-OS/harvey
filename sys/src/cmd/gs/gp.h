/* Copyright (C) 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gp.h */
/* Interface to platform-specific routines */
/* Requires gsmemory.h, gstypes.h */

/*
 * This file defines the interface to ***ALL*** platform-specific routines.
 * The routines are implemented in a gp_*.c file specific to each platform.
 * We try very hard to keep this list short!
 */

/* ------ Initialization/termination ------ */

/*
 * This routine is called early in the initialization.
 * It should do as little as possible.  In particular, it should not
 * do things like open display connections: that is the responsibility
 * of the display device driver.
 */
void	gp_init(P0());

/*
 * This routine is called just before the program exits (normally or
 * abnormally).  It too should do as little as possible.
 */
void	gp_exit(P2(int exit_status, int code));

/* ------ Miscellaneous ------ */

/*
 * Get the string corresponding to an OS error number.
 * If no string is available, return NULL.  The caller may assume
 * the string is allocated statically and permanently.
 */
const char *	gp_strerror(P1(int));

/* ------ Date and time ------ */

/*
 * Read the current date (in days since Jan. 1, 1980) into pdt[0],
 * and time (in milliseconds since midnight) into pdt[1].
 */
void	gp_get_clock(P1(long *pdt));

/* ------ Screen management ------ */

/*
 * The following routines are only relevant in a single-window environment
 * such as a PC; on platforms with window systems, the 'make current'
 * routines do nothing.
 */

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/* Initialize the console. */
void	gp_init_console(P0());

/* Write a string to the console. */
void	gp_console_puts(P2(const char *, uint));

/* Make the console current on the screen. */
int	gp_make_console_current(P1(gx_device *));

/* Make the graphics current on the screen. */
int	gp_make_graphics_current(P1(gx_device *));

/*
 * The following are only relevant for X Windows.
 */

/* Get the environment variable that specifies the display to use. */
const char *	gp_getenv_display(P0());

/* ------ Printer accessing ------ */

/*
 * Open a connection to a printer.  A null file name means use the
 * standard printer connected to the machine, if any.
 * If possible, support "|command" for opening an output pipe.
 * Return NULL if the connection could not be opened.
 */
FILE *	gp_open_printer(P2(char *fname, int binary_mode));

/* Close the connection to the printer. */
void	gp_close_printer(P2(FILE *pfile, const char *fname));

/* ------ File names ------ */

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

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
FILE *	gp_open_scratch_file(P3(const char *prefix, char *fname,
				const char *mode));

/* Answer whether a file name contains a directory/device specification, */
/* i.e. is absolute (not directory- or device-relative). */
bool	gp_file_name_is_absolute(P2(const char *fname, uint len));

/* Answer the string to be used for combining a directory/device prefix */
/* with a base file name.  The file name is known to not be absolute. */
const char *
	gp_file_name_concat_string(P4(const char *prefix, uint plen,
				      const char *fname, uint len));

/* ------ File enumeration ------ */

#ifndef file_enum_DEFINED		/* also defined in iodev.h */
#  define file_enum_DEFINED
struct file_enum_s;	/* opaque to client, defined by implementor */
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
file_enum *	gp_enumerate_files_init(P3(const char *pat, uint patlen,
					   gs_memory_t *memory));

/*
 * Return the next file name in the enumeration.  The client passes in
 * a scratch string and a max length.  If the name of the next file fits,
 * the procedure returns the length.  If it doesn't fit, the procedure
 * returns max length +1.  If there are no more files, the procedure
 * returns -1.
 */
uint	gp_enumerate_files_next(P3(file_enum *pfen, char *ptr, uint maxlen));

/*
 * Clean up a file enumeration.  This is only called to abandon
 * an enumeration partway through: ...next should do it if there are
 * no more files to enumerate.  This should deallocate the file_enum
 * structure and any subsidiary structures, strings, buffers, etc.
 */
void	gp_enumerate_files_close(P1(file_enum *pfen));
