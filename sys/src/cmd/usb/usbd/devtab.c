/* machine generated. do not edit */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbd.h"

extern int kbmain(Dev*, int, char**);
extern int diskmain(Dev*, int, char**);
extern int ethermain(Dev*, int, char**);
extern int ethermain(Dev*, int, char**);
extern int serialmain(Dev*, int, char**);
extern int serialmain(Dev*, int, char**);
extern int serialmain(Dev*, int, char**);

Devtab devtab[] = {
	/* device, entrypoint, {csp, csp, csp csp}, vid, did */
	{"kb",	kbmain,	{0x010103, 0x020103, 0, 0, }, -1, -1, ""},
	{"disk",	diskmain,	{DCL|8, 0, 0, 0, }, -1, -1, ""},
	{"ether",	ethermain,	{DCL|255, 0x00ffff, 0, 0, }, 0x0b95, -1, ""},
	{"ether",	ethermain,	{DCL|255, 0xff00ff, 0, 0, }, 0x0424, 0xec00, ""},
	{"serial",	serialmain,	{DCL|255, 0xffffff, 0, 0, }, 0x9e88, 0x9e8f, ""},
	{"serial",	serialmain,	{DCL|255, 0xffffff, 0, 0, }, 0x0403, -1, ""},
	{"serial",	serialmain,	{DCL|255, 0x0000ff, 0, 0, }, 0x10c4, -1, ""},
	{nil, nil,	{0, 0, 0, 0, }, -1, -1, nil},
};

/* end of machine generated */
