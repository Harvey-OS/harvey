% mft.ch for C compilation with web2c.
% 
% From Pierre Mackay's version for pc, which was in turn based on Howard
% Trickey's and Pavel Curtis's change file for weave.
% This file is in the public domain.

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{MFT changes for C}
@z

% [3] No need for the final label in C.
% AIX defines `class' in <math.h>, so let's take this opportunity to
% define that away.
@x
@d end_of_MFT = 9999 {go here to wrap it up}
@y
@d class == class_var
@z

@x [3] No global labels.
label end_of_MFT; {go here to finish}
@y
@z

@x [3]
procedure initialize;
  var @<Local variables for initialization@>@/
  begin @<Set initial values@>@/
@y
@<Define |parse_arguments|@>
procedure initialize;
  var @<Local variables for initialization@>@/
begin
  kpse_set_progname (argv[0]);
  parse_arguments;
  @<Set initial values@>;
@z

@x [8] Increase constants.
@!buf_size=100; {maximum length of input line}
@!line_length=80; {lines of \TeX\ output have at most this many characters,
@y
@!buf_size=3000; {maximum length of input line}
@!line_length=79; {lines of \TeX\ output have at most this many characters,
@z

% [13] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x [17] Allow any input character.
for i:=0 to @'37 do xchr[i]:=' ';
for i:=@'177 to @'377 do xchr[i]:=' ';
@y
for i:=1 to @'37 do xchr[i]:=chr(i);
for i:=@'177 to @'377 do xchr[i]:=chr(i);
@z

@x [20] Terminal I/O.
@d print(#)==write(term_out,#) {`|print|' means write on the terminal}
@y
@d term_out==stdout
@d print(#)==write(term_out,#) {`|print|' means write on the terminal}
@z

% [20] Remove term_out.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@<Globals...@>=
@!term_out:text_file; {the terminal as an output file}
@y
@z

@x [21] Don't initialize the terminal.
@ Different systems have different ways of specifying that the output on a
certain file will appear on the user's terminal. Here is one way to do this
on the \PASCAL\ system that was used in \.{WEAVE}'s initial development:
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
additional error checking needs to be done.
@^system dependencies@>

@p procedure open_input; {prepare to read the inputs}
begin reset(mf_file); reset(change_file); reset(style_file);
end;
@y
@ The following code opens the input files.
@^system dependencies@>

@p procedure open_input; {prepare to read inputs}
begin
  mf_file := kpse_open_file (cmdline (optind), kpse_mf_format);
  if change_name then begin
    reset (change_file, change_name);
  end;
  style_file := kpse_open_file (style_name, kpse_mft_format);
end;
@z

@x [26] Opening the .tex output file.
rewrite(tex_file);
@y
rewrite (tex_file, tex_name);
@z

@x [28] Fix f^.
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

@x [31] Fix jump_out.
@ The |jump_out| procedure just cuts across all active procedure levels
and jumps out of the program. This is the only non-local \&{goto} statement
in \.{MFT}. It is used when no recovery from a particular error has
been provided.

Some \PASCAL\ compilers do not implement non-local |goto| statements.
@^system dependencies@>
In such cases the code that appears at label |end_of_MFT| should be
copied into the |jump_out| procedure, followed by a call to a system procedure
that terminates the program.

@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; jump_out;
  end

@<Error handling...@>=
procedure jump_out;
begin goto end_of_MFT;
end;
@y
@ The |jump_out| procedure cleans up, prints appropriate messages,
and exits back to the operating system.

@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; jump_out;
	end

@<Error handling...@>=
procedure jump_out;
begin
@t\4\4@>{here files should be closed if the operating system requires it}
  @<Print the job |history|@>;
  new_line;
  if (history <> spotless) and (history <> harmless_message) then
    uexit(1)
  else
    uexit(0);
end;
@z

@x [79] Allow any 8 bit character in input.
for i:=0 to " "-1 do char_class[i]:=invalid_class;
char_class[carriage_return]:=end_line_class;@/
for i:=127 to 255 do char_class[i]:=invalid_class;
@y
for i:=0 to " "-1 do char_class[i]:=letter_class;
for i:=127 to 255 do char_class[i]:=letter_class;
char_class[carriage_return]:=end_line_class;
char_class[@'11]:=space_class; {tab}
char_class[@'14]:=space_class; {form feed}
@z

% [112] Print newline at end of run, exit based upon value of history,
% and remove the end_of_MFT label.
@x
print_ln(banner); {print a ``banner line''}
@y
print (banner); {print a ``banner line''}
print_ln (version_string);
@z

@x
end_of_MFT:{here files should be closed if the operating system requires it}
@<Print the job |history|@>;
end.
@y
@<Print the job |history|@>;
new_line;
if (history <> spotless) and (history <> harmless_message)
then uexit (1)
else uexit (0);
end.
@z

@x [114] System-dependent changes.
This module should be replaced, if necessary, by changes to the program
that are necessary to make \.{MFT} work at a particular installation.
It is usually best to design your change file so that all changes to
previous modules preserve the module numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new modules, can be inserted here; then only the index
itself will get a new module number.
@^system dependencies@>
@y
The user calls \.{MFT} with arguments on the command line.  These are
either filenames or flags (beginning with `\.-').  The following globals
are for communicating the user's desires to the rest of the program. The
various |name| variables contain strings with the full names of those
files, as UNIX knows them.

@<Globals...@>=
@!change_name,@!style_name,@!tex_name:c_string;

@ Look at the command line arguments and set the |name| variables accordingly.

At least one file name must be present as the first argument: the \.{mf}
file.  It may have an extension, or it may omit it to get |'.mf'| added.
If there is only one file name, the output file name is formed by
replacing the \.{mf} file name extension by |'.tex'|.  Thus, the command
line \.{mf foo} implies the use of the \METAFONT\ input file \.{foo.mf}
and the output file \.{foo.tex}.  If this style of command line, with
only one argument, is used, the default style file, |plain.mft|, will be
used to provide basic formatting.

@d argument_is (#) == (strcmp (long_options[option_index].name, #) = 0)

@<Define |parse_arguments|@> =
procedure parse_arguments;
const n_options = 4; {Pascal won't count array lengths for us.}
var @!long_options: array[0..n_options] of getopt_struct;
    @!getopt_return_val: integer;
    @!option_index: c_int_type;
    @!current_option: 0..n_options;
begin
  @<Initialize the option variables@>;
  @<Define the option table@>;
  repeat
    getopt_return_val := getopt_long_only (argc, argv, '', long_options,
                                           address_of (option_index));
    if getopt_return_val = -1 then begin
      {End of arguments; we exit the loop below.} ;
    
    end else if getopt_return_val = "?" then begin
      usage (1, 'mft');

    end else if argument_is ('help') then begin
      usage (0, MFT_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.E. Knuth');

    end else if argument_is ('change') then begin
      change_name := extend_filename (optarg, 'ch');
    
    end else if argument_is ('style') then begin
      style_name := extend_filename (optarg, 'mft');
    
    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have exactly one remaining argument.}
  if (optind + 1 <> argc) then begin
    write_ln (stderr, 'mft: Need exactly one file argument.');
    usage (1, 'mft');
  end;

  tex_name := basename_change_suffix (cmdline (optind), '.mf', '.tex');
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

@ Here is the option to set a change file.
@.-change@>

@<Define the option...@> =
long_options[current_option].name := 'change';
long_options[current_option].has_arg := 1;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ Here is the option to set the style file.
@.-style@>

@<Define the option...@> =
long_options[current_option].name := 'style';
long_options[current_option].has_arg := 1;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ |style_name| starts off as the standard thing.

@<Initialize the option...@> =
style_name := 'plain.mft';

@ An element with all zeros always ends the list of options. 

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
@z
