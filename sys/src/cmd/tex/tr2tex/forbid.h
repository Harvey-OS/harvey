/*
This file contains TeX commands that cannot be re-defined.
Re-defining them is not permitted by TeX (or they may produce
unpredictable consequences).
If the troff user happens to re-define one of these, it will be replaced.
This list is extracted from the starred entries in the index of the TeXBook.
They are entered here in alphabetical order.
If the macro has non-alphabetical characters, it will be trapped
somewhere else; it needn't be put here.
This list is added to make the program more robust.
Note that the backslash is omitted.
*/
char *forbid[] =
{
"atop",     "char",    "copy",     "count",    "cr",      "crcr",
"day",      "def",     "divide",   "dp",       "dump",    "edef",
"else",     "end",     "eqno",     "fam",      "fi",      "font",
"gdef",     "global",  "halign",   "hbox",     "hfil",    "hfill",
"hfuzz",    "hoffset", "hrule",    "hsize",    "hskip",   "hss",
"ht",       "if",      "ifcase",   "ifcat",    "ifnum",   "ifodd",
"iftrue",   "ifx",     "indent",   "input",    "insert",  "kern",
"left",     "leqno",   "let",      "limits",   "long",    "lower",
"mag",      "mark",    "mkern",    "month",    "mskip",   "multiply",
"muskip",   "omit",    "or",       "outer",    "output",  "over",
"overline", "par",     "raise",    "read",     "right",   "show",
"span",     "special", "string",   "the",      "time",    "toks",
"topmark",  "topskip", "unkern",   "unskip",   "vbox",    "vfil",
"vfill",    "vfuzz",   "voffset",  "vrule",    "vsize",   "vskip",
"vsplit",   "vss",     "vtop",     "wd",       "write",   "xdef"
};
