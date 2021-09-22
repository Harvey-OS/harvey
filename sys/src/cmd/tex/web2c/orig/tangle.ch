% tangle.ch for C compilation with web2c.
%
%  10/9/82 (HT) Original version
%  11/29   (HT) New version, with conversion to lowercase handled properly
%               Also, new control sequence:
%                       @=...text...@>   Put ...text... verbatim on a line
%                                        by itself in the Pascal output.
%                                        (argument must fit on one line)
%               This control sequence facilitates putting #include "gcons.h"
%               (for example) in files meant for the pc compiler.
%               Also, changed command line usage, so that the absence of a
%               change file implies no change file, rather than one with the
%               same name as the web file, with .ch at the end.
%  1/15/83 (HT) Changed to work with version 1.2, which incorporates the
%               above change (though unbundling the output line breaking),
%               so mainly had to remove stuff.
%  2/17    (HT) Fixed bug that caused 0-9 in identifiers to be converted to
%               Q-Y on output.
%  3/18    (HT) Brought up to work with Version 1.5.  Added -r command line
%               flag to cause a .rpl file to be written with all the lines
%               of the .web file that were replaced because of the .ch file
%               (useful for comparing with previous .rpl files, to see if a
%               change file will still work with a new version of a .web file)
%               Also, made it write a newline just before exit.
%  4/12    (PC) Merged with Pavel's version, including adding a call to exit()
%               at the end depending upon the value of history.
%  4/16    (PC) Brought up to date with version 1.5 released April, 1983.
%  6/28   (HWT) Brought up to date with version 1.7 released June, 1983.
%               With new change file format, the -r option is now unnecessary.
%  7/17   (HWT) Brought up to date with version 2.0 released July, 1983.
% 12/18/83 (ETM) Brought up to date with version 2.5 released November, 1983.
% 11/07/84 (ETM) Brought up to date with version 2.6.
% 12/15/85 (ETM) Brought up to date with version 2.8.
% 03/07/88 (ETM) Converted for use with WEB2C
% 01/02/89 (PAM) Cosmetic upgrade to version 2.9
% 11/30/89 (KB)  Version 4.
% (more recent changes in the ChangeLog)

@x [0] Print only changes.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{TANGLE changes for C}
@z

@x [?] Define and call parse_arguments.
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
    @<Set initial values@>@/
@z

@x [8] Constants: increase id lengths, for TeX--XeT and tex2pdf.
@!buf_size=100; {maximum length of input line}
@y
@!buf_size=3000; {maximum length of input line}
@z
@x
@!max_toks=50000; {|1/zz| times the number of bytes in compressed \PASCAL\ code;
  must be less than 65536}
@!max_names=4000; {number of identifiers, strings, module names;
  must be less than 10240}
@!max_texts=2000; {number of replacement texts, must be less than 10240}
@y
@!max_toks=60000; {|1/zz| times the number of bytes in compressed \PASCAL\ code;
  must be less than 65536}
@!max_names=6000; {number of identifiers, strings, module names;
  must be less than 10240}
@!max_texts=5000; {number of replacement texts, must be less than 10240}
@z

@x
@!stack_size=50; {number of simultaneous levels of macro expansion}
@!max_id_length=12; {long identifiers are chopped to this length, which must
  not exceed |line_length|}
