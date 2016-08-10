/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// lsmtags.c: list all available virtio mount tags 

// The program looks over all existing virtio devices, finding those with name starting with virtio-9p.
// Mount tag is contained in the device configuration area, preceded by 2 bytes holding string length.
// Devices already claimed by other processes deny access to their config area, so mount tag cannot be read.

#include <u.h>
#include <libc.h>
#include <ctype.h>

char *argv0;

// without options: only tags are printed
// -d: print device name along with tag like device tag
// -j <json_tag>: jsonify output so it looks like
//	"json_tag": {
//    "device1": "mount_tag1",
//    ...
//    "devicen": "mount_tagn"
//	} or
//  "json_tag": [
//     "mount_tag1",
//     ...
//     "mount_tagn"
//  ]
// based on the -d option.

#define vddir "/dev/virtqs"
#define STRL(s) s, strlen(s)

static void
usage(void)
{
	fprint(2,
		"usage: %s [-d] [-j json_tag]\n",
			argv0);
	exits("usage");
}

void
json_prolog(char *json_tag, int showdev)
{
	if(json_tag == nil)
		return;
	print("\"%s\": %s\n", json_tag, showdev?"{":"[");
}

void
json_line(int start, char *json_tag, char *dir, char *name, char *mtag, int showdev)
{
	print("%s%s%s%s%s%s%s%s%s%s",
		start?"":(json_tag?",\n":"\n"),
		json_tag?"\t":"",
		(json_tag && showdev)?"\"":"",
		showdev?dir:"",
		showdev?"/":"",
		showdev?name:"",
		showdev?(json_tag?"\": ":" "):"",
		json_tag?"\"":"",
		mtag,
		json_tag?"\"":"");
}

void
json_epilog(char *json_tag, int showdev)
{
	if(json_tag == nil)
		print("\n");
	else
		print("\n%s\n", showdev?"}":"]");
}

void
main(int argc, char **argv)
{
	int showdev;
	char *json_tag = nil;
	
	showdev = 0;
	
	ARGBEGIN {
	case 'd':
		showdev = 1;
		break;
	case 'j':
		json_tag = EARGF(usage());
		break;
	default:
		usage();
		break;
	} ARGEND;
	
	char ebuf[100];
	int fd = open(vddir, OREAD);
	if(fd == -1) {
		errstr(ebuf, 100);
		exits(ebuf);
	}
	Dir *pd;
	long ndir = dirreadall(fd, &pd);
	if(ndir < 0){
		errstr(ebuf, 100);
		exits(ebuf);
	}
	int start = 1;
	json_prolog(json_tag, showdev);
	for(int i = 0; i < ndir; i++) {
		if(strncmp(pd[i].name, STRL("virtio-9p")))
			continue;
		int dcplen = strlen(vddir) + 1 + strlen(pd[i].name) + 1 + strlen("devcfg") + 2;
		char *dcppath = malloc(dcplen);
		if(dcppath == nil)
			exits("no memory to allocate dev cfg area file path");
		strncpy(dcppath, vddir, dcplen);
		strncat(dcppath, "/", dcplen - strlen(dcppath));
		strncat(dcppath, pd[i].name, dcplen - strlen(dcppath));
		strncat(dcppath, "/devcfg", dcplen - strlen(dcppath));
		int fd = open(dcppath, OREAD);
		if(fd >= 0) {
			uint16_t taglen;
			int rc = read(fd, &taglen, sizeof(taglen));
			if(rc == 2) {
				char *mtag = malloc(taglen + 1);
				memset(mtag, 0, taglen + 1);
				read(fd, mtag, taglen);
				json_line(start, json_tag, vddir, pd[i].name, mtag, showdev);
				start = 0;
				free(mtag);
			}
		}
		free(dcppath);
	}
	json_epilog(json_tag, showdev);
	free(pd);
}

