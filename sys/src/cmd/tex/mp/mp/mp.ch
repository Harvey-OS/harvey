% Change file for C version of MetaPost
%
% Derived from cmf.ch (the change file for the C version of mf)
% Comments beginning with bracketed numbers of the form [pp.nnn] refer to
% the corresponding part and module number in mf.web as it appears in Vol D.

% Modification History:
%
% Revision 0.0  90/02/18  hobby
% Base version (first cut) for MetaPost 0.0
% 
%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: only print changes
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\def\botofcontents{\vskip 0pt plus 1fil minus 1.5in}
@y
\def\botofcontents{\vskip 0pt plus 1fil minus 1.5in}
\let\maybe=\iffalse
\def\title{{\logo opqrstuq} changes for C}
\def\glob{13}\def\gglob{20, 25} % these are defined in module 1
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.2] banner line
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is MetaPost, Version 0.15' {printed when \MP\ starts}
@y
@d banner=='This is MetaPost, C Version 0.15' {printed when \MP\ starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.7] debug..gubed, stat..tats
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d debug==@{ {change this to `$\\{debug}\equiv\null$' when debugging}
@d gubed==@t@>@} {change this to `$\\{gubed}\equiv\null$' when debugging}
@f debug==begin
@f gubed==end
@#
@d stat==@{ {change this to `$\\{stat}\equiv\null$' when gathering
  usage statistics}
@d tats==@t@>@} {change this to `$\\{tats}\equiv\null$' when gathering
  usage statistics}
@y
@d stat==ifdef('STAT')
@d tats==endif('STAT')
@d debug==ifdef('DEBUG')
@d gubed==endif('DEBUG')
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.8] init..tini
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d init== {change this to `$\\{init}\equiv\.{@@\{}$' in the production version}
@d tini== {change this to `$\\{tini}\equiv\.{@@\}}$' in the production version}
@y
@d init==ifdef('INIMP')
@d tini==endif('INIMP')
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.9] Get rid of compiler directives
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Compiler directives@>=
@{@&$C-,A+,D-@} {no range check, catch arithmetic overflow, no debug overhead}
@!debug @{@&$C+,D+@}@+ gubed {but turn everything on when debugging}
@y
@<Compiler directives@>=
@{No compiler directives for C@}
@z

% ADD-OTHERCASES-CHANGES-HERE	%%% SENTINEL LINE -- DO NOT REMOVE %%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.11] compile-time constants, use logical names
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x %% mem_max %% NEEDED FOR ini_to_vir
@!mem_max=30000; {greatest index in \MP's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMP}, otherwise |>=mem_top|}
@y
@!mem_max=60000; {greatest index in \MP's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMP}, otherwise |>=mem_top|}
@z
@x %% error_line half_error_line max_print_line %%
@!error_line=72; {width of context lines on terminal error messages}
@!half_error_line=42; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line=79; {width of longest text lines output; should be at least 60}
@y
@!error_line=79; {width of context lines on terminal error messages}
@!half_error_line=50; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line=79; {width of longest text lines output; should be at least 60}
@z
@x %% file_name_size pool_name %%
@!file_name_size=40; {file names shouldn't be longer than this}
@!pool_name='MPlib:MP.POOL                         ';
  {string of length |file_name_size|; tells where the string pool appears}
@y
@!file_name_size=FILENAMESIZE; {file names shouldn't be longer than this}
@!pool_name='mp.pool';
  {string of length |file_name_size|; tells where the string pool appears}
@!mem_top=60000; {largest index in the |mem| array dumped by \.{INIMP};
  must be substantially larger than |mem_min|
  and not greater than |mem_max|}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.12] sensitive compile-time constants
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d mem_min=0 {smallest index in the |mem| array, must not be less
  than |min_halfword|}
@d mem_top==30000 {largest index in the |mem| array dumped by \.{INIMP};
  must be substantially larger than |mem_min|
  and not greater than |mem_max|}
@y
@d mem_min=0 {smallest index in the |mem| array, must not be less
  than |min_halfword|}
@z
@x %% |max_in_open| is too small for many current fonts (e.g., Greek) %%
@d max_in_open=6 {maximum number of input files and error insertions that
  can be going on simultaneously}
@y
@d max_in_open=15 {maximum number of input files and error insertions that
  can be going on simultaneously}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.16] Use C macros for incr() and decr()
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d incr(#) == #:=#+1 {increase a variable by unity}
@d decr(#) == #:=#-1 {decrease a variable by unity}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.22] accept <tab> and <ff> as valid input chars
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@^character set dependencies@>
@^system dependencies@>

@<Set init...@>=
for i:=0 to @'37 do xchr[i]:=' ';
for i:=@'177 to @'377 do xchr[i]:=' ';

@y
@^character set dependencies@>
@^system dependencies@>

@d tab = @'11 { ASCII horizontal tab }
@d form_feed = @'14 { ASCII form-feed }

@<Set init...@>=
for i:=0 to @'37 do xchr[i]:=' ';
for i:=@'177 to @'377 do xchr[i]:=' ';
xchr[tab]:=chr(tab);
xchr[form_feed]:=chr(form_feed);

@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3.25] add real_name_of_file array for search path resolution
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
is crucial for our purposes. We shall assume that |name_of_file| is a variable
of an appropriate type such that the \PASCAL\ run-time system being used to
implement \MP\ can open a file whose external name is specified by
|name_of_file|.
@^system dependencies@>

@<Glob...@>=
@!name_of_file:packed array[1..file_name_size] of char;@;@/
  {on some systems this may be a \&{record} variable}
@!name_length:0..file_name_size;@/{this many characters are actually
  relevant in |name_of_file| (the rest are blank)}
@y
is crucial for our purposes.  We make real_name_of_file hold the
|name_of_file| plus a directory specifier to open the file in Unix.
@^system dependencies@>

@<Glob...@>=
@!name_of_file,@!real_name_of_file:packed array[1..file_name_size] of char;@;@/
  {on some systems this may be a \&{record} variable}
@!name_length:0..file_name_size;@/{this many characters are actually
  relevant in |name_of_file| (the rest are blank)}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3.26] file opening
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
begin rewrite(f,name_of_file,'/O'); b_open_in:=rewrite_OK(f);
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
@ The \ph\ compiler with which the original version of \MF\ was prepared
extends the rules of \PASCAL\ in a very convenient way for file opening.
{\mc UNIX} \PASCAL\ isn't nearly as nice as \ph.
Normally, it bombs out if a file open fails.
An external C procedure, |test_access| is used to check whether or not the
open will work, and it returns
|true| or |false|. The |name_of_file| global holds the file name whose access
is to be tested.
The first parameter for |test_access| is the access mode,
one of |read_access_mode| or |write_access_mode|.

We also implement path searching in |test_access|:  its second parameter is
one of the ``file path'' constants defined below.  If |name_of_file|
doesn't start with |'/'| then |test_access| tries prepending pathnames
from the appropriate path list until success or the end of path list
is reached.
On return, |real_name_of_file| contains the original name with the path
that succeeded (if any) prepended.  It is the name used in the various
open procedures.

Note that |a_open_in| has been redefined to take an additional argument,
which should be one of the ``file path'' specifiers.
Path searching is not done for output files.

In the C-Version |X_open_in| and |X_open_out|, |w_open_in| and |w_open_out|
are completely replaced by C preprocessor macros.  We retain
the testaccess() routine since it was already written in C and it
implements the path searching mechanism.
@d read_access_mode=4  {``read'' mode for |test_access|}
@d write_access_mode=2 {``write'' mode for |test_access|}

@d no_file_path=0    {no path searching should be done}
@d TeX_font_file_path=3 {path specifier for \.{TFM} files}
@d MF_input_file_path=6 {path specifier for \.{.mf} \.{input} files}
@d MP_input_file_path=9 {path specifier for \.{input} files}
@d MP_mem_file_path=10 {path specifier for \.{MEM} files}
@d MP_pool_file_path=11  {path specifier for the pool file}

@z

@x
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
@ Files are closed in C via macros aclose, bclose, and wclose.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3.30] we have our own input_ln implemented in C
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
@ The |input_ln| function will be defined in an external C routine.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3.32] don't need to open terminal files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ Here is how to open the terminal files
in \ph. The `\.{/I}' switch suppresses the first |get|.
@^system dependencies@>

@d t_open_in==reset(term_in,'TTY:','/O/I') {open the terminal for text input}
@d t_open_out==rewrite(term_out,'TTY:','/O') {open the terminal for text output}
@y
@ Here is how to open the terminal files
in C: simply use stdin and stdout.
@^system dependencies@>

@d t_open_in==term_in:=stdin {assign stdin as terminal input}
@d t_open_out==term_out:=stdout {assign stdout as terminal output}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3.33] flushing output
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
these operations can be specified in \ph:
@^system dependencies@>

@d update_terminal == break(term_out) {empty the terminal output buffer}
@d clear_terminal == break_in(term_in,true) {clear the terminal input buffer}
@d wake_up_terminal == do_nothing {cancel the user's cancellation of output}
@y
these operations can be specified in C:
@^system dependencies@>

@d update_terminal == flush(term_out) {empty the terminal output buffer}
@d clear_terminal == flush(term_in) {clear the terminal input buffer}
@d wake_up_terminal == do_nothing {cancel the user's cancellation of output}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3.36] rescanning the command line 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
@ The following program does the required initialization
and also retrieves a possible command line.
@^system dependencies@>

@p
function init_terminal:boolean; {gets the terminal input started}
label exit;

var
    i, j, k: integer;
    arg: packed array[1..100] of char;
    first_time: boolean;

begin
    t_open_in;
    if argc > 1 then begin
        last := first;
        for i := 2 to argc do begin
	    argv(i, arg);
	    j := 1;
	    k := 1; {find last non-blank char in arg}
	    while (k < 100) and (arg[k] <> 0) do@/
		incr(k);
	    decr(k);
	    while (k > 1) and (arg[k] = ' ') do@/
		decr(k);
	    while (j <= k) do begin
	        buffer[last] := xord[arg[j]];
		incr(j); incr(last);
            end;
	    if k > 1 then begin
	        buffer[last] := xord[' '];
	        incr(last);
            end;
	end;
	if last > first then begin
	    loc := first;
            init_terminal := true;
            return;
	end;
    end;
    first_time := true;
    loop@+begin
        wake_up_terminal; write(term_out, '**'); update_terminal;
@.**@>
        if not input_ln(term_in,not first_time) then begin {this shouldn't happen}
            write_ln(term_out);
            write_ln(term_out, '! End of file on the terminal... why?');
@.End of file on the terminal@>
            init_terminal:=false;
	    return;
        end;
	first_time:=false;
        loc:=first;
        while (loc<last)and(buffer[loc]=" ") do
            incr(loc);

        if loc<last then begin
           init_terminal:=true;
           return; {return unless the line was all blank}
        end;
        write_ln(term_out, 'Please type the name of your input file.');
@.Please type the name...@>
    end;
exit:
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4.51] a_open_in of pool file needs path specifier
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if a_open_in(pool_file) then
@y
if a_open_in(pool_file,MP_pool_file_path) then
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4.51,52,53] make MF.POOL lowercase in messages
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
else  bad_pool('! I can''t read MP.POOL.')
@y
else  bad_pool('! I can''t read mp.pool.')
@z
@x
begin if eof(pool_file) then bad_pool('! MP.POOL has no check sum.');
@y
begin if eof(pool_file) then bad_pool('! mp.pool has no check sum.');
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
done: if a<>@$ then bad_pool('! mp.pool doesn''t match; tangle me again.');
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [6.76] eliminate non-local goto
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
procedure that quietly terminates the program.

@<Error hand...@>=
procedure jump_out;
begin goto end_of_MP;
end;
@y
procedure that quietly terminates the program.

Use the value of |history| to determine what exit-code to use.  We use
1 if |history <> spotless| and 0 otherwise.

@<Error hand...@>=
procedure jump_out;
begin
close_files_and_terminate;
ready_already:=0;
if (history <> spotless) and (history <> warning_issued) then
    uexit(1)
else
    uexit(0);
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [6.79] switch-to-editor option
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
We do this by calling the external procedure |calledit| with a pointer to
the filename, its length, and the line number.
However, here we just set up the variables that will be used as arguments,
since we don't want to do the switch-to-editor until after TeX has closed
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [7.107-7.111] replace make_fraction etc. with external routines
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Not implemented.  The only versions I know of are in cmf/MFlib/mf_arith.c
% These use "asm" feature to generate assembly for certain machines with
% default that is essentially identical to what is in mf.web
%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [9.155] Efficiency in C
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The operation of subtracting |min_halfword| occurs rather frequently in
\MP, so it is convenient to abbreviate this operation by using the macro
|ho| defined here.  \MP\ will run faster with respect to compilers that
don't optimize the expression `|x-0|', if this macro is simplified in the
obvious way when |min_halfword=0|. Similarly, |qi| and |qo| are used for
input to and output from quarterwords.
@^system dependencies@>