@!unambig_length=7; {identifiers must be unique if chopped to this length}
  {note that 7 is more strict than \PASCAL's 8, but this can be varied}
@y
@!stack_size=100; {number of simultaneous levels of macro expansion}
@!max_id_length=50; {long identifiers are chopped to this length, which must
  not exceed |line_length|}
@!unambig_length=32; {identifiers must be unique if chopped to this length}
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

@x [20] terminal output: use standard i/o
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

@x [21] init terminal
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
 {Nothing need be done for C.}
@z

@x [22] flush terminal buffer
@d update_terminal == break(term_out) {empty the terminal output buffer}
@y
@d update_terminal == fflush(term_out) {empty the terminal output buffer}
@z

@x [24] open input files
begin reset(web_file); reset(change_file);
@y
begin reset(web_file, web_name);
if chg_name then reset(change_file, chg_name);
@z

@x [26] Open output files (except for the pool file).
rewrite(Pascal_file); rewrite(pool);
@y
rewrite (Pascal_file, pascal_name);
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

@x [??] Fix `jump_out'.
@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; jump_out;
  end

@<Error handling...@>=
procedure jump_out;
begin goto end_of_TANGLE;
end;
@y
@d jump_out==uexit(1)
@d fatal_error(#)==begin new_line; write(stderr, #); 
     error; mark_fatal; uexit(1);
  end
@z

@x [38] Provide for a larger `byte_mem' and `tok_mem'. Extra capacity:
@d ww=2 {we multiply the byte capacity by approximately this amount}
@d zz=3 {we multiply the token capacity by approximately this amount}
@y
@d ww=3 {we multiply the byte capacity by approximately this amount}
@d zz=4 {we multiply the token capacity by approximately this amount}
@z

@x [58] Remove conversion to uppercase
    begin if buffer[i]>="a" then chopped_id[s]:=buffer[i]-@'40
    else chopped_id[s]:=buffer[i];
@y
    begin chopped_id[s]:=buffer[i];
@z

@x [63] Remove conversion to uppercase
    begin if c>="a" then c:=c-@'40; {merge lowercase with uppercase}
@y
    begin 
@z

@x [64] Delayed pool file opening.
@<Define and output a new string...@>=
begin ilk[p]:=numeric; {strings are like numeric macros}
if l-double_chars=2 then {this string is for a single character}
  equiv[p]:=buffer[id_first+1]+@'100000
else  begin equiv[p]:=string_ptr+@'100000;
  l:=l-double_chars-1;
@y
@<Define and output a new string...@>=
begin ilk[p]:=numeric; {strings are like numeric macros}
if l-double_chars=2 then {this string is for a single character}
  equiv[p]:=buffer[id_first+1]+@'100000
else  begin
  {Avoid creating empty pool files.}
  if string_ptr = 256 then begin
    {Change |".web"| to |".pool"| and use the current directory.}
    pool_name := basename_change_suffix (web_name, '.web', '.pool');
    rewrite (pool, pool_name);
  end;
  equiv[p]:=string_ptr+@'100000;
  l:=l-double_chars-1;
@z

@x [105] Accept DIV, div, MOD, and mod
 (((out_contrib[1]="D")and(out_contrib[2]="I")and(out_contrib[3]="V")) or@|
 ((out_contrib[1]="M")and(out_contrib[2]="O")and(out_contrib[3]="D")) ))or@|
@^uppercase@>
@y
  (((out_contrib[1]="D")and(out_contrib[2]="I")and(out_contrib[3]="V")) or@|
  ((out_contrib[1]="d")and(out_contrib[2]="i")and(out_contrib[3]="v")) or@|
  ((out_contrib[1]="M")and(out_contrib[2]="O")and(out_contrib[3]="D")) or@|
  ((out_contrib[1]="m")and(out_contrib[2]="o")and(out_contrib[3]="d")) ))or@|
@z

@x [110] lowercase ids
@^uppercase@>
  if ((out_buf[out_ptr-3]="D")and(out_buf[out_ptr-2]="I")and
    (out_buf[out_ptr-1]="V"))or @/
     ((out_buf[out_ptr-3]="M")and(out_buf[out_ptr-2]="O")and
    (out_buf[out_ptr-1]="D")) then@/ goto bad_case
@y
  if ((out_buf[out_ptr-3]="D")and(out_buf[out_ptr-2]="I")and
    (out_buf[out_ptr-1]="V"))or @/
     ((out_buf[out_ptr-3]="d")and(out_buf[out_ptr-2]="i")and
    (out_buf[out_ptr-1]="v"))or @/
     ((out_buf[out_ptr-3]="M")and(out_buf[out_ptr-2]="O")and
    (out_buf[out_ptr-1]="D"))or @/
     ((out_buf[out_ptr-3]="m")and(out_buf[out_ptr-2]="o")and
    (out_buf[out_ptr-1]="d")) then@/ goto bad_case
@z

