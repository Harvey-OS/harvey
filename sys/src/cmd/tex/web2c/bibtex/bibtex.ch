% Change file for BibTeX for Berkeley UNIX, by H. Trickey, et al.
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


@x banner
@d banner=='This is BibTeX, Version 0.99c' {printed when the program starts}
@y
@d banner=='This is BibTeX, C Version 0.99c' {printed when the program starts}
@z

@x terminal
@d term_out == tty
@d term_in == tty
@y
@d term_out == stdout
@d term_in == stdin
@z

@x debug..gubed, stat..tats, trace..ecart
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

@x
@d incr(#) == #:=#+1    {increase a variable by unity}
@d decr(#) == #:=#-1    {decrease a variable by unity}
@y
{These are defined as C macros}
@z

@x compiler directives
@{@&$C-,A+,D-@}  {no range check, catch arithmetic overflow, no debug overhead}
@!debug @{@&$C+,D+@}@+ gubed            {but turn everything on when debugging}
@y
{Don't need 'em for C}
@z

@x
    goto exit_program;
@y
    uexit(0);
@z

@x increase buf_size
@!buf_size=1000; {maximum number of characters in an input line (or string)}
@y
@!buf_size=3000; {maximum number of characters in an input line (or string)}
@z

@x increase pool_size and other parameters
@!pool_size=65000; {maximum number of characters in strings}
@!max_strings=4000; {maximum number of strings, including pre-defined;
                                                        must be |<=hash_size|}
@!max_cites=750; {maximum number of distinct cite keys; must be
                                                        |<=max_strings|}
@y
@!pool_size=300000; {maximum number of characters in strings}
@!max_strings=9000; {maximum number of strings, including pre-defined;
                                                        must be |<=hash_size|}
@!max_cites=2000; {maximum number of distinct cite keys; must be
                                                        |<=max_strings|}
@z

@x increase max_fields
@!max_fields=17250; {maximum number of fields (entries $\times$ fields,
                                        about |23*max_cites| for consistency)}
@y
@!max_fields=46000; {maximum number of fields (entries $\times$ fields,
                                        about |23*max_cites| for consistency)}
@z


@x
@d hash_size=5000       {must be |>= max_strings| and |>= hash_prime|}
@d hash_prime=4253      {a prime number about 85\% of |hash_size| and |>= 128|
                                                and |< @t$2^{14}-2^6$@>|}
@d file_name_size=40    {file names shouldn't be longer than this}
@y
@d hash_size=10000       {must be |>= max_strings| and |>= hash_prime|}
@d hash_prime=8501      {a prime number about 85\% of |hash_size| and |>= 128|
                                                and |< @t$2^{14}-2^6$@>|}
@d file_name_size == FILENAMESIZE  {Get value from \.{site.h}.}
@z

@x declare real_name_of_file
Most of what we need to do with respect to input and output can be handled
by the I/O facilities that are standard in \PASCAL, i.e., the routines
called |get|, |put|, |eof|, and so on. But
standard \PASCAL\ does not allow file variables to be associated with file
names that are determined at run time, so it cannot be used to implement
\BibTeX; some sort of extension to \PASCAL's ordinary |reset| and |rewrite|
is crucial for our purposes. We shall assume that |name_of_file| is a variable
of an appropriate type such that the \PASCAL\ run-time system being used to
implement \BibTeX\ can open a file whose external name is specified by
|name_of_file|. \BibTeX\ does no case conversion for file names.

@<Globals in the outer block@>=
@!name_of_file:packed array[1..file_name_size] of char;
                         {on some systems this is a \&{record} variable}
@!name_length:0..file_name_size;
  {this many characters are relevant in |name_of_file| (the rest are blank)}
@!name_ptr:0..file_name_size+1;         {index variable into |name_of_file|}
@y
Most of what we need to do with respect to input and output can be handled
by the I/O facilities that are standard in \PASCAL, i.e., the routines
called |get|, |put|, |eof|, and so on. But
standard \PASCAL\ does not allow file variables to be associated with file
names that are determined at run time, so it cannot be used to implement
\BibTeX; some sort of extension to \PASCAL's ordinary |reset| and |rewrite|
is crucial for our purposes. We shall assume that |name_of_file| is a variable
of an appropriate type such that the \PASCAL\ run-time system being used to
implement \BibTeX\ can open a file whose external name is specified by
|name_of_file|. \BibTeX\ does no case conversion for file names.

The C version of BibTeX uses search paths to look for files to open.
We use |real_name_of_file| to hold the |name_of_file| with a directory name
from the path in front of it.

@<Globals in the outer block@>=
@!name_of_file,@!real_name_of_file:packed array[1..file_name_size] of char;
@!name_length:0..file_name_size;
  {this many characters are relevant in |name_of_file| (the rest are blank)}
@!name_ptr:integer;             {index variable into |name_of_file|}
@z

@x opening files
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
@ The \ph\ compiler with which the present version of \TeX\ was prepared has
extended the rules of \PASCAL\ in a very convenient way for file opening.
Berkeley {\mc UNIX} \PASCAL\ isn't nearly as nice as \ph.
Normally, it bombs out if a file open fails.
An external C procedure, |test_access| is used to check whether or not the
open will work.  It is declared in the ``ext.h'' include file, and it returns
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

Path searching is not done for output files.

@d read_access_mode=4  {``read'' mode for |test_access|}
@d write_access_mode=2 {``write'' mode for |test_access|}

@d no_file_path=0    {no path searching should be done}
@d input_file_path=1 {path specifier for input files}
@d bib_file_path=2   {path specifier for .bib files}

@<Procedures and functions for file-system interacting@>=
function a_open_in(var f:palpha_file; pathspec:integer):boolean;
  {open a text file for input}
var @!ok:boolean;
begin
if test_access(read_access_mode,pathspec) then
    begin reset(f,real_name_of_file); ok:=true@+end
else
    ok:=false;
a_open_in:=ok;
end;
@#
function a_open_out(var f:palpha_file):boolean;
  {open a text file for output}
var @!ok:boolean;
begin
if test_access(write_access_mode,no_file_path) then
    begin rewrite(f,real_name_of_file); ok:=true @+end
else ok:=false;
a_open_out:=ok;
end;
@z

@x
@<Procedures and functions for file-system interacting@>=
procedure a_close(var f:alpha_file);            {close a text file}
begin close(f);
end;
@y
{aclose will be defined as a C macro}
@z

%%%%% overflow and confusion go here 
@x faster input_ln
Standard \PASCAL\ says that a file should have |eoln| immediately
before |eof|, but \BibTeX\ needs only a weaker restriction: If |eof|
occurs in the middle of a line, the system function |eoln| should return
a |true| result (even though |f^| will be undefined).

@<Procedures and functions for all file I/O, error messages, and such@>=
function input_ln(var f:alpha_file) : boolean;
                                {inputs the next line or returns |false|}
label loop_exit;
begin
last:=0;
if (eof(f)) then input_ln:=false
else
  begin
  while (not eoln(f)) do
    begin
    if (last >= buf_size) then
        buffer_overflow;
    buffer[last]:=xord[f^];
    get(f); incr(last);
    end;
  get(f);
  while (last > 0) do           {remove trailing |white_space|}
    if (lex_class[buffer[last-1]] = white_space) then
      decr(last)
     else
      goto loop_exit;
loop_exit:
  input_ln:=true;
  end;
end;
@y
With Berkeley {\mc UNIX} we call an external C procedure, |line_read|.
That routine fills |buffer| from |0| onwards with the |xord|'ed values
of the next line, setting |last| appropriately.  It will stop if
|last=buf_size|, and the following will cause an ``overflow'' abort.

@<Procedures and functions for all file I/O, error messages, and such@>=
function input_ln(var f:alpha_file) : boolean;
  {inputs the next line or returns |false|}
label loop_exit;
begin
last:=0;
if eof(f) then input_ln:=false
else
  begin
  line_read(f,buf_size);
  if last>=buf_size then
        overflow('buffer size ',buf_size);
  while (last > 0) do           {remove trailing |white_space|}
    if lex_class[buffer[last-1]] = white_space then
      decr(last)
     else
      goto loop_exit;
loop_exit:
  input_ln:=true;
  end;
end;
@z

@x
if (length(file_name) > file_name_size) then
    begin
    print ('File=');
    print_pool_str (file_name);
    print_ln (',');
    file_nm_size_overflow;
    end;
name_ptr := 1;
@y
if (length(file_name) > file_name_size) then
    begin
    print ('File=');
    print_pool_str (file_name);
    print_ln (',');
    file_nm_size_overflow;
    end;
name_ptr := 0;
@z

@x
name_ptr := name_length + 1;
p_ptr := str_start[ext];
while (p_ptr < str_start[ext+1]) do
    begin
    name_of_file[name_ptr] := chr (str_pool[p_ptr]);
    incr(name_ptr); incr(p_ptr);
    end;
name_length := name_length + length(ext);
name_ptr := name_length+1;
while (name_ptr <= file_name_size) do   {pad with blanks}
    begin
    name_of_file[name_ptr] := ' ';
    incr(name_ptr);
    end;
@y
name_ptr := name_length;
p_ptr := str_start[ext];
while (p_ptr < str_start[ext+1]) do
    begin
    name_of_file[name_ptr] := chr (str_pool[p_ptr]);
    incr(name_ptr); incr(p_ptr);
    end;
name_length := name_length + length(ext);
name_of_file[name_length] := ' ';
@z

@x
    print_pool_str (area); print (name_of_file,',');
    file_nm_size_overflow;
    end;
name_ptr := name_length;
while (name_ptr > 0) do         {shift up name}
    begin
    name_of_file[name_ptr+length(area)] := name_of_file[name_ptr];
    decr(name_ptr);
    end;
name_ptr := 1;
p_ptr := str_start[area];
while (p_ptr < str_start[area+1]) do
@y
    print_pool_str (area); print_str (name_of_file,',');
    file_nm_size_overflow;
    end;
name_ptr := name_length;
while (name_ptr > 0) do         {shift up name}
    begin
    name_of_file[name_ptr+length(area)] := name_of_file[name_ptr];
    decr(name_ptr);
    end;
name_ptr := 0;
p_ptr := str_start[area];
while (p_ptr < str_start[area+1]) do
@z

@x
for i:=1 to len do
    buffer[i] := xord[pds[i]];
@y
for i:=1 to len do
    buffer[i] := xord[pds[i-1]];
@z

@x
@!aux_name_length : 0..file_name_size+1;        {\.{.aux} name sans extension}
@y
@!aux_name_length : integer;
@z

@x
procedure sam_too_long_file_name_print;
begin
write (term_out,'File name `');
name_ptr := 1;
while (name_ptr <= aux_name_length) do
    begin
    write (term_out,name_of_file[name_ptr]);
@y
procedure sam_too_long_file_name_print;
begin
write (term_out,'File name `');
name_ptr := 0;
while (name_ptr < aux_name_length) do
    begin
    write (term_out,name_of_file[name_ptr]);
@z

@x
procedure sam_wrong_file_name_print;
begin
write (term_out,'I couldn''t open file name `');
name_ptr := 1;
while (name_ptr <= name_length) do
    begin
    write (term_out,name_of_file[name_ptr]);
    incr(name_ptr);
    end;
write_ln (term_out,'''');
end;
@y
procedure sam_wrong_file_name_print;
begin
write (term_out,'I couldn''t open file name `');
name_ptr := 0;
while (name_ptr < name_length) do
    begin
    write (term_out,name_of_file[name_ptr]);
    incr(name_ptr);
    end;
write_ln (term_out,'''');
end;
@z

@x reading the command line
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
    if (gargc > 1) then
        @<Process a possible command line@>
      else begin
        write (term_out,'Please type input file name (no extension)--');
        aux_name_length := 0;
        while (not eoln(term_in)) do begin
            if (aux_name_length = file_name_size) then begin
                readln(term_in);
                sam_you_made_the_file_name_too_long;
            end;
            name_of_file[aux_name_length] := getc(term_in);
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
    gargc := 0;
    end;
aux_found:                      {now we're ready to read the \.{.aux} file}
end;
@z

@x
@<Variables for possible command-line processing@>=
@!check_cmnd_line : boolean;    {|true| if we're to check the command line}
@y
@z

@x
@<Process a possible command line@>=
begin
do_nothing;             {the ``default system'' doesn't use the command line}
end
@y
@<Process a possible command line@>=
aux_name_length := get_cmd_line(name_of_file, file_name_size)
@z

@x
if (not a_open_in(cur_aux_file)) then
    sam_you_made_the_file_name_wrong;
@y
if (not a_open_in(cur_aux_file,no_file_path)) then
    sam_you_made_the_file_name_wrong;
@z

@x
while (name_ptr <= name_length) do
    begin
    buffer[name_ptr] := xord[name_of_file[name_ptr]];
    incr(name_ptr);
    end;
@y
while (name_ptr <= name_length) do
    begin
    buffer[name_ptr] := xord[name_of_file[name_ptr-1]];
    incr(name_ptr);
    end;
@z

% Handle path searching on opening files
@x
if (not a_open_in(cur_bib_file)) then
    begin
    add_area (s_bib_area);
    if (not a_open_in(cur_bib_file)) then
        open_bibdata_aux_err ('I couldn''t open database file ');
    end;
@y
if (not a_open_in(cur_bib_file,bib_file_path)) then
        open_bibdata_aux_err ('I couldn''t open database file ');
@z

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
if (not a_open_in(bst_file,input_file_path)) then
    begin
        print ('I couldn''t open style file ');
        print_bst_name;@/
        bst_str := 0;                           {mark as unused again}
        aux_err_return;
    end;
@z

@x
name_ptr := name_length+1;
@y
name_ptr := name_length;
@z

% More search path stuff
@x
if (not a_open_in(cur_aux_file)) then
@y
if (not a_open_in(cur_aux_file, no_file_path)) then
@z

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

@x `int' is a bad variable name in some cc's.
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