@d ho(#)==#-min_halfword
  {to take a sixteen-bit item from a halfword}
@d qo(#)==#-min_quarterword {to read eight bits from a quarterword}
@d qi(#)==#+min_quarterword {to store eight bits in a quarterword}

@y
@ The operation of subtracting |min_halfword| occurs rather frequently in
\MP, so it is convenient to abbreviate this operation by using the macro
|ho| defined here.  \MP\ will run faster with respect to compilers that
don't optimize the expression `|x-0|', if this macro is simplified in the
obvious way when |min_halfword=0|. Similarly, |qi| and |qo| are used for
input to and output from quarterwords.

We need not do this in C, since the min_xxx values are all zero, and
we can't depend on most C compilers to optimize this.
@^system dependencies@>

@d ho(#)==#
@d qo(#)==#
@d qi(#)==#
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [9.156] Delete two_halves, four_quarters and memory_word and leave them
% for direct C definition
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [10.169] Fix an unsigned/signed problem in getnode
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if r>p+1 then @<Allocate from the top of node |p| and |goto found|@>;
@y
if r>toint(p+1) then @<Allocate from the top of node |p| and |goto found|@>;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [11.178] fix the word "free" so that it doesn't conflict with a runtime proc
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
been included. (You may want to decrease the size of |mem| while you
@^debugging@>
are debugging.)
@y
been included. (You may want to decrease the size of |mem| while you
@^debugging@>
are debugging.)

@d free==free_arr
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [11.182] Eliminate two unsigned comparisons to zero
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
repeat if (p>=lo_mem_max)or(p<mem_min) then clobbered:=true
  else if (rlink(p)>=lo_mem_max)or(rlink(p)<mem_min) then clobbered:=true
@y
repeat if (p>=lo_mem_max) then clobbered:=true
  else if (rlink(p)>=lo_mem_max) then clobbered:=true
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [12.194] fix_date_and_time
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [12.199] allow <tab> and <ff> as input
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
for k:=0 to " "-1 do char_class[k]:=invalid_class;
@y
for k:=0 to " "-1 do char_class[k]:=invalid_class;
char_class[tab]:=space_class;
char_class[form_feed]:=space_class;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.768] area and extension rules
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The file names we shall deal with for illustrative purposes have the
following structure:  If the name contains `\.>' or `\.:', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@^system dependencies@>

We can scan such file names easily by using two global variables that keep track
of the occurrences of area and extension delimiters.  Note that these variables
cannot be of type |pool_ptr| because a string pool compaction could occur while
scanning a file name.

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
cannot be of type |pool_ptr| because a string pool compaction could occur while
scanning a file name.

@<Glob...@>=
@!area_delimiter:integer; {most recent `\./' relative to |str_start[str_ptr]|}
@!ext_delimiter:integer; {the relevant `\..', if any}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.769] MF area directories
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d MP_area=="MPinputs:"
@.MPinputs@>
@d MF_area=="MFinputs:"
@.MFinputs@>
@d MP_font_area=="TeXfonts:"
@.TeXfonts@>
@y
In C, the default paths are specified in a seprate
file, ``site.h''.  The file opening procedures do path searching
based either on those default paths, or on paths given by the user
in ``environment'' variables.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.771] more_name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
begin if c=" " then more_name:=false
else  begin if (c=">")or(c=":") then
@y
begin if (c=" ")or(c=tab) then more_name:=false
else  begin if (c="/") then
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.775] default mem
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d mem_default_length=15 {length of the |MP_mem_default| string}
@d mem_area_length=6 {length of its area part}
@d mem_ext_length=4 {length of its `\.{.mem}' part}
@y
In C, we don't give the area part, instead depending
on the path searching that will happen during file opening.

@d mem_default_length=9 {length of the |MP_mem_default| string}
@d mem_area_length=0 {length of its area part}
@d mem_ext_length=4 {length of its `\.{.mem}' part}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.776] plain mem location
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
MP_mem_default:='MPlib:plain.mem';
@y
MP_mem_default:='plain.mem';
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.779] w_open_in of mem file needs to be called only once
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  pack_buffered_name(0,loc,j-1); {try first without the system file area}
  if w_open_in(mem_file) then goto found;
  pack_buffered_name(mem_area_length,loc,j-1);
    {now try the system mem file area}
  if w_open_in(mem_file) then goto found;
@y
  pack_buffered_name(0,loc,j-1);
  if w_open_in(mem_file) then goto found;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.780] make_name_string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
which simply makes a \MP\ string from the value of |name_of_file|, should
ideally be changed to deduce the full name of file~|f|, which is the file
most recently opened, if it is possible to do this in a \PASCAL\ program.
@^system dependencies@>

This routine might be called after string memory has overflowed, hence
we dare not use `|str_room|'.

@p function make_name_string:str_number;
var @!k:1..file_name_size; {index into |name_of_file|}
begin if (pool_ptr+name_length>pool_size)or(str_ptr=max_strings) then
  make_name_string:="?"
else  begin for k:=1 to name_length do append_char(xord[name_of_file[k]]);
  make_name_string:=make_string;
  end;
end;
@y
which simply makes a \MP\ string from the value of |name_of_file|, should
ideally be changed to deduce the full name of file~|f|, which is the file
most recently opened, if it is possible to do this in a \PASCAL\ program.
With the C version, we know that |real_name_of_file|
contains |name_of_file| prepended with the directory name that was found
by path searching.
If |real_name_of_file| starts with |'./'|, we don't use that part of the
name, since {\mc UNIX} users understand that.
@^system dependencies@>

This routine might be called after string memory has overflowed, hence
we dare not use `|str_room|'.

@p function make_name_string:str_number;
var @!k,@!kstart:1..file_name_size; {index into |name_of_file|}
begin
k:=1;
while (k<file_name_size) and (xord[real_name_of_file[k]]<>" ") do
    incr(k);
name_length:=k-1; {the real |name_length|}
if (pool_ptr+name_length>pool_size)or(str_ptr=max_strings) then
  make_name_string:="?"
else  begin
  if (xord[real_name_of_file[1]]=".") and (xord[real_name_of_file[2]]="/") then
    kstart:=3
  else
    kstart:=1;
  for k:=kstart to name_length do append_char(xord[real_name_of_file[k]]);
  make_name_string:=make_string;
  end;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.780] change X_make_name_string to be C macros
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
function a_make_name_string(var @!f:alpha_file):str_number;
begin a_make_name_string:=make_name_string;
end;
function b_make_name_string(var @!f:byte_file):str_number;
begin b_make_name_string:=make_name_string;
end;
function w_make_name_string(var @!f:word_file):str_number;
begin w_make_name_string:=make_name_string;
end;
@y
{These are all replaced by macros in C which call makenamestring}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.781] scan_file_name ignore leading tabs as well as spaces
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.787] <scan_file_name...> needs similar leading tab treatment
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Scan file name in the buffer@>=
begin begin_name; k:=first;
while (buffer[k]=" ")and(k<last) do incr(k);
@y
@ @<Scan file name in the buffer@>=
begin begin_name; k:=first;
while ((buffer[k]=" ")or(buffer[k]=tab))and(k<last) do incr(k);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.793] a_open_in of input file needs path selector
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if a_open_in(cur_file) then try_extension:=true
else begin if str_vs_str(ext,".mf")=0 then in_area:=MF_area
  else in_area:=MP_area;
  pack_file_name(cur_name,in_area,ext);
  try_extension:=a_open_in(cur_file);
  end;