@x [114] lowercase operators (`and', `or', etc.)
and_sign: begin out_contrib[1]:="A"; out_contrib[2]:="N"; out_contrib[3]:="D";
@^uppercase@>
  send_out(ident,3);
  end;
not_sign: begin out_contrib[1]:="N"; out_contrib[2]:="O"; out_contrib[3]:="T";
  send_out(ident,3);
  end;
set_element_sign: begin out_contrib[1]:="I"; out_contrib[2]:="N";
  send_out(ident,2);
  end;
or_sign: begin out_contrib[1]:="O"; out_contrib[2]:="R"; send_out(ident,2);
@y
and_sign: begin out_contrib[1]:="a"; out_contrib[2]:="n"; out_contrib[3]:="d";
  send_out(ident,3);
  end;
not_sign: begin out_contrib[1]:="n"; out_contrib[2]:="o"; out_contrib[3]:="t";
  send_out(ident,3);
  end;
set_element_sign: begin out_contrib[1]:="i"; out_contrib[2]:="n";
  send_out(ident,2);
  end;
or_sign: begin out_contrib[1]:="o"; out_contrib[2]:="r"; send_out(ident,2);
@z

@x [116] Remove conversion to uppercase
@ Single-character identifiers represent themselves, while longer ones
appear in |byte_mem|. All must be converted to uppercase,
with underlines removed. Extremely long identifiers must be chopped.

(Some \PASCAL\ compilers work with lowercase letters instead of
uppercase. If this module of \.{TANGLE} is changed, it's also necessary
to change from uppercase to lowercase in the modules that are
listed in the index under ``uppercase''.)
@^system dependencies@>
@^uppercase@>

@d up_to(#)==#-24,#-23,#-22,#-21,#-20,#-19,#-18,#-17,#-16,#-15,#-14,
  #-13,#-12,#-11,#-10,#-9,#-8,#-7,#-6,#-5,#-4,#-3,#-2,#-1,#

@<Cases related to identifiers@>=
"A",up_to("Z"): begin out_contrib[1]:=cur_char; send_out(ident,1);
  end;
"a",up_to("z"): begin out_contrib[1]:=cur_char-@'40; send_out(ident,1);
  end;
identifier: begin k:=0; j:=byte_start[cur_val]; w:=cur_val mod ww;
  while (k<max_id_length)and(j<byte_start[cur_val+ww]) do
    begin incr(k); out_contrib[k]:=byte_mem[w,j]; incr(j);
    if out_contrib[k]>="a" then out_contrib[k]:=out_contrib[k]-@'40
    else if out_contrib[k]="_" then decr(k);
    end;
  send_out(ident,k);
  end;
@y
@ Single-character identifiers represent themselves, while longer ones
appear in |byte_mem|. All must be converted to lowercase,
with underlines removed. Extremely long identifiers must be chopped.
@^system dependencies@>

@d up_to(#)==#-24,#-23,#-22,#-21,#-20,#-19,#-18,#-17,#-16,#-15,#-14,
  #-13,#-12,#-11,#-10,#-9,#-8,#-7,#-6,#-5,#-4,#-3,#-2,#-1,#

@<Cases related to identifiers@>=
"A",up_to("Z"),
"a",up_to("z"): begin out_contrib[1]:=cur_char; send_out(ident,1);
  end;
identifier: begin k:=0; j:=byte_start[cur_val]; w:=cur_val mod ww;
  while (k<max_id_length)and(j<byte_start[cur_val+ww]) do
    begin incr(k); out_contrib[k]:=byte_mem[w,j]; incr(j);
    if out_contrib[k]="_" then decr(k);
    end;
  send_out(ident,k);
  end;
@z

@x [??] Fix casting bug
@d add_in(#)==begin accumulator:=accumulator+next_sign*(#); next_sign:=+1;
  end
@y
@d add_in(#)==begin accumulator:=accumulator+next_sign*toint(#); next_sign:=+1;
  end
@z

@x [179] make term_in = input
any error stop will set |debug_cycle| to zero.
@y
any error stop will set |debug_cycle| to zero.

@d term_in==stdin
@z

@x
@!term_in:text_file; {the user's terminal as an input file}
@y
@z

@x [180] remove term_in reset
reset(term_in,'TTY:','/I'); {open |term_in| as the terminal, don't do a |get|}
@y
@z

@x [182] write newline just before exit; use value of |history|
print_ln(banner); {print a ``banner line''}
@y
print (banner); {print a ``banner line''}
print_ln (version_string);
@z

@x
@<Print the job |history|@>;
@y
@<Print the job |history|@>;
new_line;
if (history <> spotless) and (history <> harmless_message)
then uexit (1)
else uexit (0);
@z

@x [188] System-dependent changes.
This module should be replaced, if necessary, by changes to the program
that are necessary to make \.{TANGLE} work at a particular installation.
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
const n_options = 3; {Pascal won't count array lengths for us.}
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
      usage (1, 'tangle');

    end else if argument_is ('help') then begin
      usage (0, TANGLE_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.E. Knuth');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) then begin
    write_ln (stderr, 'tangle: Need one or two file arguments.');
    usage (1, 'tangle');
  end;
  
  {Supply |".web"| and |".ch"| extensions if necessary.}
  web_name := extend_filename (cmdline (optind), 'web');
  if optind + 2 = argc then begin
    chg_name := extend_filename (cmdline (optind + 1), 'ch');
  end;
  
  {Change |".web"| to |".p"| and use the current directory.}
  pascal_name := basename_change_suffix (web_name, '.web', '.p');
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

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;

@ Global filenames.

@<Globals...@>=
@!web_name,@!chg_name,@!pascal_name,@!pool_name:c_string;
@z
