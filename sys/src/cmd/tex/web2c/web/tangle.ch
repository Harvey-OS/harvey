% tangle.ch for C compilation with web2c.
% The original version of this file was created by Howard Trickey and
% Pavel Curtis.
%
% History:
% 
% (more recent changes in ../ChangeLog.W2C)
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
% (more recent changes in ../ChangeLog.W2C and ./ChangeLog)


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print only changes
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{TANGLE changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner message
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is TANGLE, Version 4.3'
@y
@d banner=='This is TANGLE, C Version 4.3'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [2] add input and output, remove other files, add ref to scan_args,
% and #include external definition for exit().
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d end_of_TANGLE = 9999 {go here to wrap it up}

@p @t\4@>@<Compiler directives@>@/
program TANGLE(@!web_file,@!change_file,@!Pascal_file,@!pool);
label end_of_TANGLE; {go here to finish}
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Error handling procedures@>@/
@y
@d end_of_TANGLE = 9999 {go here to wrap it up}

@p program TANGLE;
label end_of_TANGLE; {go here to finish}
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Error handling procedures@>@/
@<Declaration of |scan_args|@>@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] compiler options
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@{@&$C-,A+,D-@} {no range check, catch arithmetic overflow, no debug overhead}
@!debug @{@&$C+,D+@}@+ gubed {but turn everything on when debugging}
@y
@=(*$C-*)@> {no range check}
@!debug @=(*$C+*)@>@+ gubed {but turn everything on when debugging}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [8] Constants: increase id lengths
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
@!unambig_length=20; {identifiers must be unique if chopped to this length}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [17] enable maximum character set
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
for i:=1 to @'37 do xchr[i]:=' ';
for i:=@'200 to @'377 do xchr[i]:=' ';
@y
for i:=1 to @'37 do xchr[i]:=chr(i);
for i:=@'200 to @'377 do xchr[i]:=chr(i);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [20] terminal output: use standard i/o
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [21] init terminal
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [22] flush terminal buffer
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d update_terminal == break(term_out) {empty the terminal output buffer}
@y
@d update_terminal == flush(term_out) {empty the terminal output buffer}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [24] open input files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The following code opens the input files.  Since these files were listed
in the program header, we assume that the \PASCAL\ runtime system has
already checked that suitable file names have been given; therefore no
additional error checking needs to be done.
@^system dependencies@>

@p procedure open_input; {prepare to read |web_file| and |change_file|}
begin reset(web_file); reset(change_file);
end;
@y
@ The following code opens the input files.
This happens after the |initialize| procedure has executed.
That will have called the |scan_args| procedure to set up the global
variables |web_name| and |chg_name| to the appropriate file
names.
These globals, and the |scan_args| procedure will be defined at the end
where they won't disturb the module numbering.
@^system dependencies@>

@p procedure open_input; {prepare to read |web_file| and |change_file|}
begin
reset(web_file,web_name); reset(change_file,chg_name);
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [26] Open output files (except for the pool file).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The following code opens |Pascal_file| and |pool|.
Since these files were listed in the program header, we assume that the
\PASCAL\ runtime system has checked that suitable external file names have
been given.
@^system dependencies@>

@<Set init...@>=
rewrite(Pascal_file); rewrite(pool);
@y
@ The following code opens |Pascal_file| and |pool|.
Use the |scan_args| procedure to fill the global file names,
according to the names given on the command line.
@^system dependencies@>

@<Set init...@>=
scan_args;
rewrite(Pascal_file,pascal_file_name);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [28] Fix f^.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Fix `jump_out'.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; jump_out;
  end

@<Error handling...@>=
procedure jump_out;
begin goto end_of_TANGLE;
end;
@y
@d jump_out==uexit(1)
@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; uexit(1);
  end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38] Provide for a larger `byte_mem' and `tok_mem'.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x Extra capacity:
@d ww=2 {we multiply the byte capacity by approximately this amount}
@d zz=3 {we multiply the token capacity by approximately this amount}
@y
@d ww=3 {we multiply the byte capacity by approximately this amount}
@d zz=4 {we multiply the token capacity by approximately this amount}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [63] Remove conversion to uppercase
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
    begin if c>="a" then c:=c-@'40; {merge lowercase with uppercase}
