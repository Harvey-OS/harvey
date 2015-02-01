/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

char *errmsg[ESIZE] = {
	[Enevermind]	"never mind",
	[Eformat]	"unknown format",
	[Eio]		"I/O error",
	[Enoauth]	"dossrv: authentication not required",
	[Enomem]	"server out of memory",
	[Enonexist]	"file does not exist",
	[Eperm]		"permission denied",
	[Enofilsys]	"no file system device specified",
	[Eauth]		"authentication failed",
	[Econtig]	"out of contiguous disk space",
	[Ebadfcall]	"bad fcall type",
	[Ebadstat]	"bad stat format",
	[Etoolong]	"file name too long",
	[Eversion]	"unknown 9P version",
	[Eerrstr]	"system call error",
};
