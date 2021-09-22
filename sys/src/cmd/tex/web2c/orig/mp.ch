% mp.ch for C compilation with web2c.
%
% Copyright 1990 - 1995 by AT&T Bell Laboratories.
%
% Permission to use, copy, modify, and distribute this software
% and its documentation for any purpose and without fee is hereby
% granted, provided that the above copyright notice appear in all
% copies and that both that the copyright notice and this
% permission notice and warranty disclaimer appear in supporting
% documentation, and that the names of AT&T Bell Laboratories or
% any of its entities not be used in advertising or publicity
% pertaining to distribution of the software without specific,
% written prior permission.
%
% Derived from mf.ch and John Hobby's mp.ch (the change file for the C
% version of mf).
% 
% Numbers of the form [pp.nnn] refer to the corresponding part and
% module number of mp.web as implementors who read this change file are
% much better advised to use a current WEB listing of MetaPost.
%
% (more recent changes in ChangeLog)
% Revision 0.62  Jan 4 '95  by John Hobby
% (Changes that only affect the banner line are not listed here)
%
% Updated for web2c-6.1/kpathsea-2.6  Jan 29 '95  by Ulrik Vieth
% according to the current version of mf.ch.
%
% - produce separate binaries for INIMP, VIRMF (no '-I' flag!)
% - make a big version of MP by default (cf. TeX and METAFONT)
% - allow any input characters (0..255) (cf. TeX and METAFONT)
% - use web2c/kpathsea routines in file opening and searching
%
% Updated for Revision 0.63  Apr 12 '95  by Ulrik Vieth
%
% - fixed the parsing routines for ps_tab_file to be able to handle 
%   comments and blank lines in dvipsk's version of psfonts.map.
% - increased max_read_files to 30 to avoid running out of read files
%   when reading stops before reaching EOF. (Better solution: write 
%   a 'closefrom' macro that reads to EOF, which causes file closing.)

@x [0] WEAVE: print changes only.
\def\botofcontents{\vskip 0pt plus 1fil minus 1.5in}
@y
\def\botofcontents{\vskip 0pt plus 1fil minus 1.5in}
\let\maybe=\iffalse
\def\title{\MP\ changes for C}
\def\glob{13}\def\gglob{20, 25} % these are defined in module 1
\font\mc=cmr9
@z

