% pktype.ch for C compilation with web2c.
%
% 09/27/88 Pierre A. MacKay	Version 2.2.
% 12/02/89 Karl Berry		cosmetic changes.
% 02/04/90 Karl			new file-searching routines.
% (more recent changes in ../ChangeLog.W2C)
%
% There is no terminal input to this program.  
% Output is to stdout, and may, of course, be redirected.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{PK$\,$\lowercase{type} changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is PKtype, Version 2.3' {printed when the program starts}
@y
@d banner=='This is PKtype, C Version 2.3' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] Redirect output to stdout.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d print_ln(#)==write_ln(output,#)
@d print(#)==write(output,#)
@d t_print_ln(#)==write_ln(typ_file,#)
@d t_print(#)==write(typ_file,#)
@y
@d term_out==stdout
@d print_ln(#)==write_ln(term_out,#)
@d print(#)==write(term_out,#)
@d t_print_ln(#)==write_ln(term_out,#)
@d t_print(#)==write(term_out,#)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] Change program header to remove IO, and eliminate label.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p program PKtype(@!input,@!output);
label @<Labels in the outer block@>@/
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin print_ln(banner);@/
  @<Set initial values@>@/
  end;
@y
@p program PKtype;
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin print_ln(banner);@/
  set_paths (PK_FILE_PATH_BIT);
  @<Set initial values@>@/
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [5] Remove the unused label.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [6] Make |name_length| match the value of PATH_MAX, and remove
% terminal line length, since there is no dialog.  Since these were the
% only constants, the <Constants...> module is not needed.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Constants...@>=
@!name_length=80; {maximum length of a file name}
@!terminal_line_length=132; {maximum length of an input line}
@y
@d name_length==PATH_MAX
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [8] Change abort to get rid of non-local goto.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ It is possible that a malformed packed file (heaven forbid!) or some other
error might be detected by this program.  Such errors might occur in a deeply
nested procedure, so the procedure called |jump_out| has been added to transfer
to the very end of the program with an error message.

@d abort(#)==begin print_ln(' ',#); t_print_ln(' ',#); jump_out; end

@p procedure jump_out;
begin goto final_end;
end;
@y
@ We use a call to the external C exit to avoid a non-local |goto|.

@d abort(#)==begin print_ln(#); uexit(1) end

@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [32] Remove typ_file from globals.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Glob...@>=
@!pk_file:byte_file;  {where the input comes from}
@!typ_file:text_file; {where the final output goes}
@^system dependencies@>
@y
@ @<Glob...@>=
@!pk_file:byte_file;  {where the input comes from}
@^system dependencies@>
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [33] Redo open_pk_file; scrap open_typ_file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare these files for input and output, we |reset| and |rewrite| them.
An extension of \PASCAL\ is needed, since we want to associate files
with external names that are specified dynamically (i.e., not
known at compile time). The following code assumes that `|reset(f,s)|'
does this, when |f| is a file variable and |s| is a string variable that
specifies the file name. If |eof(f)| is true immediately after
|reset(f,s)| has acted, we assume that no file named |s| is accessible.
@^system dependencies@>

@p procedure open_pk_file; {prepares the input for reading}
begin reset(pk_file,pk_name);
pk_loc := 0 ;
end;
@#
procedure open_typ_file; {prepares to write text data to the |typ_file|}
begin rewrite(typ_file,typ_name);
end;

@y
@ In C, we use the external |test_read_access| procedure, which also
does path searching based on the user's environment or the default path.  

@p procedure open_pk_file; {prepares to read packed bytes in |pk_file|}
var j,k:integer;
begin
    k := 1;
    if argc <> 2 then abort ('Usage: pktype <pk file>.');
    argv (1, pk_name);
    
    if test_read_access (pk_name, PK_FILE_PATH)
    then begin
        reset (pk_file, pk_name);
        cur_loc := 0;
    end else begin
        print_pascal_string (pk_name);
        abort (': PK file not found.');
    end;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [34] Change pk_loc to cur_loc.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!pk_loc:integer; {how many bytes have we read?}
@y
@!cur_loc:integer; {how many bytes have we read?}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Use modified routines to access pk_file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function pk_byte : eight_bits ;
var temp : eight_bits ;
begin 
   temp := pk_file^ ;
   get(pk_file) ;
   incr(pk_loc) ;
   pk_byte := temp ;
end ;
@y
We shall use a set of simple functions to read the next byte or
bytes from |pk_file|. There are seven possibilities, each of which is
treated as a separate function in order to minimize the overhead for
subroutine calls.  We comment out the ones we don't need
@^system dependencies@>

@d pk_byte==get_byte
@d pk_loc==cur_loc 

@p function get_byte:integer; {returns the next byte, unsigned}
var b:eight_bits;
begin if eof(pk_file) then get_byte:=0
else  begin read(pk_file,b); incr(cur_loc); get_byte:=b;
  end;
end;
@{
function signed_byte:integer; {returns the next byte, signed}
var b:eight_bits;
begin read(pk_file,b); incr(cur_loc);
if b<128 then signed_byte:=b @+ else signed_byte:=b-256;
end;
@}
function get_two_bytes:integer; {returns the next two bytes, unsigned}
var a,@!b:eight_bits;
begin read(pk_file,a); read(pk_file,b);
cur_loc:=cur_loc+2;
get_two_bytes:=a*256+b;
end;
@{
function signed_pair:integer; {returns the next two bytes, signed}
var a,@!b:eight_bits;
begin read(pk_file,a); read(pk_file,b);
cur_loc:=cur_loc+2;
if a<128 then signed_pair:=a*256+b
else signed_pair:=(a-256)*256+b;
end;
@#
function get_three_bytes:integer; {returns the next three bytes, unsigned}
var a,@!b,@!c:eight_bits;
begin read(pk_file,a); read(pk_file,b); read(pk_file,c);
cur_loc:=cur_loc+3;
get_three_bytes:=(a*256+b)*256+c;
end;
@#
function signed_trio:integer; {returns the next three bytes, signed}
var a,@!b,@!c:eight_bits;
begin read(pk_file,a); read(pk_file,b); read(pk_file,c);
cur_loc:=cur_loc+3;
if a<128 then signed_trio:=(a*256+b)*256+c
else signed_trio:=((a-256)*256+b)*256+c;
end;
@}
function signed_quad:integer; {returns the next four bytes, signed}
var a,@!b,@!c,@!d:eight_bits;
begin read(pk_file,a); read(pk_file,b); read(pk_file,c); read(pk_file,d);
cur_loc:=cur_loc+4;
if a<128 then signed_quad:=((a*256+b)*256+c)*256+d
else signed_quad:=(((a-256)*256+b)*256+c)*256+d;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [36] Don't need the <Open files> module.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ Now we are ready to open the files.

@<Open files@>=
open_pk_file ;
open_typ_file ;
t_print_ln(banner) ;
t_print('Input file: ') ;
i := 1 ;
while pk_name[i] <> ' ' do begin
   t_print(pk_name[i]) ; incr(i) ;
end ;
t_print_ln(' ')

@y
@ This module was needed when output was directed to |typ_file|.
It is not needed when output goes to |stdout|.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [37] Redefine get_16 and get_32.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function get_16 : integer ;
var a : integer ;
begin a := pk_byte ; get_16 := a * 256 + pk_byte ; end ;
@#
function get_32 : integer ;
var a : integer ;
begin a := get_16 ; if a > 32767 then a := a - 65536 ;
get_32 := a * 65536 + get_16 ; end ;
@y
@d get_16==get_two_bytes
@d get_32==signed_quad
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [54--55] Eliminate the ``Terminal communication'' chapter.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@* Terminal communication.
We must get the file names and determine whether input is to be in
hexadecimal or binary.  To do this, we use the standard input path
name.  We need a procedure to flush the input buffer.  For most systems,
this will be an empty statement.  For other systems, a |print_ln| will
provide a quick fix.  We also need a routine to get a line of input from
the terminal.  On some systems, a simple |read_ln| will do.  Finally,
a macro to print a string to the first blank is required.

@d flush_buffer == begin end
@d get_line(#) == if eoln(input) then read_ln(input) ;
   i := 1 ;
   while not (eoln(input) or eof(input)) do begin
      #[i] := input^ ;
      incr(i) ;
      get(input) ;
   end ;
   #[i] := ' '

@ @p procedure dialog ;
var i : integer ; {index variable}
buffer : packed array [1..name_length] of char; {input buffer}
begin
   for i := 1 to name_length do begin
      typ_name[i] := ' ' ;
      pk_name[i] := ' ' ;
   end;
   print('Input file name:  ') ;
   flush_buffer ;
   get_line(pk_name) ;
   print('Output file name:  ') ;
   flush_buffer ;
   get_line(typ_name) ;
end ;

@y
@* Terminal communication. There isn't any.

@ So there is no |procedure dialog|.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [56] Restructure the main program.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p begin
initialize ;
dialog ;
@<Open files@> ;
@<Read preamble@> ;
skip_specials ;
while flag_byte <> pk_post do begin
   @<Unpack and write character@> ;
   skip_specials ;
end ;
j := 0 ;
while not eof(pk_file) do begin
   i := pk_byte ;
   if i <> pk_no_op then abort('Bad byte at end of file: ',i:1) ;
@.Bad byte at end of file@>
   t_print_ln((pk_loc-1):1,':  No op') ;
   incr(j) ;
end ;
t_print_ln(pk_loc:1,' bytes read from packed file.');
final_end :
end .
@y
@p begin
initialize ;
open_pk_file ;
@<Read preamble@> ;
skip_specials ;
while flag_byte <> pk_post do begin
   @<Unpack and write character@> ;
   skip_specials ;
end ;
j := 0 ;
while not eof(pk_file) do begin
   i := pk_byte ;
   if i <> pk_no_op then abort('Bad byte at end of file: ',i:1) ;
   t_print_ln((pk_loc-1):1,':  No op') ;
   incr(j) ;
end ;
t_print_ln(pk_loc:1,' bytes read from packed file.');
end .
@z
