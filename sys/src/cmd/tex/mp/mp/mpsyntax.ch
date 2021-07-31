% MetaPost change file for syntax checking and static analysis 
%
% The local PASCAL compiler could never create a useful version of MetaPost
% but its error recovery and static analysis are far superior to that in
% web2c.  This change file should make it compile except for a few repeated
% case labels.  (This is because |othercases| becomes |29:|).

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
% [1.4]  Put files in the program heading
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
program MP; {all file names are defined dynamically}
@y
program MP(term_in,term_out,tfm_file,tfm_infile,mem_file,log_file,ps_file,
    output init ,pool_file tini);
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
@d debug== {change this to `$\\{debug}\equiv\null$' when debugging}
@d gubed== {change this to `$\\{gubed}\equiv\null$' when debugging}
@f debug==begin
@f gubed==end
@#
@d stat== {change this to `$\\{stat}\equiv\null$' when gathering
  usage statistics}
@d tats== {change this to `$\\{tats}\equiv\null$' when gathering
  usage statistics}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.8] init..tini
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d init== {change this to `$\\{init}\equiv\.{@@\{}$' in the production version}
@d tini== {change this to `$\\{tini}\equiv\.{@@\}}$' in the production version}
@y
@d init== {change this to `$\\{init}\equiv\.{@@\{}$' in the production version}
@d tini== {change this to `$\\{tini}\equiv\.{@@\}}$' in the production version}
@z

% ADD-OTHERCASES-CHANGES-HERE	%%% SENTINEL LINE -- DO NOT REMOVE %%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.11] compile-time constants, use logical names
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!mem_max=30000; {greatest index in \MP's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMP}, otherwise |>=mem_top|}
@y
@!mem_max=7000; {greatest index in \MP's internal |mem| array;
  must be strictly less than |max_halfword|;
  must be equal to |mem_top| in \.{INIMP}, otherwise |>=mem_top|}
@z
@x
@!font_mem_size=10000; {number of words for \.{TFM} information for text fonts}
@y
@!font_mem_size=7000; {number of words for \.{TFM} information for text fonts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1.12] sensitive compile-time constants
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d mem_top==30000 {largest index in the |mem| array dumped by \.{INIMP};
  must be substantially larger than |mem_min|
  and not greater than |mem_max|}
@y
@d mem_top==7000 {largest index in the |mem| array dumped by \.{INIMP};
  must be substantially larger than |mem_min|
  and not greater than |mem_max|}
@d others==29 {eliminate error messages when |othercases| is not implemented}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3.26] file opening
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
\MP's file-opening procedures return |false| if no file identified by
|name_of_file| could be opened.

@d reset_OK(#)==true
@d rewrite_OK(#)==true

@p function a_open_in(var @!f:alpha_file):boolean;
  {open a text file for input}
begin reset(f,name_of_file); a_open_in:=reset_OK(f);
end;
@#
function a_open_out(var @!f:alpha_file):boolean;
  {open a text file for output}
begin rewrite(f,name_of_file); a_open_out:=rewrite_OK(f);
end;
@#
function b_open_in(var @!f:byte_file):boolean;
  {open a binary file for input}
begin rewrite(f,name_of_file); b_open_in:=rewrite_OK(f);
end;
@#
function b_open_out(var @!f:byte_file):boolean;
  {open a binary file for output}
begin rewrite(f,name_of_file); b_open_out:=rewrite_OK(f);
end;
@#
function w_open_in(var @!f:word_file):boolean;
  {open a word file for input}
begin reset(f,name_of_file); w_open_in:=reset_OK(f);
end;
@#
function w_open_out(var @!f:word_file):boolean;
  {open a word file for output}
begin rewrite(f,name_of_file); w_open_out:=rewrite_OK(f);
end;
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
@ There is no standard way to close a file in \.{PASCAL}

@d a_close(#)==do_nothing {close a text file}
@d b_close(#)==do_nothing {close a binary file}
@d w_close(#)==do_nothing {close a word file}
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
@ There is no standard way to open the terminal files
@^system dependencies@>

@d t_open_in==do_nothing {assign stdin as terminal input}
@d t_open_out==do_nothing {assign stdout as terminal output}
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
these operations are ignored when checking syntax:
@^system dependencies@>

@d update_terminal == do_nothing {empty the terminal output buffer}
@d clear_terminal == do_nothing {clear the terminal input buffer}
@d wake_up_terminal == do_nothing {cancel the user's cancellation of output}
@z