@y
if str_vs_str(ext,".mf")=0 then
  try_extension:=a_open_in(cur_file,MF_input_file_path)
else try_extension:=a_open_in(cur_file,MP_input_file_path);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38.793] get rid of return of name to string pool
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Flush |name| and replace it with |cur_name| if it won't be needed@>=
flush_string(name); name:=cur_name; cur_name:=0
@y
@<Flush |name| and replace it with |cur_name| if it won't be needed@>=
do_nothing
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Give a_open_in a path selector
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if not a_open_in(cur_file) then
  begin end_file_reading;
@y
if not a_open_in(cur_file,no_file_path) then
  begin end_file_reading;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Invoke makempx
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Try to make sure |name_of_file| refers to a valid \.{MPX} file and
  |goto not_found| if there is a problem@>=
copy_old_name(name)
{System-dependent code should be added here}
@y
@<Try to make sure |name_of_file| refers to a valid \.{MPX} file and
  |goto not_found| if there is a problem@>=
copy_old_name(name);
if not call_make_mpx(old_file_name,name_of_file) then goto not_found
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [45.1120,1121] Fix threshold conflict with local variable names
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

@ The |skimp| procedure reduces the current list to at most |m| entries,
by changing values if necessary. It also sets |info(p):=k| if |value(p)|
is the |k|th distinct value on the resulting list, and it sets
|perturbation| to the maximum amount by which a |value| field has
been changed. The size of the resulting list is returned as the
value of |skimp|.

