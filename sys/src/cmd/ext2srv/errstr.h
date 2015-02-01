/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

char *errmsg[] = {
	[Enevermind]	"never mind",
	[Eformat]	"unknown format",
	[Eio]		"I/O error",
	[Enomem]	"server out of memory",
	[Enonexist]	"file does not exist",
	[Eexist]	"file already exist",
	[Eperm]		"permission denied",
	[Enofilsys]	"no file system device specified",
	[Eauth]		"authentication failed",
	[Enospace]	"no space on device",
	[Elink]	"write is only allowed in regular files",
	[Elongname]	"name is too long",
	[Eintern]	"internal Ext2 error",
	[Ecorrupt]	"corrupt filesystem",
	[Enotclean] "fs not clean ... running e2fsck is recommended"	
};
