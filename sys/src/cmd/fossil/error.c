/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

int8_t EBadAddr[] = "illegal block address";
int8_t EBadDir[] = "corrupted directory entry";
int8_t EBadEntry[] = "corrupted file entry";
int8_t EBadLabel[] = "corrupted block label";
int8_t EBadMeta[] = "corrupted meta data";
int8_t EBadMode[] = "illegal mode";
int8_t EBadOffset[] = "illegal offset";
int8_t EBadPath[] = "illegal path element";
int8_t EBadRoot[] = "root of file system is corrupted";
int8_t EBadSuper[] = "corrupted super block";
int8_t EBlockTooBig[] = "block too big";
int8_t ECacheFull[] = "no free blocks in memory cache";
int8_t EConvert[] = "protocol botch";
int8_t EExists[] = "file already exists";
int8_t EFsFill[] = "file system is full";
int8_t EIO[] = "i/o error";
int8_t EInUse[] = "file is in use";
int8_t ELabelMismatch[] = "block label mismatch";
int8_t ENilBlock[] = "illegal block address";
int8_t ENoDir[] = "directory entry is not allocated";
int8_t ENoFile[] = "file does not exist";
int8_t ENotDir[] = "not a directory";
int8_t ENotEmpty[] = "directory not empty";
int8_t ENotFile[] = "not a file";
int8_t EReadOnly[] = "file is read only";
int8_t ERemoved[] = "file has been removed";
int8_t ENotArchived[] = "file is not archived";
int8_t EResize[] = "only support truncation to zero length";
int8_t ERoot[] = "cannot remove root";
int8_t ESnapOld[] = "snapshot has been deleted";
int8_t ESnapRO[] = "snapshot is read only";
int8_t ETooBig[] = "file too big";
int8_t EVentiIO[] = "venti i/o error";
