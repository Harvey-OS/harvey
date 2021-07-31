#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_compat.c,v 1.15 92/03/11 09:16:49 sam Exp $";
#endif

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
 * Copyright (c) 1991, 1992 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library Compatibility Routines.
 */
#include "tiffioP.h"

#if defined(unix) || defined(__unix) || defined(MSDOS) || defined(VMS) || defined(plan9)
#include <sys/stat.h>

long
TIFFGetFileSize(fd)
	int fd;
{
	struct stat sb;

	return (fstat(fd, &sb) < 0 ? 0 : sb.st_size);
}
#endif

#if (defined(unix) || defined(__unix)) && defined(MMAP_SUPPORT)
#include <sys/mman.h>

int
TIFFMapFileContents(fd, pbase, psize)
	int fd;
	char **pbase;
	long *psize;
{
	long size = TIFFGetFileSize(fd);
	if (size != -1) {
		*pbase = (char *) mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
		if (*pbase != (char *)-1) {
			*psize = size;
			return (1);
		}
	}
	return (0);
}

void
TIFFUnmapFileContents(base, size)
	char *base;
	long size;
{
	(void) munmap(base, size);
}
#endif /* (defined(unix) || defined(__unix)) && defined(MMAP_SUPPORT) */

#if defined(VMS) && defined(MMAP_SUPPORT)
#include <fab.h>
#include <secdef.h>

/*
 * Table for storing information on current open sections. 
 * (You may want to substitute a linked list...)
 */
#define MAX_MAPPED 100
static int no_mapped = 0;
static struct {
	void *base;
	void *top;
	unsigned channel;
} map_table[MAX_MAPPED];

/* 
 * This routine maps a file into a private section. Note that this 
 * method of accessing a file is by far the fastest under VMS.
 * The routine may fail (i.e. return 0) for several reasons, for
 * example:
 * - There is no more room for storing the info on sections.
 * - The process is out of open file quota, channels, ...
 * - fd does not describe an opened file.
 * - The file is already opened for write access by this process
 *   or another process
 * - There is no free "hole" in virtual memory that fits the
 *   size of the file
 */
int
TIFFMapFileContents(fd, pbase, psize)
	int fd;
	char **pbase;
	long *psize;
{
	char name[256];
	struct FAB fab;
	unsigned short channel;
	void *inadr[2], *retadr[2];
	unsigned long status;
	long size;
	
	if (no_mapped >= MAX_MAPPED)
		return(0);
	/*
	 * We cannot use a file descriptor, we
	 * must open the file once more.
	 */
	if (getname(fd, name, 1) == NULL)
		return(0);
	/* prepare the FAB for a user file open */
	fab = cc$rms_fab;
	fab.fab$v_ufo = 1;
	fab.fab$b_fac = FAB$M_GET;
	fab.fab$b_shr = FAB$M_SHRGET;
	fab.fab$l_fna = name;
	fab.fab$b_fns = strlen(name);
	status = sys$open(&fab);	/* open file & get channel number */
	if ((status&1) == 0)
		return(0);
	channel = (unsigned short)fab.fab$l_stv;
	inadr[0] = inadr[1] = &channel;	/* just an address in P0 space */
	/*
	 * Map the blocks of the file up to
	 * the EOF block into virtual memory.
	 */
	size = TIFFGetFileSize(fd);
	status = sys$crmpsc(inadr, retadr, 0, SEC$M_EXPREG, 0,0,0, channel,
		howmany(size,512), 0,0,0);
	if ((status&1) == 0)
		return(0);
	*pbase = retadr[0];		/* starting virtual address */
	/*
	 * Use the size of the file up to the
	 * EOF mark for UNIX compatibility.
	 */
	*psize = size;
	/* Record the section in the table */
	map_table[no_mapped].base = retadr[0];
	map_table[no_mapped].top = retadr[1];
	map_table[no_mapped].channel = channel;
	no_mapped++;

        return(1);
}

/*
 * This routine unmaps a section from the virtual address space of 
 * the process, but only if the base was the one returned from a
 * call to TIFFMapFileContents.
 */
void
TIFFUnmapFileContents(base, size)
	char *base;
	long size;
{
	void *inadr[2];
	int i, j;
	
	/* Find the section in the table */
	for (i = 0;i < no_mapped; i++) {
		if (map_table[i].base == base) {
			/* Unmap the section */
			inadr[1] = base;
			inadr[0] = map_table[i].top;
			sys$deltva(inadr, 0, 0);
			sys$dassgn(map_table[i].channel);
			/* Remove this section from the list */
			for (j = i+1; j < no_mapped; j++)
				map_table[j-1] = map_table[j];
			no_mapped--;
			return;
		}
	}
}
#endif /* defined(VMS) && defined(MMAP_SUPPORT) */

#if defined(THINK_C) || defined(applec)
long
TIFFGetFileSize(int fd)
{
	long pos, eof;
	
	pos = lseek(fd, 0, SEEK_CUR);
	eof = lseek(fd, 0, SEEK_END);
	lseek(fd, pos, SEEK_SET);
	return(eof);
}
#endif /* THINK_C || applec */

#if defined(applec)
#include <ioctl.h>
#include <Files.h>
#undef lseek

long
mpw_lseek(int fd, long offset, int whence)
{
	long filepos, filesize, newpos;
	short macfd;
	
	if ((filepos = lseek(fd, 0, SEEK_CUR)) < 0 ||
	    (filesize = lseek(fd, 0, SEEK_END)) < 0)
		return (EOF);
	newpos = offset + (whence == SEEK_SET ? 0 : 
			   whence == SEEK_CUR ? filepos :
						filesize);
	if (newpos > filesize)
		if (ioctl(fd, FIOREFNUM, &macfd) == -1 ||
		    SetEOF(macfd, newpos) != 0)
			return (EOF);
	return (lseek(fd, newpos, SEEK_SET));
}
#endif /* applec */