@y
    begin 
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [64] Delayed pool file opening.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
  if string_ptr = 256 then  rewrite(pool,pool_file_name);
  equiv[p]:=string_ptr+@'100000;
  l:=l-double_chars-1;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [105] Accept DIV, div, MOD, and mod
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
 (((out_contrib[1]="D")and(out_contrib[2]="I")and(out_contrib[3]="V")) or@|
 ((out_contrib[1]="M")and(out_contrib[2]="O")and(out_contrib[3]="D")) ))or@|
@^uppercase@>
@y
  (((out_contrib[1]="D")and(out_contrib[2]="I")and(out_contrib[3]="V")) or@|
  ((out_contrib[1]="d")and(out_contrib[2]="i")and(out_contrib[3]="v")) or@|
  ((out_contrib[1]="M")and(out_contrib[2]="O")and(out_contrib[3]="D")) or@|
  ((out_contrib[1]="m")and(out_contrib[2]="o")and(out_contrib[3]="d")) ))or@|
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [110] lowercase ids
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [114] lowercase operators (`and', `or', etc.)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [116] Remove conversion to uppercase
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Fix casting bug
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d add_in(#)==begin accumulator:=accumulator+next_sign*(#); next_sign:=+1;
  end
@y
@d add_in(#)==begin accumulator:=accumulator+next_sign*toint(#); next_sign:=+1;
  end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [179] make term_in = input
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
any error stop will set |debug_cycle| to zero.
@y
any error stop will set |debug_cycle| to zero.

@d term_in==stdin
@z

@x
@!term_in:text_file; {the user's terminal as an input file}
@y

@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [180] remove term_in reset
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
reset(term_in,'TTY:','/I'); {open |term_in| as the terminal, don't do a |get|}
@y

@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [182] write newline just before exit; use value of |history|
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
We have defined plenty of procedures, and it is time to put the last
pieces of the puzzle in place. Here is where \.{TANGLE} starts, and where
it ends.
@^system dependencies@>
@y
We have defined plenty of procedures, and it is time to put the last
pieces of the puzzle in place. Here is where \.{TANGLE} starts, and where
it ends.
@^system dependencies@>
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [188] system dependent changes--the |scan_args| procedure.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
This module should be replaced, if necessary, by changes to the program
that are necessary to make \.{TANGLE} work at a particular installation.
It is usually best to design your change file so that all changes to
previous modules preserve the module numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new modules, can be inserted here; then only the index
itself will get a new module number.
@^system dependencies@>
@y
This module should be replaced, if necessary, by changes to the program
that are necessary to make \.{TANGLE} work at a particular installation.
It is usually best to design your change file so that all changes to
previous modules preserve the module numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new modules, can be inserted here; then only the index
itself will get a new module number.
@^system dependencies@>

@ The user calls \.{TANGLE} with arguments on the command line.  These
are either file names or flags (beginning with `\.-').  The following
globals are for communicating the user's desires to the rest of the
program. The various filename variables contain strings with the full
names of those files, as {\mc UNIX} knows them.

There are no flags that affect \.{TANGLE} at the moment.

@d max_file_name_length==PATH_MAX

@<Globals...@>=
@!web_name,@!chg_name,@!pascal_file_name,@!pool_file_name:
        array[1..max_file_name_length] of char;

@ The |scan_args| procedure looks at the command line arguments and sets
the |file_name| variables accordingly.  At least one file name must be
present: the \.{WEB} file.  It may have an extension, or it may omit it
to get |'.web'| added.  The \PASCAL\ output file name is formed by
replacing the \.{WEB} file name extension by |'.p'|.  Similarly, the
pool file name is formed using a |'.pool'| extension.

If there is another file name present among the arguments, it is the
change file, again either with an extension or without one to get
|'.ch'| An omitted change file argument means that |'/dev/null'| should
be used, when no changes are desired.

@<Declaration of |scan_args|@>=
procedure scan_args;
  var dot_pos, slash_pos, i, a: integer; {indices}
  c: char;
  @!fname: array[1..max_file_name_length] of char; {temporary argument holder}
  @!found_web,@!found_change: boolean; {|true| when those file names have
                                        been seen}
begin
  found_web := false;
  found_change := false;

  for a := 1 to argc - 1
  do begin
    argv(a,fname); {put argument number |a| into |fname|}
    if fname[1] <> '-'
    then begin
      if not found_web
      then @<Get |web_name|, |pascal_file_name|,
             and |pool_file_name| variables from |fname|@>
      else if not found_change
      then @<Get |chg_name| from |fname|@>
      else  @<Print usage error message and quit@>;
    end else
      @<Handle flag argument in |fname|@>;
  end;
    
  if not found_web then @<Print usage error message and quit@>;
  if not found_change then @<Set up null change file@>;
end;

@ Use all of |fname| for the |web_name| if there is a |'.'| in it,
otherwise add |'.web'|.  The other file names come from adding things
after the dot.  The |argv| procedure will not put more than
|max_file_name_length-5| characters into |fname|, and this leaves enough
room in the |file_name| variables to add the extensions.

The end of a file name is marked with a |' '|, the convention assumed by 
the |reset| and |rewrite| procedures.

@<Get |web_name|...@>=
begin
  dot_pos := -1;
  slash_pos := -1;
  i := 1;
  while (fname[i] <> ' ') and (i <= max_file_name_length - 5)
  do begin
    web_name[i] := fname[i];
    if fname[i] = '.' then dot_pos := i;
    if fname[i] = '/' then slash_pos := i;
    incr (i);
  end;
  web_name[i] := ' ';
  
  if (dot_pos = -1) or (dot_pos < slash_pos)
  then begin
    dot_pos := i;
    web_name[dot_pos] :=   '.';
    web_name[dot_pos+1] := 'w';
    web_name[dot_pos+2] := 'e';
    web_name[dot_pos+3] := 'b';
    web_name[dot_pos+4] := ' ';
  end;

  for i := 1 to dot_pos
  do begin
    c := web_name[i];
    pascal_file_name[i] := c;
    pool_file_name[i] := c;
  end;

  pascal_file_name[dot_pos+1] := 'p';
  pascal_file_name[dot_pos+2] := ' ';

  pool_file_name[dot_pos+1] := 'p';
  pool_file_name[dot_pos+2] := 'o';
  pool_file_name[dot_pos+3] := 'o';
  pool_file_name[dot_pos+4] := 'l';
  pool_file_name[dot_pos+5] := ' ';

  found_web := true;
end

@ @<Get |chg_name|...@>=
begin
  dot_pos := -1;
  slash_pos := -1;
  i := 1;
  while (fname[i] <> ' ') and (i <= max_file_name_length - 5)
  do begin
    chg_name[i] := fname[i];
    if fname[i] = '.' then dot_pos := i;
    if fname[i] = '/' then slash_pos := i;
    incr (i);
  end;
  chg_name[i] := ' ';

  if (dot_pos = -1) or (dot_pos < slash_pos)
  then begin
    dot_pos := i;
    chg_name[dot_pos]   := '.';
    chg_name[dot_pos+1] := 'c';
    chg_name[dot_pos+2] := 'h';
    chg_name[dot_pos+3] := ' ';
  end;

  found_change := true;
end

@ @<Set up null...@>=
begin
        chg_name[1]:='/';
        chg_name[2]:='d';
        chg_name[3]:='e';
        chg_name[4]:='v';
        chg_name[5]:='/';
        chg_name[6]:='n';
        chg_name[7]:='u';
        chg_name[8]:='l';
        chg_name[9]:='l';
        chg_name[10]:=' ';
end

@ There are no flags currently used by \.{TANGLE}, but this module can be
used as a hook to introduce flags.

@<Handle flag...@>=
begin
  @<Print usage error message and quit@>;
end

@ @<Print usage error message and quit@>=
begin
  print_ln ('Usage: tangle webfile[.web] [changefile[.ch]].');
  uexit (1);
end
@z
