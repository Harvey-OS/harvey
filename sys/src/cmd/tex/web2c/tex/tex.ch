% tex.ch for C compilation with web2c, derived from various other change files.
% By Tim Morgan, UC Irvine ICS Department, and many others.
%
% Be very careful when making changes to this file, as it is used to
% generate TeX, e-TeX, and PDFTeX, and most changes require similar
% changes to be made to the Omega sources.
%
% (05/28/86) ETM Started with TeX 2.0
% (06/03/87) ETM Brought up to TeX 2.2
% (09/26/87) ETM Brought up to TeX 2.3
% (10/01/87) ETM Brought up to TeX 2.5
% (12/21/87) ETM Brought up to TeX 2.7
% (01/14/88) ETM Brought up to TeX 2.9
% (02/20/88) PAM Revised format and module numbers
% (03/01/88) ETM Eliminated some unused variables and unnecesary tests
% (05/09/88) ETM Added yet another casting bug fix
% (06/21/88) ETM Brought up to TeX version 2.93
% (12/11/88) ETM Brought up to TeX version 2.94
% (01/12/89) PAM Brought up to TeX version 2.95
% (02/14/89) ETM Brought up to TeX version 2.96
% (03/10/89) ETM Brought up to TeX version 2.98
% (07/06/89) ETM Brought up to TeX version 2.991
% (11/30/89) KB  To version 2.992 (8-bit).
% (01/10/90) SR  To version 2.993.
% (03/27/90) KY  To version 3.0.
% (more recent changes in ChangeLog)
%
% The TeX program is copyright (C) 1982 by D. E. Knuth.
% TeX is a trademark of the American Mathematical Society.
%
% Includes MLTEX.CH (Version 2.2) in text format, as of Dec 17, 1995.
% MLTeX is copyright (C) 1990-92 by Michael J. Ferguson; all rights reserved.
% MLTeX Version 2.2 is copyright (C) 1995 by B. Raichle; all rights reserved.
%
% The MLTeX changes are copyrighted so that we have some chance to
% forbid unauthorized copies; we explicitly authorize copying of
% correct MLTeX implementations, and not of incorrect ones!
%
% (This means that you can use the MLTeX changes as free as you can
% use TeX and its algorithm.)
%
% Copying of this file is authorized only if either
% (1) you make absolutely no changes to your copy, including name, or
% (2) if you do make changes, you name it to something other than
%     "mltex.ch", "char_sub.ch", or "charsub.ch".
%
% The module numbers in this change file refer to TEX.WEB 3.14159 as
% of March, 1995 (published as Donald E. Knuth, TeX: The Program,
% Volume B of Computers & Typesetting).

@x [0.0] l.83 - WEAVE: print changes only.
  \def\?##1]{\hbox to 1in{\hfil##1.\ }}
  }
@y 83
  \def\?##1]{\hbox{Changes to \hbox to 1em{\hfil##1}.\ }}
  }
\let\maybe=\iffalse
@z

@x [1.2] l.185 - MLTeX: add comment about banner line change
November 1984].
@y
November 1984].

ML\TeX{} will add new primitives changing the behaviour of \TeX.  The
|banner| string has to be changed.  We do not change the |banner|
string, but will output an additional line to make clear that this is
a modified \TeX{} version.

@z

@x [1.4] l.233 - program header
Actually the heading shown here is not quite normal: The |program| line
does not mention any |output| file, because \ph\ would ask the \TeX\ user
to specify a file name if |output| were specified here.
@^system dependencies@>
@y
@z

@x [1.4] l.243 - labels in outer block not needed
program TEX; {all file names are defined dynamically}
label @<Labels in the outer block@>@/
@y
program TEX; {all file names are defined dynamically}
@z

@x [1.6] l.267 - labels in outer block not needed
@ Three labels must be declared in the main program, so we give them
symbolic names.