@x [1.7] Convert `debug..gubed' and `stat..tats' into #ifdefs.
@d debug==@{ {change this to `$\\{debug}\equiv\null$' when debugging}
@d gubed==@t@>@} {change this to `$\\{gubed}\equiv\null$' when debugging}
@y
@d debug==ifdef('TEXMF_DEBUG')
@d gubed==endif('TEXMF_DEBUG')
@z

@x
@d stat==@{ {change this to `$\\{stat}\equiv\null$' when gathering
  usage statistics}
@d tats==@t@>@} {change this to `$\\{tats}\equiv\null$' when gathering
  usage statistics}
@y
@d stat==ifdef('STAT')
@d tats==endif('STAT')
@z

@x [1.8] Same, for `init..tini'.
@d init== {change this to `$\\{init}\equiv\.{@@\{}$' in the production version}
@d tini== {change this to `$\\{tini}\equiv\.{@@\}}$' in the production version}
@y
@d init==ifdef('INIMP')
@d tini==endif('INIMP')
@z

% [1.11] Compile-time constants.  Although we only change a few of
% these, listing them all makes the patch file for a big MetaPost simpler.
% 16K for BSD I/O; file_name_size is set from the system constant.
@x
@<Constants...@>=
@!mem_max=30000; {greatest index in \MP's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMP}, otherwise |>=mem_top|}
@!max_internal=100; {maximum number of internal quantities}
@!buf_size=500; {maximum number of characters simultaneously present in
  current lines of open files; must not exceed |max_halfword|}
@!error_line=72; {width of context lines on terminal error messages}
@!half_error_line=42; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line=79; {width of longest text lines output; should be at least 60}
@!emergency_line_length=255;
  {\ps\ output lines can be this long in unusual circumstances}
@!stack_size=30; {maximum number of simultaneous input sources}
@!max_read_files=4; {maximum number of simultaneously open \&{readfrom} files}
@!max_strings=2500; {maximum number of strings; must not exceed |max_halfword|}
@!string_vacancies=9000; {the minimum number of characters that should be
  available for the user's identifier names and strings,
  after \MP's own error messages are stored}
@!strings_vacant=1000; {the minimum number of strings that should be available}
@!pool_size=32000; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all identifiers;
  must exceed |string_vacancies| by the total
  length of \MP's own strings, which is currently about 22000}
@!font_max=50; {maximum font number for included text fonts}
@!font_mem_size=10000; {number of words for \.{TFM} information for text fonts}
@!file_name_size=40; {file names shouldn't be longer than this}
@!pool_name='MPlib:MP.POOL                         ';
  {string of length |file_name_size|; tells where the string pool appears}
@.MPlib@>
@!ps_tab_name='MPlib:PSFONTS.MAP                     ';
  {string of length |file_name_size|; locates font name translation table}
@!path_size=300; {maximum number of knots between breakpoints of a path}
@!bistack_size=785; {size of stack for bisection algorithms;
  should probably be left at this value}
@!header_size=100; {maximum number of \.{TFM} header words, times~4}
@!lig_table_size=5000; {maximum number of ligature/kern steps, must be
  at least 255 and at most 32510}
@!max_kerns=500; {maximum number of distinct kern amounts}
@!max_font_dimen=50; {maximum number of \&{fontdimen} parameters}
@y
@d file_name_size == maxint
@d ssup_error_line = 255
@d ssup_max_strings = 32767 {max value allowed by \.{TANGLE}}

@<Constants...@>=
@!max_internal=300; {maximum number of internal quantities}
@!buf_size=3000; {maximum number of characters simultaneously present in
  current lines of open files; must not exceed |max_halfword|}
@!emergency_line_length=255;
  {\ps\ output lines can be this long in unusual circumstances}
@!stack_size=300; {maximum number of simultaneous input sources}
@!max_read_files=30; {maximum number of simultaneously open \&{readfrom} files}
@!strings_vacant=1000; {the minimum number of strings that should be available}
@!font_max=50; {maximum font number for included text fonts}
@!font_mem_size=10000; {number of words for \.{TFM} information for text fonts}
@!pool_name='mp.pool';
  {string of length |file_name_size|; tells where the string pool appears}
@.MPlib@>
@!ps_tab_name='psfonts.map';
  {string of length |file_name_size|; locates font name translation table}
@!path_size=1000; {maximum number of knots between breakpoints of a path}
@!bistack_size=1500; {size of stack for bisection algorithms;
  should probably be left at this value}
@!header_size=100; {maximum number of \.{TFM} header words, times~4}
@!lig_table_size=15000; {maximum number of ligature/kern steps, must be
  at least 255 and at most 32510}
@!max_kerns=2500; {maximum number of distinct kern amounts}
@!max_font_dimen=50; {maximum number of \&{fontdimen} parameters}
@#
@!inf_main_memory = 2999;
@!sup_main_memory = 8000000;

@!inf_max_strings = 2500;
@!sup_max_strings = ssup_max_strings;

@!inf_pool_size = 32000;
@!sup_pool_size = 10000000;
@!inf_pool_free = 1000;
@!sup_pool_free = sup_pool_size;
@!inf_string_vacancies = 8000;
@!sup_string_vacancies = sup_pool_size - 23000;
@z

@x [1.12] Constants defined as WEB macros.
@d mem_min=0 {smallest index in the |mem| array, must not be less
  than |min_halfword|}
@d mem_top==30000 {largest index in the |mem| array dumped by \.{INIMP};
  must be substantially larger than |mem_min|
  and not greater than |mem_max|}
@d hash_size=2100 {maximum number of symbolic tokens,
  must be less than |max_halfword-3*param_size|}
@d hash_prime=1777 {a prime number equal to about 85\pct! of |hash_size|}
@d max_in_open=6 {maximum number of input files and error insertions that
  can be going on simultaneously}
@d param_size=150 {maximum number of simultaneous macro parameters}
@d max_write_files=4 {maximum number of simultaneously open \&{write} files}
@y
@d mem_min=0 {smallest index in the |mem| array, must not be less
  than |min_halfword|}
@d hash_size=9500 {maximum number of symbolic tokens,
  must be less than |max_halfword-3*param_size|}
@d hash_prime=7919 {a prime number equal to about 85\pct! of |hash_size|}
@d max_in_open=15 {maximum number of input files and error insertions that
  can be going on simultaneously}
@d param_size=150 {maximum number of simultaneous macro parameters}
@d max_write_files=4 {maximum number of simultaneously open \&{write} files}
@z

@x [1.13] Global parameters that can be changed in texmf.cnf.
@<Glob...@>=
@!bad:integer; {is some ``constant'' wrong?}
@y
@<Glob...@>=
@!bad:integer; {is some ``constant'' wrong?}
@#
@!init
@!ini_version:boolean; {are we \.{INIMP}? Set in \.{lib/texmfmp.c}}
@!dump_option:boolean; {was the dump name option used?}
@!dump_line:boolean; {was a \.{\%\AM mem} line seen?}
tini@/
@#
@!bound_default:integer; {temporary for setup}
@!bound_name:^char; {temporary for setup}
@#
@!main_memory:integer; {total memory words allocated in initex}
@!mem_top:integer; {largest index in the |mem| array dumped by \.{INIMP};
  must be substantially larger than |mem_min|
  and not greater than |mem_max|}
@!mem_max:integer; {greatest index in \MP's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMP}, otherwise |>=mem_top|}
@!error_line:integer; {width of context lines on terminal error messages}
@!half_error_line:integer; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line:integer; {width of longest text lines output;
  should be at least 60}
@!pool_size:integer; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all identifiers;
  must exceed |string_vacancies| by the total
  length of \MP's own strings, which is currently about 22000}
@!string_vacancies:integer; {the minimum number of characters that should be
  available for the user's identifier names and strings,
  after \MP's own error messages are stored}
@!pool_free:integer;{minimum pool space free after format loaded}
@!max_strings:integer; {maximum number of strings; must not exceed |max_halfword|}
@z

@x [1.16] Use C macros for `incr' and `decr'.
@d incr(#) == #:=#+1 {increase a variable by unity}
@d decr(#) == #:=#-1 {decrease a variable by unity}
@y
@z

% [2.19] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x [2.22] Allow any character as input.
@^character set dependencies@>
@^system dependencies@>

@<Set init...@>=
for i:=0 to @'37 do xchr[i]:=' ';
for i:=@'177 to @'377 do xchr[i]:=' ';
@y
@^character set dependencies@>
@^system dependencies@>

@d tab = @'11 { ASCII horizontal tab }
@d form_feed = @'14 { ASCII form feed }

@<Set init...@>=
for i:=0 to @'37 do xchr[i]:=chr(i);
for i:=@'177 to @'377 do xchr[i]:=chr(i);
@z

% [3.25] Declare name_of_file as a C string.  See comments in tex.ch for
% why we change the element type to text_char.
@x
@!name_of_file:packed array[1..file_name_size] of char;@;@/
  {on some systems this may be a \&{record} variable}
@y
@!name_of_file:^text_char;
@z

@x [3.26] Do file opening in C.
@ The \ph\ compiler with which the original version of \MF\ was prepared
extends the rules of \PASCAL\ in a very convenient way. To open file~|f|,
we can write
$$\vbox{\halign{#\hfil\qquad&#\hfil\cr
|reset(f,@t\\{name}@>,'/O')|&for input;\cr
|rewrite(f,@t\\{name}@>,'/O')|&for output.\cr}}$$
The `\\{name}' parameter, which is of type `\ignorespaces|packed
array[@t\<\\{any}>@>] of text_char|', stands for the name of
the external file that is being opened for input or output.
Blank spaces that might appear in \\{name} are ignored.

The `\.{/O}' parameter tells the operating system not to issue its own
error messages if something goes wrong. If a file of the specified name
cannot be found, or if such a file cannot be opened for some other reason
(e.g., someone may already be trying to write the same file), we will have
|@!erstat(f)<>0| after an unsuccessful |reset| or |rewrite|.  This allows
\MP\ to undertake appropriate corrective action.
@:PASCAL H}{\ph@>
@^system dependencies@>

\MP's file-opening procedures return |false| if no file identified by
|name_of_file| could be opened.

@d reset_OK(#)==erstat(#)=0
@d rewrite_OK(#)==erstat(#)=0

@p function a_open_in(var @!f:alpha_file):boolean;
  {open a text file for input}
begin reset(f,name_of_file,'/O'); a_open_in:=reset_OK(f);
end;
@#
function a_open_out(var @!f:alpha_file):boolean;
  {open a text file for output}
begin rewrite(f,name_of_file,'/O'); a_open_out:=rewrite_OK(f);
end;
@#
function b_open_in(var @!f:byte_file):boolean;
  {open a binary file for input}
begin reset(f,name_of_file,'/O'); b_open_in:=reset_OK(f);
end;
@#
function b_open_out(var @!f:byte_file):boolean;
  {open a binary file for output}
begin rewrite(f,name_of_file,'/O'); b_open_out:=rewrite_OK(f);
end;
@#
function w_open_in(var @!f:word_file):boolean;
  {open a word file for input}
begin reset(f,name_of_file,'/O'); w_open_in:=reset_OK(f);
end;
@#
function w_open_out(var @!f:word_file):boolean;
  {open a word file for output}
begin rewrite(f,name_of_file,'/O'); w_open_out:=rewrite_OK(f);
end;
@y
@ All of the file opening functions are defined in C.
@d no_file_path = -1
@z

@x [3.27] Do file closing in C.
@ Files can be closed with the \ph\ routine `|close(f)|', which
@^system dependencies@>
should be used when all input or output with respect to |f| has been completed.
This makes |f| available to be opened again, if desired; and if |f| was used for
output, the |close| operation makes the corresponding external file appear
on the user's area, ready to be read.

@p procedure a_close(var @!f:alpha_file); {close a text file}
begin close(f);
end;
@#
procedure b_close(var @!f:byte_file); {close a binary file}
begin close(f);
end;
@#
procedure w_close(var @!f:word_file); {close a word file}
begin close(f);
end;
@y
@ And all the file closing routines as well.
@z

@x [3.30] Do `input_ln' in C.
Standard \PASCAL\ says that a file should have |eoln| immediately
before |eof|, but \MP\ needs only a weaker restriction: If |eof|
occurs in the middle of a line, the system function |eoln| should return
a |true| result (even though |f^| will be undefined).

@p function input_ln(var @!f:alpha_file;@!bypass_eoln:boolean):boolean;
  {inputs the next line or returns |false|}
var @!last_nonblank:0..buf_size; {|last| with trailing blanks removed}
begin if bypass_eoln then if not eof(f) then get(f);
  {input the first character of the line into |f^|}
last:=first; {cf.\ Matthew 19\thinspace:\thinspace30}
if eof(f) then input_ln:=false
else  begin last_nonblank:=first;
  while not eoln(f) do
    begin if last>=max_buf_stack then
      begin max_buf_stack:=last+1;
      if max_buf_stack=buf_size then
        @<Report overflow of the input buffer, and abort@>;
      end;
    buffer[last]:=xord[f^]; get(f); incr(last);
    if buffer[last-1]<>" " then last_nonblank:=last;
    end;
  last:=last_nonblank; input_ln:=true;
  end;
end;
@y
We define |input_ln| in C, for efficiency.  Nevertheless we quote the module
`Report overflow of the input buffer, and abort' here in order to make
\.{WEAVE} happy.

@p @{ @<Report overflow of the input buffer, and abort@> @}
@z

@x [3.31] `term_in' and `term_out' are standard input and output.
@<Glob...@>=
@!term_in:alpha_file; {the terminal as an input file}
@!term_out:alpha_file; {the terminal as an output file}
@y
@d term_in==stdin {the terminal as an input file}
@d term_out==stdout {the terminal as an output file}
@z

@x [3.32] We don't need to open the terminal files.
@ Here is how to open the terminal files
in \ph. The `\.{/I}' switch suppresses the first |get|.
@^system dependencies@>

@d t_open_in==reset(term_in,'TTY:','/O/I') {open the terminal for text input}
@d t_open_out==rewrite(term_out,'TTY:','/O') {open the terminal for text output}
@y
@ Here is how to open the terminal files.  |t_open_out| does nothing.
|t_open_in|, on the other hand, does the work of ``rescanning,'' or getting
any command line arguments the user has provided.  It's defined in C.
  
@d t_open_out == {output already open for text output}
@z

@x [3.33] Flushing output.
these operations can be specified in \ph:
@^system dependencies@>

@d update_terminal == break(term_out) {empty the terminal output buffer}
@d clear_terminal == break_in(term_in,true) {clear the terminal input buffer}
@y
these operations can be specified with {\mc UNIX}.  |update_terminal|
does an |fflush| (via the macro |flush|). |clear_terminal| is redefined
to do nothing, since the user should control the terminal.
@^system dependencies@>

@d update_terminal == fflush(term_out)
@d clear_terminal == do_nothing
@z

@x [3.36] Reading the command line.
@ The following program does the required initialization
without retrieving a possible command line.
It should be clear how to modify this routine to deal with command lines,
if the system permits them.
@^system dependencies@>

@p function init_terminal:boolean; {gets the terminal input started}
label exit;
begin t_open_in;
loop@+begin wake_up_terminal; write(term_out,'**'); update_terminal;
@.**@>
  if not input_ln(term_in,true) then {this shouldn't happen}
    begin write_ln(term_out);
    write(term_out,'! End of file on the terminal... why?');
@.End of file on the terminal@>
    init_terminal:=false; return;
    end;
  loc:=first;
  while (loc<last)and(buffer[loc]=" ") do incr(loc);
  if loc<last then
    begin init_terminal:=true;
    return; {return unless the line was all blank}
    end;
  write_ln(term_out,'Please type the name of your input file.');
  end;
exit:end;
@y
@ The following program does the required initialization.
Iff anything has been specified on the command line, then |t_open_in|
will return with |last > first|.
@^system dependencies@>

@p
function init_terminal:boolean; {gets the terminal input started}
label exit;
begin
    t_open_in;
    if last > first then begin
        loc := first;
        while (loc < last) and (buffer[loc]=' ') do
	    incr(loc);
        if loc < last then begin
            init_terminal := true;
            goto exit;
        end;
    end;
    loop@+begin
        wake_up_terminal; write(term_out, '**'); update_terminal;
@.**@>
        if not input_ln(term_in,true) then begin {this shouldn't happen}
            write_ln(term_out);
            write(term_out, '! End of file on the terminal... why?');
@.End of file on the terminal@>
            init_terminal:=false;
	    return;
        end;

        loc:=first;
        while (loc<last)and(buffer[loc]=" ") do
            incr(loc);

        if loc<last then begin
           init_terminal:=true;
           return; {return unless the line was all blank}
        end;
        write_ln(term_out, 'Please type the name of your input file.');
    end;
exit:
end;
@z

@x [4.38] Dynamically allocate pool arrays.
@!str_pool:packed array[pool_pointer] of pool_ASCII_code; {the characters}
@!str_start : array[str_number] of pool_pointer; {the starting pointers}
@!next_str : array[str_number] of str_number; {for linking strings in order}
@y
@!str_pool:^pool_ASCII_code; {the characters}
@!str_start : ^pool_pointer; {the starting pointers}
@!next_str : ^str_number; {for linking strings in order}
@z

@x [4.44] One more array.
@!str_ref:array[str_number] of 0..max_str_ref;
@y
@!str_ref:^str_ref_type; {web2c only does |^identifier|}
@z

% [4.66] Open the pool file using a path, and can't do string
% assignments directly.  (`strcpy' and `strlen' work here because
% `pool_name' is a constant string, and thus ends in a null and doesn't
% start with a space.)
@x
name_of_file:=pool_name; {we needn't set |name_length|}
if a_open_in(pool_file) then
@y
name_length := strlen (pool_name);
name_of_file := xmalloc (1 + name_length + 1);
strcpy (name_of_file+1, pool_name); {copy the string}
if a_open_in (pool_file, kpse_mppool_format) then
@z

@x [4.66,67,68] Make `MP.POOL' lowercase in messages.
else  bad_pool('! I can''t read MP.POOL.')
@y
else  bad_pool('! I can''t read mp.pool; bad path?')
@z
@x
begin if eof(pool_file) then bad_pool('! MP.POOL has no check sum.');
@.MP.POOL has no check sum@>
read(pool_file,m,n); {read two digits of string length}
@y
begin if eof(pool_file) then bad_pool('! mp.pool has no check sum.');
@.MP.POOL has no check sum@>
read(pool_file,m); read(pool_file,n); {read two digits of string length}
@z
@x
    bad_pool('! MP.POOL line doesn''t begin with two digits.');
@y
    bad_pool('! mp.pool line doesn''t begin with two digits.');
@z
@x
  bad_pool('! MP.POOL check sum doesn''t have nine digits.');
@y
  bad_pool('! mp.pool check sum doesn''t have nine digits.');
@z
@x
done: if a<>@$ then bad_pool('! MP.POOL doesn''t match; TANGLE me again.');
@y
done: if a<>@$ then
  bad_pool('! mp.pool doesn''t match; tangle me again (or fix the path).');
@z

@x [5.69] error_line is a variable, so can't be a subrange array bound
@!trick_buf:array[0..error_line] of ASCII_code; {circular buffer for
@y
@!trick_buf:array[0..ssup_error_line] of ASCII_code; {circular buffer for
@z

@x [5.74/75] l.1674 -- simplify |print| and remove |slow_print|
|print_char("c")| is quicker, so \MP\ goes directly to the |print_char|
routine when it knows that this is safe. (The present implementation
assumes that it is always safe to print a visible ASCII character.)
@^system dependencies@>
@y
|print_char("c")| is quicker, so \MP\ could go directly to the |print_char|
routine when it knows that this is safe. (The present implementation
assumes that it is always safe to print a visible ASCII character.)
@^system dependencies@>

Old versions of \MP\ needed a procedure called |slow_print| whose function
is now subsumed by |print|. We retain the old name here as a possible aid
to future software arch\ae ologists.

@d slow_print == print
@z

@x [5.76] Print rest of banner, eliminate misleading `no preloaded'
@ By popular demand, \MP\ prints the banner line only on the transcript file.
Thus there is nothing special to be printed here.

@<Initialize the output...@>=
update_terminal;
@y
@ Here is the very first thing that \MP\ prints: a headline that identifies
the version number and base name. The |term_offset| variable is temporarily
incorrect, but the discrepancy is not serious since we assume that the banner
and mem identifier together will occupy at most |max_print_line|
character positions.

@<Initialize the output...@>=
wterm (banner);
wterm (version_string);
if mem_ident>0 then slow_print(mem_ident); print_ln;
update_terminal;
@z

@x [6.83] l.1815 - Add unspecified_mode.
@d error_stop_mode=3 {stops at every opportunity to interact}
@y
@d error_stop_mode=3 {stops at every opportunity to interact}
@d unspecified_mode=4 {extra value for command-line switch}
@z

@x [6.83] l.1822 - Add interaction_option.
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@y
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@!interaction_option:batch_mode..unspecified_mode; {set from command line}
@z

@x [6.84] l.1824 - Allow override by command line switch.
@ @<Set init...@>=interaction:=error_stop_mode;
@y
@ @<Set init...@>=if interaction_option=unspecified_mode then
  interaction:=error_stop_mode
else
  interaction:=interaction_option;
@z

@x [6.90] Eliminate non-local goto.
@<Error hand...@>=
procedure jump_out;
begin goto end_of_MP;
end;
@y
@d do_final_end==begin
   update_terminal;
   ready_already:=0;
   if (history <> spotless) and (history <> warning_issued) then
       uexit(1)
   else
       uexit(0);
   end
@<Error hand...@>=
procedure jump_out;
begin
close_files_and_terminate;
do_final_end;
end;
@z

@x [6.93] Handle the switch-to-editor option.
line ready to be edited. But such an extension requires some system
wizardry, so the present implementation simply types out the name of the
file that should be
edited and the relevant line number.
@^system dependencies@>

There is a secret `\.D' option available when the debugging routines haven't
been commented~out.
@^debugging@>
@y
line ready to be edited.
We do this by calling the external procedure |call_edit| with a pointer to
the filename, its length, and the line number.
However, here we just set up the variables that will be used as arguments,
since we don't want to do the switch-to-editor until after \MP\ has closed
its files.
@^system dependencies@>

There is a secret `\.D' option available when the debugging routines have
not been commented out.
@^debugging@>
@d edit_file==input_stack[file_ptr]
@z
@x
"E": if file_ptr>0 then
  begin print_nl("You want to edit file ");
@.You want to edit file x@>
  print(input_stack[file_ptr].name_field);
  print(" at line "); print_int(true_line);@/
  interaction:=scroll_mode; jump_out;
@y
"E": if file_ptr>0 then
    begin
    edit_name_start:=str_start[edit_file.name_field];
    edit_name_length:=length(edit_file.name_field);
    edit_line:=true_line;
    jump_out;
@z

@x [7.111] Do half and halfp in cpascal.h.
@d half(#)==(#) div 2
@d halfp(#)==(#) div 2
@y
@z

@x [7.122-7.130] Optionally replace make_fraction etc. with external routines.
@p function make_fraction(@!p,@!q:integer):fraction;
@y
In the C version, there are external routines that use double precision
floating point to simulate functions such as |make_fraction|.  This is carefully
done to be virtually machine-independent and it gives up to 12 times speed-up
on machines with hardware floating point.  Since some machines do not have fast
double-precision floating point, we provide a C preprocessor switch that allows
selecting the standard versions given below.

@p ifdef('FIXPT')@/
function make_fraction(@!p,@!q:integer):fraction;
@z
@x
  if negative then make_fraction:=-(f+n)@+else make_fraction:=f+n;
  end;
end;
@y
  if negative then make_fraction:=-(f+n)@+else make_fraction:=f+n;
  end;
end;@/
endif('FIXPT')
@z
@x
@p function take_fraction(@!q:integer;@!f:fraction):integer;
@y
@p ifdef('FIXPT')@/
function take_fraction(@!q:integer;@!f:fraction):integer;
@z
@x
else take_fraction:=n+p;
end;
@y
else take_fraction:=n+p;
end;@/
endif('FIXPT')
@z
@x
@p function take_scaled(@!q:integer;@!f:scaled):integer;
@y
@p ifdef('FIXPT')@/
function take_scaled(@!q:integer;@!f:scaled):integer;
@z
@x
else take_scaled:=n+p;
end;
@y
else take_scaled:=n+p;
end;@/
endif('FIXPT')
@z
@x
operands are positive. \ (This procedure is not used especially often,
so it is not part of \MP's inner loop.)

@p function make_scaled(@!p,@!q:integer):scaled;
@y
operands are positive. \ (This procedure is not used especially often,
so it is not part of \MP's inner loop, but we might as well allow for
an external C routine.)

@p ifdef('FIXPT')@/
function make_scaled(@!p,@!q:integer):scaled;
@z
@x
  if negative then make_scaled:=-(f+n)@+else make_scaled:=f+n;
  end;
end;
@y
  if negative then make_scaled:=-(f+n)@+else make_scaled:=f+n;
  end;
end;@/
endif('FIXPT')
@z

@x [7.134] Do floor_scaled, floor_unscaled, round_unscaled, round_fraction in C.
@p function floor_scaled(@!x:scaled):scaled;
  {$2^{16}\lfloor x/2^{16}\rfloor$}
var @!be_careful:integer; {temporary register}
begin if x>=0 then floor_scaled:=x-(x mod unity)
else  begin be_careful:=x+1;
  floor_scaled:=x+((-be_careful) mod unity)+1-unity;
  end;
end;
@#
function round_unscaled(@!x:scaled):integer;
  {$\lfloor x/2^{16}+.5\rfloor$}
var @!be_careful:integer; {temporary register}
begin if x>=half_unit then round_unscaled:=1+((x-half_unit) div unity)
else if x>=-half_unit then round_unscaled:=0
else  begin be_careful:=x+1;
  round_unscaled:=-(1+((-be_careful-half_unit) div unity));
  end;
end;
@#
function round_fraction(@!x:fraction):scaled;
  {$\lfloor x/2^{12}+.5\rfloor$}
var @!be_careful:integer; {temporary register}
begin if x>=2048 then round_fraction:=1+((x-2048) div 4096)
else if x>=-2048 then round_fraction:=0
else  begin be_careful:=x+1;
  round_fraction:=-(1+((-be_careful-2048) div 4096));
  end;
end;
@y
@z

@x [9.168] Increase memory size.
@d min_quarterword=0 {smallest allowable value in a |quarterword|}
@d max_quarterword=255 {largest allowable value in a |quarterword|}
@d min_halfword==0 {smallest allowable value in a |halfword|}
@d max_halfword==65535 {largest allowable value in a |halfword|}
@y
@d min_quarterword=0 {smallest allowable value in a |quarterword|}
@d max_quarterword=255 {largest allowable value in a |quarterword|}
@d min_halfword==0 {smallest allowable value in a |halfword|}
@d max_halfword==@"FFFFFFF {largest allowable value in a |halfword|}
@z

@x [9.170] Don't bother to subtract zero.
@d ho(#)==#-min_halfword
  {to take a sixteen-bit item from a halfword}
@d qo(#)==#-min_quarterword {to read eight bits from a quarterword}
@d qi(#)==#+min_quarterword {to store eight bits in a quarterword}
@y
@d ho(#)==#
@d qo(#)==#
@d qi(#)==#
@z

@x [9.171] memory_word is defined externally.
@!two_halves = packed record@;@/
  @!rh:halfword;
  case two_choices of
  1: (@!lh:halfword);
  2: (@!b0:quarterword; @!b1:quarterword);
  end;
@!four_quarters = packed record@;@/
  @!b0:quarterword;
  @!b1:quarterword;
  @!b2:quarterword;
  @!b3:quarterword;
  end;
@!memory_word = record@;@/
  case three_choices of
  1: (@!int:integer);
  2: (@!hh:two_halves);
  3: (@!qqqq:four_quarters);
  end;
@y
@=#include "texmfmem.h";@>
@z

@x [9.174] mem is dynamically allocated.
@!mem : array[mem_min..mem_max] of memory_word; {the big dynamic storage area}
@y
@!mem : ^memory_word; {the big dynamic storage area}
@z

@x [10.184] Fix an unsigned/signed problem in getnode.
if r>p+1 then @<Allocate from the top of node |p| and |goto found|@>;
@y
if r>toint(p+1) then @<Allocate from the top of node |p| and |goto found|@>;
@z

% [11.193] Change the word `free' so that it doesn't conflict with the
% standard C library routine of the same name. Also change arrays that
% use mem_max, since that's a variable now, effectively disabling the feature.
@x
are debugging.)

@<Glob...@>=
@!debug @!free: packed array [mem_min..mem_max] of boolean; {free cells}
@t\hskip1em@>@!was_free: packed array [mem_min..mem_max] of boolean;
@y
are debugging.)

@d free==free_arr

@<Glob...@>=
@!debug @!free: packed array [0..1] of boolean; {free cells}
@t\hskip1em@>@!was_free: packed array [0..1] of boolean;
@z

@x [11.197] Eliminate unsigned comparisons to zero.
repeat if (p>=lo_mem_max)or(p<mem_min) then clobbered:=true
  else if (rlink(p)>=lo_mem_max)or(rlink(p)<mem_min) then clobbered:=true
@y
repeat if (p>=lo_mem_max) then clobbered:=true
  else if (rlink(p)>=lo_mem_max) then clobbered:=true
@z

@x [12.212] Do `fix_date_and_time' in C.
@ The following procedure, which is called just before \MP\ initializes its
input and output, establishes the initial values of the date and time.
@^system dependencies@>
Since standard \PASCAL\ cannot provide such information, something special
is needed. The program here simply specifies July 4, 1776, at noon; but
users probably want a better approximation to the truth.

Note that the values are |scaled| integers. Hence \MP\ can no longer
be used after the year 32767.

@p procedure fix_date_and_time;
begin internal[time]:=12*60*unity; {minutes since midnight}
internal[day]:=4*unity; {fourth day of the month}
internal[month]:=7*unity; {seventh month of the year}
internal[year]:=1776*unity; {Anno Domini}
end;
@y
@ The following procedure, which is called just before \MP\ initializes its
input and output, establishes the initial values of the date and time.
It is calls an externally defined |date_and_time|, even though it could
be done from Pascal.
The external procedure also sets up interrupt catching.
@^system dependencies@>

Note that the values are |scaled| integers. Hence \MP\ can no longer
be used after the year 32767.

@p procedure fix_date_and_time;
begin
    date_and_time(internal[time],internal[day],internal[month],internal[year]);
    internal[time] := internal[time] * unity;
    internal[day] := internal[day] * unity;
    internal[month] := internal[month] * unity;
    internal[year] := internal[year] * unity;
end;
@z

@x [12.217] Allow tab and form feed as input.
for k:=127 to 255 do char_class[k]:=invalid_class;
@y
for k:=127 to 255 do char_class[k]:=invalid_class;
char_class[tab]:=space_class;
char_class[form_feed]:=space_class;
@z

@x [35.745] area and extension rules.
@ The file names we shall deal with for illustrative purposes have the
following structure:  If the name contains `\.>' or `\.:', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@^system dependencies@>

We can scan such file names easily by using two global variables that keep track
of the occurrences of area and extension delimiters.  Note that these variables
cannot be of type |pool_pointer| because a string pool compaction could occur
while scanning a file name.

@<Glob...@>=
@!area_delimiter:integer;
  {most recent `\.>' or `\.:' relative to |str_start[str_ptr]|}
@!ext_delimiter:integer; {the relevant `\..', if any}
@y
@ The file names we shall deal with have the
following structure:  If the name contains `\./', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@^system dependencies@>

We can scan such file names easily by using two global variables that keep track
of the occurrences of area and extension delimiters.  Note that these variables
cannot be of type |pool_pointer| because a string pool compaction could occur
while scanning a file name.

@<Glob...@>=
@!area_delimiter:integer; {most recent `\./' relative to |str_start[str_ptr]|}
@!ext_delimiter:integer; {the relevant `\..', if any}
@z

@x [35.746] MP and MF area directories.
@d MP_area=="MPinputs:"
@.MPinputs@>
@d MF_area=="MFinputs:"
@.MFinputs@>
@d MP_font_area=="TeXfonts:"
@.TeXfonts@>
@y
In C, the default paths are specified separately.
@z

@x [35.748] (more_name) Generalize directory separators.
begin if c=" " then more_name:=false
else  begin if (c=">")or(c=":") then
@y
begin if (c=" ")or(c=tab) then more_name:=false
else  begin if IS_DIR_SEP (c) then
@z
@x [still 35.748] Last (not first) . is extension.
  else if (c=".")and(ext_delimiter<0) then
@y
  else if (c=".") then
@z

@x [35.751] (pack_file_name) malloc and null terminate name_of_file.
for j:=str_start[a] to str_stop(a)-1 do append_to_name(so(str_pool[j]));
@y
if name_of_file then libc_free (name_of_file);
name_of_file := xmalloc (1 + length (a) + length (n) + length (e) + 1);
for j:=str_start[a] to str_stop(a)-1 do append_to_name(so(str_pool[j]));
@z
@x
for k:=name_length+1 to file_name_size do name_of_file[k]:=' ';
@y
name_of_file[name_length + 1] := 0;
@z

@x [35.752] default mem area is nonexistent
@d mem_default_length=15 {length of the |MP_mem_default| string}
@d mem_area_length=6 {length of its area part}
@y
@d mem_area_length=0 {no fixed area in C}
@z

@x [35.753] Where `plain.mem' is.
@!MP_mem_default:packed array[1..mem_default_length] of char;

@ @<Set init...@>=
MP_mem_default:='MPlib:plain.mem';
@.MPlib@>
@.plain@>
@^system dependencies@>
@y
@!mem_default_length: integer;
@!MP_mem_default: ^char;
@!troff_mode:boolean; {has the user requested \.{troff} mode?}

@ We set the name of the default format file and the length of that name
in \.{texmfmp.c}, since we want them to depend on the name of the
program.
@z

@x [35.755] Change to pack_buffered_name as with pack_file_name.
for j:=1 to n do append_to_name(xord[MP_mem_default[j]]);
@y
if name_of_file then libc_free (name_of_file);
name_of_file := xmalloc (1 + n + (b - a + 1) + mem_ext_length + 1);
for j:=1 to n do append_to_name(xord[MP_mem_default[j]]);
@z
@x
for k:=name_length+1 to file_name_size do name_of_file[k]:=' ';
@y
name_of_file[name_length + 1] := 0;
@z

@x [35.756] Mem file opening: only try once, with path searching.
  pack_buffered_name(0,loc,j-1); {try first without the system file area}
  if w_open_in(mem_file) then goto found;
  pack_buffered_name(mem_area_length,loc,j-1);
    {now try the system mem file area}
  if w_open_in(mem_file) then goto found;
@y
  pack_buffered_name(0,loc,j-1);
  if w_open_in(mem_file) then goto found;
@z

@x [still 35.756] Replace `PLAIN' in error messages with `default'.
  wterm_ln('Sorry, I can''t find that mem file;',' will try PLAIN.');
@y
  wterm ('Sorry, I can''t find the mem file `');
  fputs (name_of_file + 1, stdout);
  wterm ('''; will try `');
  fputs (MP_mem_default + 1, stdout);
  wterm_ln ('''.');
@z
@x
  wterm_ln('I can''t find the PLAIN mem file!');
@.I can't find PLAIN...@>
@y
  wterm ('I can''t find the mem file `');
  fputs (MP_mem_default + 1, stdout);
  wterm_ln ('''!');
@.I can't find the mem...@>
@z

@x [35.758] Make scan_file_name ignore leading tabs as well as spaces.
@p procedure scan_file_name;
label done;
begin begin_name;
while buffer[loc]=" " do incr(loc);
@y
@p procedure scan_file_name;
label done;
begin begin_name;
while (buffer[loc]=" ")or(buffer[loc]=tab) do incr(loc);
@z

@x [35.760] `logname' is declared in <unistd.h> on some systems.
`\.{.mem}' and `\.{.tfm}' in order to make the names of \MP's output files.
@y
`\.{.mem}' and `\.{.tfm}' in order to make the names of \MP's output files.
@d log_name == texmf_log_name
@z

@x [35.764] <Scan file name...> needs similar leading tab treatment.
@ @<Scan file name in the buffer@>=
begin begin_name; k:=first;
while (buffer[k]=" ")and(k<last) do incr(k);
@y
@ @<Scan file name in the buffer@>=
begin begin_name; k:=first;
while ((buffer[k]=" ")or(buffer[k]=tab))and(k<last) do incr(k);
@z

@x [35.765] Adjust for C string conventions.
@!months:packed array [1..36] of char; {abbreviations of month names}
@y
@!months:^char;
@z

@x [35.767]
begin wlog(banner);
print(mem_ident); print("  ");
print_int(round_unscaled(internal[day])); print_char(" ");
months:='JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@y
begin wlog(banner);
wlog (version_string);
print(mem_ident); print("  ");
print_int(round_unscaled(internal[day])); print_char(" ");
months := ' JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@z

@x [35.768] (try_extension) a_open_in of input file needs path specifier.
begin pack_file_name(cur_name,cur_area,ext);
in_name:=cur_name; in_area:=cur_area;
if a_open_in(cur_file) then try_extension:=true
else begin if str_vs_str(ext,".mf")=0 then in_area:=MF_area
  else in_area:=MP_area;
  pack_file_name(cur_name,in_area,ext);
  try_extension:=a_open_in(cur_file);
  end;
@y
{|cur_ext| will be \.{two} if the filename was \.{one.two}.}
begin pack_file_name(cur_name,cur_area,cur_ext);
in_name:=cur_name; in_area:=cur_area;
if str_vs_str(ext,".mf")=0 then
  try_extension:=a_open_in(cur_file, kpse_mf_format)
else try_extension:=a_open_in(cur_file, kpse_mp_format);
@z

@x [35.770] We need another loop variable.
@p procedure start_input; {\MP\ will \.{input} something}
label done;
@y
@p procedure start_input; {\MP\ will \.{input} something}
label done;
var j:integer;
@z

@x [still 35.770] Kpathsea already tries with no extension.
  if cur_ext="" then
    if try_extension(".mp") then goto done
    else if try_extension("") then goto done
    else if try_extension(".mf") then goto done
    else do_nothing
  else if try_extension(cur_ext) then goto done;
@y
    if try_extension(".mp") then goto done
    else if try_extension(".mf") then goto done
    else do_nothing;
@z

@x [still 35.770] Allow jobname to be `one.two'.
  begin job_name:=cur_name; str_ref[job_name]:=max_str_ref;
@y
  begin
    j:=1;
    begin_name;
    while (j<=name_length)and(more_name(name_of_file[j])) do
      incr(j);
    end_name;
    job_name:=cur_name;
    str_ref[job_name] := max_str_ref;
    init
      if ini_version and dump_option then begin
        str_room(mem_default_length);
        for j:=1 to mem_default_length - mem_ext_length do
          append_char(xord[MP_mem_default[j]]);
        job_name:=make_string;
        str_ref[job_name] := max_str_ref;
      end;
    tini
@z

@x [35.771] Cannot return name to string pool, for the e option?
flush_string(name); name:=cur_name; cur_name:=0
@y
@z

@x [35.774] (copy_old_name) Allocate old_file_name dynamically.
for j:=str_start[s] to str_stop(s)-1 do
@y
if old_file_name then libc_free (old_file_name);
old_file_name := xmalloc (1 + length (s) + 1);
for j:=str_start[s] to str_stop(s)-1 do
@z
@x [still 35.774] Avoid blanking rest of nonexistent array.
for k:=old_name_length+1 to file_name_size do @+old_file_name[k]:=' ';
@y
old_file_name[old_name_length + 1] := 0;
@z

@x [35.775] Declare old_file_name as a regular C string.
@!old_file_name : packed array[1..file_name_size] of char;
@y
@!old_file_name : ^char;
@z

@x [35.776] [Unique to MP] Path selector for |a_open_in| of mpx file.
if not a_open_in(cur_file) then
  begin end_file_reading;
@y
if not a_open_in(cur_file,no_file_path) then
  begin end_file_reading;
@z

@x [35.777] [Unique to MP] Invoke |makempx|.
copy_old_name(name)
{System-dependent code should be added here}
@y
copy_old_name (name);
if not call_make_mpx (old_file_name + 1, name_of_file + 1) then goto not_found
@z

@x [35.778] [Unique to MP] Fix help message for our implementation.
  ("try running it manually through MPtoTeX, TeX, and DVItoMP");
@y
  ("try running it manually through MPto -tex, TeX, and DVItoMP.");
@z

@x [35.782] [Unique to MP] Path selector for |a_open_in| of readfrom file.
if not a_open_in(rd_file[n]) then goto not_found;
@y
if not a_open_in(rd_file[n],no_file_path) then goto not_found;
@z

@x [35.783] The Amiga needs a different open_write_file.
@ Open |wr_file[n]| using file name~|s| and update |wr_fname[n]|.

@p procedure open_write_file(s:str_number; n:readf_index);
begin str_scan_file(s);
pack_cur_name;
while not a_open_out(wr_file[n]) do
  prompt_file_name("file name for write output","");
wr_fname[n]:=s;
add_str_ref(s);
end;
@y
@ Open |wr_file[n]| using file name~|s| and update |wr_fname[n]|.
The Amiga operating system does not permit write access to a file that is
currently opened in read mode.  To avoid disaster, we look for the file to
be opened in the list of |read_from| files and close it if present.

@d amiga==ifdef('AMIGA')
@d agima==endif('AMIGA')
@f amiga==begin
@f agima==end

@p procedure open_write_file(s:str_number; n:readf_index);
amiga@;
label done;
var @!n0:readf_index; {Scratch variable}
agima@;
begin
  str_scan_file(s);
  pack_cur_name;
amiga@;
  for n0:=0 to read_files-1 do begin
    if rd_fname[n0]<>0 then begin
      if str_vs_str(s,rd_fname[n0])=0 then begin
        a_close(rd_file[n0]);
        delete_str_ref(rd_fname[n0]);
        rd_fname[n0]:=0;
        if n0=read_files-1 then read_files:=n0;
        goto done;
      end;
    end;
  end;
done: do_nothing;
agima@;
while not open_out_name_ok(name_of_file+1)
      or not a_open_out(wr_file[n]) do
  prompt_file_name("file name for write output","");
wr_fname[n]:=s;
add_str_ref(s);
{If on first line of input, log file is not ready yet, so don't log.}
if log_opened then begin
  old_setting:=selector;
  if (internal[tracing_online]<=0) then
    selector:=log_only  {Show what we're doing in the log file.}
  else selector:=term_and_log;  {Show what we're doing.}
  print_nl("write");
  print_int(n);
  print(" = `");
  print_file_name(cur_name,cur_area,cur_ext);
  print("'."); print_nl(""); print_ln;
  selector:=old_setting;
end;
end;
@z

@x [41.1040] if batchmode, MakeTeX... scripts should be silent.
mode_command: begin print_ln; interaction:=cur_mod;
@y
mode_command: begin print_ln; interaction:=cur_mod;
if interaction = batch_mode
then kpse_make_tex_discard_errors := 1
else kpse_make_tex_discard_errors := 0;
@z

@x [42.1151] Fix `threshold' conflict with local variable name.
@p function threshold(@!m:integer):scaled;
var @!d:scaled; {lower bound on the smallest interval size}
begin excess:=min_cover(0)-m;
if excess<=0 then threshold:=0
else  begin repeat d:=perturbation;
  until min_cover(d+d)<=m;
  while min_cover(d)>m do d:=perturbation;
  threshold:=d;
  end;
end;
@y
@p function compute_threshold(@!m:integer):scaled;
var @!d:scaled; {lower bound on the smallest interval size}
begin excess:=min_cover(0)-m;
if excess<=0 then compute_threshold:=0
else  begin repeat d:=perturbation;
  until min_cover(d+d)<=m;
  while min_cover(d)>m do d:=perturbation;
  compute_threshold:=d;
  end;
end;
@z

@x [42.1152] Change the call to the threshold function.
begin d:=threshold(m); perturbation:=0;
@y
begin d:=compute_threshold(m); perturbation:=0;
@z

@x [45.1164] Writing the tfm file.
@d tfm_out(#)==write(tfm_file,#) {output one byte to |tfm_file|}

@p procedure tfm_two(@!x:integer); {output two bytes to |tfm_file|}
begin tfm_out(x div 256); tfm_out(x mod 256);
end;
@#
procedure tfm_four(@!x:integer); {output four bytes to |tfm_file|}
begin if x>=0 then tfm_out(x div three_bytes)
else  begin x:=x+@'10000000000; {use two's complement for negative values}
  x:=x+@'10000000000;
  tfm_out((x div three_bytes) + 128);
  end;
x:=x mod three_bytes; tfm_out(x div unity);
x:=x mod unity; tfm_out(x div @'400);
tfm_out(x mod @'400);
end;
@#
procedure tfm_qqqq(@!x:four_quarters); {output four quarterwords to |tfm_file|}
@y
The default definitions don't work. Why not? So use C macros.

@d tfm_out(#) == put_byte (#, tfm_file)
@d tfm_two(#) == put_2_bytes (tfm_file, #)
@d tfm_four(#) == put_4_bytes (tfm_file, #)

@p procedure tfm_qqqq(@!x:four_quarters); {output four quarterwords to |tfm_file|}
@z

% [43.1182] [See TeX module 564] Reading tfm files. As a special case,
% whenever we open a tfm file for input, we read its first byte into
% "tfm_temp" right away.
@x
@d tfget==get(tfm_infile)
@d tfbyte==tfm_infile^
@y
@d tfget==tfm_temp:=getc(tfm_infile)
@d tfbyte==tfm_temp
@z

% [43.1186] [See TeX module 575] We only want `eof' on the TFM file
% to be true if we previously had EOF, not if we're at EOF now.
% This is like `feof', and unlike our implementation of `eof' elsewhere.
@x
if eof(tfm_infile) then goto bad_tfm;
@y
if feof(tfm_infile) then goto bad_tfm;
@z

% [43.1188] [See TeX module 563] TFM file opening.
@x
if cur_area="" then cur_area:=MP_font_area;
if cur_ext="" then cur_ext:=".tfm";
pack_cur_name;
@y
if cur_ext="" then cur_ext:=".tfm";
pack_cur_name;
@z

@x [43.1195] [Unique to MP] Path selector for |a_open_in| of ps_tab_file.
begin name_of_file:=ps_tab_name;
if a_open_in(ps_tab_file) then
@y
begin
name_length := strlen (ps_tab_name);
name_of_file := xmalloc (1 + name_length + 1);
strcpy (name_of_file+1, ps_tab_name); {copy the string}
if a_open_in(ps_tab_file, kpse_dvips_config_format) then
@z

@x [43.1198] Allow blank lines and comment lines in |ps_tab_file|.
@ @<Read at most |lmax| characters from |ps_tab_file| into string |s|...@>=
str_room(lmax);
j:=lmax;
loop @+begin if eoln(ps_tab_file) then
    fatal_error("The psfont map file is bad!");
  read(ps_tab_file,c);
  if c=' ' then goto done;
@y
@ If we encounter the end of line before we have started reading
characters from |ps_tab_file|, we have found an entirely blank 
line and we skip over it.  Otherwise, we abort if the line ends 
prematurely.  If we encounter a comment character, we also skip 
over the line, since recent versions of \.{dvips} allow comments
in the font map file.

@<Read at most |lmax| characters from |ps_tab_file| into string |s|...@>=
str_room(lmax);
j:=lmax;
loop @+begin if eoln(ps_tab_file) then
    if j=lmax then begin flush_cur_string;
      goto common_ending; {skip over blank line}
      end
    else fatal_error("The psfonts.map file is bad!");
  read(ps_tab_file,c);
  if ((c='%')or(c='*')or(c=';')or(c='#')) then begin flush_cur_string;
    goto common_ending; {skip over comment line}
    end;
  if ((c=' ')or(c=tab)) then goto done;
@z

@x [43.1199] Allow tabs as field seperators in |ps_tab_file|.
repeat if eoln(ps_tab_file) then fatal_error("The psfont map file is bad!");
  read(ps_tab_file,c);
until c<>' ';
repeat decr(j);
  if j<0 then fatal_error("The psfont map file is bad!");
  append_char(xord[c]);
  if eoln(ps_tab_file) then c:=' ' @+else read(ps_tab_file,c);
until c=' ';
@y
repeat if eoln(ps_tab_file) then fatal_error("The psfonts.map file is bad!");
  read(ps_tab_file,c);
until ((c<>' ')and(c<>tab));
repeat decr(j);
  if j<0 then fatal_error("The psfonts.map file is bad!");
  append_char(xord[c]);
  if eoln(ps_tab_file) then c:=' ' @+else read(ps_tab_file,c);
until ((c=' ')or(c=tab));
@z

@x [45.1279] INI = VIR.
mem_ident:=" (INIMP)";
@y
if ini_version then mem_ident:=" (INIMP)";
@z

@x [45.1282] Reading and writing of `mem_file' done in C.
@d dump_wd(#)==begin mem_file^:=#; put(mem_file);@+end
@d dump_int(#)==begin mem_file^.int:=#; put(mem_file);@+end
@d dump_hh(#)==begin mem_file^.hh:=#; put(mem_file);@+end
@d dump_qqqq(#)==begin mem_file^.qqqq:=#; put(mem_file);@+end
@y
@z

@x [45.1283]
@d undump_wd(#)==begin get(mem_file); #:=mem_file^;@+end
@d undump_int(#)==begin get(mem_file); #:=mem_file^.int;@+end
@d undump_hh(#)==begin get(mem_file); #:=mem_file^.hh;@+end
@d undump_qqqq(#)==begin get(mem_file); #:=mem_file^.qqqq;@+end
@y
@z

@x [45.1285] Avoid Pascal file convention.
x:=mem_file^.int;
if x<>@$ then goto off_base; {check that strings are the same}
undump_int(x);
if x<>mem_min then goto off_base;
undump_int(x);
if x<>mem_top then goto off_base;
@y
undump_int(x);
if x<>@$ then goto off_base; {check that strings are the same}
undump_int(x);
if x<>mem_min then goto off_base;
{Dynamic allocation \`a la \.{mf.ch}.}
@+init
if ini_version then begin
  {We allocated these at start-up, but now we need to reallocate.}
  libc_free (mem);
  libc_free (str_ref);
  libc_free (next_str);
  libc_free (str_start);
  libc_free (str_pool);
end;
@+tini
undump_int(mem_top);
if mem_max < mem_top then mem_max:=mem_top; {Use at least what we dumped.}
if mem_min+1100>mem_top then goto off_base;
xmalloc_array (mem, mem_max - mem_min);
@z

@x [45.1287] String pool undumping is dynamic.
undump_size(0)(pool_size)('string pool size')(pool_ptr);
undump_size(0)(max_strings-1)('max strings')(max_str_ptr);
@y
undump_size(0)(sup_pool_size-pool_free)('string pool size')(pool_ptr);
if pool_size < pool_ptr + pool_free then
  pool_size := pool_ptr+pool_free;
undump_size(0)(sup_max_strings)('max strings')(max_str_ptr);
@/
xmalloc_array (str_ref, max_strings);
xmalloc_array (next_str, max_strings);
xmalloc_array (str_start, max_strings);
xmalloc_array (str_pool, pool_size);
@z

@x [45.1293] l.22667 - Allow command line to override dumped value.
undump(batch_mode)(error_stop_mode)(interaction);
@y
undump(batch_mode)(error_stop_mode)(interaction);
if interaction_option<>unspecified_mode then interaction:=interaction_option;
@z

@x [45.1293] eof is like feof here.
undump_int(x);@+if (x<>69073)or eof(mem_file) then goto off_base
@y
undump_int(x);@+if (x<>69073)or feof(mem_file) then goto off_base
@z

@x [45.1294] Eliminate probably-wrong word `preloaded' from mem_idents.
print(" (preloaded mem="); print(job_name); print_char(" ");
print_int(round_unscaled(internal[year]) mod 100); print_char(".");
@y
print(" (mem="); print(job_name); print_char(" ");
print_int(round_unscaled(internal[year])); print_char(".");
@z

@x [46.1298] Dynamic allocation.
@p begin @!{|start_here|}
@y
@d const_chk(#) == begin if # < inf@&# then # := inf@&# else
                         if # > sup@&# then # := sup@&# end
{|setup_bound_var| stuff duplicated in \.{tex.ch}.}
@d setup_bound_var(#) == bound_default := #; setup_bound_var_end
@d setup_bound_var_end(#) == bound_name := #; setup_bound_var_end_end
@d setup_bound_var_end_end(#) ==
  setup_bound_variable (address_of (#), bound_name, bound_default);

@p begin @!{|start_here|}
  {See comments in \.{tex.ch} for why the name has to be duplicated.}
  setup_bound_var (250000)('main_memory')(main_memory);
    {|memory_word|s for |mem| in \.{INIMP}}
  setup_bound_var (100000)('pool_size')(pool_size);
  setup_bound_var (75000)('string_vacancies')(string_vacancies);
  setup_bound_var (5000)('pool_free')(pool_free); {min pool avail after fmt}
  setup_bound_var (15000)('max_strings')(max_strings);
  setup_bound_var (79)('error_line')(error_line);
  setup_bound_var (50)('half_error_line')(half_error_line);
  setup_bound_var (79)('max_print_line')(max_print_line);
  if error_line > ssup_error_line then error_line := ssup_error_line;
  
  const_chk (main_memory);
  mem_top := mem_min + main_memory;
  mem_max := mem_top;

  const_chk (pool_size);
  const_chk (string_vacancies);
  const_chk (pool_free);
  const_chk (max_strings);

@+init
if ini_version then begin
  xmalloc_array (mem, mem_top - mem_min);

  xmalloc_array (str_ref, max_strings);
  xmalloc_array (next_str, max_strings);
  xmalloc_array (str_start, max_strings);
  xmalloc_array (str_pool, pool_size);
end;
@+tini
@z

@x [46.1298] Only do get_strings_started etc. if ini.
@!init if not get_strings_started then goto final_end;
init_tab; {initialize the tables}
init_prim; {call |primitive| for each primitive}
init_str_use:=str_ptr; init_pool_ptr:=pool_ptr;@/
max_str_ptr:=str_ptr; max_pool_ptr:=pool_ptr;
fix_date_and_time;
@y
@!init if ini_version then begin
if not get_strings_started then goto final_end;
init_tab; {initialize the tables}
init_prim; {call |primitive| for each primitive}
init_str_use:=str_ptr; init_pool_ptr:=pool_ptr;@/
max_str_ptr:=str_ptr; max_pool_ptr:=pool_ptr;
fix_date_and_time;
end;
@z

@x [46.1298] Set internal[prologues] in troff mode.
history:=spotless; {ready to go!}
@y
history:=spotless; {ready to go!}
if troff_mode then @+internal[prologues]:=unity;
@z

@x [46.1298] Call do_final_end.
end_of_MP: close_files_and_terminate;
final_end: ready_already:=0;
@y
close_files_and_terminate;
final_end: do_final_end;
@z

@x [46.1299] Print new line before termination; maybe switch to editor.
    print(log_name); print_char(".");
    end;
  end;
@y
    print(log_name); print_char(".");
    end;
  end;
print_ln;
if (edit_name_start<>0) and (interaction>batch_mode) then
    call_edit(str_pool,edit_name_start,edit_name_length,edit_line);
@z

@x [46.1304] (final_cleanup) Only do dump if ini.
  begin @!init store_mem_file; return;@+tini@/
@y
  begin @!init if ini_version then begin store_mem_file; return;end;@+tini@/
@z

@x [46.1306] l.22937 - Handle %&mem line
if (mem_ident=0)or(buffer[loc]="&") then
@y
if (mem_ident=0)or(buffer[loc]="&")or dump_line then
@z

@x [47.1307] Change read of integer.
  read(term_in,m);
  if m<0 then return
  else if m=0 then
    begin goto breakpoint;@\ {go to every label at least once}
    breakpoint: m:=0; @{'BREAKPOINT'@}@\
    end
  else  begin read(term_in,n);
@y
  m:=input_int (stdin);
  if m<0 then return
  else if m=0 then
    begin goto breakpoint;@\ {go to every label at least once}
    breakpoint: m:=0; @{'BREAKPOINT'@}@\
    end
  else  begin n:=input_int (stdin);
@z

@x [47.1308]
13: begin read(term_in,l); print_cmd_mod(n,l);
@y
13: begin l:=input_int (stdin); print_cmd_mod(n,l);
@z

@x [48.1309] Add editor-switch variable to globals.
This section should be replaced, if necessary, by any special
modification of the program
that are necessary to make \MP\ work at a particular installation.
It is usually best to design your change file so that all changes to
previous sections preserve the section numbering; then everybody's version
will be consistent with the published program. More extensive changes,
which introduce new sections, can be inserted here; then only the index
itself will get a new section number.
@^system dependencies@>
@y
Here are the variables used to hold ``switch-to-editor'' information.
@^system dependencies@>

@<Global...@>=
@!edit_name_start: pool_pointer;
@!edit_name_length,@!edit_line: integer;

@ The |edit_name_start| will be set to point into |str_pool| somewhere after
its beginning if \MP\ is supposed to switch to an editor on exit.

@<Set init...@>=
edit_name_start:=0;

@ Web2c is deficient, and can only translate pointers to a type
identifier, not a general type. Easier, if more annoying, to introduce
this extra definition than to fix Web2c.

@<Types...@> =
@!str_ref_type = 0..max_str_ref;
@z
