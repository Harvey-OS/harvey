/*
This file contains the list of non-math macros and plain troff macros.
Do NOT forget to put the dot for the troff macros, and the backslash
for TeX macros (two backslashes, one for escape).
The third column in the list is 0 for macros that have no arguments
and either 1 or 2 for those that do. If it is 1, then only one line
will be read as an argument. If it is 2, then lines will be read until
properly terminated. Arguments for ms macros are terminated by an ms macro
(e.g. .PP). Plain troff macros are terminated after reading the desired number
of lines specified by the macro (e.g. .ce 5   will centerline 5 lines).
The fourth column specifies whether the macro implies a paragraph break
(par > 1) or not (par = 0). This is needed to terminate some environments.
If .LP, or .PP, par=1; if .IP, par=2; if .TP, par=3; if .QP, par=4.
*/
struct macro_table {
	char *troff_mac, *tex_mac;
	int arg, macpar;
} macro[] = {

/*  troff macro		TeX macro		   argument	par	*/
	".1C",		 "\\onecolumn",			0,	1,
	".2C",		 "\\twocolumn",			0,	1,
	".AE",		 "\\end{abstract}",		0,	1,
	".AI",		 "\\authoraff",			2,	1,
	".AU",		 "\\author",			2,	1,
	".Ac",		 "\\ACK",		  	0,	1,
	".B1",		 "\\boxit{",			0,	0,
	".B2",		 "}",				0,	0,
	".DE",		 "\\displayend",		0,	1,
	".DS",		 "\\displaybegin",		0,	1,
	".FE",		 "}",				0,	0,
	".FS",		 "\\footnote{",			0,	0,
	".Ic",		 "\\caption{",			0,	1,
	".Ie",		 "}\\end{figure}",		0,	1,
	".Is",		 "\\begin{figure}",		0,	1,
	".KS",		 "{\\nobreak",			0,	0,
	".LP",		 "\\par\\noindent",		0,	1,
	".MH",		 "\\mhead",			2,	1,
	".NH",		 "\\section",			1,	1,
	".PP",		 "\\par",			0,	1,
	".QE",		 "\\end{quotation}",		0,	1,
	".QS",		 "\\begin{quotation}",		0,	1,
	".SH",		 "\\shead",			1,	1,
	".TH",		 "\\phead",			1,	0,
	".TL",		 "\\title",			2,	1,
	".UC",		 "",				0,	0,
	".UL",		 "\\undertext",			1,	0,
	".bp",		 "\\newpage",			0,	1,
	".br",		 "\\nwl",			0,	0,
	".ce",		 "\\cntr",			2,	0,
	".cu",		 "\\undertext",			2,	0,
	".fi",		 "\\fill",			0,	0,
	".na",		 "\\raggedright",		0,	0,
	".nf",		 "\\nofill",			0,	0,
	".ns",		 "",				0,	0,
	".ul",		 "\\undertext",			2,	0
};
