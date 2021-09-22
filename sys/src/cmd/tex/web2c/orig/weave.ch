% weave.ch for C compilation using web2c.
%
%  11/29/82 HWT Original version. This modifies weave to allow a new
%               control sequence:
%                       @=...text...@>   Put ...text... verbatim on a line
%                                        by itself in the Pascal output.
%                                        (argument must fit on one line)
%               This control sequence facilitates putting #include "gcons.h"
%               (for example) in files meant for the pc compiler.
%               Also, there is a command line option, -c, which means that
%               only the modules affected by the change file are to generate
%               TeX output.  (All the limbo stuff still goes to the output
%               file, as does the index and table of contents.)
%  2/12/83 HWT  Brought up for use with version 1.3.  Uses Knuth's new
%               control sequences for verbatim Pascal (as above, without
%               the "on one line" part), and the force_line (@\) primitive.
%               Also, he added stuff to keep track of changed modules, and
%               output enough information that macros can choose only to
%               print changed modules.  This isn't as efficient as my
%               implementation because I avoided outputting the text for
%               non-changed modules altogether, but this feature won't be
%               used too often (just for TeX and TeXware), so Knuth's
%               solution is accepted.
%               The change file changes that used
%               to implement these features have been removed.
%               There is a -x flag to cause WEAVE to omit cross referencing
%               of identifiers, and index and T.of.C. processing.
%               This, too, is unnecessary, for one could simply redefine some
%               WEB macros to avoid the printing, but there are only a couple
%               of easy changes, so they have been made.
%  2/18     HWT Increased stack size to 400 (not for TeX-related programs).
%  3/18     HWT Brought up for Version 1.5.  Made it print newline at end of
%               run.
%  4/13     PC  Merged with Pavel's version, including adding a call to
%               exit() at the end of the program, based upon the value of
%               `history'.
%  4/16     PC  Brought up to version 1.5 released with TeX 0.97 in April 1983
%  6/29     HWT Brought up to version 1.7 released with TeX 0.99 in June 1983,
%		introducing a new change file format
%  7/17	    HWT Brought up to version 2.0 released with TeX 0.999 in July 1983
%  7/29     HWT Brought up to version 2.1
% 11/17     HWT Brought up to version 2.4 released with TeX 1.0.  Made
%		changes to use C routines for I/O, for speedup.
%  1/31     HWT Brought up to version 2.6
%  12/15/85 ETM Brought up to version 2.8
%  03/15/88 ETM Converted for use with WEB to C, and for version 2.9 of Weave.
%  11/30/89 KB  Version 4.
% (more recent changes in the ChangeLog)

@x [0] WEAVE: print changes only
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{WEAVE changes for C}
@z

@x [2] No global labels, define and call parse_arguments.
label end_of_WEAVE; {go here to finish}
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Error handling procedures@>@/
procedure initialize;
  var @<Local variables for initialization@>@/
  begin @<Set initial values@>@/
@y
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Define |parse_arguments|@>
@<Error handling procedures@>@/
procedure initialize;
  var @<Local variables for initialization@>@/
  begin
    kpse_set_progname (argv[0]);
    parse_arguments;
    @<Set initial values@>@/
@z

@x [8] Increase constants for tex2pdf, etc.
@!max_bytes=45000; {|1/ww| times the number of bytes in identifiers,
  index entries, and module names; must be less than 65536}
@y
@!max_bytes=60000; {|1/ww| times the number of bytes in identifiers,
  index entries, and module names; must be less than 65536}
@z
@x
@!buf_size=100; {maximum length of input line}
@y
@!buf_size=3000; {maximum length of input line}
@z
@x
@!max_toks=30000; {number of symbols in \PASCAL\ texts being parsed;
  must be less than 65536}
@!max_texts=2000; {number of phrases in \PASCAL\ texts being parsed;
  must be less than 10240}
@!max_scraps=1000; {number of tokens in \PASCAL\ texts being parsed}
@!stack_size=200; {number of simultaneous output levels}
@y
@!max_toks=65000; {number of symbols in \PASCAL\ texts being parsed;
  must be less than 65536}
@!max_texts=10000; {number of phrases in \PASCAL\ texts being parsed;
  must be less than 10240}
@!max_scraps=10000; {number of tokens in \PASCAL\ texts being parsed}
@!stack_size=2000; {number of simultaneous output levels}
@z

% [??] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x [17] enable maximum character set
for i:=1 to @'37 do xchr[i]:=' ';
for i:=@'200 to @'377 do xchr[i]:=' ';
@y
for i:=1 to @'37 do xchr[i]:=chr(i);
for i:=@'200 to @'377 do xchr[i]:=chr(i);
@z

@x [20] Terminal I/O.
@d print(#)==write(term_out,#) {`|print|' means write on the terminal}
@y
@d term_out==stdout
@d print(#)==write(term_out,#) {`|print|' means write on the terminal}
@z

@x
@<Globals...@>=
@!term_out:text_file; {the terminal as an output file}
@y
@z

@x [21] Don't initialize the terminal.
@ Different systems have different ways of specifying that the output on a
certain file will appear on the user's terminal. Here is one way to do this
on the \PASCAL\ system that was used in \.{TANGLE}'s initial development:
@^system dependencies@>

@<Set init...@>=
rewrite(term_out,'TTY:'); {send |term_out| output to the terminal}
@y
@ Different systems have different ways of specifying that the output on a
certain file will appear on the user's terminal.
@^system dependencies@>

@<Set init...@>=
{nothing need be done}
@z

@x [22] `break' is `fflush'.
@d update_terminal == break(term_out) {empty the terminal output buffer}
@y
@d update_terminal == fflush(term_out) {empty the terminal output buffer}
@z

@x [24] Open input files.
@ The following code opens the input files.  Since these files were listed
in the program header, we assume that the \PASCAL\ runtime system has
already checked that suitable file names have been given; therefore no
additional error checking needs to be done. We will see below that
\.{WEAVE} reads through the entire input twice.
@^system dependencies@>

@p procedure open_input; {prepare to read |web_file| and |change_file|}
begin reset(web_file); reset(change_file);
end;
@y
@ The following code opens the input files.
This is called after the filename variables have been set appropriately.
@^system dependencies@>

@p procedure open_input; {prepare to read |web_file| and |change_file|}
begin reset(web_file,web_name);
  if chg_name then reset(change_file,chg_name);
end;
@z

@x [26] Opening the .tex file.
rewrite(tex_file);
@y
rewrite(tex_file,tex_name);
@z

@x [28] web2c doesn't understand f^.
    begin buffer[limit]:=xord[f^]; get(f);
    incr(limit);
    if buffer[limit-1]<>" " then final_limit:=limit;
    if limit=buf_size then
      begin while not eoln(f) do get(f);
@y
    begin buffer[limit]:=xord[getc(f)];
    incr(limit);
    if buffer[limit-1]<>" " then final_limit:=limit;
    if limit=buf_size then
      begin while not eoln(f) do vgetc(f);
@z

@x [??] Fix jump_out
@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; jump_out;
  end

@<Error handling...@>=
procedure jump_out;
begin goto end_of_WEAVE;
end;
@y
@d fatal_error(#)==begin new_line; write(stderr, #);
     error; mark_fatal; jump_out;
	end

@<Error handling...@>=
procedure jump_out;
begin
stat @<Print statistics about memory usage@>;@+tats@;@/
@t\4\4@>{here files should be closed if the operating system requires it}
  @<Print the job |history|@>;
  new_line;
  if (history <> spotless) and (history <> harmless_message) then
    uexit(1)
  else
    uexit(0);
end;
@z

@x [50] don't enter xrefs if no_xref set
@d append_xref(#)==if xref_ptr=max_refs then overflow('cross reference')
  else  begin incr(xref_ptr); num(xref_ptr):=#;
    end

@p procedure new_xref(@!p:name_pointer);
label exit;
var q:xref_number; {pointer to previous cross reference}
@!m,@!n: sixteen_bits; {new and previous cross-reference value}
begin if (reserved(p)or(byte_start[p]+1=byte_start[p+ww]))and
@y
If the user has sent the |no_xref| flag (the -x option of the command line),
then it is unnecessary to keep track of cross references for identifers.
If one were careful, one could probably make more changes around module
100 to avoid a lot of identifier looking up.

@d append_xref(#)==if xref_ptr=max_refs then overflow('cross reference')
  else  begin incr(xref_ptr); num(xref_ptr):=#;
    end

@p procedure new_xref(@!p:name_pointer);
label exit;
var q:xref_number; {pointer to previous cross-reference}
@!m,@!n: sixteen_bits; {new and previous cross-reference value}
begin if no_xref then return;
if (reserved(p)or(byte_start[p]+1=byte_start[p+ww]))and
@z

@x [239] omit index and module names if no_xref set
@<Phase III: Output the cross-reference index@>=
@y
If the user has set the |no_xref| flag (the -x option on the command line),
just finish off the page, omitting the index, module name list, and table of
contents.

@<Phase III: Output the cross-reference index@>=
if no_xref then begin
        finish_line;
        out("\"); out5("v")("f")("i")("l")("l");
        out4("\")("e")("n")("d");
        finish_line;
        end
else begin
@z

@x
print('Done.');
@y
end;
print('Done.');
@z

@x [258] term_in == stdin, when debugging.
any error stop will set |debug_cycle| to zero.
@y
any error stop will set |debug_cycle| to zero.

@d term_in==stdin
@z

@x
@!term_in:text_file; {the user's terminal as an input file}
@y
@z

@x [259] Take out reset(term_in)
reset(term_in,'TTY:','/I'); {open |term_in| as the terminal, don't do a |get|}
@y
@z

@x [261] print newline at end of run and exit based upon value of history
print_ln(banner); {print a ``banner line''}
@y
print (banner); {print a ``banner line''}
print_ln (version_string);
@z

@x
end_of_WEAVE:
stat @<Print statistics about memory usage@>;@+tats@;@/
@t\4\4@>{here files should be closed if the operating system requires it}
@<Print the job |history|@>;
end.
@y
jump_out;
end.
@z

@x [264] System-dependent changes.
This module should be replaced, if necessary, by changes to the program
that are necessary to make \.{WEAVE} work at a particular installation.
It is usually best to design your change file so that all changes to
previous modules preserve the module numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new modules, can be inserted here; then only the index
itself will get a new module number.
@^system dependencies@>
@y
Parse a Unix-style command line.

@d argument_is (#) == (strcmp (long_options[option_index].name, #) = 0)

@<Define |parse_arguments|@> =
procedure parse_arguments;
const n_options = 4; {Pascal won't count array lengths for us.}
var @!long_options: array[0..n_options] of getopt_struct;
    @!getopt_return_val: integer;
    @!option_index: c_int_type;
    @!current_option: 0..n_options;
begin
  @<Define the option table@>;
  repeat
    getopt_return_val := getopt_long_only (argc, argv, '', long_options,
                                           address_of (option_index));
    if getopt_return_val = -1 then begin
      {End of arguments; we exit the loop below.} ;
    
    end else if getopt_return_val = "?" then begin
      usage (1, 'weave');

    end else if argument_is ('help') then begin
      usage (0, WEAVE_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.E. Knuth');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) then begin
    write_ln (stderr, 'weave: Need one or two file arguments.');
    usage (1, 'weave');
  end;
  
  {Supply |".web"| and |".ch"| extensions if necessary.}
  web_name := extend_filename (cmdline (optind), 'web');
  if optind + 2 = argc then begin
    chg_name := extend_filename (cmdline (optind + 1), 'ch');
  end;
  
  {Change |".web"| to |".tex"| and use the current directory.}
  tex_name := basename_change_suffix (web_name, '.web', '.tex');
end;

@ Here are the options we allow.  The first is one of the standard GNU options.
@.-help@>

@<Define the option...@> =
current_option := 0;
long_options[current_option].name := 'help';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ Another of the standard options.
@.-version@>

@<Define the option...@> =
long_options[current_option].name := 'version';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ Omit cross-referencing?
@.-x@>

@<Define the option...@> =
long_options[current_option].name := char_to_string ('x');
long_options[current_option].has_arg := 0;
long_options[current_option].flag := address_of (no_xref);
long_options[current_option].val := 1;
incr (current_option);

@
@<Global...@> =
@!no_xref:c_int_type;

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;

@ Global filenames.

@<Global...@> =
@!web_name,@!chg_name,@!tex_name:c_string;
@z