@d start_of_TEX=1 {go here when \TeX's variables are initialized}
@d end_of_TEX=9998 {go here to close files and terminate gracefully}
@d final_end=9999 {this label marks the ending of the program}

@<Labels in the out...@>=
start_of_TEX@t\hskip-2pt@>, end_of_TEX@t\hskip-2pt@>,@,final_end;
  {key control points}
@y
@ For Web2c, labels are not declared in the main program, but
we still have to declare the symbolic names.

@d start_of_TEX=1 {go here when \TeX's variables are initialized}
@d final_end=9999 {this label marks the ending of the program}
@z

% Here we change these WEB symbols, which are used much as #ifdef's
% are in C, into something which will get translated into actual #ifdef's.
@x [1.7] l.292 - debug..gubed, stat..tats
@d debug==@{ {change this to `$\\{debug}\equiv\null$' when debugging}
@d gubed==@t@>@} {change this to `$\\{gubed}\equiv\null$' when debugging}
@y
@d debug==ifdef('TEXMF_DEBUG')
@d gubed==endif('TEXMF_DEBUG')
@z
@x [1.7] l.297 - debug..gubed, stat..tats
@d stat==@{ {change this to `$\\{stat}\equiv\null$' when gathering
  usage statistics}
@d tats==@t@>@} {change this to `$\\{tats}\equiv\null$' when gathering
  usage statistics}
@y
@d stat==ifdef('STAT')
@d tats==endif('STAT')
@z

@x [1.8] Somewhat different for `init...tini'..  310 m.8
the codewords `$|init|\ldots|tini|$'.

@d init== {change this to `$\\{init}\equiv\.{@@\{}$' in the production version}
@d tini== {change this to `$\\{tini}\equiv\.{@@\}}$' in the production version}
@y 314
the codewords `$|init|\ldots|tini|$' for declarations and by the codewords
`$|Init|\ldots|Tini|$' for executable code.  This distinction is helpful for
implementations where a run-time switch differentiates between the two
versions of the program.

@d init==ifdef('INITEX')
@d tini==endif('INITEX')
@d Init==init if ini_version then begin
@d Tini==end;@+tini
@f Init==begin
@f Tini==end
@z

@x [1.8] l.319 - init...tini is dynamic
@!init @<Initialize table entries (done by \.{INITEX} only)@>@;@+tini
@y  318
@!Init @<Initialize table entries (done by \.{INITEX} only)@>@;@+Tini
@z

@x [1.11] l.375 - Compile-time constants: most removed for dynamic allocation.
@<Constants...@>=
@!mem_max=30000; {greatest index in \TeX's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INITEX}, otherwise |>=mem_top|}
@!mem_min=0; {smallest index in \TeX's internal |mem| array;
  must be |min_halfword| or more;
  must be equal to |mem_bot| in \.{INITEX}, otherwise |<=mem_bot|}
@!buf_size=500; {maximum number of characters simultaneously present in
  current lines of open files and in control sequences between
  \.{\\csname} and \.{\\endcsname}; must not exceed |max_halfword|}
@!error_line=72; {width of context lines on terminal error messages}
@!half_error_line=42; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line=79; {width of longest text lines output; should be at least 60}
@!stack_size=200; {maximum number of simultaneous input sources}
@!max_in_open=6; {maximum number of input files and error insertions that
  can be going on simultaneously}
@!font_max=75; {maximum internal font number; must not exceed |max_quarterword|
  and must be at most |font_base+256|}
@!font_mem_size=20000; {number of words of |font_info| for all fonts}
@!param_size=60; {maximum number of simultaneous macro parameters}
@!nest_size=40; {maximum number of semantic levels simultaneously active}
@!max_strings=3000; {maximum number of strings; must not exceed |max_halfword|}
@!string_vacancies=8000; {the minimum number of characters that should be
  available for the user's control sequences and font names,
  after \TeX's own error messages are stored}
@!pool_size=32000; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all fonts and
  control sequences; must exceed |string_vacancies| by the total
  length of \TeX's own strings, which is currently about 23000}
@!save_size=600; {space for saving values outside of current group; must be
  at most |max_halfword|}
@!trie_size=8000; {space for hyphenation patterns; should be larger for
  \.{INITEX} than it is in production versions of \TeX}
@!trie_op_size=500; {space for ``opcodes'' in the hyphenation patterns}
@!dvi_buf_size=800; {size of the output buffer; must be a multiple of 8}
@!file_name_size=40; {file names shouldn't be longer than this}
@!pool_name='TeXformats:TEX.POOL                     ';
  {string of length |file_name_size|; tells where the string pool appears}
@y
@d file_name_size == maxint
@d ssup_error_line = 255
@d ssup_max_strings ==65535
{Larger values may be used, but then the arrays consume much more memory.}
@d ssup_trie_opcode == 65535
@d ssup_trie_size == 65535

@d ssup_hyph_size == 65535 {Changing this requires changing (un)dumping!}
@d iinf_hyphen_size == 610 {Must be not less than |hyph_prime|!}

@d max_font_max=2000 {maximum number of internal fonts; this can be
                      increased, but |hash_size+max_font_max|
                      should not exceed 29000.}
@d font_base=0 {smallest internal font number; must be
                |>= min_quarterword|; do not change this without
                modifying the dynamic definition of the font arrays.}


@<Constants...@>=
@!mem_bot=@"8000000; {smallest index in the |mem| array dumped by \.{INITEX};
  must not be less than |mem_min|}
  {Use |mem_bot=0| for compilers which cannot decrement pointers.}
@!hash_offset=514; {smallest index in hash array, i.e., |hash_base| }
  {Use |hash_offset=0| for compilers which cannot decrement pointers.}
@!trie_op_size=1501; {space for ``opcodes'' in the hyphenation patterns;
  best if relatively prime to 313, 361, and 1009.}
@!neg_trie_op_size=-1501; {for lower |trie_op_hash| array bound;
  must be equal to |-trie_op_size|.}
@!min_trie_op=0; {first possible trie op code for any language}
@!max_trie_op=ssup_trie_opcode; {largest possible trie opcode for any language}
@!pool_name=TEX_POOL_NAME; {this is configurable, for the sake of ML-\TeX}
  {string of length |file_name_size|; tells where the string pool appears}
@#
@!inf_main_memory = 2999;
@!sup_main_memory = 8000000;

@!inf_trie_size = 8000;
@!sup_trie_size = ssup_trie_size;

@!inf_max_strings = 3000;
@!sup_max_strings = ssup_max_strings;

@!inf_buf_size = 500;
@!sup_buf_size = 30000;

@!inf_nest_size = 40;
@!sup_nest_size = 400;

@!inf_max_in_open = 6;
@!sup_max_in_open = 127;

@!inf_param_size = 60;
@!sup_param_size = 600;

@!inf_save_size = 600;
@!sup_save_size = 40000;

@!inf_stack_size = 200;
@!sup_stack_size = 3000;

@!inf_dvi_buf_size = 800;
@!sup_dvi_buf_size = 65536;

@!inf_font_mem_size = 20000;
@!sup_font_mem_size = 1000000;

@!sup_font_max = max_font_max;
@!inf_font_max = 50; {could be smaller, but why?}

@!inf_pool_size = 32000;
@!sup_pool_size = 10000000;
@!inf_pool_free = 1000;
@!sup_pool_free = sup_pool_size;
@!inf_string_vacancies = 8000;
@!sup_string_vacancies = sup_pool_size - 23000;

@!sup_hash_extra = sup_max_strings;
@!inf_hash_extra = 0;

@!sup_hyph_size = ssup_hyph_size;
@!inf_hyph_size = iinf_hyphen_size; {Must be not less than |hyph_prime|!}
@z

@x [1.12] l.427 - Constants that are WEB numeric macros.
@d mem_bot=0 {smallest index in the |mem| array dumped by \.{INITEX};
  must not be less than |mem_min|}
@d mem_top==30000 {largest index in the |mem| array dumped by \.{INITEX};
  must be substantially larger than |mem_bot|
  and not greater than |mem_max|}
@d font_base=0 {smallest internal font number; must not be less
  than |min_quarterword|}
@d hash_size=2100 {maximum number of control sequences; it should be at most
  about |(mem_max-mem_min)/10|}
@d hash_prime=1777 {a prime number equal to about 85\pct! of |hash_size|}
@d hyph_size=307 {another prime; the number of \.{\\hyphenation} exceptions}
@y
@d hash_size=10000 {maximum number of control sequences; it should be at most
  about |(mem_max-mem_min)/10|}
@d hash_prime=8501 {a prime number equal to about 85\pct! of |hash_size|}
@d hyph_prime=607 {another prime for hashing \.{\\hyphenation} exceptions;
                if you change this, you should also change |iinf_hyphen_size|.}
@z

@x [1.16] l.498 - Use C macros for `incr' and `decr'.
@d incr(#) == #:=#+1 {increase a variable by unity}
@d decr(#) == #:=#-1 {decrease a variable by unity}
@y
@z

% The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
@x [2.19] l.565 - data type text_char is 8-bit ASCII_code
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x [2.23] l.723 - Allow any character as input.
for i:=0 to @'37 do xchr[i]:=' ';
for i:=@'177 to @'377 do xchr[i]:=' ';
@y
for i:=0 to @'37 do xchr[i]:=chr(i);
for i:=@'177 to @'377 do xchr[i]:=chr(i);
@z

% @x [2.23] l.723 - Translate characters if desired, otherwise allow them all.
% for i:=0 to @'37 do xchr[i]:=' ';
% for i:=@'177 to @'377 do xchr[i]:=' ';
% @y
% ifdef('notdef'); {TCX files are probably a bad idea.}
% {The idea for this dynamic translation comes from the patch by
%  Libor Skarvada \.{<libor@@informatics.muni.cz>}
%  and Petr Sojka \.{<sojka@@informatics.muni.cz>}. I didn't use any of the
%  actual code, though, preferring a more general approach.}
%
% {This sets the |xchr|, |xord|, and |is_printable| arrays.  We overwrite the
%  |xchr| assignments from module 21, just in case someone wants to do strange
%  character translations. See the function definition in \.{texmfmp.c} for
%  more comments.}
% setup_char_set;
% endif('notdef');
%
% {We rearrange |str_pool| for the character representations in |main_body|.}
% @z
%
% @x [2.24] l.733 - Don't reinitialize xord.
% for i:=first_text_char to last_text_char do xord[chr(i)]:=invalid_code;
% for i:=@'200 to @'377 do xord[xchr[i]]:=i;
% for i:=0 to @'176 do xord[xchr[i]]:=i;
% @y
% @z

% [3.26] name_of_file is no longer an array.  And change the destination
% type to text_char, which fixes:
%
% Date: 19 Sep 1994 10:38:24 +0200
% From: thorinn@diku.dk (Lars Mathiesen)
%       When echoed to the screen and in the log, character codes
%       above '177 in file names are shown wrongly (typically as ^@).
%
@x [3.26] l.789 - name_of_file is no longer an array
@!name_of_file:packed array[1..file_name_size] of char;@;@/
  {on some systems this may be a \&{record} variable}
@y
@!name_of_file:^text_char;
@z

@x [3.27] l.794 - Do file opening in C.
@ The \ph\ compiler with which the present version of \TeX\ was prepared has
extended the rules of \PASCAL\ in a very convenient way. To open file~|f|,
we can write
$$\vbox{\halign{#\hfil\qquad&#\hfil\cr
|reset(f,@t\\{name}@>,'/O')|&for input;\cr
|rewrite(f,@t\\{name}@>,'/O')|&for output.\cr}}$$
The `\\{name}' parameter, which is of type `{\bf packed array
$[\langle\\{any}\rangle]$ of \\{char}}', stands for the name of
the external file that is being opened for input or output.
Blank spaces that might appear in \\{name} are ignored.

The `\.{/O}' parameter tells the operating system not to issue its own
error messages if something goes wrong. If a file of the specified name
cannot be found, or if such a file cannot be opened for some other reason
(e.g., someone may already be trying to write the same file), we will have
|@!erstat(f)<>0| after an unsuccessful |reset| or |rewrite|.  This allows
\TeX\ to undertake appropriate corrective action.
@:PASCAL H}{\ph@>
@^system dependencies@>

\TeX's file-opening procedures return |false| if no file identified by
|name_of_file| could be opened.

@d reset_OK(#)==erstat(#)=0
@d rewrite_OK(#)==erstat(#)=0

@p function a_open_in(var f:alpha_file):boolean;
  {open a text file for input}
begin reset(f,name_of_file,'/O'); a_open_in:=reset_OK(f);
end;
@#
function a_open_out(var f:alpha_file):boolean;
  {open a text file for output}
begin rewrite(f,name_of_file,'/O'); a_open_out:=rewrite_OK(f);
end;
@#
function b_open_in(var f:byte_file):boolean;
  {open a binary file for input}
begin reset(f,name_of_file,'/O'); b_open_in:=reset_OK(f);
end;
@#
function b_open_out(var f:byte_file):boolean;
  {open a binary file for output}
begin rewrite(f,name_of_file,'/O'); b_open_out:=rewrite_OK(f);
end;
@#
function w_open_in(var f:word_file):boolean;
  {open a word file for input}
begin reset(f,name_of_file,'/O'); w_open_in:=reset_OK(f);
end;
@#
function w_open_out(var f:word_file):boolean;
  {open a word file for output}
begin rewrite(f,name_of_file,'/O'); w_open_out:=rewrite_OK(f);
end;
@y
@ All of the file opening functions are defined in C.
@z

@x [3.28] l.850 - Do file closing in C.
@ Files can be closed with the \ph\ routine `|close(f)|', which
@^system dependencies@>
should be used when all input or output with respect to |f| has been completed.
This makes |f| available to be opened again, if desired; and if |f| was used for
output, the |close| operation makes the corresponding external file appear
on the user's area, ready to be read.

These procedures should not generate error messages if a file is
being closed before it has been successfully opened.

@p procedure a_close(var f:alpha_file); {close a text file}
begin close(f);
end;
@#
procedure b_close(var f:byte_file); {close a binary file}
begin close(f);
end;
@#
procedure w_close(var f:word_file); {close a word file}
begin close(f);
end;
@y
@ And all the file closing routines as well.
@z

@x [3.30] l.888 - Array size of input buffer is determined at runtime.
@!buffer:array[0..buf_size] of ASCII_code; {lines of characters being read}
@y
@!buffer:^ASCII_code; {lines of characters being read}
@z

@x [3.31] l.933 - Do `input_ln' in C.
@p function input_ln(var f:alpha_file;@!bypass_eoln:boolean):boolean;
  {inputs the next line or returns |false|}
var last_nonblank:0..buf_size; {|last| with trailing blanks removed}
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
We define |input_ln| in C, for efficiency. Nevertheless we quote the module
`Report overflow of the input buffer, and abort' here in order to make
\.{WEAVE} happy, since part of that module is needed by e-TeX.

@p @{ @<Report overflow of the input buffer, and abort@> @}
@z

% [3.32] `term_in' and `term_out' are standard input and output.
% Declare the variables that used to be constants.
@x [3.32] l.961 - `term_in' and `term_out' are standard input and output.
@<Glob...@>=
@!term_in:alpha_file; {the terminal as an input file}
@!term_out:alpha_file; {the terminal as an output file}
@y
@d term_in==stdin {the terminal as an input file}
@d term_out==stdout {the terminal as an output file}

@<Glob...@>=
@!init
@!ini_version:boolean; {are we \.{INITEX}?}
@!dump_option:boolean; {was the dump name option used?}
@!dump_line:boolean; {was a \.{\%\AM format} line seen?}
tini@/
@#
@!bound_default:integer; {temporary for setup}
@!bound_name:^char; {temporary for setup}
@#
@!main_memory:integer; {total memory words allocated in initex}
@!extra_mem_bot:integer; {|mem_min:=mem_bot-extra_mem_bot| except in \.{INITEX}}
@!mem_min:integer; {smallest index in \TeX's internal |mem| array;
  must be |min_halfword| or more;
  must be equal to |mem_bot| in \.{INITEX}, otherwise |<=mem_bot|}
@!mem_top:integer; {largest index in the |mem| array dumped by \.{INITEX};
  must be substantially larger than |mem_bot|,
  equal to |mem_max| in \.{INITEX}, else not greater than |mem_max|}
@!extra_mem_top:integer; {|mem_max:=mem_top+extra_mem_top| except in \.{INITEX}}
@!mem_max:integer; {greatest index in \TeX's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INITEX}, otherwise |>=mem_top|}
@!error_line:integer; {width of context lines on terminal error messages}
@!half_error_line:integer; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line:integer;
  {width of longest text lines output; should be at least 60}
@!max_strings:integer; {maximum number of strings; must not exceed |max_halfword|}
@!string_vacancies:integer; {the minimum number of characters that should be
  available for the user's control sequences and font names,
  after \TeX's own error messages are stored}
@!pool_size:integer; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all fonts and
  control sequences; must exceed |string_vacancies| by the total
  length of \TeX's own strings, which is currently about 23000}
@!pool_free:integer;{pool space free after format loaded}
@!font_mem_size:integer; {number of words of |font_info| for all fonts}
@!font_max:integer; {maximum internal font number; ok to exceed |max_quarterword|
  and must be at most |font_base|+|max_font_max|}
@!font_k:integer; {loop variable for initialization}
@!hyph_size:integer; {maximun number of hyphen exceptions}
@!trie_size:integer; {space for hyphenation patterns; should be larger for
  \.{INITEX} than it is in production versions of \TeX.  50000 is
  needed for English, German, and Portuguese.}
@!buf_size:integer; {maximum number of characters simultaneously present in
  current lines of open files and in control sequences between
  \.{\\csname} and \.{\\endcsname}; must not exceed |max_halfword|}
@!stack_size:integer; {maximum number of simultaneous input sources}
@!max_in_open:integer; {maximum number of input files and error insertions that
  can be going on simultaneously}
@!param_size:integer; {maximum number of simultaneous macro parameters}
@!nest_size:integer; {maximum number of semantic levels simultaneously active}
@!save_size:integer; {space for saving values outside of current group; must be
  at most |max_halfword|}
@!dvi_buf_size:integer; {size of the output buffer; must be a multiple of 8}
@z

@x [3.33] l.964 - We don't need to open terminal files.
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

@x [3.34] l.982 - Flushing output to terminal files.
these operations can be specified in \ph:
@^system dependencies@>

@d update_terminal == break(term_out) {empty the terminal output buffer}
@d clear_terminal == break_in(term_in,true) {clear the terminal input buffer}
@y
these operations can be specified with {\mc UNIX}.  |update_terminal|
does an |fflush|. |clear_terminal| is redefined
to do nothing, since the user should control the terminal.
@^system dependencies@>

@d update_terminal == fflush (term_out)
@d clear_terminal == do_nothing
@z

@x [3.35] l.1017 - needed for e-TeX, but differently
@<Report overflow of the input buffer, and abort@>=
if format_ident=0 then
  begin write_ln(term_out,'Buffer size exceeded!'); goto final_end;
@.Buffer size exceeded@>
  end
else begin cur_input.loc_field:=first; cur_input.limit_field:=last-1;
@y
Routine is implemented in C; part of module is, however, needed for e-TeX.

@<Report overflow of the input buffer, and abort@>=
  begin cur_input.loc_field:=first; cur_input.limit_field:=last-1;
@z

@x [3.37] l.1055 - |init_terminal|, reading the command line.
@ The following program does the required initialization
without retrieving a possible command line.
It should be clear how to modify this routine to deal with command lines,
if the system permits them.
@^system dependencies@>

@p function init_terminal:boolean; {gets the terminal input started}
label exit;
begin t_open_in;
@y
@ The following program does the required initialization.
Iff anything has been specified on the command line, then |t_open_in|
will return with |last > first|.
@^system dependencies@>

@p function init_terminal:boolean; {gets the terminal input started}
label exit;
begin t_open_in;
if last > first then
  begin loc := first;
  while (loc < last) and (buffer[loc]=' ') do incr(loc);
  if loc < last then
    begin init_terminal := true; goto exit;
    end;
  end;
@z

@x [3.37] l.1068 - |init_terminal|, output missing newline.
    write(term_out,'! End of file on the terminal... why?');
@y
    write_ln(term_out,'! End of file on the terminal... why?');
@z

@x [4.38] l.1126 - Array size for string pool is determined at runtime.
@!pool_pointer = 0..pool_size; {for variables that point into |str_pool|}
@!str_number = 0..max_strings; {for variables that point into |str_start|}
@y
@!pool_pointer = integer; {for variables that point into |str_pool|}
@!str_number = 0..ssup_max_strings; {for variables that point into |str_start|}
@z

@x [4.39] l.1131 - Dynamically size pool arrays.
@!str_pool:packed array[pool_pointer] of packed_ASCII_code; {the characters}
@!str_start : array[str_number] of pool_pointer; {the starting pointers}
@y
@!str_pool: ^packed_ASCII_code; {the characters}
@!str_start : ^pool_pointer; {the starting pointers}
@z

@x [4.47] l.1237 - string recycling
@p @!init function get_strings_started:boolean; {initializes the string pool,
@y
@p @t\4@>@<Declare additional routines for string recycling@>@/

@!init function get_strings_started:boolean; {initializes the string pool,
@z

@x [4.48] l.1252 -- Do not create "^^xy" for strings<256 in string pool
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
@y
  begin append_char(k);
@z

@x [4.49] l.1272 -- Change documentation (probably needed in more places)
would like string @'32 to be the single character @'32 instead of the
@y
would like string @'32 to be printed as the single character @'32
instead of the
@z

%%% Do NOT apply this change if the following one is applied!
%tcx-no @x [4.49] l.1295 - Do not hardwire printable ASCII.
%tcx @<Character |k| cannot be printed@>=
%tcx   (k<" ")or(k>"~")
%tcx @y
%tcx @<Character |k| cannot be printed@>=
%tcx   not is_printable[k]
%tcx @z
@x [4.49] l.1295 -- Do not hardwire printable ASCII.
@<Character |k| cannot be printed@>=
  (k<" ")or(k>"~")
@y
@<Character |k| cannot be printed@>=
   (((k<" ")or(k>"~")) and not(isprint(xord[k])))
@z

% [4.51] Open the pool file using a path, and can't do string
% assignments directly.  (`strcpy' and `strlen' work here because
% `pool_name' is a constant string, and thus ends in a null and doesn't
% start with a space.)
@x [4.51] l.1314 - Open the pool file.
name_of_file:=pool_name; {we needn't set |name_length|}
if a_open_in(pool_file) then
@y
name_length := strlen (pool_name);
name_of_file := xmalloc (1 + name_length + 1);
strcpy (name_of_file+1, pool_name); {copy the string}
if a_open_in (pool_file, kpse_texpool_format) then
@z

@x [4.51] l.1322 - Make `TEX.POOL' lowercase, and change how it's read.
else  bad_pool('! I can''t read TEX.POOL.')
@y
else  bad_pool('! I can''t read tex.pool; bad path?')
@z
@x [4.52] l.1326 - Make `TEX.POOL' lowercase, and change how it's read.
begin if eof(pool_file) then bad_pool('! TEX.POOL has no check sum.');
@.TEX.POOL has no check sum@>
read(pool_file,m,n); {read two digits of string length}
@y
begin if eof(pool_file) then bad_pool('! tex.pool has no check sum.');
@.TEX.POOL has no check sum@>
read(pool_file,m); read(pool_file,n); {read two digits of string length}
@z
@x [4.52] l.1332 - Make `TEX.POOL' lowercase, and change how it's read.
    bad_pool('! TEX.POOL line doesn''t begin with two digits.');
@y
    bad_pool('! tex.pool line doesn''t begin with two digits.');
@z
@x [4.53] l.1354 - Make `TEX.POOL' lowercase, and change how it's read.
  bad_pool('! TEX.POOL check sum doesn''t have nine digits.');
@y
  bad_pool('! tex.pool check sum doesn''t have nine digits.');
@z
@x [4.53] l.1360 - Make `TEX.POOL' lowercase, and change how it's read.
done: if a<>@$ then bad_pool('! TEX.POOL doesn''t match; TANGLE me again.');
@y
done: if a<>@$ then
  bad_pool('! tex.pool doesn''t match; tangle me again (or fix the path).');
@z

@x [5.54] l.1422 - error_line
@!trick_buf:array[0..error_line] of ASCII_code; {circular buffer for
@y
@!trick_buf:array[0..ssup_error_line] of ASCII_code; {circular buffer for
@z

@x [5.58] l.1460 -- Rename |print_char| to |print_visible_char|
@ The |print_char| procedure sends one character to the desired destination,
using the |xchr| array to map it into an external character compatible with
|input_ln|. All printing comes through |print_ln| or |print_char|.
@y
@ The |print_visible_char| procedure sends one character to the desired
destination, using the |xchr| array to map it into an external character
compatible with |input_ln|.  It assumes that it is always called with a
visible ASCII character and that the special handling for the new-line
character is done in |print_char|.  All printing comes through |print_ln|
or |print_char|, which ultimately calls |print_visible_char|.
@z
@x
procedure print_char(@!s:ASCII_code); {prints a single character}
label exit;
begin if @<Character |s| is the current new-line character@> then
 if selector<pseudo then
  begin print_ln; return;
  end;
@y
procedure print_visible_char(@!s:ASCII_code); {prints a single character}
label exit; {label is not used but nonetheless kept (for other changes?)}
begin
@z

@x [5.58/59] l.1493 -- Insert new |print_char| procedure
incr(tally);
exit:end;
@y
incr(tally);
exit:end;

@ The |print_char| procedure sends one character to the desired destination.
Control sequence names, file names and string constructed with
\.{\\string} might contain |ASCII_code| values that can't
be printed using |print_visible_char|.  These characters will be printed
in three- or four-symbol form like `\.{\^\^A}' or `\.{\^\^e4}'.

@d print_lc_hex(#)==l:=#;
  if l<10 then print_visible_char(l+"0")@+else print_visible_char(l-10+"a")

@<Basic printing...@>=
procedure print_char(@!s:ASCII_code); {prints a single character}
label exit;
var k:ASCII_code;
  @!l:0..255; {small indices or counters}
begin if selector>pseudo then
  begin print_visible_char(s); return;
  end;
if @<Character |s| is the current new-line character@> then
 if selector<pseudo then
  begin print_ln; return;
  end;
k:=s; if @<Character |k| cannot be printed@> then
  begin print_visible_char("^"); print_visible_char("^");
  if s<64 then print_visible_char(s+64)
  else if s<128 then print_visible_char(s-64)
  else begin print_lc_hex(s div 16);  print_lc_hex(s mod 16);
    end;
  end
else print_visible_char(s);
exit:end;
@z

@x [5.59/60] l.1499 -- Simplify |print| and remove |slow_print|
assumes that it is always safe to print a visible ASCII character.)
@^system dependencies@>
@y
assumes that it is always safe to print a visible ASCII character.)
@^system dependencies@>

Old versions of \TeX\ needed a procedure called |slow_print| whose function
is now subsumed by |print| and the new functionality of |print_char| and
|print_visible_char|.  We retain the old name |slow_print| here as a
possible aid to future software arch\ae ologists.

@d slow_print == print
@z

@x [5.59/60] l.1508 -- Simplify |print| for single characters (strings<256)
@!nl:integer; {new-line character to restore}
@y
@z
@x
  else begin if selector>pseudo then
      begin print_char(s); return; {internal strings are not expanded}
      end;
    if (@<Character |s| is the current new-line character@>) then
      if selector<pseudo then
        begin print_ln; return;
        end;
    nl:=new_line_char; new_line_char:=-1;
      {temporarily disable new-line character}
    j:=str_start[s];
    while j<str_start[s+1] do
      begin print_char(so(str_pool[j])); incr(j);
      end;
    new_line_char:=nl; return;
    end;
@y
  else begin print_char(s); return;
    end;
@z

@x [5.60] l.1534 -- Remove |slow_print|
@ Control sequence names, file names, and strings constructed with
\.{\\string} might contain |ASCII_code| values that can't
be printed using |print_char|. Therefore we use |slow_print| for them:

@<Basic print...@>=
procedure slow_print(@!s:integer); {prints string |s|}
var j:pool_pointer; {current character code position}
begin if (s>=str_ptr) or (s<256) then print(s)
else begin j:=str_start[s];
  while j<str_start[s+1] do
    begin print(so(str_pool[j])); incr(j);
    end;
  end;
end;
@y
@z

@x [5.61] l.1556 - Print rest of banner, eliminate misleading `(no format preloaded)'.
if format_ident=0 then wterm_ln(' (no format preloaded)')
else  begin slow_print(format_ident); print_ln;
  end;
@y
wterm(version_string);
if format_ident>0 then slow_print(format_ident);
print_ln;
@z

@x [6.73] l.1732 - Add unspecified_mode.
@d error_stop_mode=3 {stops at every opportunity to interact}
@y
@d error_stop_mode=3 {stops at every opportunity to interact}
@d unspecified_mode=4 {extra value for command-line switch}
@z

@x [6.73] l.1738 - Add interaction_option.
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@y
@!interaction:batch_mode..error_stop_mode; {current level of interaction}
@!interaction_option:batch_mode..unspecified_mode; {set from command line}
@z

@x [6.74] l.1740 - Allow override by command line switch.
@ @<Set init...@>=interaction:=error_stop_mode;
@y
@ @<Set init...@>=if interaction_option=unspecified_mode then
  interaction:=error_stop_mode
else
  interaction:=interaction_option;
@z

% [6.81] Eliminate nonlocal goto, since C doesn't have them.
% Plus, it's nicer just to do an exit with the appropriate status code
% under Unix.  We call it `uexit' because there's a WEB symbol called
% `exit' already.  We use a C macro to change `uexit' back to `exit'.
@x [6.81] l.1852 - Eliminate nonlocal goto, since C doesn't have them.
@<Error hand...@>=
procedure jump_out;
begin goto end_of_TEX;
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

@x [6.84] l.1888 - Implement the switch-to-editor option.
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
since we don't want to do the switch-to-editor until after TeX has closed
its files.
@^system dependencies@>

There is a secret `\.D' option available when the debugging routines haven't
been commented~out.
@^debugging@>
@d edit_file==input_stack[base_ptr]
@z

@x [6.84] l.1903 - Implement the switch-to-editor option.
"E": if base_ptr>0 then
  begin print_nl("You want to edit file ");
@.You want to edit file x@>
  slow_print(input_stack[base_ptr].name_field);
  print(" at line "); print_int(line);
  interaction:=scroll_mode; jump_out;
@y
"E": if base_ptr>0 then
    begin edit_name_start:=str_start[edit_file.name_field];
    edit_name_length:=str_start[edit_file.name_field+1] -
                      str_start[edit_file.name_field];
    edit_line:=line;
    jump_out;
@z

% [7.104] `remainder' is a library routine on some systems, so change
% its name to avoid conflicts.
@x [7.104] l.2227 - avoid name conflicts with lib routine remainder()
|remainder|, holds the remainder after a division.

@<Glob...@>=
@y
|remainder|, holds the remainder after a division.

@d remainder==tex_remainder

@<Glob...@>=
@z

@x [7.109] l.2352 - Define glue_ratio in C.
@!glue_ratio=real; {one-word representation of a glue expansion factor}
@y
@z

% [8.110] Make it easy to change constants.  Do not increase
% max_quarterword without changing the memoryword structure in `texmfmem.h'.
% If you set min_quarterword or min_halfword to a non-zero value, you have
% to remove the definitions of qi/qo, hi/ho in this change file!
@x [8.110] l.2405 - increase |max_halfword|
@d min_quarterword=0 {smallest allowable value in a |quarterword|}
@d max_quarterword=255 {largest allowable value in a |quarterword|}
@d min_halfword==0 {smallest allowable value in a |halfword|}
@d max_halfword==65535 {largest allowable value in a |halfword|}
@y 2407
@d min_quarterword=0 {smallest allowable value in a |quarterword|}
@d max_quarterword=255 {largest allowable value in a |quarterword|}
@d min_halfword==0 {smallest allowable value in a |halfword|}
@d max_halfword==@"FFFFFFF {largest allowable value in a |halfword|}
@z

@x [8.111] l.2422 - max_font_max
if (font_base<min_quarterword)or(font_max>max_quarterword) then bad:=15;
if font_max>font_base+256 then bad:=16;
@y
if (max_font_max<min_halfword)or(max_font_max>max_halfword) then bad:=15;
if font_max>font_base+max_font_max then bad:=16;
@z

@x [8.112] l.2435 - Efficiency.
macros are simplified in the obvious way when |min_quarterword=0|.
@^inner loop@>@^system dependencies@>

@d qi(#)==#+min_quarterword
  {to put an |eight_bits| item into a quarterword}
@d qo(#)==#-min_quarterword
  {to take an |eight_bits| item out of a quarterword}
@d hi(#)==#+min_halfword
  {to put a sixteen-bit item into a halfword}
@d ho(#)==#-min_halfword
  {to take a sixteen-bit item from a halfword}
@y
macros are simplified in the obvious way when |min_quarterword=0|.
So they have been simplified here in the obvious way.
@^inner loop@>@^system dependencies@>

@d qi(#)==# {to put an |eight_bits| item into a quarterword}
@d qo(#)==# {to take an |eight_bits| item from a quarterword}
@d hi(#)==# {to put a sixteen-bit item into a halfword}
@d ho(#)==# {to take a sixteen-bit item from a halfword}
@z

% [8.113] We've put the memory structure into the include file
% `texmf.h', since it's too hard to translate automatically.
@x [8.113] l.2453 - data structures for main memory
@!quarterword = min_quarterword..max_quarterword; {1/4 of a word}
@!halfword=min_halfword..max_halfword; {1/2 of a word}
@!two_choices = 1..2; {used when there are two variants in a record}
@!four_choices = 1..4; {used when there are four variants in a record}
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
  case four_choices of
  1: (@!int:integer);
  2: (@!gr:glue_ratio);
  3: (@!hh:two_halves);
  4: (@!qqqq:four_quarters);
  end;
@y
@!quarterword = min_quarterword..max_quarterword;
@!halfword = min_halfword..max_halfword;
@!two_choices = 1..2; {used when there are two variants in a record}
@!four_choices = 1..4; {used when there are four variants in a record}
@=#include "texmfmem.h";@>
@z

% [9.116] Change `mem' to `zmem', so we can define mem to be a register
% pointer to the memory array for speed.
@x [9.116] l.2545 - definition of main memory array
@!mem : array[mem_min..mem_max] of memory_word; {the big dynamic storage area}
@y
@!yzmem : ^memory_word; {the big dynamic storage area}
@!zmem : ^memory_word; {the big dynamic storage area}
@z

% [9.127] Fix casting problem in C.
% There are several of these.  They come from the rules C uses for
% comparing signed and unsigned quantities.  Just doing the comparison
% can result in incorrect evaluation wrt the way Pascal would do it.
@x [9.127] l.2739 - Fix casting problem in C.
if r>p+1 then @<Allocate from the top of node |p| and |goto found|@>;
@y 2738
if r>toint(p+1) then @<Allocate from the top of node |p| and |goto found|@>;
@z

@x [10.144] l.3006 - font numbers can be >255 now.
@p function new_ligature(@!f,@!c:quarterword; @!q:pointer):pointer;
@y
@p function new_ligature(@!f:internal_font_number; @!c:quarterword;
                         @!q:pointer):pointer;
@z

% [11.165] Fix the word `free' so that it doesn't conflict with the
% standard C library routine of the same name.
@x [11.165] l.3364 - avoid conflict with lib function free()
are debugging.)
@y
are debugging.)

@d free==free_arr
@z

@x [11.165] l.3367 - dummy |free| and |was_free| arrays
@!debug @!free: packed array [mem_min..mem_max] of boolean; {free cells}
@t\hskip10pt@>@!was_free: packed array [mem_min..mem_max] of boolean;
@y
 {The debug memory arrays have not been mallocated yet.}
@!debug @!free: packed array [0..9] of boolean; {free cells}
@t\hskip10pt@>@!was_free: packed array [0..9] of boolean;
@z

@x [12.174] l.3526 - Eliminate unsigned comparisons to zero.
        begin if (font(p)<font_base)or(font(p)>font_max) then
@y
        begin if (font(p)>font_max) then
@z

@x [12.176] l.3563 - Eliminate unsigned comparisons to zero.
@p procedure print_font_and_char(@!p:integer); {prints |char_node| data}
begin if p>mem_end then print_esc("CLOBBERED.")
else  begin if (font(p)<font_base)or(font(p)>font_max) then print_char("*")
@y
@p procedure print_font_and_char(@!p:integer); {prints |char_node| data}
begin if p>mem_end then print_esc("CLOBBERED.")
else  begin if (font(p)>font_max) then print_char("*")
@z

@x [12.186] l.3747 - Don't worry about strange floating point values.
  if abs(mem[p+glue_offset].int)<@'4000000 then print("?.?")
  else if abs(g)>float_constant(20000) then
@y 3747
  { The Unix |pc| folks removed this restriction with a remark that
    invalid bit patterns were vanishingly improbable, so we follow
    their example without really understanding it.
  |if abs(mem[p+glue_offset].int)<@'4000000 then print('?.?')|
  |else| }
  if fabs(g)>float_constant(20000) then
@z


@x [15.209] l.4165 - MLTeX: \charsubdef primitive
@d shorthand_def=95 {code definition ( \.{\\chardef}, \.{\\countdef}, etc.~)}
@y
@d shorthand_def=95 {code definition ( \.{\\chardef}, \.{\\countdef}, etc.~)}
  {or \.{\\charsubdef}}
@z


@x [16.213] l.4321 - texarray
@!nest:array[0..nest_size] of list_state_record;
@y
@!nest:^list_state_record;
@z

@x [16.215] l.4344 - remove mem[] reference from initialize.
prev_graf:=0; shown_mode:=0;
@<Start a new current page@>;
@y
prev_graf:=0; shown_mode:=0;
@/{The following piece of code is a copy of module 991:}
page_contents:=empty; page_tail:=page_head; {|link(page_head):=null;|}@/
last_glue:=max_halfword; last_penalty:=0; last_kern:=0;
page_depth:=0; page_max_depth:=0;
@z

@x [17.220] l.4448 - MLTeX: char_sub_code_base
paragraph shape.
@y
paragraph shape.
Additionally region~4 contains the table with ML\TeX's character
substitution definitions.
@z

@x [17.222] l.4526 - max_font_max
@d undefined_control_sequence=frozen_null_font+257 {dummy location}
@y
@d undefined_control_sequence=frozen_null_font+max_font_max+1 {dummy location}
@z

@x [17.222] l.4533 - hash_extra
for k:=active_base to undefined_control_sequence-1 do
  eqtb[k]:=eqtb[undefined_control_sequence];
@y
for k:=active_base to eqtb_top do
  eqtb[k]:=eqtb[undefined_control_sequence];
@z

@x [17.230] l.4731 - MLTeX: char_sub_code_base
@d int_base=math_code_base+256 {beginning of region 5}
@y
@d char_sub_code_base=math_code_base+256 {table of character substitutions}
@d int_base=char_sub_code_base+256 {beginning of region 5}
@z

@x [17.230] l.4752 - MLTeX: char_sub_code_base
  {Note: |math_code(c)| is the true math code plus |min_halfword|}
@y
  {Note: |math_code(c)| is the true math code plus |min_halfword|}
@d char_sub_code(#)==equiv(char_sub_code_base+#)
  {Note: |char_sub_code(c)| is the true substitution info plus |min_halfword|}
@z

@x [17.236] l.4954 - MLTeX: \charsubdefmax and \tracingcharsubdef
@d int_pars=55 {total number of integer parameters}
@y
@d char_sub_def_min_code=55 {smallest value in the charsubdef list}
@d char_sub_def_max_code=56 {largest value in the charsubdef list}
@d tracing_char_sub_def_code=57 {traces changes to a charsubdef def}
@d int_pars=58 {total number of integer parameters}
@z

@x [17.236] l.5016 - MLTeX: \charsubdefmax and \tracingcharsubdef
@d error_context_lines==int_par(error_context_lines_code)
@y
@d error_context_lines==int_par(error_context_lines_code)
@d char_sub_def_min==int_par(char_sub_def_min_code)
@d char_sub_def_max==int_par(char_sub_def_max_code)
@d tracing_char_sub_def==int_par(tracing_char_sub_def_code)
@z

@x [17.237] l.5080 - MLTeX: \charsubdefmax and \tracingcharsubdef
error_context_lines_code:print_esc("errorcontextlines");
@y
error_context_lines_code:print_esc("errorcontextlines");
char_sub_def_min_code:print_esc("charsubdefmin");
char_sub_def_max_code:print_esc("charsubdefmax");
tracing_char_sub_def_code:print_esc("tracingcharsubdef");
@z

@x [17.238] l.5200 - MLTeX: \charsubdefmax and \tracingcharsubdef
@!@:error_context_lines_}{\.{\\errorcontextlines} primitive@>
@y
@!@:error_context_lines_}{\.{\\errorcontextlines} primitive@>
if mltex_p then
  begin mltex_enabled_p:=true;  {enable character substitution}
  if false then {remove the if-clause to enable \.{\\charsubdefmin}}
  primitive("charsubdefmin",assign_int,int_base+char_sub_def_min_code);@/
@!@:char_sub_def_min_}{\.{\\charsubdefmin} primitive@>
  primitive("charsubdefmax",assign_int,int_base+char_sub_def_max_code);@/
@!@:char_sub_def_max_}{\.{\\charsubdefmax} primitive@>
  primitive("tracingcharsubdef",assign_int,int_base+tracing_char_sub_def_code);@/
@!@:tracing_char_sub_def_}{\.{\\tracingcharsubdef} primitive@>
  end;
@z

@x [17.240] l.5213 - MLTeX: \charsubdefmax and \tracingcharsubdef
for k:=int_base to del_code_base-1 do eqtb[k].int:=0;
@y
for k:=int_base to del_code_base-1 do eqtb[k].int:=0;
char_sub_def_min:=256; char_sub_def_max:=-1;
{allow \.{\\charsubdef} for char 0}@/
{|tracing_char_sub_def:=0| is already done}@/
@z

@x [17.241] l.5219 - Do `fix_date_and_time' in C.
@ The following procedure, which is called just before \TeX\ initializes its
input and output, establishes the initial values of the date and time.
@^system dependencies@>
Since standard \PASCAL\ cannot provide such information, something special
is needed. The program here simply specifies July 4, 1776, at noon; but
users probably want a better approximation to the truth.

@p procedure fix_date_and_time;
begin time:=12*60; {minutes since midnight}
day:=4; {fourth day of the month}
month:=7; {seventh month of the year}
year:=1776; {Anno Domini}
end;
@y
@ The following procedure, which is called just before \TeX\ initializes its
input and output, establishes the initial values of the date and time.
It calls a macro-defined |date_and_time| routine.  |date_and_time|
in turn is a C macro, which calls |get_date_and_time|, passing
it the addresses of the day, month, etc., so they can be set by the
routine.  |get_date_and_time| also sets up interrupt catching if that
is conditionally compiled in the C code.
@^system dependencies@>

@d fix_date_and_time==date_and_time(time,day,month,year)
@z

@x [17.252] l.5420 - hash_extra
else if n<glue_base then @<Show equivalent |n|, in region 1 or 2@>
@y
else if (n<glue_base) or ((n>eqtb_size)and(n<=eqtb_top)) then
  @<Show equivalent |n|, in region 1 or 2@>
@z

@x [17.253] l.5435 - Change eqtb to zeqtb.
@!eqtb:array[active_base..eqtb_size] of memory_word;
@y
@!zeqtb:^memory_word;
@z

@x [18.256] l.5483 - hash_extra
@!hash: array[hash_base..undefined_control_sequence-1] of two_halves;
  {the hash table}
@!hash_used:pointer; {allocation pointer for |hash|}
@y
@!hash: ^two_halves; {the hash table}
@!yhash: ^two_halves; {auxiliary pointer for freeing hash}
@!hash_used:pointer; {allocation pointer for |hash|}
@!hash_extra:pointer; {|hash_extra=hash| above |eqtb_size|}
@!hash_top:pointer; {maximum of the hash array}
@!eqtb_top:pointer; {maximum of the |eqtb|}
@!hash_high:pointer; {pointer to next high hash location}
@z

@x [18.257] l.5491 - hash_extra
next(hash_base):=0; text(hash_base):=0;
for k:=hash_base+1 to undefined_control_sequence-1 do hash[k]:=hash[hash_base];
@y
@z

@x [18.258] l.5495 - hash_extra
hash_used:=frozen_control_sequence; {nothing is used}
@y
hash_used:=frozen_control_sequence; {nothing is used}
hash_high:=0;
@z

@x [18.260] l.5531 - hash_extra
@ @<Insert a new control...@>=
begin if text(p)>0 then
  begin repeat if hash_is_full then overflow("hash size",hash_size);
@:TeX capacity exceeded hash size}{\quad hash size@>
  decr(hash_used);
  until text(hash_used)=0; {search for an empty location in |hash|}
  next(p):=hash_used; p:=hash_used;
  end;
@y
@ @<Insert a new control...@>=
begin if text(p)>0 then
  begin if hash_high<hash_extra then
      begin incr(hash_high);
      next(p):=hash_high+eqtb_size; p:=hash_high+eqtb_size;
      end
    else begin
      repeat if hash_is_full then overflow("hash size",hash_size+hash_extra);
@:TeX capacity exceeded hash size}{\quad hash size@>
      decr(hash_used);
      until text(hash_used)=0; {search for an empty location in |hash|}
    next(p):=hash_used; p:=hash_used;
    end;
  end;
@z

@x [18.262] l.5583 - hash_extra
else if p>=undefined_control_sequence then print_esc("IMPOSSIBLE.")
@y
else if ((p>=undefined_control_sequence)and(p<=eqtb_size))or(p>eqtb_top) then
  print_esc("IMPOSSIBLE.")
@z

@x [18.262] l.5584 - Remove more unsigned comparisons to zero.
else if (text(p)<0)or(text(p)>=str_ptr) then print_esc("NONEXISTENT.")
@y
else if (text(p)>=str_ptr) then print_esc("NONEXISTENT.")
@z

@x [19.271] l.5872 - texarray
@!save_stack : array[0..save_size] of memory_word;
@y
@!save_stack : ^memory_word;
@z

@x [19.283] l.6050 - hash_extra
if p<int_base then
@y
if (p<int_base)or(p>eqtb_size) then
@z

@x [20.290] l.6158 - hash_extra
if cs_token_flag+undefined_control_sequence>max_halfword then bad:=21;
@y
if cs_token_flag+eqtb_size+hash_extra>max_halfword then bad:=21;
if (hash_offset<0)or(hash_offset>hash_base) then bad:=42;
@z

@x [22.301] l.6432 - texarray
@!input_stack : array[0..stack_size] of in_state_record;
@y
@!input_stack : ^in_state_record;
@z

@x [22.304] l.6536 - texarray
@!input_file : array[1..max_in_open] of alpha_file;
@!line : integer; {current line number in the current source file}
@!line_stack : array[1..max_in_open] of integer;
@y
@!input_file : ^alpha_file;
@!line : integer; {current line number in the current source file}
@!line_stack : ^integer;
@z

@x [22.308] l.6701 - texarray
@!param_stack:array [0..param_size] of pointer;
  {token list pointers for parameters}
@y
@!param_stack: ^pointer;
  {token list pointers for parameters}
@z

@x [22.309] l.6794 - rsc@plan9.bell-labs.com plumbing change
else  begin print_nl("l."); print_int(line);
@y
else begin
	print_nl(cur_input.name_field);
	print(":");
	print_int(line);
	print(":");
@z

@x [29.513] l.9951 - Area and extension rules for filenames.
@ The file names we shall deal with for illustrative purposes have the
following structure:  If the name contains `\.>' or `\.:', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@y
@ The file names we shall deal with have the
following structure:  If the name contains `\./' or `\.:'
(for Amiga only), the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the last
`\..' to the end, otherwise the file extension is null.
@z

@x [29.513] l.9963 - Area and extension rules for filenames.
@!area_delimiter:pool_pointer; {the most recent `\.>' or `\.:', if any}
@!ext_delimiter:pool_pointer; {the relevant `\..', if any}
@y
@!area_delimiter:pool_pointer; {the most recent `\./', if any}
@!ext_delimiter:pool_pointer; {the most recent `\..', if any}
@z

@x [29.514] l.9973 - TeX area directories.
@d TEX_area=="TeXinputs:"
@.TeXinputs@>
@d TEX_font_area=="TeXfonts:"
@.TeXfonts@>
@y
In C, the default paths are specified separately.
@z

@x [29.516] l.9994 - filenames: more_name
  if (c=">")or(c=":") then
@y
  if IS_DIR_SEP(c) then
@z

@x [29.516] l.9997 - filenames: more_name
  else if (c=".")and(ext_delimiter=0) then ext_delimiter:=cur_length;
@y
  else if c="." then ext_delimiter:=cur_length;
@z

@x [29.517] l.10002 - end_name: string recycling
@ The third.
@^system dependencies@>

@p procedure end_name;
@y
@ The third.
@^system dependencies@>
If a string is already in the string pool, the function
|slow_make_string| does not create a new string but returns this string
number, thus saving string space.  Because of this new property of the
returned string number it is not possible to apply |flush_string| to
these strings.

@p procedure end_name;
var temp_str: str_number; {result of file name cache lookups}
@!j: pool_pointer; {running index}
@z

@x [29.517] l.10011 - end_name: string recycling
  str_start[str_ptr+1]:=str_start[str_ptr]+area_delimiter; incr(str_ptr);
  end;
if ext_delimiter=0 then
  begin cur_ext:=""; cur_name:=make_string;
@y
  str_start[str_ptr+1]:=str_start[str_ptr]+area_delimiter; incr(str_ptr);
  temp_str:=search_string(cur_area);
  if temp_str>0 then
    begin cur_area:=temp_str;
    decr(str_ptr);  {no |flush_string|, |pool_ptr| will be wrong!}
    for j:=str_start[str_ptr+1] to pool_ptr-1 do
      begin str_pool[j-area_delimiter]:=str_pool[j];
      end;
    pool_ptr:=pool_ptr-area_delimiter; {update |pool_ptr|}
    end;
  end;
if ext_delimiter=0 then
  begin cur_ext:=""; cur_name:=slow_make_string;
@z

@x [29.517] l.10016 - end_name: string recycling
else  begin cur_name:=str_ptr;
  str_start[str_ptr+1]:=str_start[str_ptr]+ext_delimiter-area_delimiter-1;
  incr(str_ptr); cur_ext:=make_string;
@y
else  begin cur_name:=str_ptr;
  str_start[str_ptr+1]:=str_start[str_ptr]+ext_delimiter-area_delimiter-1;
  incr(str_ptr); cur_ext:=make_string;
  decr(str_ptr); {undo extension string to look at name part}
  temp_str:=search_string(cur_name);
  if temp_str>0 then
    begin cur_name:=temp_str;
    decr(str_ptr);  {no |flush_string|, |pool_ptr| will be wrong!}
    for j:=str_start[str_ptr+1] to pool_ptr-1 do
      begin str_pool[j-ext_delimiter+area_delimiter+1]:=str_pool[j];
      end;
    pool_ptr:=pool_ptr-ext_delimiter+area_delimiter+1;  {update |pool_ptr|}
    end;
  cur_ext:=slow_make_string;  {remake extension string}
@z

% [29.519] In pack_file_name, leave room for the extra null we append at
% the end of a filename.
@x [29.519] l.10047 - pack_file_name, leave room for the extra null
for j:=str_start[a] to str_start[a+1]-1 do append_to_name(so(str_pool[j]));
@y
if name_of_file then libc_free (name_of_file);
name_of_file:= xmalloc(1 + length(a) + length(n) + length(e) + 1);
for j:=str_start[a] to str_start[a+1]-1 do append_to_name(so(str_pool[j]));
@z

@x [29.519] l.10051 - pack_file_name, append the extra null
for k:=name_length+1 to file_name_size do name_of_file[k]:=' ';
@y
name_of_file[name_length+1]:=0;
@z

@x [29.520] l.10060 - filenames: default format.
@d format_default_length=20 {length of the |TEX_format_default| string}
@d format_area_length=11 {length of its area part}
@d format_ext_length=4 {length of its `\.{.fmt}' part}
@y
Under {\mc UNIX} we don't give the area part, instead depending
on the path searching that will happen during file opening.  Also, the
length will be set in the main program.

@d format_area_length=0 {length of its area part}
@d format_ext_length=4 {length of its `\.{.fmt}' part}
@z

@x [29.521] l.10066 - filenames: default format, where `plain.fmt' is.
@!TEX_format_default:packed array[1..format_default_length] of char;

@ @<Set init...@>=
TEX_format_default:='TeXformats:plain.fmt';
@y
@!format_default_length: integer;
@!TEX_format_default: ^char;

@ We set the name of the default format file and the length of that name
in C, instead of Pascal, since we want them to depend on the name of the
program.
@z

@x [29.523] l.10095 - Change to pack_buffered_name as with pack_file_name.
for j:=1 to n do append_to_name(xord[TEX_format_default[j]]);
@y
if name_of_file then libc_free (name_of_file);
name_of_file := xmalloc (1 + n + (b - a + 1) + format_ext_length + 1);
for j:=1 to n do append_to_name(xord[TEX_format_default[j]]);
@z

@x [29.523] l.10100 - Change to pack_buffered_name as with pack_file_name.
for k:=name_length+1 to file_name_size do name_of_file[k]:=' ';
@y
name_of_file[name_length+1]:=0;
@z

@x [29.524] l.10118 - Format file opening: only try once, with path searching.
  pack_buffered_name(0,loc,j-1); {try first without the system file area}
  if w_open_in(fmt_file) then goto found;
  pack_buffered_name(format_area_length,loc,j-1);
    {now try the system format file area}
  if w_open_in(fmt_file) then goto found;
@y
  pack_buffered_name(0,loc,j-1); {Kpathsea does everything}
  if w_open_in(fmt_file) then goto found;
@z

@x [29.524] l.10124 - replace `PLAIN' in error messages with `default'.
  wterm_ln('Sorry, I can''t find that format;',' will try PLAIN.');
@y
  wterm ('Sorry, I can''t find the format `');
  fputs (name_of_file + 1, stdout);
  wterm ('''; will try `');
  fputs (TEX_format_default + 1, stdout);
  wterm_ln ('''.');
@z

@x [29.524] l.10132 - replace `PLAIN' in error messages with `default'.
  wterm_ln('I can''t find the PLAIN format file!');
@.I can't find PLAIN...@>
@y
  wterm ('I can''t find the format file `');
  fputs (TEX_format_default + 1, stdout);
  wterm_ln ('''!');
@.I can't find the format...@>
@z

@x [29.530] l.10239 - prompt_file_name: No default extension is TeX input file.
if e=".tex" then show_context;
@y
if (e=".tex") or (e="") then show_context;
@z

@x [29.532] l.10263 - avoid conflict, `logname' in <unistd.h> on some systems.
@d ensure_dvi_open==if output_file_name=0 then
@y
@d log_name == texmf_log_name
@d ensure_dvi_open==if output_file_name=0 then
@z

@x [29.534] l.10285 - Adjust for C string conventions.
@!months:packed array [1..36] of char; {abbreviations of month names}
@y
@!months:^char;
@z

@x [29.534] l.10293 - MLTeX: add MLTeX banner after loading fmt file
@<Print the banner line, including the date and time@>;
@y
@<Print the banner line, including the date and time@>;
if mltex_enabled_p then
  begin wlog_cr; wlog('MLTeX v2.2 enabled');
  end;
@z

@x [29.536] l.10324 - Print rest of banner.
slow_print(format_ident); print("  ");
@y
wlog(version_string);
slow_print(format_ident); print("  ");
@z

@x [29.536] l.10327 - Adjust for C string conventions.
months:='JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@y
months := ' JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
@z

% [29.537] Use a path when calling a_open_in to do a \input; also, try
% to open the file with and without the `.tex' extension, regardless of
% whether the file already has an extension.  This allows filenames like
% `foo' and `foo.bar.tex', as well as `foo.tex' and `foo.bar'.
@x [29.537] l.10338 - start_input
begin scan_file_name; {set |cur_name| to desired file name}
if cur_ext="" then cur_ext:=".tex";
pack_cur_name;
loop@+  begin begin_file_reading; {set up |cur_file| and new level of input}
  if a_open_in(cur_file) then goto done;
  if cur_area="" then
    begin pack_file_name(cur_name,TEX_area,cur_ext);
    if a_open_in(cur_file) then goto done;
    end;
@y
var temp_str: str_number; k: integer;
begin scan_file_name; {set |cur_name| to desired file name}
pack_cur_name;
loop@+begin
  begin_file_reading; {set up |cur_file| and new level of input}
  tex_input_type := 1; {Tell |open_input| we are \.{\\input}.}
  {Kpathsea tries all the various ways to get the file.}
  if a_open_in(cur_file, kpse_tex_format) then
    {At this point |name_of_file| contains the actual name found.
     We extract the |cur_area|, |cur_name|, and |cur_ext| from it.}
    begin k:=1;
    name_in_progress:=true;
    begin_name;
    while (k<=name_length)and(more_name(name_of_file[k])) do
      incr(k);
    end_name;
    name_in_progress:=false;
    goto done;
    end;
@z

@x [29.537] l.10348 - start_input: don't force ".tex" extension.
  prompt_file_name("input file name",".tex");
@y
  prompt_file_name("input file name","");
@z

@x [29.537] l.10350 - start_input: string recycling
done: name:=a_make_name_string(cur_file);
@y
done: name:=a_make_name_string(cur_file);
if name=str_ptr-1 then {we can try to conserve string pool space now}
  begin temp_str:=search_string(name);
  if temp_str>0 then
    begin name:=temp_str; flush_string;
    end;
  end;
@z

@x [29.537] l.10352 - start_input: use different heuristic for initex.
  begin job_name:=cur_name; open_log_file;
@y
  begin job_name:=cur_name;
    Init
      if dump_option then begin
        str_room(format_default_length);
        for k:=1 to format_default_length - format_ext_length do
          append_char(xord[TEX_format_default[k]]);
        job_name:=make_string;
      end;
    Tini
    open_log_file;
@z

@x [29.537] l.10359 - start_input: don't return filename to string pool.
if name=str_ptr-1 then {we can conserve string pool space now}
  begin flush_string; name:=cur_name;
  end;
@y
@z

@x [30.548] l.10673 - texarray
@!internal_font_number=font_base..font_max; {|font| in a |char_node|}
@!font_index=0..font_mem_size; {index into |font_info|}
@y
@!internal_font_number=integer; {|font| in a |char_node|}
@!font_index=integer; {index into |font_info|}
@!nine_bits=min_quarterword..non_char;
@z

@x [30.549] l.10682 - texarray
@!font_info:array[font_index] of memory_word;
  {the big collection of font data}
@!fmem_ptr:font_index; {first unused word of |font_info|}
@!font_ptr:internal_font_number; {largest internal font number in use}
@!font_check:array[internal_font_number] of four_quarters; {check sum}
@!font_size:array[internal_font_number] of scaled; {``at'' size}
@!font_dsize:array[internal_font_number] of scaled; {``design'' size}
@!font_params:array[internal_font_number] of font_index; {how many font
  parameters are present}
@!font_name:array[internal_font_number] of str_number; {name of the font}
@!font_area:array[internal_font_number] of str_number; {area of the font}
@!font_bc:array[internal_font_number] of eight_bits;
  {beginning (smallest) character code}
@!font_ec:array[internal_font_number] of eight_bits;
  {ending (largest) character code}
@!font_glue:array[internal_font_number] of pointer;
  {glue specification for interword space, |null| if not allocated}
@!font_used:array[internal_font_number] of boolean;
  {has a character from this font actually appeared in the output?}
@!hyphen_char:array[internal_font_number] of integer;
  {current \.{\\hyphenchar} values}
@!skew_char:array[internal_font_number] of integer;
  {current \.{\\skewchar} values}
@!bchar_label:array[internal_font_number] of font_index;
  {start of |lig_kern| program for left boundary character,
  |non_address| if there is none}
@!font_bchar:array[internal_font_number] of min_quarterword..non_char;
  {right boundary character, |non_char| if there is none}
@!font_false_bchar:array[internal_font_number] of min_quarterword..non_char;
  {|font_bchar| if it doesn't exist in the font, otherwise |non_char|}
@y
@!font_info: ^fmemory_word;
  {the big collection of font data}
@!fmem_ptr:font_index; {first unused word of |font_info|}
@!font_ptr:internal_font_number; {largest internal font number in use}
@!font_check: ^four_quarters; {check sum}
@!font_size: ^scaled; {``at'' size}
@!font_dsize: ^scaled; {``design'' size}
@!font_params: ^font_index; {how many font
  parameters are present}
@!font_name: ^str_number; {name of the font}
@!font_area: ^str_number; {area of the font}
@!font_bc: ^eight_bits;
  {beginning (smallest) character code}
@!font_ec: ^eight_bits;
  {ending (largest) character code}
@!font_glue: ^pointer;
  {glue specification for interword space, |null| if not allocated}
@!font_used: ^boolean;
  {has a character from this font actually appeared in the output?}
@!hyphen_char: ^integer;
  {current \.{\\hyphenchar} values}
@!skew_char: ^integer;
  {current \.{\\skewchar} values}
@!bchar_label: ^font_index;
  {start of |lig_kern| program for left boundary character,
  |non_address| if there is none}
@!font_bchar: ^nine_bits;
  {right boundary character, |non_char| if there is none}
@!font_false_bchar: ^nine_bits;
  {|font_bchar| if it doesn't exist in the font, otherwise |non_char|}
@z

@x [30.550] l.10723 - texarray
@!char_base:array[internal_font_number] of integer;
  {base addresses for |char_info|}
@!width_base:array[internal_font_number] of integer;
  {base addresses for widths}
@!height_base:array[internal_font_number] of integer;
  {base addresses for heights}
@!depth_base:array[internal_font_number] of integer;
  {base addresses for depths}
@!italic_base:array[internal_font_number] of integer;
  {base addresses for italic corrections}
@!lig_kern_base:array[internal_font_number] of integer;
  {base addresses for ligature/kerning programs}
@!kern_base:array[internal_font_number] of integer;
  {base addresses for kerns}
@!exten_base:array[internal_font_number] of integer;
  {base addresses for extensible recipes}
@!param_base:array[internal_font_number] of integer;
  {base addresses for font parameters}
@y
@!char_base: ^integer;
  {base addresses for |char_info|}
@!width_base: ^integer;
  {base addresses for widths}
@!height_base: ^integer;
  {base addresses for heights}
@!depth_base: ^integer;
  {base addresses for depths}
@!italic_base: ^integer;
  {base addresses for italic corrections}
@!lig_kern_base: ^integer;
  {base addresses for ligature/kerning programs}
@!kern_base: ^integer;
  {base addresses for kerns}
@!exten_base: ^integer;
  {base addresses for extensible recipes}
@!param_base: ^integer;
  {base addresses for font parameters}
@z

@x [30.551] l.10743 - texarray
for k:=font_base to font_max do font_used[k]:=false;
@y
@z

@x [30.552] l.10749 - texarray
font_ptr:=null_font; fmem_ptr:=7;
font_name[null_font]:="nullfont"; font_area[null_font]:="";
hyphen_char[null_font]:="-"; skew_char[null_font]:=-1;
bchar_label[null_font]:=non_address;
font_bchar[null_font]:=non_char; font_false_bchar[null_font]:=non_char;
font_bc[null_font]:=1; font_ec[null_font]:=0;
font_size[null_font]:=0; font_dsize[null_font]:=0;
char_base[null_font]:=0; width_base[null_font]:=0;
height_base[null_font]:=0; depth_base[null_font]:=0;
italic_base[null_font]:=0; lig_kern_base[null_font]:=0;
kern_base[null_font]:=0; exten_base[null_font]:=0;
font_glue[null_font]:=null; font_params[null_font]:=7;
param_base[null_font]:=-1;
for k:=0 to 6 do font_info[k].sc:=0;
@y
@z

@x [30.554] l.10795 - MLTeX: |effective_char| in |char_info|
as fast as possible under the circumstances.
@^inner loop@>

@d char_info_end(#)==#].qqqq
@d char_info(#)==font_info[char_base[#]+char_info_end
@y
as fast as possible under the circumstances.
@^inner loop@>

ML\TeX{} will assume that a character |c| exists iff either exists in
the current font or a character substitution definition for this
character was defined using \.{\\charsubdef}.  To avoid the
distinction between these two cases, ML\TeX{} introduces the notion
``effective character'' of an input character |c|.  If |c| exists in
the current font, the effective character of |c| is the character |c|
itself.  If it doesn't exist but a character substitution is defined,
the effective character of |c| is the base character defined in the
character substitution.  If there is an effective character for a
non-existing character |c|, the ``virtual character'' |c| will get
appended to the horizontal lists.

The effective character is used within |char_info| to access
appropriate character descriptions in the font.  For example, when
calculating the width of a box, ML\TeX{} will use the metrics of the
effective characters.  For the case of a substitution, ML\TeX{} uses
the metrics of the base character, ignoring the metrics of the accent
character.

If character substitutions are changed, it will be possible that a
character |c| neither exists in a font nor there is a valid character
substitution for |c|.  To handle these cases |effective_char| should
be called with its first argument set to |true| to ensure that it
will still return an existing character in the font.  If neither |c|
nor the substituted base character in the current character
substitution exists, |effective_char| will output a warning and
return the character |font_bc[f]| (which is incorrect, but can not be
changed within the current framework).

Sometimes character substitutions are unwanted, therefore the
original definition of |char_info| can be used using the macro
|orig_char_info|.  Operations in which character substitutions should
be avoided are, for example, loading a new font and checking the font
metric information in this font, and character accesses in math mode.

@d char_list_exists(#)==(char_sub_code(#)>hi(0))
@d char_list_accent(#)==(ho(char_sub_code(#)) div 256)
@d char_list_char(#)==(ho(char_sub_code(#)) mod 256)
@#
@d char_info_end(#)== #@=)@>].qqqq
@d char_info(#)==
  font_info[char_base[#]+effective_char@=(@>true,#,char_info_end
@#
@d orig_char_info_end(#)==#].qqqq
@d orig_char_info(#)==font_info[char_base[#]+orig_char_info_end
@#
@z

@x [30] m.560 l.10876 - MLTeX: |effective_char| in |char_info|
@p function read_font_info(@!u:pointer;@!nom,@!aire:str_number;
@y
@p @t\4@>@<Declare additional functions for ML\TeX@>@/

function read_font_info(@!u:pointer;@!nom,@!aire:str_number;
@z

@x [30.563] l.10943 - Don't use TEX_font_area.
if aire="" then pack_file_name(nom,TEX_font_area,".tfm")
else pack_file_name(nom,aire,".tfm");
@y
{|kpse_find_file| will append the |".tfm"|, and avoid searching the disk
 before the font alias files as well.}
pack_file_name(nom,aire,"");
@z

% [30.564] Reading the tfm file.  As a special case, whenever we open a
% tfm file, we read its first byte into `tfm_temp' right away.  TeX
% looks at `fbyte' before calling `fget', so it ends up seeing every
% byte.  This is Pascal-like I/O.
@x [30.564] l.10956 - reading the tfm file, define fget & fbyte
@d fget==get(tfm_file)
@d fbyte==tfm_file^
@y
@d fget==tfm_temp:=getc(tfm_file)
@d fbyte==tfm_temp
@z

@x [30.570] l.11064 - MLTeX: fix for bug while loading font
  begin qw:=char_info(f)(d);
@y
  begin qw:=orig_char_info(f)(d);
@z

@x [30.573] l.11116 - MLTeX: fix for bug while loading font
  qw:=char_info(f)(#); {N.B.: not |qi(#)|}
@y
  qw:=orig_char_info(f)(#); {N.B.: not |qi(#)|}
@z

% [32.575] We only want `eof' on the TFM file to be true if we
% previously had EOF, not if we're at EOF now.  This is like `feof', and
% unlike our implementation of `eof' elsewhere.
@x [32.575] l.11161 - Reading the tfm file, replace eof() by feof().
if eof(tfm_file) then abort;
@y
if feof(tfm_file) then abort;
@z

@x [30.576] l.11180 - MLTeX: fix for bug while loading font
  begin qw:=char_info(f)(bchar); {N.B.: not |qi(bchar)|}
@y
  begin qw:=orig_char_info(f)(bchar); {N.B.: not |qi(bchar)|}
@z

@x [30.582] l.11276 - MLTeX: call |effective_char| in |new_character|
@p function new_character(@!f:internal_font_number;@!c:eight_bits):pointer;
label exit;
var p:pointer; {newly allocated node}
begin if font_bc[f]<=c then if font_ec[f]>=c then
  if char_exists(char_info(f)(qi(c))) then
@y

This allows a character node to be used if there is an equivalent
in the |char_sub_code| list.

@p function new_character(@!f:internal_font_number;@!c:eight_bits):pointer;
label exit;
var p:pointer; {newly allocated node}
@!ec:quarterword;  {effective character of |c|}
begin ec:=effective_char(false,f,qi(c));
if font_bc[f]<=qo(ec) then if font_ec[f]>=qo(ec) then
  if char_exists(orig_char_info(f)(ec)) then  {N.B.: not |char_info|}
@z

@x [32.592] l.11820 - font numbers can be >255 now.
@!c,@!f:quarterword; {character and font in current |char_node|}
@y
 {character and font in current |char_node|}
@!c:quarterword;
@!f:internal_font_number;
@z

@x [32.595] l.11860 - texarray
@!dvi_buf:array[dvi_index] of eight_bits; {buffer for \.{DVI} output}
@!half_buf:dvi_index; {half of |dvi_buf_size|}
@!dvi_limit:dvi_index; {end of the current half buffer}
@!dvi_ptr:dvi_index; {the next available buffer address}
@y
@!dvi_buf:^eight_bits; {buffer for \.{DVI} output}
@!half_buf:integer; {half of |dvi_buf_size|}
@!dvi_limit:integer; {end of the current half buffer}
@!dvi_ptr:integer; {the next available buffer address}
@z

@x [32.597] l.11886 - write_dvi done in C.
@p procedure write_dvi(@!a,@!b:dvi_index);
var k:dvi_index;
begin for k:=a to b do write(dvi_file,dvi_buf[k]);
end;
@y
In C, we use a macro to call |fwrite| or |write| directly, writing all
the bytes in one shot.  Much better even than writing four
bytes at a time.
@z

@x [32.602] l.11944 - Allow for outputting more than 256 fonts.
begin dvi_out(fnt_def1);
dvi_out(f-font_base-1);@/
@y
begin if f<=256+font_base then
  begin dvi_out(fnt_def1);
  dvi_out(f-font_base-1);
  end
else begin dvi_out(fnt_def1+1);
  dvi_out((f-font_base-1) div @'400);
  dvi_out((f-font_base-1) mod @'400);
  end;
@z

@x [32.617] l.12261 - Use output_comment if the user set it. Assume it's short enough.
  old_setting:=selector; selector:=new_string;
@y
if output_comment then
  begin l:=strlen(output_comment); dvi_out(l);
  for s:=0 to l-1 do dvi_out(output_comment[s]);
  end
else begin {the default code is unchanged}
  old_setting:=selector; selector:=new_string;
@z

@x [32.617] l.12268 - Use output_comment if the user set it.
  pool_ptr:=str_start[str_ptr]; {flush the current string}
@y
  pool_ptr:=str_start[str_ptr]; {flush the current string}
end;
@z

@x [32.619] l.12294 - MLTeX: substitute character in |hlist_out|
procedure hlist_out; {output an |hlist_node| box}
label reswitch, move_past, fin_rule, next_p;
@y
procedure hlist_out; {output an |hlist_node| box}
label reswitch, move_past, fin_rule, next_p, continue, found;
@z

@x [32.620] l.12326 - MLTeX: replace virtual character in |hlist_out|
reaching a non-|char_node|. The program uses the fact that |set_char_0=0|.
@^inner loop@>
@y
reaching a non-|char_node|. The program uses the fact that |set_char_0=0|.

In ML\TeX{} this part looks for the existence of a substitution
definition for a character |c|, if |c| does not exist in the font,
and create appropriate \.{DVI} commands.  Former versions of ML\TeX{}
have spliced appropriate character, kern, and box nodes into the
horizontal list.
%
% 91/05/08 \charsubdefmax bug detected by Bernd Raichle
Because the user can change character substitions or
\.{\\charsubdefmax} on the fly, we have to test a again
for valid substitutions.
%
% 93/10/29 \leaders bug detected by Eberhard Mattes
(Additional it is necessary to be careful---if leaders are used
the current hlist is normally traversed more than once!)
@^inner loop@>
@z

@x [32.620] l.12334 - MLTeX: substitute character during |shipout|
  if c>=qi(128) then dvi_out(set1);
  dvi_out(qo(c));@/
  cur_h:=cur_h+char_width(f)(char_info(f)(c));
@y
  if font_ec[f]>=qo(c) then if font_bc[f]<=qo(c) then
    if char_exists(orig_char_info(f)(c)) then  {N.B.: not |char_info|}
      begin if c>=qi(128) then dvi_out(set1);
      dvi_out(qo(c));@/
      cur_h:=cur_h+char_width(f)(orig_char_info(f)(c));
      goto continue;
      end;
  if mltex_enabled_p then
    @<Output a substitution, |goto continue| if not possible@>;
continue:
@z

@x [32.622] l.12349 - more >256 font output stuff.
else  begin dvi_out(fnt1); dvi_out(f-font_base-1);
  end;
@y
else if f<=256+font_base then
  begin dvi_out(fnt1); dvi_out(f-font_base-1);
  end
else begin dvi_out(fnt1+1);
  dvi_out((f-font_base-1) div @'400);
  dvi_out((f-font_base-1) mod @'400);
  end;
@z

% We output each portion of the page as we get to it, if we are using
% IPC, so that the previewer (TeXView) can display it immediately. [SPM]
@x [32.640] l.12690 - IPC
dvi_out(eop); incr(total_pages); cur_s:=-1;
@y
dvi_out(eop); incr(total_pages); cur_s:=-1;
ifdef ('IPC')
if ipc_on>0 then
  begin if dvi_limit=half_buf then
    begin write_dvi(half_buf, dvi_buf_size-1);
    flush_dvi;
    dvi_gone:=dvi_gone+half_buf;
    end;
  if dvi_ptr>0 then
    begin write_dvi(0, dvi_ptr-1);
    flush_dvi;
    dvi_offset:=dvi_offset+dvi_ptr; dvi_gone:=dvi_gone+dvi_ptr;
    end;
  dvi_ptr:=0; dvi_limit:=dvi_buf_size;
  ipc_page(dvi_gone);
  end;
endif ('IPC');
@z

@x [32.642] l.12742 - Use dvi_offset instead of dvi_buf_size with IPC stuff.
  k:=4+((dvi_buf_size-dvi_ptr) mod 4); {the number of 223's}
@y
ifdef ('IPC')
  k:=7-((3+dvi_offset+dvi_ptr) mod 4); {the number of 223's}
endif ('IPC')
ifndef ('IPC')
  k:=4+((dvi_buf_size-dvi_ptr) mod 4); {the number of 223's}
endifn ('IPC')
@z

% The MLTeX changes never dealt with the problems of character
% substitutions in math mode.  With the new additions in v2.2,
% non-existing characters between |font_bc[f]| and |font_ec[f]|
% can be substituted => we have to avoid this in math mode
% (for compatibility reasons and to avoid other problems).
%
@x [35.708] l.13903 - MLTeX: avoid substitution in |var_delimiter|
if (qo(y)>=font_bc[g])and(qo(y)<=font_ec[g]) then
  begin continue: q:=char_info(g)(y);
@y
if (qo(y)>=font_bc[g])and(qo(y)<=font_ec[g]) then
  begin continue: q:=orig_char_info(g)(y);
@z

@x [36.722] l.14172 - MLTeX: avoid substitution in |fetch|
else  begin if (qo(cur_c)>=font_bc[cur_f])and(qo(cur_c)<=font_ec[cur_f]) then
    cur_i:=char_info(cur_f)(cur_c)
@y
else  begin if (qo(cur_c)>=font_bc[cur_f])and(qo(cur_c)<=font_ec[cur_f]) then
    cur_i:=orig_char_info(cur_f)(cur_c)
@z

@x [36.740] l.14486 - MLTeX: avoid substitution in |make_math_accent|
  i:=char_info(f)(y);
@y
  i:=orig_char_info(f)(y);
@z

@x [36.749] l.14638 - MLTeX: avoid substitution in |make_op|
    begin c:=rem_byte(cur_i); i:=char_info(cur_f)(c);
@y
    begin c:=rem_byte(cur_i); i:=orig_char_info(cur_f)(c);
@z

@x [38.859] l.16855 - Fix a casting/expression evaluation problem.
if abs(fit_class-fitness(r))>1 then d:=d+adj_demerits;
@y
if abs(toint(fit_class)-toint(fitness(r)))>1 then d:=d+adj_demerits;
@z

@x [39.875] l.17170 - Another casting problem.
  begin line_diff:=line_number(r)-best_line;
@y
  begin line_diff:=toint(line_number(r))-toint(best_line);
@z

@x [42.920] l.18056 - bigtrie: allow larger hyphenation tries.
Comparatively few different number sequences $n_0\ldots n_k$ actually occur,
since most of the |n|'s are generally zero. Therefore the number sequences
are encoded in such a way that |trie_op|$(z_k)$ is only one byte long.
If |trie_op(@t$z_k$@>)<>min_quarterword|, when $p_1\ldots p_k$ has matched
the letters in |hc[(l-k+1)..l@,]| of language |t|,
we perform all of the required operations
for this pattern by carrying out the following little program: Set
|v:=trie_op(@t$z_k$@>)|. Then set |v:=v+op_start[t]|,
|hyf[l-hyf_distance[v]]:=@tmax@>(hyf[l-hyf_distance[v]], hyf_num[v])|,
and |v:=hyf_next[v]|; repeat, if necessary, until |v=min_quarterword|.
@y
The theory that comparatively few different number sequences $n_0\ldots n_k$
actually occur, since most of the |n|'s are generally zero, seems to fail
at least for the large German hyphenation patterns.
Therefore the number sequences cannot any longer be encoded in such a way
that |trie_op|$(z_k)$ is only one byte long.
We have introduced a new constant |max_trie_op| for the maximum allowable
hyphenation operation code value; |max_trie_op| might be different for
\TeX\ and \.{INITEX} and must not exceed |max_halfword|.
An opcode will occupy a halfword if |max_trie_op| exceeds |max_quarterword|
or a quarterword otherwise.
@^system dependencies@>
If |trie_op(@t$z_k$@>)<>min_trie_op|, when $p_1\ldots p_k$ has matched
the letters in |hc[(l-k+1)..l@,]| of language |t|,
we perform all of the required operations
for this pattern by carrying out the following little program: Set
|v:=trie_op(@t$z_k$@>)|. Then set |v:=v+op_start[t]|,
|hyf[l-hyf_distance[v]]:=@tmax@>(hyf[l-hyf_distance[v]], hyf_num[v])|,
and |v:=hyf_next[v]|; repeat, if necessary, until |v=min_trie_op|.
@z

@x [42.920] l.18068 - bigtrie: allow larger hyphenation tries.
@!trie_pointer=0..trie_size; {an index into |trie|}
@y
@!trie_pointer=0..ssup_trie_size; {an index into |trie|}
@!trie_opcode=0..ssup_trie_opcode;  {a trie opcode}
@z

@x [42.921] l.18070 - bigtrie: allow larger hyphenation tries.
@ @d trie_link(#)==trie[#].rh {``downward'' link in a trie}
@d trie_char(#)==trie[#].b1 {character matched at this trie location}
@d trie_op(#)==trie[#].b0 {program for hyphenation at this trie location}
@y
@ For more than 255 trie op codes, the three fields |trie_link|, |trie_char|,
and |trie_op| will no longer fit into one memory word; thus using web2c
we define |trie| as three array instead of an array of records.
The variant will be implented by reusing the opcode field later on with
another macro.

@d trie_link(#)==trie_trl[#] {``downward'' link in a trie}
@d trie_char(#)==trie_trc[#] {character matched at this trie location}
@d trie_op(#)==trie_tro[#] {program for hyphenation at this trie location}
@z

@x [42.921] l.18075 - bigtrie: allow larger hyphenation tries.
@!trie:array[trie_pointer] of two_halves; {|trie_link|, |trie_char|, |trie_op|}
@y
{We will dynamically allocate these arrays.}
@!trie_trl:^trie_pointer; {|trie_link|}
@!trie_tro:^trie_pointer; {|trie_op|}
@!trie_trc:^quarterword; {|trie_char|}
@z

@x [42.921] l.18078 - bigtrie: allow larger hyphenation tries.
@!hyf_next:array[1..trie_op_size] of quarterword; {continuation code}
@y
@!hyf_next:array[1..trie_op_size] of trie_opcode; {continuation code}
@z

@x [42.923] l.18099 - bigtrie: allow larger hyphenation tries.
    begin if trie_op(z)<>min_quarterword then
@y
    begin if trie_op(z)<>min_trie_op then
@z

@x [42.924] l.18112 - bigtrie: allow larger hyphenation tries.
until v=min_quarterword;
@y
until v=min_trie_op;
@z

%%%%%%%% dynamic hyph_size
@x 18126 m.925
different from $\alpha$, we can conclude that $\alpha$ is not in the table.
@y  18126
different from $\alpha$, we can conclude that $\alpha$ is not in the table.
This is a clever scheme which saves the need for a hash link array.
However, it is difficult to increase the size of the hyphen exception
arrays. To make this easier, the ordered hash has been replaced by
a simple hash, using an additional array |hyph_link|. The value
|0| in |hyph_link[k]| means that there are no more entries corresponding
to the specific hash chain. When |hyph_link[k]>0|, the next entry in
the hash chain is |hyph_link[k]-1|. This value is used because the
arrays start at |0|.
@z

%%%%%%%% dynamic hyph_size
@x 18134 m.925
@!hyph_pointer=0..hyph_size; {an index into the ordered hash table}
@y  18134
@!hyph_pointer=0..ssup_hyph_size; {index into hyphen exceptions hash table;
                     enlarging this requires changing (un)dump code}
@z

%%%%%%%% dynamic hyph_size
@x 18137 m.926
@!hyph_word:array[hyph_pointer] of str_number; {exception words}
@!hyph_list:array[hyph_pointer] of pointer; {list of hyphen positions}
@!hyph_count:hyph_pointer; {the number of words in the exception dictionary}
@y  18139
@!hyph_word: ^str_number; {exception words}
@!hyph_list: ^pointer; {list of hyphen positions}
@!hyph_link: ^hyph_pointer; {link array for hyphen exceptions hash table}
@!hyph_count:integer; {the number of words in the exception dictionary}
@!hyph_next:integer; {next free slot in hyphen exceptions hash table}
@z

%%%%%%%% dynamic hyph_size
@x 18145 m.928
for z:=0 to hyph_size do
  begin hyph_word[z]:=0; hyph_list[z]:=null;
  end;
hyph_count:=0;
@y  18148
for z:=0 to hyph_size do
  begin hyph_word[z]:=0; hyph_list[z]:=null; hyph_link[z]:=0;
  end;
hyph_count:=0;
hyph_next:=hyph_prime+1; if hyph_next>hyph_size then hyph_next:=hyph_prime;
@z

%%%%%%%% dynamic hyph_size
@x 18163 m.930
h:=hc[1]; incr(hn); hc[hn]:=cur_lang;
for j:=2 to hn do h:=(h+h+hc[j]) mod hyph_size;
loop@+  begin @<If the string |hyph_word[h]| is less than \(hc)|hc[1..hn]|,
    |goto not_found|; but if the two strings are equal,
    set |hyf| to the hyphen positions and |goto found|@>;
  if h>0 then decr(h)@+else h:=hyph_size;
  end;
not_found: decr(hn)
@y  18170
h:=hc[1]; incr(hn); hc[hn]:=cur_lang;
for j:=2 to hn do h:=(h+h+hc[j]) mod hyph_prime;
loop@+  begin @<If the string |hyph_word[h]| is less than \(hc)|hc[1..hn]|,
    |goto not_found|; but if the two strings are equal,
    set |hyf| to the hyphen positions and |goto found|@>;
  h:=hyph_link[h]; if h=0 then goto not_found;
  decr(h);
  end;
not_found: decr(hn)
@z

%%%%%%%% dynamic hyph_size
@x 18172 m.931
@ @<If the string |hyph_word[h]| is less than \(hc)...@>=
k:=hyph_word[h]; if k=0 then goto not_found;
if length(k)<hn then goto not_found;
if length(k)=hn then
  begin j:=1; u:=str_start[k];
  repeat if so(str_pool[u])<hc[j] then goto not_found;
  if so(str_pool[u])>hc[j] then goto done;
  incr(j); incr(u);
  until j>hn;
  @<Insert hyphens as specified in |hyph_list[h]|@>;
  decr(hn); goto found;
  end;
done:
@y  18184
@ @<If the string |hyph_word[h]| is less than \(hc)...@>=
{This is now a simple hash list, not an ordered one, so
the module title is no longer descriptive.}
k:=hyph_word[h]; if k=0 then goto not_found;
if length(k)=hn then
  begin j:=1; u:=str_start[k];
  repeat
  if so(str_pool[u])<>hc[j] then goto done;
  incr(j); incr(u);
  until j>hn;
  @<Insert hyphens as specified in |hyph_list[h]|@>;
  decr(hn); goto found;
  end;
done:
@z

%%%%%%%% dynamic hyph_size
@x 18274 m.939
  begin h:=(h+h+hc[j]) mod hyph_size;
@y  18274
  begin h:=(h+h+hc[j]) mod hyph_prime;
@z

%%%%%%%% dynamic hyph_size
@x 18281 m.940
@ @<Insert the \(p)pair |(s,p)|...@>=
if hyph_count=hyph_size then overflow("exception dictionary",hyph_size);
@:TeX capacity exceeded exception dictionary}{\quad exception dictionary@>
incr(hyph_count);
while hyph_word[h]<>0 do
  begin @<If the string |hyph_word[h]| is less than \(or)or equal to
  |s|, interchange |(hyph_word[h],hyph_list[h])| with |(s,p)|@>;
  if h>0 then decr(h)@+else h:=hyph_size;
  end;
hyph_word[h]:=s; hyph_list[h]:=p
@y  18290
@ @<Insert the \(p)pair |(s,p)|...@>=
  if hyph_next <= hyph_prime then
     while (hyph_next>0) and (hyph_word[hyph_next-1]>0) do decr(hyph_next);
if (hyph_count=hyph_size)or(hyph_next=0) then
   overflow("exception dictionary",hyph_size);
@:TeX capacity exceeded exception dictionary}{\quad exception dictionary@>
incr(hyph_count);
while hyph_word[h]<>0 do
  begin @<If the string |hyph_word[h]| is less than \(or)or equal to
  |s|, interchange |(hyph_word[h],hyph_list[h])| with |(s,p)|@>;
  if hyph_link[h]=0 then
  begin
    hyph_link[h]:=hyph_next;
    if hyph_next >= hyph_size then hyph_next:=hyph_prime;
    if hyph_next > hyph_prime then incr(hyph_next);
  end;
  h:=hyph_link[h]-1;
  end;

found: hyph_word[h]:=s; hyph_list[h]:=p
@z

%%%%%%%% dynamic hyph_size
@x 18292 m.941
@ @<If the string |hyph_word[h]| is less than \(or)...@>=
k:=hyph_word[h];
if length(k)<length(s) then goto found;
if length(k)>length(s) then goto not_found;
u:=str_start[k]; v:=str_start[s];
repeat if str_pool[u]<str_pool[v] then goto found;
if str_pool[u]>str_pool[v] then goto not_found;
incr(u); incr(v);
until u=str_start[k+1];
found:q:=hyph_list[h]; hyph_list[h]:=p; p:=q;@/
t:=hyph_word[h]; hyph_word[h]:=s; s:=t;
not_found:
@y  18303
@ @<If the string |hyph_word[h]| is less than \(or)...@>=
{This is now a simple hash list, not an ordered one, so
the module title is no longer descriptive.}
k:=hyph_word[h];
if length(k)<>length(s) then goto not_found;
u:=str_start[k]; v:=str_start[s];
repeat if str_pool[u]<>str_pool[v] then goto not_found;
incr(u); incr(v);
until u=str_start[k+1];
{repeat hyphenation exception; flushing old data}
flush_string; s:=hyph_word[h]; {avoid |slow_make_string|!}
decr(hyph_count);
{ We could also |flush_list(hyph_list[h]);|, but it interferes
  with \.{trip.log}. }
goto found;
not_found:
@z

@x [43.943] l.18332 - bigtrie: Larger tries, also in documentation parts.
|hyf_next[@t$v^\prime$@>]=min_quarterword|.
@y
|hyf_next[@t$v^\prime$@>]=min_trie_op|.
@z

@x [43.943] l.18336 - bigtrie: Larger tries, also in documentation parts.
$$\hbox{|@t$v^\prime$@>:=new_trie_op(0,1,min_quarterword)|,\qquad
@y
$$\hbox{|@t$v^\prime$@>:=new_trie_op(0,1,min_trie_op)|,\qquad
@z

@x [43.943] l.18346 - web2c can't parse negative lower bounds in arrays.  Sorry.
@!init@! trie_op_hash:array[-trie_op_size..trie_op_size] of 0..trie_op_size;
@y
@!init@! trie_op_hash:array[neg_trie_op_size..trie_op_size] of 0..trie_op_size;
@z

@x [43.943] l.18348 - bigtrie: Larger hyphenation tries.
@!trie_used:array[ASCII_code] of quarterword;
@y
@!trie_used:array[ASCII_code] of trie_opcode;
@z

@x [43.943] l.18352 - bigtrie: Larger hyphenation tries.
@!trie_op_val:array[1..trie_op_size] of quarterword;
@y
@!trie_op_val:array[1..trie_op_size] of trie_opcode;
@z

@x [43.943] l.18355 - Dynamic trie arrays
tini
@y
tini@;
@!max_op_used:trie_opcode; {largest opcode used for any language}
@!small_op:boolean; {flag used while dumping or undumping}
@z

@x [43.944] l.18358 - bigtrie: Larger tries, also in documentation parts.
|new_trie_op| could return |min_quarterword| (thereby simply ignoring
@y
|new_trie_op| could return |min_trie_op| (thereby simply ignoring
@z

@x [43.944] l.18365 - bigtrie: Larger hyphenation tries.
function new_trie_op(@!d,@!n:small_number;@!v:quarterword):quarterword;
label exit;
var h:-trie_op_size..trie_op_size; {trial hash location}
@!u:quarterword; {trial op code}
@y
function new_trie_op(@!d,@!n:small_number;@!v:trie_opcode):trie_opcode;
label exit;
var h:neg_trie_op_size..trie_op_size; {trial hash location}
@!u:trie_opcode; {trial op code}
@z

@x [43.944] l.18370 - Another casting problem, and use |neg_trie_op_size|.
begin h:=abs(n+313*d+361*v+1009*cur_lang) mod (trie_op_size+trie_op_size)
  - trie_op_size;
@y
begin h:=abs(toint(n)+313*toint(d)+361*toint(v)+1009*toint(cur_lang))
  mod (trie_op_size - neg_trie_op_size)
  + neg_trie_op_size;
@z

@x [43.944] l.18377 - bigtrie: And larger tries again.
    if u=max_quarterword then
      overflow("pattern memory ops per language",
        max_quarterword-min_quarterword);
    incr(trie_op_ptr); incr(u); trie_used[cur_lang]:=u;
@y
    if u=max_trie_op then
      overflow("pattern memory ops per language",
      max_trie_op-min_trie_op);
    incr(trie_op_ptr); incr(u); trie_used[cur_lang]:=u;
    if u>max_op_used then max_op_used:=u;
@z

@x [43.945] l.18399 - bigtrie: And larger tries again.
op_start[0]:=-min_quarterword;
@y
op_start[0]:=-min_trie_op;
@z

@x [43.946] l.18416 - bigtrie: And larger tries again.
for k:=0 to 255 do trie_used[k]:=min_quarterword;
@y
for k:=0 to 255 do trie_used[k]:=min_trie_op;
@z

@x [43.946] l.18417 - Dynamic trie arrays.
trie_op_ptr:=0;
@y
max_op_used:=min_trie_op;
trie_op_ptr:=0;
@z

@x [43.947] l.18438 - Dynamically allocate arrays, and a casting problem.
@!init @!trie_c:packed array[trie_pointer] of packed_ASCII_code;
  {characters to match}
@t\hskip10pt@>@!trie_o:packed array[trie_pointer] of quarterword;
  {operations to perform}
@t\hskip10pt@>@!trie_l:packed array[trie_pointer] of trie_pointer;
  {left subtrie links}
@t\hskip10pt@>@!trie_r:packed array[trie_pointer] of trie_pointer;
  {right subtrie links}
@t\hskip10pt@>@!trie_ptr:trie_pointer; {the number of nodes in the trie}
@t\hskip10pt@>@!trie_hash:packed array[trie_pointer] of trie_pointer;
  {used to identify equivalent subtries}
tini
@y
@!init @!trie_c:^packed_ASCII_code;
  {characters to match}
@t\hskip10pt@>@!trie_o:^trie_opcode;
  {operations to perform}
@t\hskip10pt@>@!trie_l:^trie_pointer;
  {left subtrie links}
@t\hskip10pt@>@!trie_r:^trie_pointer;
  {right subtrie links}
@t\hskip10pt@>@!trie_ptr:trie_pointer; {the number of nodes in the trie}
@t\hskip10pt@>@!trie_hash:^trie_pointer;
  {used to identify equivalent subtries}
tini
@z

@x [43.948] l.18471 - Another casting problem.
begin h:=abs(trie_c[p]+1009*trie_o[p]+@|
    2718*trie_l[p]+3142*trie_r[p]) mod trie_size;
@y
begin h:=abs(toint(trie_c[p])+1009*toint(trie_o[p])+@|
    2718*toint(trie_l[p])+3142*toint(trie_r[p])) mod trie_size;
@z

@x [43.950] l.18521 - Dynamically allocate & larger tries.
@d trie_back(#)==trie[#].lh {backward links in |trie| holes}
@y
@d trie_back(#)==trie_tro[#] {use the opcode field now for backward links}
@z

@x [43.590] l.18524 - Dynamically allocate & larger tries.
@!init@!trie_taken:packed array[1..trie_size] of boolean;
  {does a family start here?}
@t\hskip10pt@>@!trie_min:array[ASCII_code] of trie_pointer;
  {the first possible slot for each character}
@t\hskip10pt@>@!trie_max:trie_pointer; {largest location used in |trie|}
@t\hskip10pt@>@!trie_not_ready:boolean; {is the trie still in linked form?}
tini
@y
@!init@!trie_taken: ^boolean;
  {does a family start here?}
@t\hskip10pt@>@!trie_min:array[ASCII_code] of trie_pointer;
  {the first possible slot for each character}
@t\hskip10pt@>@!trie_max:trie_pointer; {largest location used in |trie|}
@t\hskip10pt@>@!trie_not_ready:boolean; {is the trie still in linked form?}
tini
@z

@x [43.951] l.18539 - Dynamically allocate.
trie_not_ready:=true; trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;
@y
trie_not_ready:=true;
@z

@x [43.958] l.18634 - bigtrie: Larger tries.
@<Move the data into |trie|@>=
h.rh:=0; h.b0:=min_quarterword; h.b1:=min_quarterword; {|trie_link:=0|,
  |trie_op:=min_quarterword|, |trie_char:=qi(0)|}
@y
@d clear_trie == {clear |trie[r]|}
  begin trie_link(r):=0;
  trie_op(r):=min_trie_op;
  trie_char(r):=min_quarterword; {|trie_char:=qi(0)|}
  end

@<Move the data into |trie|@>=
@z

@x [43.958] l.18638 - bigtrie: Larger tries.
  begin for r:=0 to 256 do trie[r]:=h;
@y
  begin for r:=0 to 256 do clear_trie;
@z

@x [43.958] l.18643 - bigtrie: Larger tries.
  repeat s:=trie_link(r); trie[r]:=h; r:=s;
@y
  repeat s:=trie_link(r); clear_trie; r:=s;
@z

@x [43.960] l.18677 - bigtrie: Larger tries.
@!v:quarterword; {trie op code}
@y
@!v:trie_opcode; {trie op code}
@z

@x [43.963] l.18749 - bigtrie: Larger tries.
if trie_o[q]<>min_quarterword then
@y
if trie_o[q]<>min_trie_op then
@z

@x [43.964] l.18762 - bigtrie: Larger tries.
trie_c[p]:=si(c); trie_o[p]:=min_quarterword;
@y
trie_c[p]:=si(c); trie_o[p]:=min_trie_op;
@z

@x [43.965] l.18768 - bigtrie: Larger tries.
l:=k; v:=min_quarterword;
@y
l:=k; v:=min_trie_op;
@z

@x [43.966] l.18786 - bigtrie: Larger tries.
@!h:two_halves; {template used to zero out |trie|'s holes}
@y
@z

%%
%% We can rewrite the original code after "main_loop_move+2" upto the
%% "tail_append(lig_stack)" in module 1036 as
%%
%
% main_loop_move+2:
% if font_bc[main_f]<=cur_chr then
%  if cur_chr<=font_ec[main_f] then
%    begin  main_i:=char_info(main_f)(cur_l);
%    if char_exists(main_i) goto main_loop_move+3;
%    end;
% char_warning(main_f,cur_chr); free_avail(lig_stack); goto big_switch;
% main_loop_move+3:
% tail_append(lig_stack) {|main_loop_lookahead| is next}
%
%%
%% We can use the rewritten code above to include additional MLTeX
%% specific parts in the future.  Additionally it can be used when
%% optimizing |main_control| to minimize the call of the function
%% |effective_char|.
%%
%
%@x [46.1030] l.19977 - MLTeX: substitution in |main_control|
%  main_loop_move,main_loop_move+1,main_loop_move+2,main_loop_move_lig,
%@y
%  main_loop_move,main_loop_move+1,main_loop_move+2,main_loop_move+3,
%  main_loop_move_lig,
%@z

@x [46.1036] l.20138 - MLTeX: substitution in |main_control|
main_loop_move+2:if(cur_chr<font_bc[main_f])or(cur_chr>font_ec[main_f]) then
@y
main_loop_move+2:
if(qo(effective_char(false,main_f,qi(cur_chr)))>font_ec[main_f])or
  (qo(effective_char(false,main_f,qi(cur_chr)))<font_bc[main_f]) then
@z

@x [46.1036] l.20141 - MLTeX: substitution in |main_control|
main_i:=char_info(main_f)(cur_l);
@y
main_i:=effective_char_info(main_f,cur_l);
@z

@x [49.1215] l.22719 - hash_extra
if (cur_cs=0)or(cur_cs>frozen_control_sequence) then
@y
if (cur_cs=0)or(cur_cs>eqtb_top)or
  ((cur_cs>frozen_control_sequence)and(cur_cs<=eqtb_size)) then
@z

@x [49.1222] l.22794 - MLTeX: \charsubdef primitive
@d toks_def_code=6 {|shorthand_def| for \.{\\toksdef}}
@y
@d toks_def_code=6 {|shorthand_def| for \.{\\toksdef}}
@d char_sub_def_code=7 {|shorthand_def| for \.{\\charsubdef}}
@z

@x [49.1222] l.22810 - MLTeX: \charsubdef primitive
@!@:toks_def_}{\.{\\toksdef} primitive@>
@y
@!@:toks_def_}{\.{\\toksdef} primitive@>
if mltex_p then
  begin
  primitive("charsubdef",shorthand_def,char_sub_def_code);@/
@!@:char_sub_def_}{\.{\\charsubdef} primitive@>
  end;
@z

@x [49.1222] l.22820 - MLTeX: \charsubdef primitive
  othercases print_esc("toksdef")
@y
  char_sub_def_code: print_esc("charsubdef");
  othercases print_esc("toksdef")
@z

@x [49.1222] l.22833 - MLTeX: \charsubdef primitive
shorthand_def: begin n:=cur_chr; get_r_token; p:=cur_cs; define(p,relax,256);
@y
shorthand_def: if cur_chr=char_sub_def_code then
 begin scan_char_num; p:=char_sub_code_base+cur_val; scan_optional_equals;
  scan_char_num; n:=cur_val; {accent character in substitution}
  scan_char_num;
  if (tracing_char_sub_def>0) then
    begin begin_diagnostic; print_nl("New character substitution: ");
    print_ASCII(p-char_sub_code_base); print(" = ");
    print_ASCII(n); print_char(" ");
    print_ASCII(cur_val); end_diagnostic(false);
    end;
  n:=n*256+cur_val;
  define(p,data,hi(n));
  if (p-char_sub_code_base)<char_sub_def_min then
    word_define(int_base+char_sub_def_min_code,p-char_sub_code_base);
  if (p-char_sub_code_base)>char_sub_def_max then
    word_define(int_base+char_sub_def_max_code,p-char_sub_code_base);
 end
else begin n:=cur_chr; get_r_token; p:=cur_cs; define(p,relax,256);
@z

@x [49.1252] l.23230 - INI = VIR, so have to do runtime test.
    begin @!init new_patterns; goto done;@;@+tini@/
@y  23215
    begin @!Init new_patterns; goto done;@;@+Tini@/
@z

% undo Knuth's change because
%   a) the string is already replaced in |scan_file_name| and therefore
%   b) the wrong string will get flushed!!!
%
@x [49.1260] l.23331 new_font: string recycling -- already done
flushable_string:=str_ptr-1;
@y
@z

% If you don't want to remove code with the following change,
% please replace the former change by
%
% @x
% flushable_string:=str_ptr-1;
% @y
% if cur_name=str_ptr-1 then
%   flushable_string:=str_ptr-1
% else
%   flushable_string:=str_ptr;  {number of a non-existing}
% @z
%
% otherwise the wrong string will get removed by |flush_string|!!
%
@x [49.1260] l.23334 new_font: string recycling -- already done
    begin if cur_name=flushable_string then
      begin flush_string; cur_name:=font_name[f];
      end;
    if s>0 then
@y
    begin if s>0 then
@z

@x [49.1265] if batchmode, MakeTeX... scripts should be silent.
interaction:=cur_chr;
@y
interaction:=cur_chr;
if interaction = batch_mode
then kpse_make_tex_discard_errors := 1
else kpse_make_tex_discard_errors := 0;
@z

@x [49.1275] l.23441 - Same stuff as for \input, this time for \openin.
  if cur_ext="" then cur_ext:=".tex";
  pack_cur_name;
  if a_open_in(read_file[n]) then read_open[n]:=just_open;
@y
  pack_cur_name;
  tex_input_type:=0; {Tell |open_input| we are \.{\\openin}.}
  if a_open_in(read_file[n], kpse_tex_format) then read_open[n]:=just_open;
@z

@x [50.1301] l.23679 - INI = VIR, so runtime test.
format_ident:=" (INITEX)";
@y
if ini_version then format_ident:=" (INITEX)";
@z

@x [50.1302] l.23690 - Eliminate now-unused variable `w' in `store_fmt_file'.
@!w: four_quarters; {four ASCII codes}
@y
@z

@x [50.1302] l.23694 - MLTeX: dump |mltex_p| to fmt file
@<Dump constants for consistency check@>;
@y
@<Dump constants for consistency check@>;
dump_int(@"4D4C5458);  {ML\TeX's magic constant: "MLTX"}
if mltex_p then dump_int(1)
else dump_int(0);
@z

@x [50.1303] l.23722 - Ditto, for `load_fmt_file'.
@!w: four_quarters; {four ASCII codes}
@y
@z

@x [50.1303] l.23694 - MLTeX: undump |mltex_enabled_p| from fmt file
begin @<Undump constants for consistency check@>;
@y
begin @<Undump constants for consistency check@>;
undump_int(x);   {check magic constant of ML\TeX}
if x<>@"4D4C5458 then goto bad_fmt;
undump_int(x);   {undump |mltex_p| flag into |mltex_enabled_p|}
if x=1 then mltex_enabled_p:=true
else if x<>0 then goto bad_fmt;
@z

@x [50.1305] l.23751 - Do dumping and undumping of fmt files in C.
@d dump_wd(#)==begin fmt_file^:=#; put(fmt_file);@+end
@d dump_int(#)==begin fmt_file^.int:=#; put(fmt_file);@+end
@d dump_hh(#)==begin fmt_file^.hh:=#; put(fmt_file);@+end
@d dump_qqqq(#)==begin fmt_file^.qqqq:=#; put(fmt_file);@+end
@y
@z
@x [1306]
@d undump_wd(#)==begin get(fmt_file); #:=fmt_file^;@+end
@d undump_int(#)==begin get(fmt_file); #:=fmt_file^.int;@+end
@d undump_hh(#)==begin get(fmt_file); #:=fmt_file^.hh;@+end
@d undump_qqqq(#)==begin get(fmt_file); #:=fmt_file^.qqqq;@+end
@y
@z
@x [still 1306] debug format file
@d undump_size_end_end(#)==too_small(#)@+else undump_end_end
@y
@d format_debug_end(#)==
    write_ln (stderr, ' = ', #);
  end;
@d format_debug(#)==
  if debug_format_file then begin
    write (stderr, 'fmtdebug:', #);
    format_debug_end
@d undump_size_end_end(#)==
  too_small(#)@+else format_debug (#)(x); undump_end_end
@z

@x [50,1307] l.23779 - texarray
dump_int(@$);@/
@y
dump_int(@$);@/
dump_int(max_halfword);@/
dump_int(hash_high);
@z

%%%%%%%% dynamic hyph_size
@x 23784 m.1307
dump_int(hyph_size)
@y  23784
dump_int(hyph_prime)
@z

@x [50.1308] l.23793 - texarray
x:=fmt_file^.int;
if x<>@$ then goto bad_fmt; {check that strings are the same}
@y
@+Init
libc_free(font_info); libc_free(str_pool); libc_free(str_start);
libc_free(yhash); libc_free(zeqtb); libc_free(yzmem);
@+Tini
undump_int(x);
format_debug('string pool checksum')(x);
if x<>@$ then goto bad_fmt; {check that strings are the same}
undump_int(x);
if x<>max_halfword then goto bad_fmt; {check |max_halfword|}
undump_int(hash_high);
  if (hash_high<0)or(hash_high>sup_hash_extra) then goto bad_fmt;
  if hash_extra<hash_high then hash_extra:=hash_high;
  eqtb_top:=eqtb_size+hash_extra;
  if hash_extra=0 then hash_top:=undefined_control_sequence else
        hash_top:=eqtb_top;
  xmalloc_array(yhash,1+hash_top-hash_offset);
  hash:=yhash - hash_offset;
  next(hash_base):=0; text(hash_base):=0;
  for x:=hash_base+1 to hash_top do hash[x]:=hash[hash_base];
  xmalloc_array (zeqtb,eqtb_top+1);
  eqtb:=zeqtb;

  eq_type(undefined_control_sequence):=undefined_cs;
  equiv(undefined_control_sequence):=null;
  eq_level(undefined_control_sequence):=level_zero;
  for x:=eqtb_size+1 to eqtb_top do
    eqtb[x]:=eqtb[undefined_control_sequence];
@z

@x [50.1308] l.23795 - texarray
undump_int(x);
if x<>mem_bot then goto bad_fmt;
undump_int(x);
if x<>mem_top then goto bad_fmt;
@y
undump_int(x); format_debug ('mem_bot')(x);
if x<>mem_bot then goto bad_fmt;
undump_int(mem_top); format_debug ('mem_top')(mem_top);
if mem_bot+1100>mem_top then goto bad_fmt;


head:=contrib_head; tail:=contrib_head;
     page_tail:=page_head;  {page initialization}

mem_min := mem_bot - extra_mem_bot;
mem_max := mem_top + extra_mem_top;

xmalloc_array (yzmem, mem_max - mem_min);
zmem := yzmem - mem_min;   {this pointer arithmetic fails with some compilers}
mem := zmem;
@z

%%%%%%%% dynamic hyph_size
@x 23804 m.1308
if x<>hyph_size then goto bad_fmt
@y  23804
if x<>hyph_prime then goto bad_fmt
@z

% [1309] Make dumping/undumping more efficient by doing whole arrays at
% a time, via fread/fwrite in texmfmp.c.
@x [50.1309] l.23814 - Make dumping/undumping more efficient.
for k:=0 to str_ptr do dump_int(str_start[k]);
k:=0;
while k+4<pool_ptr do
  begin dump_four_ASCII; k:=k+4;
  end;
k:=pool_ptr-4; dump_four_ASCII;
@y
dump_things(str_start[0], str_ptr+1);
dump_things(str_pool[0], pool_ptr);
@z

@x [50.1310] l.23829 - Make dumping/undumping more efficient.
undump_size(0)(pool_size)('string pool size')(pool_ptr);
undump_size(0)(max_strings)('max strings')(str_ptr);
for k:=0 to str_ptr do undump(0)(pool_ptr)(str_start[k]);
k:=0;
while k+4<pool_ptr do
  begin undump_four_ASCII; k:=k+4;
  end;
k:=pool_ptr-4; undump_four_ASCII;
@y
undump_size(0)(sup_pool_size-pool_free)('string pool size')(pool_ptr);
if pool_size<pool_ptr+pool_free then
  pool_size:=pool_ptr+pool_free;
undump_size(0)(sup_max_strings)('sup strings')(str_ptr);@/
xmalloc_array(str_start, max_strings);
undump_checked_things(0, pool_ptr, str_start[0], str_ptr+1);@/
xmalloc_array(str_pool, pool_size);
undump_things(str_pool[0], pool_ptr);
@z

@x [50.1311] l.23850 - Make dumping/undumping more efficient.
repeat for k:=p to q+1 do dump_wd(mem[k]);
@y
repeat dump_things(mem[p], q+2-p);
@z

@x [50.1311] l.23855 - Make dumping/undumping more efficient.
for k:=p to lo_mem_max do dump_wd(mem[k]);
@y
dump_things(mem[p], lo_mem_max+1-p);
@z

@x [50.1311] l.23858 - Make dumping/undumping more efficient.
for k:=hi_mem_min to mem_end do dump_wd(mem[k]);
@y
dump_things(mem[hi_mem_min], mem_end+1-hi_mem_min);
@z

@x [50.1312] l.23873 - Make dumping/undumping more efficient.
repeat for k:=p to q+1 do undump_wd(mem[k]);
@y
repeat undump_things(mem[p], q+2-p);
@z

@x [50.1312] l.23878 - Make dumping/undumping more efficient.
for k:=p to lo_mem_max do undump_wd(mem[k]);
@y
undump_things(mem[p], lo_mem_max+1-p);
@z

@x [50.1312] l.23888 - Make dumping/undumping more efficient.
for k:=hi_mem_min to mem_end do undump_wd(mem[k]);
@y
undump_things (mem[hi_mem_min], mem_end+1-hi_mem_min);
@z

@x [50.1314] l.23899 - hash_extra
undump(hash_base)(frozen_control_sequence)(par_loc);
par_token:=cs_token_flag+par_loc;@/
undump(hash_base)(frozen_control_sequence)(write_loc);@/
@y
undump(hash_base)(hash_top)(par_loc);
par_token:=cs_token_flag+par_loc;@/
undump(hash_base)(hash_top)(write_loc);@/
@z

@x [50.1315] l.23925 - Make dumping/undumping more efficient - eqtb
while k<l do
  begin dump_wd(eqtb[k]); incr(k);
  end;
@y
dump_things(eqtb[k], l-k);
@z

@x [50.1316] l.23944 - Make dumping/undumping more efficient - eqtb
while k<l do
  begin dump_wd(eqtb[k]); incr(k);
  end;
@y
dump_things(eqtb[k], l-k);
@z

@x [50.1316] l.23947 - hash_extra
k:=j+1; dump_int(k-l);
until k>eqtb_size
@y
k:=j+1; dump_int(k-l);
until k>eqtb_size;
if hash_high>0 then dump_things(eqtb[eqtb_size+1],hash_high);
  {dump |hash_extra| part}
@z

@x [50.1317] l.23958 - Make dumping/undumping more efficient - eqtb
for j:=k to k+x-1 do undump_wd(eqtb[j]);
@y
undump_things(eqtb[k], x);
@z

@x [50.1317] l.23960 - hash_extra
until k>eqtb_size
@y
until k>eqtb_size;
if hash_high>0 then undump_things(eqtb[eqtb_size+1],hash_high);
  {undump |hash_extra| part}
@z

@x [50.1318] l.23968 - hash_extra
dump_int(hash_used); cs_count:=frozen_control_sequence-1-hash_used;
@y  23968
dump_int(hash_used); cs_count:=frozen_control_sequence-1-hash_used+hash_high;
@z

@x [50.1318] l.23972 - Make dumping/undumping more efficient, hash_extra
for p:=hash_used+1 to undefined_control_sequence-1 do dump_hh(hash[p]);
@y
dump_things(hash[hash_used+1], undefined_control_sequence-1-hash_used);
if hash_high>0 then dump_things(hash[eqtb_size+1], hash_high);
@z

@x [50.1319] l.23980 - Make dumping/undumping more efficient, hash_extra
for p:=hash_used+1 to undefined_control_sequence-1 do undump_hh(hash[p]);
@y
undump_things (hash[hash_used+1], undefined_control_sequence-1-hash_used);
if debug_format_file then begin
  print_csnames (hash_base, undefined_control_sequence - 1);
end;
if hash_high > 0 then begin
  undump_things (hash[eqtb_size+1], hash_high);
  if debug_format_file then begin
    print_csnames (eqtb_size + 1, hash_high - (eqtb_size + 1));
  end;
end;
@z

@x [50.1320] l.23985 - Make dumping/undumping more efficient - tfm
for k:=0 to fmem_ptr-1 do dump_wd(font_info[k]);
dump_int(font_ptr);
for k:=null_font to font_ptr do
  @<Dump the array info for internal font number |k|@>;
@y
dump_things(font_info[0], fmem_ptr);
dump_int(font_ptr);
@<Dump the array info for internal font number |k|@>;
@z

@x [50.1321] l.23994 - texarray
undump_size(7)(font_mem_size)('font mem size')(fmem_ptr);
for k:=0 to fmem_ptr-1 do undump_wd(font_info[k]);
undump_size(font_base)(font_max)('font max')(font_ptr);
for k:=null_font to font_ptr do
  @<Undump the array info for internal font number |k|@>
@y
undump_size(7)(sup_font_mem_size)('font mem size')(fmem_ptr);
if fmem_ptr>font_mem_size then font_mem_size:=fmem_ptr;
xmalloc_array(font_info, font_mem_size);
undump_things(font_info[0], fmem_ptr);@/
undump_size(font_base)(font_base+max_font_max)('font max')(font_ptr);
{This undumps all of the font info, despite the name.}
@<Undump the array info for internal font number |k|@>;
@z

% [50.1322] Dumping font_info.
% Knuth's code writes all the information relevant to a single font
% in the same section of the fmt file.  But it's a lot faster to
% write the arrays of information out, one whole array at a time.
% So that's the way we handle dumping and undumping font info.
@x [50.1322] l.24000 - Make dumping/undumping more efficient - tfm
@ @<Dump the array info for internal font number |k|@>=
begin dump_qqqq(font_check[k]);
dump_int(font_size[k]);
dump_int(font_dsize[k]);
dump_int(font_params[k]);@/
dump_int(hyphen_char[k]);
dump_int(skew_char[k]);@/
dump_int(font_name[k]);
dump_int(font_area[k]);@/
dump_int(font_bc[k]);
dump_int(font_ec[k]);@/
dump_int(char_base[k]);
dump_int(width_base[k]);
dump_int(height_base[k]);@/
dump_int(depth_base[k]);
dump_int(italic_base[k]);
dump_int(lig_kern_base[k]);@/
dump_int(kern_base[k]);
dump_int(exten_base[k]);
dump_int(param_base[k]);@/
dump_int(font_glue[k]);@/
dump_int(bchar_label[k]);
dump_int(font_bchar[k]);
dump_int(font_false_bchar[k]);@/
print_nl("\font"); print_esc(font_id_text(k)); print_char("=");
print_file_name(font_name[k],font_area[k],"");
if font_size[k]<>font_dsize[k] then
  begin print(" at "); print_scaled(font_size[k]); print("pt");
  end;
end
@y
@ @<Dump the array info for internal font number |k|@>=
begin
dump_things(font_check[null_font], font_ptr+1-null_font);
dump_things(font_size[null_font], font_ptr+1-null_font);
dump_things(font_dsize[null_font], font_ptr+1-null_font);
dump_things(font_params[null_font], font_ptr+1-null_font);
dump_things(hyphen_char[null_font], font_ptr+1-null_font);
dump_things(skew_char[null_font], font_ptr+1-null_font);
dump_things(font_name[null_font], font_ptr+1-null_font);
dump_things(font_area[null_font], font_ptr+1-null_font);
dump_things(font_bc[null_font], font_ptr+1-null_font);
dump_things(font_ec[null_font], font_ptr+1-null_font);
dump_things(char_base[null_font], font_ptr+1-null_font);
dump_things(width_base[null_font], font_ptr+1-null_font);
dump_things(height_base[null_font], font_ptr+1-null_font);
dump_things(depth_base[null_font], font_ptr+1-null_font);
dump_things(italic_base[null_font], font_ptr+1-null_font);
dump_things(lig_kern_base[null_font], font_ptr+1-null_font);
dump_things(kern_base[null_font], font_ptr+1-null_font);
dump_things(exten_base[null_font], font_ptr+1-null_font);
dump_things(param_base[null_font], font_ptr+1-null_font);
dump_things(font_glue[null_font], font_ptr+1-null_font);
dump_things(bchar_label[null_font], font_ptr+1-null_font);
dump_things(font_bchar[null_font], font_ptr+1-null_font);
dump_things(font_false_bchar[null_font], font_ptr+1-null_font);
for k:=null_font to font_ptr do
  begin print_nl("\font"); print_esc(font_id_text(k)); print_char("=");
  print_file_name(font_name[k],font_area[k],"");
  if font_size[k]<>font_dsize[k] then
    begin print(" at "); print_scaled(font_size[k]); print("pt");
    end;
  end;
end
@z

@x [50.1322] l.24031 - Make dumping/undumping more efficient - tfm
@ @<Undump the array info for internal font number |k|@>=
begin undump_qqqq(font_check[k]);@/
undump_int(font_size[k]);
undump_int(font_dsize[k]);
undump(min_halfword)(max_halfword)(font_params[k]);@/
undump_int(hyphen_char[k]);
undump_int(skew_char[k]);@/
undump(0)(str_ptr)(font_name[k]);
undump(0)(str_ptr)(font_area[k]);@/
undump(0)(255)(font_bc[k]);
undump(0)(255)(font_ec[k]);@/
undump_int(char_base[k]);
undump_int(width_base[k]);
undump_int(height_base[k]);@/
undump_int(depth_base[k]);
undump_int(italic_base[k]);
undump_int(lig_kern_base[k]);@/
undump_int(kern_base[k]);
undump_int(exten_base[k]);
undump_int(param_base[k]);@/
undump(min_halfword)(lo_mem_max)(font_glue[k]);@/
undump(0)(fmem_ptr-1)(bchar_label[k]);
undump(min_quarterword)(non_char)(font_bchar[k]);
undump(min_quarterword)(non_char)(font_false_bchar[k]);
end
@y
@ This module should now be named `Undump all the font arrays'.

@<Undump the array info for internal font number |k|@>=
begin {Allocate the font arrays}
xmalloc_array(font_check, font_max);
xmalloc_array(font_size, font_max);
xmalloc_array(font_dsize, font_max);
xmalloc_array(font_params, font_max);
xmalloc_array(font_name, font_max);
xmalloc_array(font_area, font_max);
xmalloc_array(font_bc, font_max);
xmalloc_array(font_ec, font_max);
xmalloc_array(font_glue, font_max);
xmalloc_array(hyphen_char, font_max);
xmalloc_array(skew_char, font_max);
xmalloc_array(bchar_label, font_max);
xmalloc_array(font_bchar, font_max);
xmalloc_array(font_false_bchar, font_max);
xmalloc_array(char_base, font_max);
xmalloc_array(width_base, font_max);
xmalloc_array(height_base, font_max);
xmalloc_array(depth_base, font_max);
xmalloc_array(italic_base, font_max);
xmalloc_array(lig_kern_base, font_max);
xmalloc_array(kern_base, font_max);
xmalloc_array(exten_base, font_max);
xmalloc_array(param_base, font_max);

undump_things(font_check[null_font], font_ptr+1-null_font);
undump_things(font_size[null_font], font_ptr+1-null_font);
undump_things(font_dsize[null_font], font_ptr+1-null_font);
undump_checked_things(min_halfword, max_halfword,
                      font_params[null_font], font_ptr+1-null_font);
undump_things(hyphen_char[null_font], font_ptr+1-null_font);
undump_things(skew_char[null_font], font_ptr+1-null_font);
undump_upper_check_things(str_ptr, font_name[null_font], font_ptr+1-null_font);
undump_upper_check_things(str_ptr, font_area[null_font], font_ptr+1-null_font);
{There's no point in checking these values against the range $[0,255]$,
 since the data type is |unsigned char|, and all values of that type are
 in that range by definition.}
undump_things(font_bc[null_font], font_ptr+1-null_font);
undump_things(font_ec[null_font], font_ptr+1-null_font);
undump_things(char_base[null_font], font_ptr+1-null_font);
undump_things(width_base[null_font], font_ptr+1-null_font);
undump_things(height_base[null_font], font_ptr+1-null_font);
undump_things(depth_base[null_font], font_ptr+1-null_font);
undump_things(italic_base[null_font], font_ptr+1-null_font);
undump_things(lig_kern_base[null_font], font_ptr+1-null_font);
undump_things(kern_base[null_font], font_ptr+1-null_font);
undump_things(exten_base[null_font], font_ptr+1-null_font);
undump_things(param_base[null_font], font_ptr+1-null_font);
undump_checked_things(min_halfword, lo_mem_max,
                     font_glue[null_font], font_ptr+1-null_font);
undump_checked_things(0, fmem_ptr-1,
                     bchar_label[null_font], font_ptr+1-null_font);
undump_checked_things(min_quarterword, non_char,
                     font_bchar[null_font], font_ptr+1-null_font);
undump_checked_things(min_quarterword, non_char,
                     font_false_bchar[null_font], font_ptr+1-null_font);
end
@z

%%%%%%%% dynamic hyph_size
@x 24058 m.1324
dump_int(hyph_count);
for k:=0 to hyph_size do if hyph_word[k]<>0 then
  begin dump_int(k); dump_int(hyph_word[k]); dump_int(hyph_list[k]);
  end;
@y  24061
dump_int(hyph_count);
if hyph_next <= hyph_prime then hyph_next:=hyph_size;
dump_int(hyph_next);{minumum value of |hyphen_size| needed}
for k:=0 to hyph_size do if hyph_word[k]<>0 then
  begin dump_int(k+65536*hyph_link[k]);
        {assumes number of hyphen exceptions does not exceed 65535}
   dump_int(hyph_word[k]); dump_int(hyph_list[k]);
  end;
@z

@x [50.1324] l.24066 - Make dumping/undumping more efficient - trie
for k:=0 to trie_max do dump_hh(trie[k]);
dump_int(trie_op_ptr);
for k:=1 to trie_op_ptr do
  begin dump_int(hyf_distance[k]);
  dump_int(hyf_num[k]);
  dump_int(hyf_next[k]);
  end;
@y
dump_things(trie_trl[0], trie_max+1);
dump_things(trie_tro[0], trie_max+1);
dump_things(trie_trc[0], trie_max+1);
dump_int(trie_op_ptr);
dump_things(hyf_distance[1], trie_op_ptr);
dump_things(hyf_num[1], trie_op_ptr);
dump_things(hyf_next[1], trie_op_ptr);
@z

@x [50.1325] l.24086 - Make dumping/undumping more efficient - trie
@<Undump the hyphenation tables@>=
@y
{This is only used for the hyphenation tries below, and the size is
 always |j+1|.}
@d xmalloc_and_undump(#) ==
  if not # then xmalloc_array(#, j+1);
  undump_things(#[0], j+1);

@<Undump the hyphenation tables@>=
@z

%%%%%%%% dynamic hyph_size
@x 24087 m.1325
undump(0)(hyph_size)(hyph_count);
for k:=1 to hyph_count do
  begin undump(0)(hyph_size)(j);
  undump(0)(str_ptr)(hyph_word[j]);
  undump(min_halfword)(max_halfword)(hyph_list[j]);
  end;
@y  24092
undump_size(0)(hyph_size)('hyph_size')(hyph_count);
undump_size(hyph_prime)(hyph_size)('hyph_size')(hyph_next);
j:=0;
for k:=1 to hyph_count do
  begin undump_int(j); if j<0 then goto bad_fmt;
   if j>65535 then
   begin hyph_next:= j div 65536; j:=j - hyph_next * 65536; end
       else hyph_next:=0;
   if (j>=hyph_size)or(hyph_next>hyph_size) then goto bad_fmt;
   hyph_link[j]:=hyph_next;
  undump(0)(str_ptr)(hyph_word[j]);
  undump(min_halfword)(max_halfword)(hyph_list[j]);
  end;
  {|j| is now the largest occupied location in |hyph_word|}
  incr(j);
  if j<hyph_prime then j:=hyph_prime;
  hyph_next:=j;
  if hyph_next >= hyph_size then hyph_next:=hyph_prime else
  if hyph_next >= hyph_prime then incr(hyph_next);
@z


@x [50.1325] l.24094 - Make dumping/undumping more efficient - trie
for k:=0 to j do undump_hh(trie[k]);
undump_size(0)(trie_op_size)('trie op size')(j); @+init trie_op_ptr:=j;@+tini
for k:=1 to j do
  begin undump(0)(63)(hyf_distance[k]); {a |small_number|}
  undump(0)(63)(hyf_num[k]);
  undump(min_quarterword)(max_quarterword)(hyf_next[k]);
  end;
@y
{These first three haven't been allocated yet unless we're \.{INITEX};
 we do that precisely so we don't allocate more space than necessary.}
xmalloc_and_undump(trie_trl);
xmalloc_and_undump(trie_tro);
xmalloc_and_undump(trie_trc);
undump_size(0)(trie_op_size)('trie op size')(j); @+init trie_op_ptr:=j;@+tini
{I'm not sure we have such a strict limitation (64) on these values, so
 let's leave them unchecked.}
undump_things(hyf_distance[1], j);
undump_things(hyf_num[1], j);
undump_upper_check_things(max_trie_op, hyf_next[1], j);
@z

@x [50.1327] l.24117 - Allow command line to override dumped value.
undump(batch_mode)(error_stop_mode)(interaction);
@y
undump(batch_mode)(error_stop_mode)(interaction);
if interaction_option<>unspecified_mode then interaction:=interaction_option;
@z

% [50.1327] As with TFM files, `eof' here means `have we previously
% encountered the end-of-file', not `are we at the end of the file'.
@x [50.1327] l.24120 - dumping/undumping, replace eof() by feof()
if (x<>69069)or eof(fmt_file) then goto bad_fmt
@y
if (x<>69069)or feof(fmt_file) then goto bad_fmt
@z

% Eliminate probably wrong word `preloaded' from format_idents.
% Use four-digit years.
@x [50.1328] l.24124 
print(" (preloaded format="); print(job_name); print_char(" ");
print_int(year mod 100); print_char(".");
@y
print(" (format="); print(job_name); print_char(" ");
print_int(year); print_char(".");
@z

@x [51.1332] l.24203 - make the main program a procedure, for eqtb hack.
@p begin @!{|start_here|}
@y
@d const_chk(#)==begin if # < inf@&# then # := inf@&# else
                         if # > sup@&# then # := sup@&# end

{|setup_bound_var| stuff duplicated in \.{mf.ch}.}
@d setup_bound_var(#)==bound_default:=#; setup_bound_var_end
@d setup_bound_var_end(#)==bound_name:=#; setup_bound_var_end_end
@d setup_bound_var_end_end(#)==
  setup_bound_variable(address_of(#), bound_name, bound_default);

@p procedure main_body;
begin @!{|start_here|}

{Bounds that may be set from the configuration file. We want the user to
 be able to specify the names with underscores, but \.{TANGLE} removes
 underscores, so we're stuck giving the names twice, once as a string,
 once as the identifier. How ugly.}
  setup_bound_var (250000)('main_memory')(main_memory);
    {|memory_word|s for |mem| in \.{INITEX}}
  setup_bound_var (0)('extra_mem_top')(extra_mem_top);
    {increase high mem in \.{VIRTEX}}
  setup_bound_var (0)('extra_mem_bot')(extra_mem_bot);
    {increase low mem in \.{VIRTEX}}
  setup_bound_var (100000)('pool_size')(pool_size);
  setup_bound_var (75000)('string_vacancies')(string_vacancies);
  setup_bound_var (5000)('pool_free')(pool_free); {min pool avail after fmt}
  setup_bound_var (15000)('max_strings')(max_strings);
  setup_bound_var (100000)('font_mem_size')(font_mem_size);
  setup_bound_var (500)('font_max')(font_max);
  setup_bound_var (20000)('trie_size')(trie_size);
    {if |ssup_trie_size| increases, recompile}
  setup_bound_var (659)('hyph_size')(hyph_size);
  setup_bound_var (3000)('buf_size')(buf_size);
  setup_bound_var (50)('nest_size')(nest_size);
  setup_bound_var (15)('max_in_open')(max_in_open);
  setup_bound_var (60)('param_size')(param_size);
  setup_bound_var (4000)('save_size')(save_size);
  setup_bound_var (300)('stack_size')(stack_size);
  setup_bound_var (16384)('dvi_buf_size')(dvi_buf_size);
  setup_bound_var (79)('error_line')(error_line);
  setup_bound_var (50)('half_error_line')(half_error_line);
  setup_bound_var (79)('max_print_line')(max_print_line);
  setup_bound_var (0)('hash_extra')(hash_extra);

  const_chk (main_memory);
@+Init
  extra_mem_top := 0;
  extra_mem_bot := 0;
@+Tini
  if extra_mem_bot>mem_bot then extra_mem_bot:=mem_bot;
  if extra_mem_bot>sup_main_memory then extra_mem_bot:=sup_main_memory;
  if extra_mem_top>sup_main_memory then extra_mem_top:=sup_main_memory;
  mem_top := mem_bot + main_memory;
  mem_min := mem_bot;
  mem_max := mem_top;

  {Check other constants against their sup and inf.}
  const_chk (trie_size);
  const_chk (hyph_size);
  const_chk (buf_size);
  const_chk (nest_size);
  const_chk (max_in_open);
  const_chk (param_size);
  const_chk (save_size);
  const_chk (stack_size);
  const_chk (dvi_buf_size);
  const_chk (pool_size);
  const_chk (string_vacancies);
  const_chk (pool_free);
  const_chk (max_strings);
  const_chk (font_mem_size);
  const_chk (font_max);
  const_chk (hash_extra);
  if error_line > ssup_error_line then error_line := ssup_error_line;

  {array memory allocation}
  xmalloc_array (buffer, buf_size);
  xmalloc_array (nest, nest_size);
  xmalloc_array (save_stack, save_size);
  xmalloc_array (input_stack, stack_size);
  xmalloc_array (input_file, max_in_open);
  xmalloc_array (line_stack, max_in_open);
  xmalloc_array (param_stack, param_size);
  xmalloc_array (dvi_buf, dvi_buf_size);
  xmalloc_array (hyph_word , hyph_size);
  xmalloc_array (hyph_list , hyph_size);
  xmalloc_array (hyph_link , hyph_size);
@+Init
  xmalloc_array (yzmem, mem_top - mem_bot);
  zmem := yzmem - mem_bot;   {Some compilers require |mem_bot=0|}
  eqtb_top := eqtb_size+hash_extra;
  if hash_extra=0 then hash_top:=undefined_control_sequence else
        hash_top:=eqtb_top;
  xmalloc_array (yhash,1+hash_top-hash_offset);
  hash:=yhash - hash_offset;   {Some compilers require |hash_offset=0|}
  next(hash_base):=0; text(hash_base):=0;
  for hash_used:=hash_base+1 to hash_top do hash[hash_used]:=hash[hash_base];
  xmalloc_array (zeqtb, eqtb_top);
  eqtb:=zeqtb;

  xmalloc_array (str_start, max_strings);
  xmalloc_array (str_pool, pool_size);
  xmalloc_array (font_info, font_mem_size);
@+Tini
@z

@x [51.1332] l.24215 - INI = VIR, so pool init needs runtime test
@!init if not get_strings_started then goto final_end;
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr; fix_date_and_time;
tini@/
@y
@!Init if not get_strings_started then goto final_end;
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr; fix_date_and_time;
Tini@/
@z

@x [51.1332] l.24225 - main
end_of_TEX: close_files_and_terminate;
final_end: ready_already:=0;
end.
@y
close_files_and_terminate;
final_end: do_final_end;
end {|main_body|};
@z

@x [51.1333] l.24254 - Print new line before termination; switch to editor if necessary.
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

@x [51.1334] l.24275 - hash_extra
  wlog_ln(' ',cs_count:1,' multiletter control sequences out of ',
    hash_size:1);@/
@y  24276
  wlog_ln(' ',cs_count:1,' multiletter control sequences out of ',
    hash_size:1, '+', hash_extra:1);@/
@z

@x [51.1335] l.24335 - Only do dump if ini.
  begin @!init for c:=top_mark_code to split_bot_mark_code do
@y
  begin @!Init for c:=top_mark_code to split_bot_mark_code do
@z

@x [51.1335] l.24337 - Only do dump if ini.
  store_fmt_file; return;@+tini@/
@y
  store_fmt_file; return;@+Tini@/
@z

@x [51.1337] l.24361 - Handle %&format in all cases.
if (format_ident=0)or(buffer[loc]="&") then
@y
if (format_ident=0)or(buffer[loc]="&")or dump_line then
@z

@x [51.1337] l.24366 - Dynamic arrays size.
  w_close(fmt_file);
@y
  w_close(fmt_file);
  eqtb:=zeqtb;
@z

%% [51] m.1337 l.24371 - MLTeX: add. MLTeX banner after loading fmt file
%%                     (MLTeX change: only "if mltex_enabled_p then ....;")
@x [51.1337] l.24371 - Allocate hyphenation tries, do char translation, MLTeX
fix_date_and_time;@/
@y
if mltex_enabled_p then
  begin wterm_ln('MLTeX v2.2 enabled');
  end;
fix_date_and_time;@/

ifdef ('notdef'); {TCX files are probably a bad idea.}
{If a character translation file changed anything $\ldots$
 Have to do this after initializing the string pool, since we're
 rearranging it.}
if chars_saved_by_charset > 0 then begin
  {Now for the fun part. We want users to be able to specify which
   characters are printable dynamically---generally, we can't assume
   that eight-bit characters are printable, but many character sets (ISO
   Latin 1, ISO Latin 2) do use them. In the original code, the first
   256 strings are generated once, at \.{INITEX}-time, using module
   49. Well, now we have to regenerate the first 256 strings at the
   beginning of every run (if we're not using the default character
   set).}

  {Save the variables we're going to mess with.}
  save_str_ptr := str_ptr;
  save_pool_ptr := pool_ptr;

  {\TeX\ assumes in many places that string |s| are those characters in
   |str_pool| which lie between |str_start[s]| and |str_start[s+1]|.

   So the characters in the string pool must remain consecutive; for
   example, the last character of string 256 (which is the
   representation of character \.{0xff}, which may be either a single
   byte, or the default four characters \.{^^ff}) must come right before
   the first character of string 257 (which happens to be the word
   |"buffer"|, from the buffer overflow a few sections previous).

   The dumped string pool always contains the same first 256 strings;
   it's not controllable by the user.  {\it And} (this is crucial), a
   character translation \.{.tcx} file can only {\it decrease} the
   number of characters used, never increase. This is a consequence of
   the fact that we do not allow \.{.tcx} files to specify certain
   characters are {\it un\/}printable; therefore, the only changes the
   user can make will be ones that change a representation from four
   characters (\.{^^ff}) to one (a single byte with all bits turned on).

   Conclusion: we can accomodate \TeX's assumptions by regenerating the
   first 256 strings, starting (in |str_pool|) at the precise index
   which is the number of characters the character set saved us, and by
   resetting |str_start[0]| to that position. (\TeX\ does not assume
   |str_start[0]=0|.) This way, we do not have to touch the bulk of the
   strings (some 24K), and reshuffle every |str_start|.

   Fun, huh?}
  str_ptr := 0;
  pool_ptr := chars_saved_by_charset; {computed by |setup_char_set|}
  str_start[0] := pool_ptr;
  @<Make the first 256 strings@>;

  {Restore the variables.}
  str_ptr := save_str_ptr;
  pool_ptr := save_pool_ptr;
end;
endif('notdef');

@!init
if trie_not_ready then begin {initex without format loaded}
  xmalloc_array (trie_trl, trie_size);
  xmalloc_array (trie_tro, trie_size);
  xmalloc_array (trie_trc, trie_size);

  xmalloc_array (trie_c, trie_size);
  xmalloc_array (trie_o, trie_size);
  xmalloc_array (trie_l, trie_size);
  xmalloc_array (trie_r, trie_size);
  xmalloc_array (trie_hash, trie_size);
  xmalloc_array (trie_taken, trie_size);

  trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;

  {Allocate and initialize font arrays}
  xmalloc_array(font_check, font_max);
  xmalloc_array(font_size, font_max);
  xmalloc_array(font_dsize, font_max);
  xmalloc_array(font_params, font_max);
  xmalloc_array(font_name, font_max);
  xmalloc_array(font_area, font_max);
  xmalloc_array(font_bc, font_max);
  xmalloc_array(font_ec, font_max);
  xmalloc_array(font_glue, font_max);
  xmalloc_array(hyphen_char, font_max);
  xmalloc_array(skew_char, font_max);
  xmalloc_array(bchar_label, font_max);
  xmalloc_array(font_bchar, font_max);
  xmalloc_array(font_false_bchar, font_max);
  xmalloc_array(char_base, font_max);
  xmalloc_array(width_base, font_max);
  xmalloc_array(height_base, font_max);
  xmalloc_array(depth_base, font_max);
  xmalloc_array(italic_base, font_max);
  xmalloc_array(lig_kern_base, font_max);
  xmalloc_array(kern_base, font_max);
  xmalloc_array(exten_base, font_max);
  xmalloc_array(param_base, font_max);

  font_ptr:=null_font; fmem_ptr:=7;
  font_name[null_font]:="nullfont"; font_area[null_font]:="";
  hyphen_char[null_font]:="-"; skew_char[null_font]:=-1;
  bchar_label[null_font]:=non_address;
  font_bchar[null_font]:=non_char; font_false_bchar[null_font]:=non_char;
  font_bc[null_font]:=1; font_ec[null_font]:=0;
  font_size[null_font]:=0; font_dsize[null_font]:=0;
  char_base[null_font]:=0; width_base[null_font]:=0;
  height_base[null_font]:=0; depth_base[null_font]:=0;
  italic_base[null_font]:=0; lig_kern_base[null_font]:=0;
  kern_base[null_font]:=0; exten_base[null_font]:=0;
  font_glue[null_font]:=null; font_params[null_font]:=7;
  param_base[null_font]:=-1;
  for font_k:=0 to 6 do font_info[font_k].sc:=0;
  end;
  tini@/

  xmalloc_array (font_used, font_max);
  for font_k:=font_base to font_max do font_used[font_k]:=false;
@z

% [52.1338] Core-dump in debugging mode on 0 input.  Under Unix, it's
% not possible to portably switch into the debugger while a program is
% running.  The best approximation is to do a core dump, then run the
% debugger on it later.
@x [52.1338] l.24411 - Core-dump in debugging mode on 0 input.
    begin goto breakpoint;@\ {go to every label at least once}
    breakpoint: m:=0; @{'BREAKPOINT'@}@\
    end
@y
    dump_core {do something to cause a core dump}
@z

@x [52.1339] l.24429 - debug - print tfm info
5: print_word(font_info[n]);
@y 24397
5: begin print_scaled(font_info[n].sc); print_char(" ");@/
  print_int(font_info[n].qqqq.b0); print_char(":");@/
  print_int(font_info[n].qqqq.b1); print_char(":");@/
  print_int(font_info[n].qqqq.b2); print_char(":");@/
  print_int(font_info[n].qqqq.b3);
  end;
@z

% [53.1350] (new_write_whatsit) Allow 18 as a \write stream. We never
% refer to an actual file, though, so we don't need to change the
% write_file or write_open arrays. We provide for disabling this at
% runtime, for paranoids.
@x [53.1350] l.24609 - system: Allow 18 as a \write stream.
  else if cur_val>15 then cur_val:=16;
@y
  else if (cur_val>15) and (cur_val <> 18) then cur_val:=16;
@z

@x [53.1370] l.24770 - system: (write_out) \write18{foo} => system(foo).
begin @<Expand macros in the token list
@y
@!d:integer; {number of characters in incomplete current string}
@!clobbered:boolean; {system string is ok?}
begin @<Expand macros in the token list
@z

@x [53.1370] l.24773 - system: (write_out) \write18{foo} => system(foo).
if write_open[j] then selector:=j
@y
if shell_enabled_p and (j=18) then
  begin selector := new_string;
  end
else if write_open[j] then selector:=j
@z

% Then call system(3) on that string.
@x [53.1370] l.24779 - system: (write_out) \write18{foo} => system(foo).
flush_list(def_ref); selector:=old_setting;
@y
flush_list(def_ref);
if j=18 then
  begin if (tracing_online<=0) then
    selector:=log_only  {Show what we're doing in the log file.}
  else selector:=term_and_log;  {Show what we're doing.}
  print_nl("system(");
  for d:=0 to cur_length-1 do
    begin {|print| gives up if passed |str_ptr|, so do it by hand.}
    print(so(str_pool[str_start[str_ptr]+d])); {N.B.: not |print_char|}
    end;
  print(")...");
  if shell_enabled_p then
    begin str_room(1); append_char(0); {Append a null byte to the expansion.}
    clobbered:=false;
    for d:=0 to cur_length-1 do {Convert to external character set.}
      begin str_pool[str_start[str_ptr]+d]:=xchr[str_pool[str_start[str_ptr]+d]];
      if (str_pool[str_start[str_ptr]+d]=null_code)
         and (d<cur_length-1) then clobbered:=true;
        {minimal checking: NUL not allowed in argument string of |system|()}
      end;
    if clobbered then print("clobbered")
    else begin {We have the string; run system(3). We don't have anything
            reasonable to do with the return status, unfortunately discard it.}
      system(address_of(str_pool[str_start[str_ptr]]));
      print("executed");
      end;
    pool_ptr:=str_start[str_ptr];  {erase the string}
    end
  else begin print("disabled");
  end;
  print_char("."); print_nl(""); print_ln;
end;
selector:=old_setting;
@z

@x [53.1373] Need new local.
procedure out_what(@!p:pointer);
var j:small_number; {write stream number}
@y
procedure out_what(@!p:pointer);
var j:small_number; {write stream number}
    @!old_setting:0..max_selector;
@z

@x [53.1374] Disallow certain \openout filenames, and log results.
      while not a_open_out(write_file[j]) do
        prompt_file_name("output file name",".tex");
      write_open[j]:=true;
@y
      while not open_out_name_ok(name_of_file+1)
            or not a_open_out(write_file[j]) do
        prompt_file_name("output file name",".tex");
      write_open[j]:=true;
      {If on first line of input, log file is not ready yet, so don't log.}
      if log_opened then begin
        old_setting:=selector;
        if (tracing_online<=0) then
          selector:=log_only  {Show what we're doing in the log file.}
        else selector:=term_and_log;  {Show what we're doing.}
        print_nl("\openout");
        print_int(j);
        print(" = `");
        print_file_name(cur_name,cur_area,cur_ext);
        print("'."); print_nl(""); print_ln;
        selector:=old_setting;
      end;
@z

%tcx-no @!is_printable: array[0..255] of ASCII_code; {use \.{\^\^} notation?}
@x [54.1376] l.24903 - Add editor-switch variables to globals.
@* \[54] System-dependent changes.
@y
@* \[54/web2c] System-dependent changes for Web2c.
Here are extra variables for Web2c.  (This numbering of the
system-dependent section allows easy integration of Web2c and e-\TeX, etc.)
@^<system dependencies@>

@<Glob...@>=
@!edit_name_start: pool_pointer; {where the filename to switch to starts}
@!edit_name_length,@!edit_line: integer; {what line to start editing at}
@!ipc_on: integer; {level of IPC action, 0 for none [default]}
@!chars_saved_by_charset: integer; {bytes of |str_pool| that will be
  unused with tcx files; only needed to declare the identifier, this is
  never used.}

@ The |edit_name_start| will be set to point into |str_pool| somewhere after
its beginning if \TeX\ is supposed to switch to an editor on exit.

@<Set init...@>=
edit_name_start:=0;

@ These are used when we regenerate the representation of the first 256
strings.

@<Global...@> =
@!save_str_ptr: str_number;
@!save_pool_ptr: pool_pointer;
@!shell_enabled_p: boolean;
@!output_comment: ^char;
@!k,l: 0..255; {used by `Make the first 256 strings', etc.}

@ When debugging a macro package, it can be useful to see the exact
control sequence names in the format file.  For example, if ten new
csnames appear, it's nice to know what they are, to help pinpoint where
they came from.  (This isn't a truly ``basic'' printing procedure, but
that's a convenient module in which to put it.)

@<Basic printing procedures@> =
procedure print_csnames (hstart:integer; hfinish:integer);
var c,h,where:integer;
begin
  write_ln (stderr, 'fmtdebug:csnames from ', hstart, ' to ', hfinish, ':');
  for h := hstart to hfinish do begin
    if text (h) > 0 then begin {if have anything at this position}
      where := h;
      repeat
        for c := str_start[text (where)] to str_start[text (where) + 1] - 1
        do begin
          put_byte (str_pool[c], stderr); {print the characters}
        end;
        write_ln (stderr, '');
        where := next (where);
      until where = 0;
    end;
  end;
end;

@ Are we printing extra info as we read the format file?

@<Glob...@> =
@!debug_format_file: boolean;


@* \[54/web2c-string] The string recycling routines.  \TeX{} uses 2
upto 4 {\it new\/} strings when scanning a filename in an \.{\\input},
\.{\\openin}, or \.{\\openout} operation.  These strings are normally
lost because the reference to them are not saved after finishing the
operation.  |search_string| searches through the string pool for the
given string and returns either 0 or the found string number.

@<Declare additional routines for string recycling@>=
function search_string(@!search:str_number):str_number;
label found;
var result: str_number;
@!s: str_number; {running index}
@!len: integer; {length of searched string}
begin result:=0; len:=length(search);
if len=0 then  {trivial case}
  begin result:=""; goto found;
  end
else  begin s:=search-1;  {start search with newest string below |s|; |search>1|!}
  while s>255 do  {first 256 strings depend on implementation!!}
    begin if length(s)=len then
      if str_eq_str(s,search) then
        begin result:=s; goto found;
        end;
    decr(s);
    end;
  end;
found:search_string:=result;
end;

@ The following routine is a variant of |make_string|.  It searches
the whole string pool for a string equal to the string currently built
and returns a found string.  Otherwise a new string is created and
returned.  Be cautious, you can not apply |flush_string| to a replaced
string!

@<Declare additional routines for string recycling@>=
function slow_make_string : str_number;
label exit;
var s: str_number; {result of |search_string|}
@!t: str_number; {new string}
begin t:=make_string; s:=search_string(t);
if s>0 then
  begin flush_string; slow_make_string:=s; return;
  end;
slow_make_string:=t;
exit:end;


@* \[54/ML\TeX] System-dependent changes for ML\TeX.

The boolean variable |mltex_p| is set by web2c according to the given
command line option (or an entry in the configuration file) before any
\TeX{} function is called.

@<Global...@> =
@!mltex_p: boolean;

@ The boolean variable |mltex_enabled_p| is used to enable ML\TeX's
character substitution.  It is initialised to |false|.  When loading
a \.{FMT} it is set to the value of the boolean |mltex_p| saved in
the \.{FMT} file.  Additionally it is set to the value of |mltex_p|
in Ini\TeX.

@<Glob...@>=
@!mltex_enabled_p:boolean;  {enable character substitution}


@ @<Set init...@>=
mltex_enabled_p:=false;


@ The function |effective_char| computes the effective character with
respect to font information.  The effective character is either the
base character part of a character substitution definition, if the
character does not exist in the font or the character itself.

Inside |effective_char| we can not use |char_info| because the macro
|char_info| uses |effective_char| calling this function a second time
with the same arguments.

If neither the character |c| exists in font |f| nor a character
substitution for |c| was defined, you can not use the function value
as a character offset in |char_info| because it will access an
undefined or invalid |font_info| entry!  Therefore inside |char_info|
and in other places, |effective_char|'s boolean parameter |err_p| is
set to |true| to issue a warning and return the incorrect
replacement, but always existing character |font_bc[f]|.
@^inner loop@>

@<Declare additional functions for ML\TeX@>=
function effective_char(@!err_p:boolean;
                        @!f:internal_font_number;@!c:quarterword):integer;
label found;
var base_c: integer; {or |eightbits|: replacement base character}
@!result: integer; {or |quarterword|}
begin result:=c;  {return |c| unless it does not exist in the font}
if not mltex_enabled_p then goto found;
if font_ec[f]>=qo(c) then if font_bc[f]<=qo(c) then
  if char_exists(orig_char_info(f)(c)) then  {N.B.: not |char_info|(f)(c)}
    goto found;
if qo(c)>=char_sub_def_min then if qo(c)<=char_sub_def_max then
  if char_list_exists(qo(c)) then
    begin base_c:=char_list_char(qo(c));
    result:=qi(base_c);  {return |base_c|}
    if not err_p then goto found;
    if font_ec[f]>=base_c then if font_bc[f]<=base_c then
      if char_exists(orig_char_info(f)(qi(base_c))) then goto found;
    end;
if err_p then  {print error and return existing character?}
  begin begin_diagnostic;
  print_nl("Missing character: There is no "); print("substitution for ");
@.Missing character@>
  print_ASCII(qo(c)); print(" in font ");
  slow_print(font_name[f]); print_char("!"); end_diagnostic(false);
  result:=qi(font_bc[f]); {N.B.: not non-existing character |c|!}
  end;
found: effective_char:=result;
end;


@ The function |effective_char_info| is equivalent to |char_info|,
except it will return |null_character| if neither the character |c|
exists in font |f| nor is there a substitution definition for |c|.
(For these cases |char_info| using |effective_char| will access an
undefined or invalid |font_info| entry.  See the documentation of
|effective_char| for more information.)
@^inner loop@>

@<Declare additional functions for ML\TeX@>=
function effective_char_info(@!f:internal_font_number;
                             @!c:quarterword):four_quarters;
label exit;
var ci:four_quarters; {character information bytes for |c|}
@!base_c:integer; {or |eightbits|: replacement base character}
begin if not mltex_enabled_p then
  begin effective_char_info:=orig_char_info(f)(c); return;
  end;
if font_ec[f]>=qo(c) then if font_bc[f]<=qo(c) then
  begin ci:=orig_char_info(f)(c);  {N.B.: not |char_info|(f)(c)}
  if char_exists(ci) then
    begin effective_char_info:=ci; return;
    end;
  end;
if qo(c)>=char_sub_def_min then if qo(c)<=char_sub_def_max then
  if char_list_exists(qo(c)) then
    begin {|effective_char_info:=char_info(f)(qi(char_list_char(qo(c))));|}
    base_c:=char_list_char(qo(c));
    if font_ec[f]>=base_c then if font_bc[f]<=base_c then
      begin ci:=orig_char_info(f)(qi(base_c));  {N.B.: not |char_info|(f)(c)}
      if char_exists(ci) then
        begin effective_char_info:=ci; return;
        end;
      end;
    end;
effective_char_info:=null_character;
exit:end;


@ This code is called for a virtual character |c| in |hlist_out|
during |ship_out|.  It tries to built a character substitution
construct for |c| generating appropriate \.{DVI} code using the
character substitution definition for this character.  If a valid
character substitution exists \.{DVI} code is created as if
|make_accent| was used.  In all other cases the status of the
substituion for this character has been changed between the creation
of the character node in the hlist and the output of the page---the
created \.{DVI} code will be correct but the visual result will be
undefined.

Former ML\TeX\ versions have replaced the character node by a
sequence of character, box, and accent kern nodes splicing them into
the original horizontal list.  This version does not do this to avoid
a)~a memory overflow at this processing stage, b)~additional code to
add a pointer to the previous node needed for the replacement, and
c)~to avoid wrong code resulting in anomalies because of the use
within a \.{\\leaders} box.

@<Output a substitution, |goto continue| if not possible@>=
  begin
  @<Get substitution information, check it, goto |found|
  if all is ok, otherwise goto |continue|@>;
found: @<Print character substition tracing log@>;
  @<Rebuild character using substitution information@>;
  end


@ The global variables for the code to substitute a virtual character
can be declared as local.  Nonetheless we declare them as global to
avoid stack overflows because |hlist_out| can be called recursivly.

@<Glob...@>=
@!accent_c,@!base_c,@!replace_c:integer;
@!ia_c,@!ib_c:four_quarters; {accent and base character information}
@!base_slant,@!accent_slant:real; {amount of slant}
@!base_x_height:scaled; {accent is designed for characters of this height}
@!base_width,@!base_height:scaled; {height and width for base character}
@!accent_width,@!accent_height:scaled; {height and width for accent}
@!delta:scaled; {amount of right shift}


@ Get the character substitution information in |char_sub_code| for
the character |c|.  The current code checks that the substition
exists and is valid and all substitution characters exist in the
font, so we can {\it not\/} substitute a character used in a
substitution.  This simplifies the code because we have not to check
for cycles in all character substitution definitions.

@<Get substitution information, check it...@>=
  if qo(c)>=char_sub_def_min then if qo(c)<=char_sub_def_max then
    if char_list_exists(qo(c)) then
      begin  base_c:=char_list_char(qo(c));
      accent_c:=char_list_accent(qo(c));
      if (font_ec[f]>=base_c) then if (font_bc[f]<=base_c) then
        if (font_ec[f]>=accent_c) then if (font_bc[f]<=accent_c) then
          begin ia_c:=char_info(f)(qi(accent_c));
          ib_c:=char_info(f)(qi(base_c));
          if char_exists(ib_c) then
            if char_exists(ia_c) then goto found;
          end;
      begin_diagnostic;
      print_nl("Missing character: Incomplete substitution ");
@.Missing character@>
      print_ASCII(qo(c)); print(" = "); print_ASCII(accent_c);
      print(" "); print_ASCII(base_c); print(" in font ");
      slow_print(font_name[f]); print_char("!"); end_diagnostic(false);
      goto continue;
      end;
  begin_diagnostic;
  print_nl("Missing character: There is no "); print("substitution for ");
@.Missing character@>
  print_ASCII(qo(c)); print(" in font ");
  slow_print(font_name[f]); print_char("!"); end_diagnostic(false);
  goto continue


@ For |tracinglostchars>99| the substitution is shown in the log file.

@<Print character substition tracing log@>=
 if tracing_lost_chars>99 then
   begin begin_diagnostic;
   print_nl("Using character substitution: ");
   print_ASCII(qo(c)); print(" = ");
   print_ASCII(accent_c); print(" "); print_ASCII(base_c);
   print(" in font "); slow_print(font_name[f]); print_char(".");
   end_diagnostic(false);
   end


@ This outputs the accent and the base character given in the
substitution.  It uses code virtually identical to the |make_accent|
procedure, but without the node creation steps.
 
Additionally if the accent character has to be shifted vertically it
does {\it not\/} create the same code.  The original routine in
|make_accent| and former versions of ML\TeX{} creates a box node
resulting in |push| and |pop| operations, whereas this code simply
produces vertical positioning operations.  This can influence the
pixel rounding algorithm in some \.{DVI} drivers---and therefore will
probably be changed in one of the next ML\TeX{} versions.

@<Rebuild character using substitution information@>=
  base_x_height:=x_height(f);
  base_slant:=slant(f)/float_constant(65536);
@^real division@>
  accent_slant:=base_slant; {slant of accent character font}
  base_width:=char_width(f)(ib_c);
  base_height:=char_height(f)(height_depth(ib_c));
  accent_width:=char_width(f)(ia_c);
  accent_height:=char_height(f)(height_depth(ia_c));
  @/{compute necessary horizontal shift (don't forget slant)}@/
  delta:=round((base_width-accent_width)/float_constant(2)+
            base_height*base_slant-base_x_height*accent_slant);
@^real multiplication@>
@^real addition@>
  dvi_h:=cur_h;  {update |dvi_h|, similar to the last statement in module 620}
  @/{1. For centering/horizontal shifting insert a kern node.}@/
  cur_h:=cur_h+delta; synch_h;
  @/{2. Then insert the accent character possibly shifted up or down.}@/
  if ((base_height<>base_x_height) and (accent_height>0)) then
    begin {the accent must be shifted up or down}
    cur_v:=base_line+(base_x_height-base_height); synch_v;
    if accent_c>=128 then dvi_out(set1);
    dvi_out(accent_c);@/
    cur_v:=base_line;
    end
  else begin synch_v;
    if accent_c>=128 then dvi_out(set1);
    dvi_out(accent_c);@/
    end;
  cur_h:=cur_h+accent_width; dvi_h:=cur_h;
  @/{3. For centering/horizontal shifting insert another kern node.}@/
  cur_h:=cur_h+(-accent_width-delta);
  @/{4. Output the base character.}@/
  synch_h; synch_v;
  if base_c>=128 then dvi_out(set1);
  dvi_out(base_c);@/
  cur_h:=cur_h+base_width;
  dvi_h:=cur_h {update of |dvi_h| is unnecessary, will be set in module 620}


@* \[54] System-dependent changes.
@z
