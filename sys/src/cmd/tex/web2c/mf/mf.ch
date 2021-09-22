% mf.ch for C compilation with web2c, derived from various other change
% files: INITEX.CH for Berkeley Unix TeX 1.1 (by Howard Trickey and
% Pavel Curtis), by Paul Richards.  web2c modifications by Tim Morgan, et al.
%
% (more recent changes in ChangeLog)
% Revision 2.0  90/3/27   20:20:00  ken        To version 2.0.
% Revision 1.9  90/1/20   09:05:32  karl       To version 1.9.
% Revision 1.8  89/11/30  09:08:16  karl       To version 1.8.
% Revision 1.7  88/12/27  15:02:24  mackay     Cosmetic upgrade for version 1.7
% Revision 1.6  88/12/11  15:59:15  morgan     Brought up to MF version 1.6
% Revision 1.5  88/03/02  13:25:44  morgan     More C changes
% Revision 1.4  87/12/09  12:50:00  hesse      Changes for C version
% Revision 1.3  87/03/07  21:15:21  mackay
% 	Minor changes found on archive version on SCORE
% Revision 1.2  86/09/29  21:46:43  mackay
%	Made no-debug the default, and changed version number
%	to correspond with improved mf.web file
%	(Got rid of debug code to avoid bug in range check
%	code of VAX4.3 BSD and SUN3 version 3.1 Os pc interpreter)
% Revision 1.0  86/01/31  15:46:08  richards
% 	Incorporates: New binary I/O library, separate optimized
% 	arithmetic for takefraction/makefraction, new graphics interface.

@x [0] WEAVE: print changes only.
\def\botofcontents{\vskip 0pt plus 1fil minus 1.5in}
@y
\def\botofcontents{\vskip 0pt plus 1fil minus 1.5in}
\let\maybe=\iffalse
\def\title{\MF\ changes for C}
\def\glob{13}\def\gglob{20, 25} % these are defined in module 1
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
@d init==ifdef('INIMF')
@d tini==endif('INIMF')
@z

