% Change file for BibTeX in C, originally by Howard Trickey, for
% Berkeley Unix.
% 
% History:
% 05/28/84      Initial implementation, version 0.41 of BibTeX
% 07/01/84      Version 0.41a of BibTeX.
% 12/17/84      Version 0.97c of BibTeX.
% 02/12/85      Version 0.98c of BibTeX.
% 02/25/85      Newer version 0.98c of BibTeX.
% 03/25/85      Version 0.98f of BibTeX
% 05/23/85      Version 0.98i of BibTeX
% 02/11/88      Version 0.99b of BibTeX
% 04/04/88      Version 0.99c; converted for use with web2c (ETM).
% 11/30/89      Use FILENAMESIZE instead of 1024 (KB).
% 03/09/90	`int' is a bad variable name for some cc's.
% (more recent changes in ../ChangeLog.W2C)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] banner
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d banner=='This is BibTeX, Version 0.99c' {printed when the program starts}
@y
@d banner=='This is BibTeX, C Version 0.99c' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [2] `term_in' and `term_out' are standard input and output.  But
% there is a complication: BibTeX passes `term_out' to some routines as
% a var parameter.  web2c turns a var parameter f into &f at the calling
% side -- and stdout is sometimes implemented as `&_iob[1]' or some
% such.  An address of an address is invalid.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d term_out == tty
@d term_in == tty
@y
@d term_out == standard_output
@d term_in == standard_input
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] Turn debug..gubed et al. into #ifdef's.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x 
@d debug == @{          { remove the `|@{|' when debugging }
@d gubed == @t@>@}      { remove the `|@}|' when debugging }
@f debug == begin
@f gubed == end
@#
@d stat == @{           { remove the `|@{|' when keeping statistics }
@d tats == @t@>@}       { remove the `|@}|' when keeping statistics }
@f stat == begin
@f tats == end
@#
@d trace == @{          { remove the `|@{|' when in |trace| mode }
@d ecart == @t@>@}      { remove the `|@}|' when in |trace| mode }
@f trace == begin
@f ecart == end
@y
@d debug == ifdef('DEBUG')
@d gubed == endif('DEBUG')
@f debug == begin
@f gubed == end
@#
@d stat == ifdef('STAT')
@d tats == endif('STAT')
@f stat==begin
@f tats==end
@#
@d trace == ifdef@&('TRACE')
@d ecart == endif@&('TRACE')
@f trace == begin
@f ecart == end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [10] Possibly exit with bad status.  It doesn't seem worth it to move
% the definitions of the |history| values to above this module; hence
% the 1.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
exit_program:
end.
@y
exit_program:
if (history > 1) then uexit (history);
end.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [11] Remove compiler directives.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@{@&$C-,A+,D-@}  {no range check, catch arithmetic overflow, no debug overhead}
@!debug @{@&$C+,D+@}@+ gubed            {but turn everything on when debugging}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [13] Remove nonlocal goto.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
    goto exit_program;
@y
    uexit (1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [14] Increase some constants.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@!buf_size=1000; {maximum number of characters in an input line (or string)}
@y
@!buf_size=3000; {maximum number of characters in an input line (or string)}
@z
@x
@!pool_size=65000; {maximum number of characters in strings}
@y
@!pool_size=512000; {maximum number of characters in strings}
@z
@x
@!max_strings=4000; {maximum number of strings, including pre-defined;
                                                        must be |<=hash_size|}
@y
@!max_strings=20000; {maximum number of strings, including pre-defined;
                                                        must be |<=hash_size|}
@z
@x
@!max_cites=750; {maximum number of distinct cite keys; must be
                                                        |<=max_strings|}
@y
@!max_cites=3000; {maximum number of distinct cite keys; must be
                                                        |<=max_strings|}
@z
@x
@!min_crossrefs=2; {minimum number of cross-refs required for automatic
                                                        |cite_list| inclusion}
@y
@!min_crossrefs=2000; {minimum number of cross-refs required for automatic
                                                        |cite_list| inclusion}
@z

@x
@!max_ent_ints=3000; {maximum number of |int_entry_var|s
                                        (entries $\times$ |int_entry_var|s)}
@y
@!max_ent_ints=25000; {maximum number of |int_entry_var|s
                                        (entries $\times$ |int_entry_var|s)}
@z
@x
@!max_ent_strs=3000; {maximum number of |str_entry_var|s
                                        (entries $\times$ |str_entry_var|s)}
@y
@!max_ent_strs=10000; {maximum number of |str_entry_var|s
                                        (entries $\times$ |str_entry_var|s)}
@z
@x
@!max_fields=17250; {maximum number of fields (entries $\times$ fields,
                                        about |23*max_cites| for consistency)}
@y
@!max_fields=69000; {maximum number of fields (entries $\times$ fields,
                                        about |23*max_cites| for consistency)}
@z
@x
@d hash_size=5000       {must be |>= max_strings| and |>= hash_prime|}
@y
@d hash_size=21000       {must be |>= max_strings| and |>= hash_prime|}
@z
@x
@d hash_prime=4253      {a prime number about 85\% of |hash_size| and |>= 128|
                                                and |< @t$2^{14}-2^6$@>|}
@y
@d hash_prime=16319      {a prime number about 85\% of |hash_size| and |>= 128|
                                                and |< @t$2^{14}-2^6$@>|}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [15] Use the system constant.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d file_name_size=40    {file names shouldn't be longer than this}
@y
@d file_name_size == PATH_MAX
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [36] Define `alpha_file' in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@!alpha_file=packed file of text_char;  {files that contain textual data}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [37] Can't do arithmetic with |file_name_size| any longer, sigh.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@!name_ptr:0..file_name_size+1;         {index variable into |name_of_file|}
@y
@!name_ptr:integer;         {index variable into |name_of_file|}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38] File opening.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
The \ph\ compiler with which the present version of \TeX\ was prepared has
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
\TeX\ to undertake appropriate corrective action.

\TeX's file-opening procedures return |false| if no file identified by
|name_of_file| could be opened.

@d reset_OK(#)==erstat(#)=0
@d rewrite_OK(#)==erstat(#)=0

@<Procedures and functions for file-system interacting@>=
function erstat(var f:file):integer; extern;    {in the runtime library}
@#@t\2@>
function a_open_in(var f:alpha_file):boolean;   {open a text file for input}
begin reset(f,name_of_file,'/O'); a_open_in:=reset_OK(f);
end;
@#
function a_open_out(var f:alpha_file):boolean;  {open a text file for output}
begin rewrite(f,name_of_file,'/O'); a_open_out:=rewrite_OK(f);
end;
@y
@ File opening will be done in C.
@d no_file_path = -1
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [39] Do file closing in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@<Procedures and functions for file-system interacting@>=
procedure a_close(var f:alpha_file);            {close a text file}
begin close(f);
end;
@y
File closing will be done in C, too.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [47] web2c doesn't understand f^.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
    buffer[last]:=xord[f^];
    get(f); incr(last);
    end;
  get(f);
@y
    buffer[last] := xord[getc (f)];
    incr (last);
    end;
  vgetc (f); {skip the eol}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [77] The predefined string array starts at zero instead of one.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
for i:=1 to len do
    buffer[i] := xord[pds[i]];
@y
for i:=1 to len do
    buffer[i] := xord[pds[i-1]];
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [97] Can't do this tangle-time arithmetic.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@!aux_name_length : 0..file_name_size+1;        {\.{.aux} name sans extension}
@y
@!aux_name_length : integer;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [100] Reading the aux file name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
This procedure consists of a loop that reads and processes a (nonnull)
\.{.aux} file name.  It's this module and the next two that must be
changed on those systems using command-line arguments.  Note: The
|term_out| and |term_in| files are system dependent.

@<Procedures and functions for the reading and processing of input files@>=
procedure get_the_top_level_aux_file_name;
label aux_found,@!aux_not_found;
var @<Variables for possible command-line processing@>@/
begin
check_cmnd_line := false;                       {many systems will change this}
loop
    begin
    if (check_cmnd_line) then
        @<Process a possible command line@>
      else
        begin
        write (term_out,'Please type input file name (no extension)--');
        if (eoln(term_in)) then                 {so the first |read| works}
            read_ln (term_in);
        aux_name_length := 0;
        while (not eoln(term_in)) do
            begin
            if (aux_name_length = file_name_size) then
                begin
                while (not eoln(term_in)) do    {discard the rest of the line}
                    get(term_in);
                sam_you_made_the_file_name_too_long;
                end;
            incr(aux_name_length);
            name_of_file[aux_name_length] := term_in^;
            get(term_in);
            end;
        end;
    @<Handle this \.{.aux} name@>;
aux_not_found:
    check_cmnd_line := false;
    end;
aux_found:                      {now we're ready to read the \.{.aux} file}
end;
@y
@<Procedures and functions for the reading and processing of input files@>=
procedure get_the_top_level_aux_file_name;
label aux_found,@!aux_not_found;
begin
loop
    begin
    {initialize the path variables}
    set_paths (BIB_INPUT_PATH_BIT + BST_INPUT_PATH_BIT);
    if (argc > 1) then
        @<Process a possible command line@>
      else begin
        write (term_out,'Please type input file name (no extension)--');
        aux_name_length := 0;
        while (not eoln(term_in)) do begin
            if (aux_name_length = file_name_size) then begin
                readln(term_in);
                sam_you_made_the_file_name_too_long;
            end;
            name_of_file[aux_name_length+1] := getc(term_in);
            incr(aux_name_length);
        end;
        if (eof(term_in)) then begin
            writeln(term_out);
            writeln(term_out,'Unexpected end of file on terminal---giving up!');
            uexit(1);
        end;
        readln(term_in);
      end;
    @<Handle this \.{.aux} name@>;
aux_not_found:
    argc := 0;
    end;
aux_found:                      {now we're ready to read the \.{.aux} file}
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [101] Don't need this variable; we use argc to check if we have a
% command line.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@<Variables for possible command-line processing@>=
@!check_cmnd_line : boolean;    {|true| if we're to check the command line}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [102] Get the aux file name from the command line.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@<Process a possible command line@>=
begin
do_nothing;             {the ``default system'' doesn't use the command line}
end
@y
@<Process a possible command line@>=
begin
argv (1, name_of_file);
aux_name_length := 1;
while name_of_file[aux_name_length] <> ' '
  do incr (aux_name_length);
decr (aux_name_length);
end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [106] Don't use a path to find the aux file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
if (not a_open_in(cur_aux_file)) then
    sam_you_made_the_file_name_wrong;
@y
if (not a_open_in(cur_aux_file,no_file_path)) then
    sam_you_made_the_file_name_wrong;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [123] Use BIBINPUTS to search for the .bib file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
if (not a_open_in(cur_bib_file)) then
    begin
    add_area (s_bib_area);
    if (not a_open_in(cur_bib_file)) then
        open_bibdata_aux_err ('I couldn''t open database file ');
    end;
@y
if (not a_open_in(cur_bib_file,BIB_INPUT_PATH)) then
        open_bibdata_aux_err ('I couldn''t open database file ');
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [127] Use BSTINPUTS/TEXINPUTS to search for .bst files.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
add_extension (s_bst_extension);
if (not a_open_in(bst_file)) then
    begin
    add_area (s_bst_area);
    if (not a_open_in(bst_file)) then
        begin
        print ('I couldn''t open style file ');
        print_bst_name;@/
        bst_str := 0;                           {mark as unused again}
        aux_err_return;
        end;
    end;
@y
add_extension (s_bst_extension);
if (not a_open_in(bst_file,BST_INPUT_PATH)) then
    begin
        print ('I couldn''t open style file ');
        print_bst_name;@/
        bst_str := 0;                           {mark as unused again}
        aux_err_return;
    end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [141] Don't use a path to search for subsidiary aux files, either.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
if (not a_open_in(cur_aux_file)) then
@y
if (not a_open_in(cur_aux_file, no_file_path)) then
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [151] This goto gets turned into a setjmp/longjmp by ./convert --
% unfortunately, it is a nonlocal goto.  ekrell@ulysses@att.com
% implemented the conversion.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
buf_ptr2 := last;       {to get the first input line}
loop
    begin
    if (not eat_bst_white_space) then   {the end of the \.{.bst} file}
        goto bst_done;
    get_bst_command_and_process;
    end;
bst_done: a_close (bst_file);
@y
buf_ptr2 := last;       {to get the first input line}
hack1;
    begin
    if (not eat_bst_white_space) then   {the end of the \.{.bst} file}
        hack2;
    get_bst_command_and_process;
    end;
bst_done: a_close (bst_file);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [198] Broken cc's can't handle a variable named `int'.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
This procedure takes the integer |int|, copies the appropriate
|ASCII_code| string into |int_buf| starting at |int_begin|, and sets
the |var| parameter |int_end| to the first unused |int_buf| location.
The ASCII string will consist of decimal digits, the first of which
will be not be a~0 if the integer is nonzero, with a prepended minus
sign if the integer is negative.

@<Procedures and functions for handling numbers, characters, and strings@>=
procedure int_to_ASCII (@!int:integer; var int_buf:buf_type;
                        @!int_begin:buf_pointer; var int_end:buf_pointer);
var int_ptr,@!int_xptr : buf_pointer;   {pointers into |int_buf|}
  @!int_tmp_val : ASCII_code;           {the temporary element in an exchange}
begin
int_ptr := int_begin;
if (int < 0) then       {add the |minus_sign| and use the absolute value}
    begin
    append_int_char (minus_sign);
    int := -int;
    end;
int_xptr := int_ptr;
repeat                          {copy digits into |int_buf|}
    append_int_char ("0" + (int mod 10));
    int := int div 10;
  until (int = 0);
@y
This procedure takes the integer |the_int|, copies the appropriate
|ASCII_code| string into |int_buf| starting at |int_begin|, and sets
the |var| parameter |int_end| to the first unused |int_buf| location.
The ASCII string will consist of decimal digits, the first of which
will be not be a~0 if the integer is nonzero, with a prepended minus
sign if the integer is negative.

@<Procedures and functions for handling numbers, characters, and strings@>=
procedure int_to_ASCII (@!the_int:integer; var int_buf:buf_type;
                        @!int_begin:buf_pointer; var int_end:buf_pointer);
var int_ptr,@!int_xptr : buf_pointer;   {pointers into |int_buf|}
  @!int_tmp_val : ASCII_code;           {the temporary element in an exchange}
begin
int_ptr := int_begin;
if (the_int < 0) then       {add the |minus_sign| and use the absolute value}
    begin
    append_int_char (minus_sign);
    the_int := -the_int;
    end;
int_xptr := int_ptr;
repeat                          {copy digits into |int_buf|}
    append_int_char ("0" + (the_int mod 10));
    the_int := the_int div 10;
  until (the_int = 0);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [388] bibtex.web has mutually exclusive tests here; Oren said he
% doesn't want to fix it until 1.0, since it's obviously of no practical
% import (or someone would have found it before GCC 2 did).  Changing
% the second `and' to an `or' makes all but the last of multiple authors
% be omitted in the bbl file, so I simply removed the statement.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
while ((ex_buf_xptr < ex_buf_ptr) and
                        (lex_class[ex_buf[ex_buf_ptr]] = white_space) and
                        (lex_class[ex_buf[ex_buf_ptr]] = sep_char)) do
        incr(ex_buf_xptr);                      {this removes leading stuff}
@y
@z