@p function skimp(@!m:integer):integer;
var @!d:scaled; {the size of intervals being coalesced}
@!p,@!q,@!r:pointer; {list manipulation registers}
@!l:scaled; {the least value in the current interval}
@!v:scaled; {a compromise value}
begin d:=threshold(m); perturbation:=0;
q:=temp_head; m:=0; p:=link(temp_head);
while p<>inf_val do
  begin incr(m); l:=value(p); info(p):=m;
  if value(link(p))<=l+d then
    @<Replace an interval of values by its midpoint@>;
  q:=p; p:=link(p);
  end;
skimp:=m;
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

@ The |skimp| procedure reduces the current list to at most |m| entries,
by changing values if necessary. It also sets |info(p):=k| if |value(p)|
is the |k|th distinct value on the resulting list, and it sets
|perturbation| to the maximum amount by which a |value| field has
been changed. The size of the resulting list is returned as the
value of |skimp|.

@p function skimp(@!m:integer):integer;
var @!d:scaled; {the size of intervals being coalesced}
@!p,@!q,@!r:pointer; {list manipulation registers}
@!l:scaled; {the least value in the current interval}
@!v:scaled; {a compromise value}
begin d:=compute_threshold(m); perturbation:=0;
q:=temp_head; m:=0; p:=link(temp_head);
while p<>inf_val do
  begin incr(m); l:=value(p); info(p):=m;
  if value(link(p))<=l+d then
    @<Replace an interval of values by its midpoint@>;
  q:=p; p:=link(p);
  end;