% [1.11] Compile-time constants.  Although we only change a few of
% these, listing them all makes the patch file for a big Metafont simpler.
% 16K for BSD I/O; file_name_size is set from the system constant.
@x
@<Constants...@>=
@!mem_max=30000; {greatest index in \MF's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMF}, otherwise |>=mem_top|}
@!max_internal=100; {maximum number of internal quantities}
@!buf_size=500; {maximum number of characters simultaneously present in
  current lines of open files; must not exceed |max_halfword|}
@!error_line=72; {width of context lines on terminal error messages}
@!half_error_line=42; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line=79; {width of longest text lines output; should be at least 60}
@!screen_width=768; {number of pixels in each row of screen display}
@!screen_depth=1024; {number of pixels in each column of screen display}
@!stack_size=30; {maximum number of simultaneous input sources}
@!max_strings=2000; {maximum number of strings; must not exceed |max_halfword|}
@!string_vacancies=8000; {the minimum number of characters that should be
  available for the user's identifier names and strings,
  after \MF's own error messages are stored}
@!pool_size=32000; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all identifiers;
  must exceed |string_vacancies| by the total
  length of \MF's own strings, which is currently about 22000}
@!move_size=5000; {space for storing moves in a single octant}
@!max_wiggle=300; {number of autorounded points per cycle}
@!gf_buf_size=800; {size of the output buffer, must be a multiple of 8}
@!file_name_size=40; {file names shouldn't be longer than this}
@!pool_name='MFbases:MF.POOL                         ';
  {string of length |file_name_size|; tells where the string pool appears}
@.MFbases@>
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

@<Constants...@>=
@!max_internal=300; {maximum number of internal quantities}
@!buf_size=3000; {maximum number of characters simultaneously present in
  current lines of open files; must not exceed |max_halfword|}
@!screen_width=1664; {number of pixels in each row of screen display}
@!screen_depth=1200; {number of pixels in each column of screen display}
@!stack_size=300; {maximum number of simultaneous input sources}
@!max_strings=7500; {maximum number of strings; must not exceed |max_halfword|}
@!string_vacancies=74000; {the minimum number of characters that should be
  available for the user's identifier names and strings,
  after \MF's own error messages are stored}
@!pool_size=100000; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all identifiers;
  must exceed |string_vacancies| by the total
  length of \MF's own strings, which is currently about 22000}
@!move_size=20000; {space for storing moves in a single octant}
@!max_wiggle=1000; {number of autorounded points per cycle}
@!pool_name='mf.pool';
  {string that tells where the string pool appears}
@!path_size=1000; {maximum number of knots between breakpoints of a path}
@!bistack_size=1500; {size of stack for bisection algorithms;
  should probably be left at this value}
@!header_size=100; {maximum number of \.{TFM} header words, times~4}
@!lig_table_size=15000; {maximum number of ligature/kern steps, must be
  at least 255 and at most 32510}
@!max_kerns=2500; {maximum number of distinct kern amounts}
@!max_font_dimen=60; {maximum number of \&{fontdimen} parameters}
@#
@!inf_main_memory = 2999;
@!sup_main_memory = 8000000;
@z

@x [1.12] Constants defined as WEB macros.
@d mem_min=0 {smallest index in the |mem| array, must not be less
  than |min_halfword|}
@d mem_top==30000 {largest index in the |mem| array dumped by \.{INIMF};
  must be substantially larger than |mem_min|
  and not greater than |mem_max|}
@d hash_size=2100 {maximum number of symbolic tokens,
  must be less than |max_halfword-3*param_size|}
@d hash_prime=1777 {a prime number equal to about 85\pct! of |hash_size|}
@d max_in_open=6 {maximum number of input files and error insertions that
  can be going on simultaneously}
@d param_size=150 {maximum number of simultaneous macro parameters}
@y
@d mem_min=0 {smallest index in the |mem| array, must not be less
  than |min_halfword|}
@d hash_size=9500 {maximum number of symbolic tokens,
  must be less than |max_halfword-3*param_size|}
@d hash_prime=7919 {a prime number equal to about 85\pct! of |hash_size|}
@d max_in_open=15 {maximum number of input files and error insertions that
  can be going on simultaneously}
@d param_size=150 {maximum number of simultaneous macro parameters}
@z

@x [1.13] Global parameters that can be changed in texmf.cnf.
@<Glob...@>=
@!bad:integer; {is some ``constant'' wrong?}
@y
@<Glob...@>=
@!bad:integer; {is some ``constant'' wrong?}
@#
@!init
@!ini_version:boolean; {are we \.{INIMF}? Set in \.{lib/texmfmp.c}}
@!dump_option:boolean; {was the dump name option used?}
@!dump_line:boolean; {was a \.{\%\AM base} line seen?}
tini@/
@#
@!bound_default:integer; {temporary for setup}
@!bound_name:^char; {temporary for setup}
@#
@!main_memory:integer; {total memory words allocated in initex}
@!mem_top:integer; {largest index in the |mem| array dumped by \.{INIMF};
  must be substantially larger than |mem_bot|,
  equal to |mem_max| in \.{INIMF}, else not greater than |mem_max|}
@!mem_max:integer; {greatest index in \MF's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMF}, otherwise |>=mem_top|}
@!error_line:integer; {width of context lines on terminal error messages}
@!half_error_line:integer; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line:integer; {width of longest text lines output;
  should be at least 60}
@!gf_buf_size:integer; {size of the output buffer, must be a multiple of 8}
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
@ The \ph\ compiler with which the present version of \MF\ was prepared has
extended the rules of \PASCAL\ in a very convenient way. To open file~|f|,
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
\MF\ to undertake appropriate corrective action.
@:PASCAL H}{\ph@>
@^system dependencies@>

\MF's file-opening procedures return |false| if no file identified by
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
before |eof|, but \MF\ needs only a weaker restriction: If |eof|
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
does an |fflush|. |clear_terminal| is redefined
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

@x [4.48] l.1219 -- do not create "^^xy" in string pool
@ @d app_lc_hex(#)==l:=#;
  if l<10 then append_char(l+"0")@+else append_char(l-10+"a")
@y
@ The first 256 strings will consist of a single character only.
@z
@x
  begin if (@<Character |k| cannot be printed@>) then
    begin append_char("^"); append_char("^");
    if k<@'100 then append_char(k+@'100)
    else if k<@'200 then append_char(k-@'100)
    else begin app_lc_hex(k div 16); app_lc_hex(k mod 16);
      end;
    end
  else append_char(k);
  g:=make_string; str_ref[g]:=max_str_ref;
@y
  begin append_char(k);
  g:=make_string; str_ref[g]:=max_str_ref;
@z

@x [4.49] l.1239 -- change documentation (probably needed in more places)
would like string @'32 to be the single character @'32 instead of the
@y
would like string @'32 to be printed as the single character @'32 instead
of the
@z

% [4.51] Open the pool file using a path, and can't do string
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
if a_open_in (pool_file, kpse_mfpool_format) then
@z

@x [4.51,52,53] Make `MF.POOL' lowercase, and change how it's read.
else  bad_pool('! I can''t read MF.POOL.')
@y
else  bad_pool('! I can''t read mf.pool; bad path?')
@z
@x
begin if eof(pool_file) then bad_pool('! MF.POOL has no check sum.');
@.MF.POOL has no check sum@>
read(pool_file,m,n); {read two digits of string length}
@y
begin if eof(pool_file) then bad_pool('! mf.pool has no check sum.');
@.MF.POOL has no check sum@>
read(pool_file,m); read(pool_file,n); {read two digits of string length}
@z
@x
    bad_pool('! MF.POOL line doesn''t begin with two digits.');
@y
    bad_pool('! mf.pool line doesn''t begin with two digits.');
@z
@x
  bad_pool('! MF.POOL check sum doesn''t have nine digits.');
@y
  bad_pool('! mf.pool check sum doesn''t have nine digits.');
@z
@x
done: if a<>@$ then bad_pool('! MF.POOL doesn''t match; TANGLE me again.');
@y
done: if a<>@$ then
  bad_pool('! mf.pool doesn''t match; tangle me again (or fix the path).');
@z

@x [5.54] error_line is a variable, so can't be a subrange array bound
@!trick_buf:array[0..error_line] of ASCII_code; {circular buffer for
@y
@!trick_buf:array[0..ssup_error_line] of ASCII_code; {circular buffer for
@z

@x [5.58] l.1420 -- rename |print_char| to |print_visible_char|
@ The |print_char| procedure sends one character to the desired destination,
using the |xchr| array to map it into an external character compatible with
|input_ln|. All printing comes through |print_ln| or |print_char|.

@<Basic printing...@>=
procedure print_char(@!s:ASCII_code); {prints a single character}
@y
@ The |print_visible_char| procedure sends one character to the desired
destination, using the |xchr| array to map it into an external character
compatible with |input_ln|.  (It assumes that it is always called with
a visible ASCII character.)

@<Basic printing...@>=
procedure print_visible_char(@!s:ASCII_code); {prints a single character}
@z

@x [5.58/59] l.1447 -- insert new |print_char| procedure
incr(tally);
end;
@y
incr(tally);
end;

@ The |print_char| procedure sends one character to the desired destination.
File names and string expressions might contain |ASCII_code| values that
can't be printed using |print_visible_char|. These characters will be
printed in three- or four-symbol form like `\.{\^\^A}' or `\.{\^\^e4}'.
All printing comes through |print_ln| or |print_char|.

@d print_lc_hex(#)==l:=#;
  if l<10 then print_visible_char(l+"0")@+else print_visible_char(l-10+"a")

@<Basic printing...@>=
procedure print_char(@!s:ASCII_code); {prints a single character}
label exit;
var k:ASCII_code;
  @!l:0..255; {small indices or counters}
begin 
  k:=s; if @<Character |k| cannot be printed@> then
    begin if selector>pseudo then
      begin print_visible_char(s); return;
      end;
    print_visible_char("^"); print_visible_char("^");
    if s<64 then print_visible_char(s+64)
    else if s<128 then print_visible_char(s-64)
    else begin print_lc_hex(s div 16);  print_lc_hex(s mod 16);
      end;
    end
  else print_visible_char(s);
exit:end;
@z

@x [5.59/60] l.1455 -- simplify |print| and remove |slow_print|
assumes that it is always safe to print a visible ASCII character.)
@^system dependencies@>
@y
assumes that it is always safe to print a visible ASCII character.)
@^system dependencies@>
  
Old versions of \MF\ needed a procedure called |slow_print| whose function
is now subsumed by |print|. We retain the old name here as a possible aid
to future software arch\ae ologists.

@d slow_print == print
@z
@x
if (s<256)and(selector>pseudo) then print_char(s)
@y
if (s<256) then print_char(s)
@z

@x [5.60] l.1471 -- remove |slow_print|
@ Sometimes it's necessary to print a string whose characters
may not be visible ASCII codes. In that case |slow_print| is used.

@<Basic print...@>=
procedure slow_print(@!s:integer); {prints string |s|}
var @!j:pool_pointer; {current character code position}
begin if (s<0)or(s>=str_ptr) then s:="???"; {this can't happen}
@.???@>
if (s<256)and(selector>pseudo) then print_char(s)
else begin j:=str_start[s];
  while j<str_start[s+1] do
    begin print(so(str_pool[j])); incr(j);
    end;
  end;
end;
@y
@z

@x [5.61] Print rest of banner, eliminate misleading `(no base preloaded)'.
wterm(banner);
if base_ident=0 then wterm_ln(' (no base preloaded)')
else  begin slow_print(base_ident); print_ln;
  end;
@y
wterm (banner);
wterm (version_string);
if base_ident>0 then slow_print(base_ident); print_ln;
@z

@x [6.68] l.1603 - Add unspecified_mode.
@d error_stop_mode=3 {stops at every opportunity to interact}
@y
@d error_stop_mode=3 {stops at every opportunity to interact}
@d unspecified_mode=4 {extra value for command-line switch}
@z

@x [6.68] l.1610 - Add interaction_option.
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@y
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@!interaction_option:batch_mode..unspecified_mode; {set from command line}
@z

@x [6.69] l.1612 - Allow override by command line switch.
@ @<Set init...@>=interaction:=error_stop_mode;
@y
@ @<Set init...@>=if interaction_option=unspecified_mode then
  interaction:=error_stop_mode
else
  interaction:=interaction_option;
@z

@x [6.76] Eliminate non-local goto.
@<Error hand...@>=
procedure jump_out;
begin goto end_of_MF;
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

@x [6.79] Handle the switch-to-editor option.
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
since we don't want to do the switch-to-editor until after \MF\ has closed
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
  slow_print(input_stack[file_ptr].name_field);
  print(" at line "); print_int(line);@/
  interaction:=scroll_mode; jump_out;
@y
"E": if file_ptr>0 then
    begin
    edit_name_start:=str_start[edit_file.name_field];
    edit_name_length:=str_start[edit_file.name_field+1] -
    		      str_start[edit_file.name_field];
    edit_line:=line;
    jump_out;
@z

@x [7.96] Do half in cpascal.h. And add halfp as in MetaPost for speed.
@d half(#)==(#) div 2
@y
@z

@x [102] Use halfp.
round_decimals:=half(a+1);
@y
round_decimals:=halfp(a+1);
@z

@x [7.107-7.115] Optionally replace make_fraction etc. with external routines
@p function make_fraction(@!p,@!q:integer):fraction;
@y
In the C version, there are external routines that use double precision
floating point to simulate functions such as |make_fraction|.  This is
carefully done to be virtually machine-independent and it gives up to 12
times speed-up on machines with hardware floating point.  Since some
machines do not have fast double-precision floating point, we provide a
C preprocessor switch that allows selecting the standard versions given
below. (There's no configure option to select FIXPT, however, since I
don't expect anyone will actually notice.)

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

@x [111]
@<Compute $p=\lfloor qf/2^{28}+{1\over2}\rfloor-q$@>=
p:=fraction_half; {that's $2^{27}$; the invariants hold now with $k=28$}
if q<fraction_four then
  repeat if odd(f) then p:=half(p+q)@+else p:=half(p);
  f:=half(f);
  until f=1
else  repeat if odd(f) then p:=p+half(q-p)@+else p:=half(p);
  f:=half(f);
  until f=1
@y
@<Compute $p=\lfloor qf/2^{28}+{1\over2}\rfloor-q$@>=
p:=fraction_half; {that's $2^{27}$; the invariants hold now with $k=28$}
if q<fraction_four then
  repeat if odd(f) then p:=halfp(p+q)@+else p:=halfp(p);
  f:=halfp(f);
  until f=1
else  repeat if odd(f) then p:=p+halfp(q-p)@+else p:=halfp(p);
  f:=halfp(f);
  until f=1
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

@x [113]
@ @<Compute $p=\lfloor qf/2^{16}+{1\over2}\rfloor-q$@>=
p:=half_unit; {that's $2^{15}$; the invariants hold now with $k=16$}
@^inner loop@>
if q<fraction_four then
  repeat if odd(f) then p:=half(p+q)@+else p:=half(p);
  f:=half(f);
  until f=1
else  repeat if odd(f) then p:=p+half(q-p)@+else p:=half(p);
  f:=half(f);
  until f=1
@y
@ @<Compute $p=\lfloor qf/2^{16}+{1\over2}\rfloor-q$@>=
p:=half_unit; {that's $2^{15}$; the invariants hold now with $k=16$}
@^inner loop@>
if q<fraction_four then
  repeat if odd(f) then p:=halfp(p+q)@+else p:=halfp(p);
  f:=halfp(f);
  until f=1
else  repeat if odd(f) then p:=p+halfp(q-p)@+else p:=halfp(p);
  f:=halfp(f);
  until f=1
@z

@x
operands are positive. \ (This procedure is not used especially often,
so it is not part of \MF's inner loop.)

@p function make_scaled(@!p,@!q:integer):scaled;
@y
operands are positive. \ (This procedure is not used especially often,
so it is not part of \MF's inner loop, but we might as well allow for
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

@x [7.119] Do floor_scaled, floor_unscaled, round_unscaled, round_fraction in C.
@p function floor_scaled(@!x:scaled):scaled;
  {$2^{16}\lfloor x/2^{16}\rfloor$}
var @!be_careful:integer; {temporary register}
begin if x>=0 then floor_scaled:=x-(x mod unity)
else  begin be_careful:=x+1;
  floor_scaled:=x+((-be_careful) mod unity)+1-unity;
  end;
end;
@#
function floor_unscaled(@!x:scaled):integer;
  {$\lfloor x/2^{16}\rfloor$}
var @!be_careful:integer; {temporary register}
begin if x>=0 then floor_unscaled:=x div unity
else  begin be_careful:=x+1; floor_unscaled:=-(1+((-be_careful) div unity));
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

@x [121]
  square_rt:=half(q);
@y
  square_rt:=halfp(q);
@z

@x [126]
@p function pyth_sub(@!a,@!b:integer):integer;
label done;
var @!r:fraction; {register used to transform |a| and |b|}
@!big:boolean; {is the input dangerously near $2^{31}$?}
begin a:=abs(a); b:=abs(b);
if a<=b then @<Handle erroneous |pyth_sub| and set |a:=0|@>
else  begin if a<fraction_four then big:=false
  else  begin a:=half(a); b:=half(b); big:=true;
    end;
  @<Replace |a| by an approximation to $\psqrt{a^2-b^2}$@>;
  if big then a:=a+a;
  end;
pyth_sub:=a;
end;
@y
@p function pyth_sub(@!a,@!b:integer):integer;
label done;
var @!r:fraction; {register used to transform |a| and |b|}
@!big:boolean; {is the input dangerously near $2^{31}$?}
begin a:=abs(a); b:=abs(b);
if a<=b then @<Handle erroneous |pyth_sub| and set |a:=0|@>
else  begin if a<fraction_four then big:=false
  else  begin a:=halfp(a); b:=halfp(b); big:=true;
    end;
  @<Replace |a| by an approximation to $\psqrt{a^2-b^2}$@>;
  if big then a:=a+a;
  end;
pyth_sub:=a;
end;
@z

@x [133]
@ @<Increase |k| until |x| can...@>=
begin z:=((x-1) div two_to_the[k])+1; {$z=\lceil x/2^k\rceil$}
while x<fraction_four+z do
  begin z:=half(z+1); k:=k+1;
  end;
y:=y+spec_log[k]; x:=x-z;
end
@y
@ @<Increase |k| until |x| can...@>=
begin z:=((x-1) div two_to_the[k])+1; {$z=\lceil x/2^k\rceil$}
while x<fraction_four+z do
  begin z:=halfp(z+1); k:=k+1;
  end;
y:=y+spec_log[k]; x:=x-z;
end
@z

@x [142]
@<Set variable |z| to the arg...@>=
while x>=fraction_two do
  begin x:=half(x); y:=half(y);
  end;
z:=0;
if y>0 then
  begin while x<fraction_one do
    begin double(x); double(y);
    end;
  @<Increase |z| to the arg of $(x,y)$@>;
  end
@y
@<Set variable |z| to the arg...@>=
while x>=fraction_two do
  begin x:=halfp(x); y:=halfp(y);
  end;
z:=0;
if y>0 then
  begin while x<fraction_one do
    begin double(x); double(y);
    end;
  @<Increase |z| to the arg of $(x,y)$@>;
  end
@z

@x [150]
@p procedure init_randoms(@!seed:scaled);
var @!j,@!jj,@!k:fraction; {more or less random integers}
@!i:0..54; {index into |randoms|}
begin j:=abs(seed);
while j>=fraction_one do j:=half(j);
k:=1;
for i:=0 to 54 do
  begin jj:=k; k:=j-k; j:=jj;
  if k<0 then k:=k+fraction_one;
  randoms[(i*21)mod 55]:=j;
  end;
new_randoms; new_randoms; new_randoms; {``warm up'' the array}
end;
@y
@p procedure init_randoms(@!seed:scaled);
var @!j,@!jj,@!k:fraction; {more or less random integers}
@!i:0..54; {index into |randoms|}
begin j:=abs(seed);
while j>=fraction_one do j:=halfp(j);
k:=1;
for i:=0 to 54 do
  begin jj:=k; k:=j-k; j:=jj;
  if k<0 then k:=k+fraction_one;
  randoms[(i*21)mod 55]:=j;
  end;
new_randoms; new_randoms; new_randoms; {``warm up'' the array}
end;
@z

@x [9.153] Increase memory size.
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

@x [9.155] Don't bother to subtract zero.
@d ho(#)==#-min_halfword
  {to take a sixteen-bit item from a halfword}
@d qo(#)==#-min_quarterword {to read eight bits from a quarterword}
@d qi(#)==#+min_quarterword {to store eight bits in a quarterword}
@y
@d ho(#)==#
@d qo(#)==#
@d qi(#)==#
@z

@x [9.156] memory_word is defined externally.
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

@x [10.159] mem is dynamically allocated.
@!mem : array[mem_min..mem_max] of memory_word; {the big dynamic storage area}
@y
@!mem : ^memory_word; {the big dynamic storage area}
@z

@x [10.167] Fix an unsigned/signed problem in getnode.
if r>p+1 then @<Allocate from the top of node |p| and |goto found|@>;
@y
if r>toint(p+1) then @<Allocate from the top of node |p| and |goto found|@>;
@z

% [11.178] Change the word `free' so that it doesn't conflict with the
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
@!debug @!free: packed array [0..1] of boolean; {free cells; this loses}
@t\hskip1em@>@!was_free: packed array [0..1] of boolean; {this loses too}
@z

@x [11.182] Eliminate unsigned comparisons to zero.
repeat if (p>=lo_mem_max)or(p<mem_min) then clobbered:=true
  else if (rlink(p)>=lo_mem_max)or(rlink(p)<mem_min) then clobbered:=true
@y
repeat if (p>=lo_mem_max) then clobbered:=true
  else if (rlink(p)>=lo_mem_max) then clobbered:=true
@z

@x [12.194] Do `fix_date_and_time' in C.
@ The following procedure, which is called just before \MF\ initializes its
input and output, establishes the initial values of the date and time.
@^system dependencies@>
Since standard \PASCAL\ cannot provide such information, something special
is needed. The program here simply specifies July 4, 1776, at noon; but
users probably want a better approximation to the truth.

Note that the values are |scaled| integers. Hence \MF\ can no longer
be used after the year 32767.

@p procedure fix_date_and_time;
begin internal[time]:=12*60*unity; {minutes since midnight}
internal[day]:=4*unity; {fourth day of the month}
internal[month]:=7*unity; {seventh month of the year}
internal[year]:=1776*unity; {Anno Domini}
end;
@y
@ The following procedure, which is called just before \MF\ initializes its
input and output, establishes the initial values of the date and time.
It is calls an externally defined |date_and_time|, even though it could
be done from Pascal.
The external procedure also sets up interrupt catching.
@^system dependencies@>

Note that the values are |scaled| integers. Hence \MF\ can no longer
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

@x [12.199] Allow tab and form feed as input.
for k:=127 to 255 do char_class[k]:=invalid_class;
@y
for k:=127 to 255 do char_class[k]:=invalid_class;
char_class[tab]:=space_class;
char_class[form_feed]:=space_class;
@z

@x [232] Use halfp.
@p procedure init_big_node(@!p:pointer);
var @!q:pointer; {the new node}
@!s:small_number; {its size}
begin s:=big_node_size[type(p)]; q:=get_node(s);
repeat s:=s-2; @<Make variable |q+s| newly independent@>;
name_type(q+s):=half(s)+x_part_sector; link(q+s):=null;
until s=0;
link(q):=p; value(p):=q;
end;
@y
@p procedure init_big_node(@!p:pointer);
var @!q:pointer; {the new node}
@!s:small_number; {its size}
begin s:=big_node_size[type(p)]; q:=get_node(s);
repeat s:=s-2; @<Make variable |q+s| newly independent@>;
name_type(q+s):=halfp(s)+x_part_sector; link(q+s):=null;
until s=0;
link(q):=p; value(p):=q;
end;
@z

 [20.329] |valid_range| uses |abs|, which we have defined as a C
% macro.  Some C preprocessors cannot expand the giant argument here.
% So we add a temporary.
@x
@p procedure edge_prep(@!ml,@!mr,@!nl,@!nr:integer);
var @!delta:halfword; {amount of change}
@y
@p procedure edge_prep(@!ml,@!mr,@!nl,@!nr:integer);
var @!delta:halfword; {amount of change}
temp:integer;
@z

@x
if not valid_range(m_min(cur_edges)+m_offset(cur_edges)-zero_field) or@|
 not valid_range(m_max(cur_edges)+m_offset(cur_edges)-zero_field) then
@y
temp := m_offset (cur_edges) - zero_field;
if not valid_range (m_min (cur_edges) + temp)
   or not valid_range (m_max (cur_edges) + temp)
then
@z

@x [442] Use halfp.
@<Compute a good coordinate at a diagonal transition@>=
begin if cur_pen=null_pen then pen_edge:=0
else if cur_path_type=double_path_code then @<Compute a compromise |pen_edge|@>
else if right_type(q)<=switch_x_and_y then pen_edge:=diag_offset(right_type(q))
else pen_edge:=-diag_offset(right_type(q));
if odd(right_type(q)) then a:=good_val(b,pen_edge+half(cur_gran))
else a:=good_val(b-1,pen_edge+half(cur_gran));
end
@y
@<Compute a good coordinate at a diagonal transition@>=
begin if cur_pen=null_pen then pen_edge:=0
else if cur_path_type=double_path_code then @<Compute a compromise |pen_edge|@>
else if right_type(q)<=switch_x_and_y then pen_edge:=diag_offset(right_type(q))
else pen_edge:=-diag_offset(right_type(q));
if odd(right_type(q)) then a:=good_val(b,pen_edge+halfp(cur_gran))
else a:=good_val(b-1,pen_edge+halfp(cur_gran));
end
@z

% [25.530] |make_fraction| and |take_fraction| arguments are too long for
% some preprocessors, when they were defined as macros, just as in the
% previous change.
@x
  alpha:=take_fraction(take_fraction(major_axis,
      make_fraction(gamma,beta)),n_cos)@|
    -take_fraction(take_fraction(minor_axis,
      make_fraction(delta,beta)),n_sin);
  alpha:=(alpha+half_unit) div unity;
  gamma:=pyth_add(take_fraction(major_axis,n_cos),
    take_fraction(minor_axis,n_sin));
@y
  alpha := make_fraction (gamma, beta);
  alpha := take_fraction (major_axis, alpha);
  alpha := take_fraction (alpha, n_cos);
  alpha := (alpha+half_unit) div unity;
  gamma := take_fraction (minor_axis, n_sin);
  gamma := pyth_add (take_fraction (major_axis, n_cos), gamma);
@z

@x [556]
@p procedure cubic_intersection(@!p,@!pp:pointer);
label continue, not_found, exit;
var @!q,@!qq:pointer; {|link(p)|, |link(pp)|}
begin time_to_go:=max_patience; max_t:=2;
@<Initialize for intersections at level zero@>;
loop@+  begin continue:
  if delx-tol<=stack_max(x_packet(xy))-stack_min(u_packet(uv)) then
   if delx+tol>=stack_min(x_packet(xy))-stack_max(u_packet(uv)) then
   if dely-tol<=stack_max(y_packet(xy))-stack_min(v_packet(uv)) then
   if dely+tol>=stack_min(y_packet(xy))-stack_max(v_packet(uv)) then
    begin if cur_t>=max_t then
      begin if max_t=two then {we've done 17 bisections}
        begin cur_t:=half(cur_t+1); cur_tt:=half(cur_tt+1); return;
        end;
      double(max_t); appr_t:=cur_t; appr_tt:=cur_tt;
      end;
    @<Subdivide for a new level of intersection@>;
    goto continue;
    end;
  if time_to_go>0 then decr(time_to_go)
  else  begin while appr_t<unity do
      begin double(appr_t); double(appr_tt);
      end;
    cur_t:=appr_t; cur_tt:=appr_tt; return;
    end;
  @<Advance to the next pair |(cur_t,cur_tt)|@>;
  end;
exit:end;
@y
@p procedure cubic_intersection(@!p,@!pp:pointer);
label continue, not_found, exit;
var @!q,@!qq:pointer; {|link(p)|, |link(pp)|}
begin time_to_go:=max_patience; max_t:=2;
@<Initialize for intersections at level zero@>;
loop@+  begin continue:
  if delx-tol<=stack_max(x_packet(xy))-stack_min(u_packet(uv)) then
   if delx+tol>=stack_min(x_packet(xy))-stack_max(u_packet(uv)) then
   if dely-tol<=stack_max(y_packet(xy))-stack_min(v_packet(uv)) then
   if dely+tol>=stack_min(y_packet(xy))-stack_max(v_packet(uv)) then
    begin if cur_t>=max_t then
      begin if max_t=two then {we've done 17 bisections}
        begin cur_t:=halfp(cur_t+1); cur_tt:=halfp(cur_tt+1); return;
        end;
      double(max_t); appr_t:=cur_t; appr_tt:=cur_tt;
      end;
    @<Subdivide for a new level of intersection@>;
    goto continue;
    end;
  if time_to_go>0 then decr(time_to_go)
  else  begin while appr_t<unity do
      begin double(appr_t); double(appr_tt);
      end;
    cur_t:=appr_t; cur_tt:=appr_tt; return;
    end;
  @<Advance to the next pair |(cur_t,cur_tt)|@>;
  end;
exit:end;
@z

@x [561]
@ @<Descend to the previous level...@>=
begin cur_t:=half(cur_t); cur_tt:=half(cur_tt);
if cur_t=0 then return;
bisect_ptr:=bisect_ptr-int_increment; three_l:=three_l-tol_step;
delx:=stack_dx; dely:=stack_dy; tol:=stack_tol; uv:=stack_uv; xy:=stack_xy;@/
goto not_found;
end
@y
@ @<Descend to the previous level...@>=
begin cur_t:=halfp(cur_t); cur_tt:=halfp(cur_tt);
if cur_t=0 then return;
bisect_ptr:=bisect_ptr-int_increment; three_l:=three_l-tol_step;
delx:=stack_dx; dely:=stack_dy; tol:=stack_tol; uv:=stack_uv; xy:=stack_xy;@/
goto not_found;
end
@z

@x [27.564] The window functions are defined externally, in C.
@p function init_screen:boolean;
begin init_screen:=false;
end;
@#
procedure update_screen; {will be called only if |init_screen| returns |true|}
begin @!init wlog_ln('Calling UPDATESCREEN');@+tini {for testing only}
end;
@y
{These functions/procedures are defined externally in C.}
@z

@x [27.567]
@p procedure blank_rectangle(@!left_col,@!right_col:screen_col;
  @!top_row,@!bot_row:screen_row);
var @!r:screen_row;
@!c:screen_col;
begin @{@+for r:=top_row to bot_row-1 do
  for c:=left_col to right_col-1 do
    screen_pixel[r,c]:=white;@+@}@/
@!init wlog_cr; {this will be done only after |init_screen=true|}
wlog_ln('Calling BLANKRECTANGLE(',left_col:1,',',
  right_col:1,',',top_row:1,',',bot_row:1,')');@+tini
end;
@y
{Same thing.}
@z

@x [27.568]
@p procedure paint_row(@!r:screen_row;@!b:pixel_color;var @!a:trans_spec;
  @!n:screen_col);
var @!k:screen_col; {an index into |a|}
@!c:screen_col; {an index into |screen_pixel|}
begin @{ k:=0; c:=a[0];
repeat incr(k);
  repeat screen_pixel[r,c]:=b; incr(c);
  until c=a[k];
  b:=black-b; {$|black|\swap|white|$}
  until k=n;@+@}@/
@!init wlog('Calling PAINTROW(',r:1,',',b:1,';');
  {this is done only after |init_screen=true|}
for k:=0 to n do
  begin wlog(a[k]:1); if k<>n then wlog(',');
  end;
wlog_ln(')');@+tini
end;
@y
{Same thing}
@z

@x [596] Use halfp.
@ @<Contribute a term from |q|, multiplied by~|f|@>=
begin if tt=dependent then v:=take_fraction(f,value(q))
else v:=take_scaled(f,value(q));
if abs(v)>half(threshold) then
  begin s:=get_node(dep_node_size); info(s):=qq; value(s):=v;
  if abs(v)>=coef_bound then if watch_coefs then
    begin type(qq):=independent_needing_fix; fix_needed:=true;
    end;
  link(r):=s; r:=s;
  end;
q:=link(q); qq:=info(q);
end
@y
@ @<Contribute a term from |q|, multiplied by~|f|@>=
begin if tt=dependent then v:=take_fraction(f,value(q))
else v:=take_scaled(f,value(q));
if abs(v)>halfp(threshold) then
  begin s:=get_node(dep_node_size); info(s):=qq; value(s):=v;
  if abs(v)>=coef_bound then if watch_coefs then
    begin type(qq):=independent_needing_fix; fix_needed:=true;
    end;
  link(r):=s; r:=s;
  end;
q:=link(q); qq:=info(q);
end
@z

@x [30] Plumbing help by rsc@plan9.bell-labs.com
else  begin print_nl("l."); print_int(line);
@y
else begin
	print_nl(cur_input.name_field);
	print(":");
	print_int(line);
	print(":");
@z

@x [38.768] Area and extension rules.
@ The file names we shall deal with for illustrative purposes have the
following structure:  If the name contains `\.>' or `\.:', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@^system dependencies@>

We can scan such file names easily by using two global variables that keep track
of the occurrences of area and extension delimiters:

@<Glob...@>=
@!area_delimiter:pool_pointer; {the most recent `\.>' or `\.:', if any}
@!ext_delimiter:pool_pointer; {the relevant `\..', if any}
@y
@ The file names we shall deal with for illustrative purposes have the
following structure:  If the name contains `\./', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@^system dependencies@>

We can scan such file names easily by using two global variables that keep
track of the occurrences of area and extension delimiters:

@<Glob...@>=
@!area_delimiter:pool_pointer; {the most recent `\./', if any}
@!ext_delimiter:pool_pointer; {the most recent `\..', if any}
@z

@x [38.769] MF area directories.
@d MF_area=="MFinputs:"
@.MFinputs@>
@y
In C, the default paths are specified separately.
@z

@x [38.771] more_name
begin if c=" " then more_name:=false
else  begin if (c=">")or(c=":") then
@y
begin if (c=" ")or(c=tab) then more_name:=false
else  begin if IS_DIR_SEP (c) then
@z

@x [38.771] more_name
  else if (c=".")and(ext_delimiter=0) then ext_delimiter:=pool_ptr;
@y
  else if c="." then ext_delimiter:=pool_ptr;
@z

@x [38.774] (pack_file_name) malloc and null terminate name_of_file.
for j:=str_start[a] to str_start[a+1]-1 do append_to_name(so(str_pool[j]));
@y
if name_of_file then libc_free (name_of_file);
name_of_file := xmalloc (1 + length (a) + length (n) + length (e) + 1);
for j:=str_start[a] to str_start[a+1]-1 do append_to_name(so(str_pool[j]));
@z
@x
for k:=name_length+1 to file_name_size do name_of_file[k]:=' ';
@y
name_of_file[name_length + 1] := 0;
@z

@x [38.775] The default base.
@d base_default_length=18 {length of the |MF_base_default| string}
@d base_area_length=8 {length of its area part}
@y
@d base_area_length=0 {no fixed area in C}
@z

@x [38.776] Where `plain.base' is.
@!MF_base_default:packed array[1..base_default_length] of char;

@ @<Set init...@>=
MF_base_default:='MFbases:plain.base';
@y
@!base_default_length: integer;
@!MF_base_default: ^char;

@ We set the name of the default format file and the length of that name
in \.{texmfmp.c}, since we want them to depend on the name of the
program.
@z

@x [38.778] Change to pack_buffered_name as with pack_file_name.
for j:=1 to n do append_to_name(xord[MF_base_default[j]]);
@y
if name_of_file then libc_free (name_of_file);
name_of_file := xmalloc (1 + n + (b - a + 1) + base_ext_length + 1);
for j:=1 to n do append_to_name(xord[MF_base_default[j]]);
@z
@x
for k:=name_length+1 to file_name_size do name_of_file[k]:=' ';
@y
name_of_file[name_length + 1] := 0;
@z

@x [38.779] Base file opening: do path searching for the default, not plain.
  pack_buffered_name(0,loc,j-1); {try first without the system file area}
  if w_open_in(base_file) then goto found;
  pack_buffered_name(base_area_length,loc,j-1);
    {now try the system base file area}
  if w_open_in(base_file) then goto found;
@y
  pack_buffered_name(0,loc,j-1);
  if w_open_in(base_file) then goto found;
@z
@x
  wterm_ln('Sorry, I can''t find that base;',' will try PLAIN.');
@y
  wterm ('Sorry, I can''t find the base `');
  fputs (name_of_file + 1, stdout);
  wterm ('''; will try `');
  fputs (MF_base_default + 1, stdout);
  wterm_ln ('''.');
@z
@x
  wterm_ln('I can''t find the PLAIN base file!');
@.I can't find PLAIN...@>
@y
  wterm ('I can''t find the base file `');
  fputs (MF_base_default + 1, stdout);
  wterm_ln ('''!');
@.I can't find the base...@>
@z

@x [38.781] Make scan_file_name ignore leading tabs as well as spaces.
while buffer[loc]=" " do incr(loc);
@y
while (buffer[loc]=" ")or(buffer[loc]=tab) do incr(loc);
@z

@x [38.782] `logname' is declared in <unistd.h> on some systems.
`\.{.base}' and `\.{.tfm}' in the names of \MF's output files.
@y
`\.{.base}' and `\.{.tfm}' in the names of \MF's output files.
@d log_name == texmf_log_name
@z

@x [38.787] <Scan file name...> needs similar leading tab treatment.
while (buffer[k]=" ")and(k<last) do incr(k);
@y
while ((buffer[k]=" ")or(buffer[k]=tab))and(k<last) do incr(k);
@z

@x [38.788] Adjust for C string conventions.
@!months:packed array [1..36] of char; {abbreviations of month names}
@y
@!months:^char;
@z

@x [38.790]
begin wlog(banner);
slow_print(base_ident); print("  ");
print_int(round_unscaled(internal[day])); print_char(" ");
months:='JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@y
begin wlog(banner);
wlog (version_string);
slow_print(base_ident); print("  ");
print_int(round_unscaled(internal[day])); print_char(" ");
months := ' JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@z

@x [38.793] (start_input) a_open_in of input file needs path specifier.
begin @<Put the desired file name in |(cur_name,cur_ext,cur_area)|@>;
if cur_ext="" then cur_ext:=".mf";
pack_cur_name;
loop@+  begin begin_file_reading; {set up |cur_file| and new level of input}
  if a_open_in(cur_file) then goto done;
  if cur_area="" then
    begin pack_file_name(cur_name,MF_area,cur_ext);
    if a_open_in(cur_file) then goto done;
    end;
@y Don't assume a single . in filenames.
var temp_str: str_number; k: integer;
begin @<Put the desired file name in |(cur_name,cur_ext,cur_area)|@>;
pack_cur_name;
loop@+begin
  begin_file_reading; {set up |cur_file| and new level of input}
  if cur_ext = ".mf" then begin
    cur_ext := "";
    pack_cur_name;
    end;
  {Kpathsea tries all the various ways to get the file.}
  if a_open_in (cur_file, kpse_mf_format) then
    {See \.{tex.ch} for an explanation.}
    begin k:=1;
    begin_name;
    while (k<=name_length)and(more_name(name_of_file[k])) do
      incr(k);
    end_name;
    goto done;
    end;
@z

@x [38.793] l.15938 - Different job_name heuristic for ini version.
  begin job_name:=cur_name; open_log_file;
@y
  begin job_name:=cur_name;
    init
      if ini_version and dump_option then begin
        str_room(base_default_length);
        for k:=1 to base_default_length - base_ext_length do
          append_char(xord[MF_base_default[k]]);
        job_name:=make_string;
      end;
    tini
    open_log_file;
@z

@x [38.793] Can't return name to sring pool because of editor option?
if name=str_ptr-1 then {we can conserve string pool space now}
  begin flush_string(name); name:=cur_name;
  end;
@y
@z

@x [866] Use halfp.
@<Change node |q|...@>=
begin tx:=x_coord(q); ty:=y_coord(q);
txx:=left_x(q)-tx; tyx:=left_y(q)-ty;
txy:=right_x(q)-tx; tyy:=right_y(q)-ty;
a_minus_b:=pyth_add(txx-tyy,tyx+txy); a_plus_b:=pyth_add(txx+tyy,tyx-txy);
major_axis:=half(a_minus_b+a_plus_b); minor_axis:=half(abs(a_plus_b-a_minus_b));
if major_axis=minor_axis then theta:=0 {circle}
else theta:=half(n_arg(txx-tyy,tyx+txy)+n_arg(txx+tyy,tyx-txy));
free_node(q,knot_node_size);
q:=make_ellipse(major_axis,minor_axis,theta);
if (tx<>0)or(ty<>0) then @<Shift the coordinates of path |q|@>;
end
@y
@<Change node |q|...@>=
begin tx:=x_coord(q); ty:=y_coord(q);
txx:=left_x(q)-tx; tyx:=left_y(q)-ty;
txy:=right_x(q)-tx; tyy:=right_y(q)-ty;
a_minus_b:=pyth_add(txx-tyy,tyx+txy); a_plus_b:=pyth_add(txx+tyy,tyx-txy);
major_axis:=halfp(a_minus_b+a_plus_b); minor_axis:=halfp(abs(a_plus_b-a_minus_b));
if major_axis=minor_axis then theta:=0 {circle}
else theta:=half(n_arg(txx-tyy,tyx+txy)+n_arg(txx+tyy,tyx-txy));
free_node(q,knot_node_size);
q:=make_ellipse(major_axis,minor_axis,theta);
if (tx<>0)or(ty<>0) then @<Shift the coordinates of path |q|@>;
end
@z

% [42.912] l.18096 -- no need to create new strings of length one for char_op
% because the first 256 strings now are stored as single characters.
@x 
  else  begin cur_exp:=round_unscaled(cur_exp) mod 256; cur_type:=string_type;
    if cur_exp<0 then cur_exp:=cur_exp+256;
    if length(cur_exp)<>1 then
      begin str_room(1); append_char(cur_exp); cur_exp:=make_string;
      end;
    end;
@y
  else  begin cur_exp:=round_unscaled(cur_exp) mod 256; cur_type:=string_type;
    if cur_exp<0 then cur_exp:=cur_exp+256;
    end;
@z

@x [42.965] A C casting problem.
if (m_min(cur_edges)+tx<=0)or(m_max(cur_edges)+tx>=8192)or@|
 (n_min(cur_edges)+ty<=0)or(n_max(cur_edges)+ty>=8191)or@|
@y
if (toint(m_min(cur_edges))+tx<=0)or(m_max(cur_edges)+tx>=8192)or@|
 (toint(n_min(cur_edges))+ty<=0)or(n_max(cur_edges)+ty>=8191)or@|
@z

@x [44.1023] if batchmode, MakeTeX... scripts should be silent.
mode_command: begin print_ln; interaction:=cur_mod;
@y
mode_command: begin print_ln; interaction:=cur_mod;
if interaction = batch_mode
then kpse_make_tex_discard_errors := 1
else kpse_make_tex_discard_errors := 0;
@z

% [45.1120] `threshold' is both a function and a variable.  Since the
% function is used much less often than the variable, we'll change that
@x
@p function threshold(@!m:integer):scaled;
var @!d:scaled; {lower bound on the smallest interval size}
begin excess:=min_cover(0)-m;
if excess<=0 then threshold:=0
else  begin repeat d:=perturbation;
  until min_cover(d+d)<=m;
  while min_cover(d)>m do d:=perturbation;
  threshold:=d;
@y
@p function threshold_fn(@!m:integer):scaled;
var @!d:scaled; {lower bound on the smallest interval size}
begin excess:=min_cover(0)-m;
if excess<=0 then threshold_fn:=0
else  begin repeat d:=perturbation;
  until min_cover(d+d)<=m;
  while min_cover(d)>m do d:=perturbation;
  threshold_fn:=d;
@z

@x [45.1121] Change the call to the threshold function.
begin d:=threshold(m); perturbation:=0;
@y
begin d:=threshold_fn(m); perturbation:=0;
@z

@x [1122]
@ @<Replace an interval...@>=
begin repeat p:=link(p); info(p):=m;
decr(excess);@+if excess=0 then d:=0;
until value(link(p))>l+d;
v:=l+half(value(p)-l);
if value(p)-v>perturbation then perturbation:=value(p)-v;
r:=q;
repeat r:=link(r); value(r):=v;
until r=p;
link(q):=p; {remove duplicate values from the current list}
end
@y
@ @<Replace an interval...@>=
begin repeat p:=link(p); info(p):=m;
decr(excess);@+if excess=0 then d:=0;
until value(link(p))>l+d;
v:=l+halfp(value(p)-l);
if value(p)-v>perturbation then perturbation:=value(p)-v;
r:=q;
repeat r:=link(r); value(r):=v;
until r=p;
link(q):=p; {remove duplicate values from the current list}
end
@z

@x [45.1133] Use C macros to do the TFM writing, to avoid casting(?) problems.
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
The default definitions for |tfm_two| and |tfm_four| don't work.
I don't know why not. Some casting problem?

@d tfm_out(#) == put_byte (#, tfm_file)
@d tfm_two(#) == put_2_bytes (tfm_file, #)
@d tfm_four(#) == put_4_bytes (tfm_file, #)

@p procedure tfm_qqqq(@!x:four_quarters); {output four quarterwords to |tfm_file|}
@z

@x [47.1152] declare gf_buf as a pointer, for dynamic allocated
@!gf_buf:array[gf_index] of eight_bits; {buffer for \.{GF} output}
@y
@!gf_buf:^eight_bits; {dynamically-allocated buffer for \.{GF} output}
@z

@x [47.1154] omit write_gf
@<Declare generic font output procedures@>=
procedure write_gf(@!a,@!b:gf_index);
var k:gf_index;
begin for k:=a to b do write(gf_file,gf_buf[k]);
end;
@y
In C, we use a macro to call |fwrite| or |write| directly, writing all
the bytes to be written in one shot.  Much better than writing four
bytes at a time.
@z

@x [47.1163] C needs k to be 0..256 instead of 0..255.
procedure init_gf;
var @!k:eight_bits; {runs through all possible character codes}
@y
procedure init_gf;
var @!k:0..256; {runs through all possible character codes}
@z

@x [47.1169] Fix signed/unsigned comparison problem in C.
if prev_m-m_offset(cur_edges)+x_off>gf_max_m then
  gf_max_m:=prev_m-m_offset(cur_edges)+x_off
@y
if prev_m-toint(m_offset(cur_edges))+x_off>gf_max_m then
  gf_max_m:=prev_m-m_offset(cur_edges)+x_off
@z

@x [48.1185] INI = VIR.
base_ident:=" (INIMF)";
@y
if ini_version then base_ident:=" (INIMF)";
@z

@x [48.1188] Reading and writing of `base_file' is done in C.
@d dump_wd(#)==begin base_file^:=#; put(base_file);@+end
@d dump_int(#)==begin base_file^.int:=#; put(base_file);@+end
@d dump_hh(#)==begin base_file^.hh:=#; put(base_file);@+end
@d dump_qqqq(#)==begin base_file^.qqqq:=#; put(base_file);@+end
@y
@z

@x [48.1189]
@d undump_wd(#)==begin get(base_file); #:=base_file^;@+end
@d undump_int(#)==begin get(base_file); #:=base_file^.int;@+end
@d undump_hh(#)==begin get(base_file); #:=base_file^.hh;@+end
@d undump_qqqq(#)==begin get(base_file); #:=base_file^.qqqq;@+end
@y
@z

@x [48.1191] Avoid Pascal file convention.
x:=base_file^.int;
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
{Now we deal with dynamically allocating the memory. We don't provide
 all the fancy features \.{tex.ch} does---all that matters is enough to
 run the trap test with a memory size of 3000.}
@+init
if ini_version then begin
  {We allocated this at start-up, but now we need to reallocate.}
  libc_free (mem);
end;
@+tini
undump_int (mem_top); {Overwrite whatever we had.}
if mem_max < mem_top then mem_max:=mem_top; {Use at least what we dumped.}
if mem_min+1100>mem_top then goto off_base;
xmalloc_array (mem, mem_max - mem_min);
@z

@x [48.1199] l.22725 -  - Allow command line to override dumped value.
undump(batch_mode)(error_stop_mode)(interaction);
@y
undump(batch_mode)(error_stop_mode)(interaction);
if interaction_option<>unspecified_mode then interaction:=interaction_option;
@z

@x [48.1199] eof is like C feof here.
undump_int(x);@+if (x<>69069)or eof(base_file) then goto off_base
@y
undump_int(x);@+if (x<>69069)or feof(base_file) then goto off_base
@z

@x [48.1200] Eliminate probably-wrong word `preloaded' from base_idents.
print(" (preloaded base="); print(job_name); print_char(" ");
print_int(round_unscaled(internal[year]) mod 100); print_char(".");
@y
print(" (base="); print(job_name); print_char(" ");
print_int(round_unscaled(internal[year])); print_char(".");
@z

@x [49.1204] Dynamic allocation.
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
    {|memory_word|s for |mem| in \.{INIMF}}
  setup_bound_var (79)('error_line')(error_line);
  setup_bound_var (50)('half_error_line')(half_error_line);
  setup_bound_var (79)('max_print_line')(max_print_line);
  setup_bound_var (16384)('gf_buf_size')(gf_buf_size);
  if error_line > ssup_error_line then error_line := ssup_error_line;
  
  const_chk (main_memory);
  mem_top := mem_min + main_memory;
  mem_max := mem_top;

  xmalloc_array (gf_buf, gf_buf_size);

@+init
if ini_version then begin
  xmalloc_array (mem, mem_top - mem_min);
end;
@+tini
@z

@x [49.1204] Only do get_strings_started if ini.
@!init if not get_strings_started then goto final_end;
init_tab; {initialize the tables}
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr;@/
max_str_ptr:=str_ptr; max_pool_ptr:=pool_ptr; fix_date_and_time;
tini@/
@y  22833
@!init
if ini_version then begin
if not get_strings_started then goto final_end;
init_tab; {initialize the tables}
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr;@/
max_str_ptr:=str_ptr; max_pool_ptr:=pool_ptr; fix_date_and_time;
end;
tini@/
@z

@x
end_of_MF: close_files_and_terminate;
final_end: ready_already:=0;
@y
close_files_and_terminate;
final_end: do_final_end;
@z

% [49.1205] close_files_and_terminate: Print new line before
% termination; switch to editor if necessary.
@x
    slow_print(log_name); print_char(".");
    end;
  end;
@y
    slow_print(log_name); print_char(".");
    end;
  end;
print_ln;
if (edit_name_start<>0) and (interaction>batch_mode) then
    call_edit(str_pool,edit_name_start,edit_name_length,edit_line);
@z

@x [49.1209] Only do dump if ini.
  begin @!init store_base_file; return;@+tini@/
@y
  begin
    @!init if ini_version then begin store_base_file; return;end;@+tini@/
@z

%@x [49.1211] l.23002 - Handle %&base line.
%if (base_ident=0)or(buffer[loc]="&") then
%@y
%if (base_ident=0)or(buffer[loc]="&")or dump_line then
%@z

@x [51.1214] Add editor-switch variable to globals.
This section should be replaced, if necessary, by any special
modifications of the program
that are necessary to make \MF\ work at a particular installation.
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
its beginning if \MF\ is supposed to switch to an editor on exit.

@<Set init...@>=
edit_name_start:=0;
@z
