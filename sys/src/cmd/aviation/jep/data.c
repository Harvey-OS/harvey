#define	EXTERN
#include "jep.h"

char*	pname[] =
{
	[Pcol]	"col",
	[Psiz]	"siz",
	[Pdsh]	"dsh",
	[Pjux]	"jux",
	[Pfil]	"fil",
	[Pfnt]	"fnt",
	[Prot]	"rot",
	[P7]	"P7",
	[Px]	"Px",
};

char*	tname[] =
{
	[Tstring]	"s0",
	[Tpoint]	"p0",
	[Tline1]	"l1",
	[Tline2]	"l2",
	[Tline3]	"l3",
	[Tline4]	"l4",
	[Tgroup1]	"g1",
	[Tgroup2]	"g2",
	[Tgroup3]	"g3",
	[Tvect1]	"v1",
	[Tvect2]	"v2",
	[Tvect3]	"v3",
	[Tcirc1]	"c1",
	[Tcirc2]	"c2",
	[Tcirc3]	"c3",
	[Tarc]		"a0",
};

char*	psdat[] =
{
	"%!PS-Adobe-2.0 EPSF-1.2",
	"%%Pages: 1",
	"%%BoundingBox: 0 0 612 792",
	"%%EndComments",

	"%Begin preamble",
	"/FSD {findfont exch scalefont def} bind def",
	"/ltxt {",
	"	gsave moveto",
	"	currentpoint translate rotate",
	"	show grestore",
	"} bind def",
	"/ctxt {",
	"	gsave moveto",
	"	currentpoint translate rotate",
	"	dup stringwidth pop 2 div neg 0 rmoveto",
	"	show grestore",
	"} bind def",
	"/rtxt {",
	"	gsave moveto",
	"	currentpoint translate rotate",
	"	dup stringwidth pop neg 0 rmoveto",
	"	show grestore",
	"} bind def",
	"/latinfont {",
	"	findfont",
	"	dup length dict begin",
	"		{1 index /FID ne {def} {pop pop} ifelse} forall",
	"		/Encoding ISOLatin1Encoding def",
	"		currentdict",
	"	end",
	"	definefont pop",
	"} def",

	"/F00 /Helvetica latinfont",				// unknown/default
	"/F10 /Helvetica latinfont",				// regular
	"/F11 /Helvetica-Oblique latinfont",			// italic
	"/F12 /Helvetica-Narrow latinfont",			// condensed
	"/F13 /Helvetica-Bold latinfont",			// bold
	"/F14 /Helvetica-BoldOblique latinfont",		// bold italic
	"/F18 /Helvetica latinfont",
	"/F19 /Times-Roman latinfont",

	"/F15 /Helvetica latinfont",				// black box
	"/F16 /Helvetica latinfont",				// clearance regular
	"/F17 /Helvetica-Bold latinfont",			// clearance bold
	"/F1c /Helvetica-Bold latinfont",			// jeppesen
	"/F1d /Helvetica-Bold latinfont",			// morse code
	"/F1f /ZapfDingbats findfont definefont",		// footnote

	"%End preamble",
	"%%Page: 1 1",
	0
};

char*	morse[256] =
{
	['A']	".-",
	['B']	"-...",
	['C']	"-.-.",
	['D']	"-..",
	['E']	".",
	['F']	"..-.",
	['G']	"--.",
	['H']	"....",
	['I']	"..",
	['J']	".---",
	['K']	"-.-",
	['L']	".-..",
	['M']	"--",
	['N']	"-.",
	['O']	"---",
	['P']	".--.",
	['Q']	"--.-",
	['R']	".-.",
	['S']	"...",
	['T']	"-",
	['U']	"..-",
	['V']	"...-",
	['W']	".--",
	['X']	"-..-",
	['Y']	"-.--",
	['Z']	"--..",
	['0']	"-----",
	['1']	".----",
	['2']	"..---",
	['3']	"...--",
	['4']	"....-",
	['5']	".....",
	['6']	"-....",
	['7']	"--...",
	['8']	"---..",
	['9']	"----.",
};

