/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

char *xxxvers = "\n@(#) MAKE.  VERSION 2.78     22 MAY 1986\n";
static char *sccsid = "@(#)ident.c	8th Edition (Bell Labs) 85/10/28";

/*
2.1 4/24/76	Base version

2.2 4/26/76     Error found by SRB in overriding pattern rules;
		corrected gram.y

2.3 4/27/76	Further correction for overriding pattern rules;
		corrected doname.c

2.4		Removed .CLEAR name, added .IGNORE.
		A .SUFFIXES rule without dependents clears the list

2.5		Stripped output

2.6		Changed doshell to accomodate new shell.

2.7		Following SRB's sugestion, added ${...} as
		alternate macro name

2.8		Defined macros AS and DTGEN in files.c.

2.9		Put in a fix to prevent removal of files
		upon interrupt in a  ::  rule.

2.10		Fixed bugs involving messages for ::
		and closing standard input

2.11		Changed time test from <= to <
		(equal times are considered in sync)

2.12		Installed -t flag (touch and update time of
		files rather than issue commands)
		Fixed bug in dosys

2.13		Fixed lex.c to allow sharps (#) in commands

2.14		Added .DEFAULT rule

2.15		Changed to <lS> I/O System (stdio.h)

2.16		Removed references to double floats and macro HAVELONGS;
		committed to use of long ints for times.
2.17		Corrected metacharacter list in dosys.c.
2.18		Miscellaneous fixes
2.19		Updated files.c to use include file stat.h
2.20		Added -q flag for Mike Lesk
2.21		Added AWK rules and  .w  suffix to  files.c
2.22		Added colon to the list of metacharacters
2.23		Macro substitutions on dependency lines.
		Redid argument and macro setting.
		Close files before exec'ing.
		Print > at beginning of command lines.
		No printing of commands beginnng with @.
2.24	Parametrized propt sequence in doname.c (4/1/77)
2.25	Added $? facility
2.26	Fixed bug in macro expansion
2.27	Repaired interrupt handling
2.28	Repaired bug in -n
2.29	Repaired bug in file closing and $? string creation
2.30	Repaired bug in grammar about command lines
2.31	Added -k flag, modified doname.c and defs
2.32	Made "keepgoing" the default, added -S flag,
		changed handling of funny characters internally
2.3	Small fixups to interrupt and quit handling.
	       Changed default back to -k.
2.34	Added  .PRECIOUS rule for interrupts
2.35	Added references to include files (due to TLL)
2.36	Fixed bug in lex.c so = permitted in rules on :; line
2.37	Miscellaneous code cleanups
2.38	Sleep one second after each touch in -t mode
2.39	Extended string[] declaration in doname.c
2.40	Permit recursive macro references
2.41	Separated YYLMAX into INMAX and OUTMAX macros, specifying longest
	input and output lines respectively.
2.42	Fixed bug involving :: lines without dependents
2.43	Main name is first name that contains a slash or doesn't
	begin with a dot
2.44	Fixed bug involving $$ on command line
2.45	Changed files.c to put .f before .e, .r and to use f77 instead of fc.
2.46	Changed dosys.c to eliminate copying and to call execvp.
2.47	Changed files.c to add ".out" suffix and rules.
2.48	Changed misc.c to permit tabs preceding = in macro definition
2.49	Added reference to <ctyp.h>. Removed -lS references from files.c
2.50	General cleanup to reduce lint messages.  (changes in declarations
	and in uses of variables)
2.51	Further cleanup making use of new Yacc features.
2.52
2.53	Changed handling of "touch"
2.54	Fixed bug involving comments in lexical analyzer.
2.55	Ignore commands that begin with a # are comments.
2.56	Added = to list of META characters (to permit shell commands)
2.57	Changed lookarch and getobj to fix bugs.
2.58	Fixed interrupt handling.
2.59	Changed references to sprintf to accomodate new function definition
	Also fixed extern declarations.
2.60	Limited the number of open directories.
2.61	Added code to handle archives with ascii headers.
2.62	Joe Condon Fixes to archive formats
2.63	Pattern Matching (%) stuff.
2.64	Reinstalled $(TGS) as $^ from other version
2.65	Installed dynamic macros ( := commands).
2.66	Sped up pattern matching code
2.67	Changed pattern matching code to permit multiple dependents
2.68	Added + (do it despite -n) prefix to command lines.
	Fixed bug involving metacharacter expansions on dependency lines.
2.69	Added & to dependency lines and new background process spawning
2.70	Added Bradford's macros: $/, $@, *D, *F, <D, <F, @D, @F.
2.71	Added include stack to input.
	Added check for sccs makefiles:  s.[Mm]akefile
2.72	Load environment into macro tables. Added Bradford's -e flag.
2.73	Pass changed environment macros out to commands.
2.74	Fixed limit on args in dosys.c.
	Non-existent archives now treated as other non-existent files.
2.75	Fixed bug in rehash.
2.76	Fixed bug when pattern searching in non-existent directory
	Fixed infinite loop when awaiting failed process
	Now wait till all subjobs finish before returning
	make, unless a subjob fails
2.77	Added -z option that always forces shell invocation
	rather than direct fork-exec
2.78	Check for error (-1) returned from fork
*/
