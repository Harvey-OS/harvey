/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "vac.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

int8_t ENoDir[] = "directory entry is not allocated";
int8_t ENoFile[] = "no such file or directory";
int8_t EBadPath[] = "bad path";
int8_t EBadDir[] = "corrupted directory entry";
int8_t EBadMeta[] = "corrupted meta data";
int8_t ENotDir[] = "not a directory";
int8_t ENotFile[] = "not a file";
int8_t EIO[] = "i/o error";
int8_t EBadOffset[] = "illegal offset";
int8_t ETooBig[] = "file too big";
int8_t EReadOnly[] = "read only";
int8_t ERemoved[] = "file has been removed";
int8_t ENilBlock[] = "illegal block address";
int8_t ENotEmpty[] = "directory not empty";
int8_t EExists[] = "file already exists";
int8_t ERoot[] = "cannot remove root";