uchar	dashc[512] =
{
	[0x00*2]	10, 10,		// unspecified
	[0x01*2]	10, 10,		// gok
	[0x03*2]	10, 10,		// gok
	[0x05*2]	10, 10,		// hashed boundary of R zone
	[0x07*2]	10, 10,		// gok
	[0x08*2]	10, 10,		// turd in /n/jep1/charts/kole191.tcl
	[0x09*2]	10, 10,		// gok
	[0x0b*2]	10, 10,		// gok
	[0x0d*2]	10, 10,		// turd in /n/jep1/charts/kewr132.tcl

	[0x52*2]	10, 10,		// gok
	[0x53*2]	10, 10,		// turd in /n/jep2/charts/lvgz191.tcl
	[0x55*2]	10, 10,		// gok

	[0x90*2]	 3,  6,		// turd in /n/jep1/charts/5b3131.tcl
	[0x91*2]	 3,  6,		// turd in /n/jep1/charts/n23132.tcl
	[0x92*2]	10, 10,		// gok
	[0x93*2]	10, 10,		// gok
	[0x94*2]	 3,  6,		// ndb
	[0x95*2]	10, 10,		// gok
	[0x96*2]	10, 10,		// gok
	[0x97*2]	 6,  6,		// profile chart from faf to map
	[0x9a*2]	25,  8,		// runway centerline extension
	[0x9b*2]	32, 10,		// B4 hold line
	[0x9c*2]	40, 12,		// profile chart missed approach
	[0x9d*2]	35, 12,		// arrival chart feeder
	[0x9e*2]	10, 10,		// gok
	[0x9f*2]	30,  8,		// main chart missed approach

	[0xa0*2]	10, 10,		// ctz boundary /n/jep1/charts/nzwn191.tcl
	[0xa1*2]	10, 10,		// turd in /n/jep1/charts/n81apt.tcl
	[0xa2*2]	10, 10,		// turd in /n/jep1/charts/kgvq131.tcl
	[0xa6*2]	5, 0,		// nopt sector
	[0xa7*2]	10, 10,		// turd in /n/jep1/charts/kbdr181.tcl
	[0xa8*2]	10, 10,		// gok
	[0xa9*2]	70, 20,		// dash dot dot international border
	[0xaa*2]	10, 10,		// gok
	[0xab*2]	10, 10,		// gok
	[0xac*2]	 5, 50,		// railroad tracks
	[0xad*2]	10, 10,		// some sort of feeder /n/jep2/charts/lfbd191.tcl
	[0xae*2]	10, 10,		// some sort of feeder /n/jep2/charts/lypr191.tcl

	[0xb2*2]	10, 10,		// snow fence triangles
	[0xb3*2]	10, 10,		// snow fence triangles
	[0xb4*2]	10, 10,		// gok
	[0xb6*2]	10, 10,		// gok
	[0xba*2]	10, 10,		// gok
	[0xbf*2]	10, 10,		// diagonal slash

	[0xc2*2]	10, 10,		// gok
	[0xc3*2]	10, 10,		// gok
	[0xc4*2]	10, 10,		// sector divider tel hand sets
	[0xc5*2]	10, 10,		// gok
	[0xc6*2]	10, 10,		// arrow triangles
	[0xcb*2]	10, 10,		// half bushs
	[0xcc*2]	10, 10,		// gok
	[0xcd*2]	5, 0,		// solid w arrowhead
	[0xce*2]	10, 10,		// arrow heads
};

/*
 *	(color/2)
 *	(size/4)
 *	(dash/2)
 *	(jux/2)
 *	(fil/4)
 *	(fnt/2)
 *	(rot/4)
 *	(p7/2)
 *	(px/2)
 */
char*	S0s1 =	"^S0-"
	"(..)"
	"(....)"
	"(..)"
	"(..)"
	"(XXXX)"
	"(..)"
	"(....)"
	"(XX)"
	"(..)"
	"-"
	"XXXXXXXXXXXXXXXXXXXXXXXX%";

char*	S0s2 =	"^S0-"
	"(..)"
	"(....)"
	"(..)"
	"(..)"
	"(XXXX)"
	"(..)"
	"(....)"
	"(XX)"
	"(..)"
	"-"
	"XXXXXXXXXXXXXXXXXXXXXX..-"
	"XXXXXXXXXXXXXXXXXXXXXXXX%";
