/*
 * amd29k definition
 */
#include <u.h>
#include <bio.h>
#include "/29000/include/ureg.h"
#include <mach.h>

#define	REGOFF(x)	(ulong)(&((struct Ureg *) 0)->x)

Reglist amd29kreglist[] = {
	{"CAUSE",	REGOFF(cause),		RINT|RRDONLY, 'X'},
	{"STATUS",	REGOFF(status),		RINT|RRDONLY, 'X'},
	{"CHA",		REGOFF(cha),		RINT|RRDONLY, 'X'},
	{"CHD",		REGOFF(chd),		RINT|RRDONLY, 'X'},
	{"CHC",		REGOFF(chc),		RINT|RRDONLY, 'X'},
	{"PC0",		REGOFF(pc0),		RINT|RRDONLY, 'X'},
	{"PC1",		REGOFF(pc1),		RINT|RRDONLY, 'X'},
	{"PC2",		REGOFF(pc2),		RINT|RRDONLY, 'X'},
	{"IPC",		REGOFF(ipc),		RINT|RRDONLY, 'X'},
	{"IPA",		REGOFF(ipa),		RINT|RRDONLY, 'X'},
	{"IPB",		REGOFF(ipb),		RINT|RRDONLY, 'X'},
	{"IPA",		REGOFF(ipa),		RINT|RRDONLY, 'X'},
	{"ALUSTAT",	REGOFF(alustat),	RINT|RRDONLY, 'X'},
	{"CR",		REGOFF(cr),		RINT|RRDONLY, 'X'},
	{"R64",		REGOFF(r64),		RINT, 'X'},
	{"SP",		REGOFF(sp),		RINT, 'X'},
	{"R66",		REGOFF(r66),		RINT, 'X'},
	{"R67",		REGOFF(r67),		RINT, 'X'},
	{"R68",		REGOFF(r68),		RINT, 'X'},
	{"R69",		REGOFF(r69),		RINT, 'X'},
	{"R70",		REGOFF(r70),		RINT, 'X'},
	{"R71",		REGOFF(r71),		RINT, 'X'},
	{"R72",		REGOFF(r72),		RINT, 'X'},
	{"R73",		REGOFF(r73),		RINT, 'X'},
	{"R74",		REGOFF(r74),		RINT, 'X'},
	{"R75",		REGOFF(r75),		RINT, 'X'},
	{"R76",		REGOFF(r76),		RINT, 'X'},
	{"R77",		REGOFF(r77),		RINT, 'X'},
	{"R78",		REGOFF(r78),		RINT, 'X'},
	{"R79",		REGOFF(r79),		RINT, 'X'},
	{"R80",		REGOFF(r80),		RINT, 'X'},
	{"R81",		REGOFF(r81),		RINT, 'X'},
	{"R82",		REGOFF(r82),		RINT, 'X'},
	{"R83",		REGOFF(r83),		RINT, 'X'},
	{"R84",		REGOFF(r84),		RINT, 'X'},
	{"R85",		REGOFF(r85),		RINT, 'X'},
	{"R86",		REGOFF(r86),		RINT, 'X'},
	{"R87",		REGOFF(r87),		RINT, 'X'},
	{"R88",		REGOFF(r88),		RINT, 'X'},
	{"R89",		REGOFF(r89),		RINT, 'X'},
	{"R90",		REGOFF(r90),		RINT, 'X'},
	{"R91",		REGOFF(r91),		RINT, 'X'},
	{"R92",		REGOFF(r92),		RINT, 'X'},
	{"R93",		REGOFF(r93),		RINT, 'X'},
	{"R94",		REGOFF(r94),		RINT, 'X'},
	{"R95",		REGOFF(r95),		RINT, 'X'},
	{"R96",		REGOFF(r96),		RINT, 'X'},
	{"R97",		REGOFF(r97),		RINT, 'X'},
	{"R98",		REGOFF(r98),		RINT, 'X'},
	{"R99",		REGOFF(r99),		RINT, 'X'},
	{"R100",	REGOFF(r100),		RINT, 'X'},
	{"R101",	REGOFF(r101),		RINT, 'X'},
};

	/* the machine description */
Mach m29000 =
{
	"29000",
	M29000,		/* machine type */
	amd29kreglist,	/* register set */
	52*4,		/* number of bytes in reg set */
	0,		/* number of bytes in fp reg set */
	"PC0",		/* name of PC */
	"SP",		/* name of SP */
	"R64",		/* name of link register */
	"setR67",	/* static base register name */
	0,		/* value */
	0x1000,		/* page size */
	0x01000000,	/* kernel base */
	0x01000000,	/* kernel text mask */
	4,		/* quantization of pc */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
