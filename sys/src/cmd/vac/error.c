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

char ENoDir[] = "directory entry is not allocated";
char ENoFile[] = "no such file or directory";
char EBadPath[] = "bad path";
char EBadDir[] = "corrupted directory entry";
char EBadMeta[] = "corrupted meta data";
char ENotDir[] = "not a directory";
char ENotFile[] = "not a file";
char EIO[] = "i/o error";
char EBadOffset[] = "illegal offset";
char ETooBig[] = "file too big";
char EReadOnly[] = "read only";
char ERemoved[] = "file has been removed";
char ENilBlock[] = "illegal block address";
char ENotEmpty[] = "directory not empty";
char EExists[] = "file already exists";
char ERoot[] = "cannot remove root";
