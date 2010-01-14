#include <u.h>
#include <libc.h>

/*
 * This file is derrived from nterr.h in the samba distribution
 */

/*
   Unix SMB/CIFS implementation.
   DOS error code constants
   Copyright (C) Andrew Tridgell              1992-2000
   Copyright (C) John H Terpstra              1996-2000
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000
   Copyright (C) Paul Ashton                  1998-2000

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


static struct {
	int	err;
	char	*msg;
} DOSerrs[] = {
	/* smb x/open error codes for the errdos error class */
	{ (0<<16)|1,	"no error" },
	{ (1<<16)|1,	"invalid function" },
	{ (2<<16)|1,	"file not found" },
	{ (3<<16)|1,	"directory not found" },
	{ (4<<16)|1,	"too many open files" },
	{ (5<<16)|1,	"access denied" },
	{ (6<<16)|1,	"invalid fid" },
	{ (7<<16)|1,	"memory control blocks destroyed." },
	{ (8<<16)|1,	"out of memory" },
	{ (9<<16)|1,	"invalid memory block address" },
	{ (10<<16)|1,	"invalid environment" },
	{ (12<<16)|1,	"invalid open mode" },
	{ (13<<16)|1,	"invalid data (only from ioctl call)" },
	{ (14<<16)|1,	"reserved" },
	{ (15<<16)|1,	"invalid drive" },
	{ (16<<16)|1,	"attempt to delete current directory" },
	{ (17<<16)|1,	"rename/move across filesystems" },
	{ (18<<16)|1,	"no more files found" },
	{ (31<<16)|1,	"general failure" },
	{ (32<<16)|1,	"share mode conflict with open mode" },
	{ (33<<16)|1,	"lock conflicts" },
	{ (50<<16)|1,	"request unsupported" },
	{ (64<<16)|1,	"network name not available" },
	{ (66<<16)|1,	"ipc unsupported (guess)" },
	{ (67<<16)|1,	"invalid share name" },
	{ (80<<16)|1,	"file already exists" },
	{ (87<<16)|1,	"invalid paramater" },
	{ (110<<16)|1,	"cannot open" },
	{ (122<<16)|1,  "insufficent buffer" },
	{ (123<<16)|1,	"invalid name" },
	{ (124<<16)|1,	"unknown level" },
	{ (158<<16)|1,	"this region already locked" },

	{ (183<<16)|1,	"rename failed" },

	{ (230<<16)|1,	"named pipe invalid" },
	{ (231<<16)|1,	"pipe busy" },
	{ (232<<16)|1,	"close in progress" },
	{ (233<<16)|1,	"no reader of named pipe" },
	{ (234<<16)|1,	"more data to be returned" },
	{ (259<<16)|1,	"no more items" },
	{ (267<<16)|1,	"invalid directory name in a path" },
	{ (282<<16)|1,	"extended attributes" },
	{ (1326<<16)|1,	"authentication failed" },
	{ (2123<<16)|1,	"buffer too small" },
	{ (2142<<16)|1,	"unknown ipc" },
	{ (2151<<16)|1,	"no such print job" },
	{ (2455<<16)|1,	"invalid group" },

	/* Error codes for the ERRSRV class */
	{ (1<<16)|2,	"non specific error" },
	{ (2<<16)|2,	"bad password" },
	{ (3<<16)|2,	"reserved" },
	{ (4<<16)|2,	"permission denied" },
	{ (5<<16)|2,	"tid invalid" },
	{ (6<<16)|2,	"invalid server name" },
	{ (7<<16)|2,	"invalid device" },
	{ (22<<16)|2,	"unknown smb" },
	{ (49<<16)|2,	"print queue full" },
	{ (50<<16)|2,	"queued item too big" },
	{ (52<<16)|2,	"fid invalid in print file" },
	{ (64<<16)|2,	"unrecognised command" },
	{ (65<<16)|2,	"smb server internal error" },
	{ (67<<16)|2,	"fid/pathname invalid" },
	{ (68<<16)|2,	"reserved 68" },
	{ (69<<16)|2,	"access is invalid" },
	{ (70<<16)|2,	"reserved 70" },
	{ (71<<16)|2,	"attribute mode invalid" },
	{ (81<<16)|2,	"message server paused" },
	{ (82<<16)|2,	"not receiving messages" },
	{ (83<<16)|2,	"no room for message" },
	{ (87<<16)|2,	"too many remote usernames" },
	{ (88<<16)|2,	"operation timed out" },
	{ (89<<16)|2,	"no resources" },
	{ (90<<16)|2,	"too many userids" },
	{ (91<<16)|2,	"bad userid" },
	{ (250<<16)|2,	"retry with mpx mode" },
	{ (251<<16)|2,	"retry with standard mode" },
	{ (252<<16)|2,	"resume mpx mode" },
	{ (0xffff<<16)|2, "function not supported" },

	/* Error codes for the ERRHRD class */
	{ (19<<16)|3,	"read only media" },
	{ (20<<16)|3,	"unknown device" },
	{ (21<<16)|3,	"drive not ready" },
	{ (22<<16)|3,	"unknown command" },
	{ (23<<16)|3,	"data (CRC) error" },
	{ (24<<16)|3,	"bad request length" },
	{ (25<<16)|3,	"seek failed" },
	{ (26<<16)|3,	"bad media" },
	{ (27<<16)|3,	"bad sector" },
	{ (28<<16)|3,	"no paper" },
	{ (29<<16)|3,	"write fault" },
	{ (30<<16)|3,	"read fault" },
	{ (31<<16)|3,	"general hardware failure" },
	{ (34<<16)|3,	"wrong disk" },
	{ (35<<16)|3,	"FCB unavailable" },
	{ (36<<16)|3,	"share buffer exceeded" },
	{ (39<<16)|3,	"disk full" },

};

char *
doserrstr(uint err)
{
	int i, match;
	char *class;
	static char buf[0xff];

	switch(err & 0xff){
 	case 1:
		class = "dos";
		break;
	case 2:
		class = "network";
		break;
	case 3:
		class = "hardware";
		break;
	case 4:
		class = "Xos";
		break;
	case 0xe1:
		class = "mx1";
		break;
	case 0xe2:
		class = "mx2";
		break;
	case 0xe3:
		class = "mx3";
		break;
	case 0xff:
		class = "packet";
		break;
	default:
		class = "unknown";
		break;
	}

	match = -1;
	for(i = 0; i < nelem(DOSerrs); i++)
		if(DOSerrs[i].err == err)
			match = i;

	if(match != -1)
		snprint(buf, sizeof(buf), "%s, %s", class, DOSerrs[match].msg);
	else
		snprint(buf, sizeof(buf), "%s, %ud/0x%ux - unknown error",
			class, err >> 16, err >> 16);
	return buf;
}