skimp:=m;
end;
@z


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [45.1133] writing the tfm file
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
begin tfm_out(qo(x.b0)); tfm_out(qo(x.b1)); tfm_out(qo(x.b2));
tfm_out(qo(x.b3));
end;
@y
Under {\mc UNIX}, we are using the binary input and output routines.
Hence, we redefine all the {\mc TFM} input and output in terms of those
routines.

@d tfm_out(#) == b_write_byte(tfm_file, #)
@d tfm_two(#) == b_write_2_bytes(tfm_file, #)
@d tfm_four(#) == b_write_4_bytes(tfm_file, #)

@p procedure tfm_qqqq(@!x:four_quarters); {output four quarterwords to |tfm_file|}
begin tfm_out(qo(x.b0)); tfm_out(qo(x.b1)); tfm_out(qo(x.b2));
tfm_out(qo(x.b3));
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [See TeX module 560] Fix the font_ps_name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x Be prepared to use the font family name if \.{TFM} isn't always OK
var @!file_opened:boolean; {has |tfm_infile| been opened?}
@y
var @!file_opened:boolean; {has |tfm_infile| been opened?}
@!l:eight_bits; {length of the font family name}
@z

@x
font_ps_name[n]:=fname;
add_str_ref(fname); {the preceding two statements are system dependent}
@y
if font_ps_name[n]=0 then
  begin font_ps_name[n]:=fname;
  add_str_ref(fname);
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [See TeX module 564] Reading tfm files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% As a special case, whenever we open a tfm file for input, we read its
% first byte into "tfm_temp" right away.
@x
@d tfget==get(tfm_infile)
@d tfbyte==tfm_infile^
@y
@d tfget==tfm_temp:=getc(tfm_infile)
@d tfbyte==tfm_temp
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [See TeX module 560] Fix the font_ps_name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x Uncomment the |if| statement if you want to use the font family
tf_ignore(4*(lh-2))
@y
lh:=4*(lh-2); {from now on, |lh| is bytes left in the header}
font_ps_name[n]:=0;
{open comment}
if lh>40 then
  begin tf_ignore(40); lh:=lh-40;
  tfget; l:=tfbyte; decr(lh);
  if (l<20)and(l<=lh) then
    begin lh:=lh-l;
    str_room(l);
    for jj:=l downto 1 do
      begin tfget; append_char(tfbyte);
      end;
    font_ps_name[n]:=make_string;
    end;
  end;
{close comment}
tf_ignore(lh)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [See TeX module 563] Fix TFM file opening
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if cur_area="" then cur_area:=MP_font_area;
if cur_ext="" then cur_ext:=".tfm";
pack_cur_name;
if not b_open_in(tfm_infile) then goto bad_tfm;
@y
if cur_ext="" then cur_ext:=".tfm";
pack_cur_name;
if not b_open_in(tfm_infile) then goto bad_tfm;
tfget; {load the first byte into |tfm_temp|}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [48.1188,1189,1191] leave dump and undump macros for the C preprocessor
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d dump_wd(#)==begin mem_file^:=#; put(mem_file);@+end
@d dump_int(#)==begin mem_file^.int:=#; put(mem_file);@+end
@d dump_hh(#)==begin mem_file^.hh:=#; put(mem_file);@+end
@d dump_qqqq(#)==begin mem_file^.qqqq:=#; put(mem_file);@+end
@y
@z

@x
@d undump_wd(#)==begin get(mem_file); #:=mem_file^;@+end
@d undump_int(#)==begin get(mem_file); #:=mem_file^.int;@+end
@d undump_hh(#)==begin get(mem_file); #:=mem_file^.hh;@+end
@d undump_qqqq(#)==begin get(mem_file); #:=mem_file^.qqqq;@+end
@y
@z

@x
x:=mem_file^.int;
@y
undump_int(x);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [49.1204] Add call to exit() depending upon value of `history'
%           Also, add call to set_paths
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
t_open_out; {open the terminal for output}
@y
t_open_out; {open the terminal for output}
set_paths;
@z

@x
end_of_MP: close_files_and_terminate;
final_end: ready_already:=0;
@y
close_files_and_terminate;
final_end: ready_already:=0;
if (history <> spotless) and (history <> warning_issued) then
    uexit(1)
else
    uexit(0);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [49.1205] print new line before termination; switch to editor if nec.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
    print(log_name); print_char(".");
    end;
  end;
@y
    print(log_name); print_char(".");
    end;
  end;
print_ln;
if (edit_name_start<>0) and (interaction>batch_mode) then
    calledit(str_pool,edit_name_start,edit_name_length,edit_line);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [50.1212,1213] debugging
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
program below. (If |m=13|, there is an additional argument, |l|.)
@.debug \#@>

@d breakpoint=888 {place where a breakpoint is desirable}

@<Last-minute...@>=
@!debug procedure debug_help; {routine to display various things}
@y
program below. (If |m=13|, there is an additional argument, |l|.)
@.debug \#@>

Since the C version of |read| only works for |text_char| variables,
we need another routine to read integers from the terminal.

@d breakpoint=888 {place where a breakpoint is desirable}

@<Last-minute...@>=
@!debug function read_int:integer;
var @!c:text_char;
@!k:ASCII_code;
@!n:integer;
@!neg:boolean;
begin read(term_in,c);
n:=0; neg:=c=xchr['-'];
if neg then read(term_in,c);
k:=xord[c];
while (k>="0")and(k<="9") do
  begin n:=10*n+k-"0";
  read(term_in,c);
  k:=xord[c];
  end;
if neg then read_int:=-n else read_int:=n;
end;
@#
procedure debug_help; {routine to display various things}
@z

@x
  read(term_in,m);
  if m<0 then return
  else if m=0 then
    begin goto breakpoint;@\ {go to every label at least once}
    breakpoint: m:=0; @{'BREAKPOINT'@}@\
    end
  else  begin read(term_in,n);
@y
  m:=read_int;
  if m<0 then return
  else if m=0 then
    begin goto breakpoint;@\ {go to every label at least once}
    breakpoint: m:=0; @{'BREAKPOINT'@}@\
    end
  else  begin n:=read_int;
@z

@x
13: begin read(term_in,l); print_cmd_mod(n,l);
  end;
@y
13: begin l:=read_int; print_cmd_mod(n,l);
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [51.1207,1208] add editor-switch variable to globals
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@* \[48] System-dependent changes.
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
@* \[48] System-dependent changes.
Here are the variables used to hold ``switch-to-editor'' information.
@^system dependencies@>

@<Global...@>=
@!edit_name_start: pool_pointer;
@!edit_name_length,@!edit_line,@!tfm_temp: integer;

@ The |edit_name_start| will be set to point into |str_pool| somewhere after
its beginning if \MP\ is supposed to switch to an editor on exit.

@<Set init...@>=
edit_name_start:=0;
@z
