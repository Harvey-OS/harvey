% pktogf.ch for C compilation with web2c.
%
% 09/19/88 Pierre A. MacKay	version 1.0.
% 12/02/89 Karl Berry		cosmetic changes.
% 02/04/90 Karl			new file-searching routines.
% (more recent changes in ../ChangeLog.W2C)
%
% One major change in output format is incorporated by this change
% file.  The local pktogf preamble comment is ignored and the 
% dated METAFONT comment is passed through unaltered.  This
% provides a continuous check on the origin of fonts in both
% gf and pk formats.  PKtoGF runs silently unless it is given the
% -v switch in the command line.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{PK$\,$\lowercase{to}$\,$GF changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is PKtoGF, Version 1.1'
         {printed when the program starts}
@y
@d banner=='This is PKtoGF, C Version 1.1 ' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3] Change program header to standard input/output
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ Both the input and output come from binary files.  On line interaction
is handled through \PASCAL's standard |input| and |output| files.

@d print_ln(#)==write_ln(output,#)
@d print(#)==write(output,#)

@p program PKtoGF(input, output);
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
@ Both the input and output come from binary files.  On line
interaction is handled through \PASCAL's standard |input| and |output|
files.  For C compilation terminal input and output is directed to
|stdin| and |stdout|.  In this program there is no terminal input.
Since the terminal output is really not very interesting, it is
produced only when the \.{-v} command line flag is presented.

@d term_out == stdout {standard output}
@d print_ln(#)==if verbose then write_ln(term_out, #)
@d print(#)==if verbose then write(term_out, #)

@p program PK_to_GF;
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin
  set_paths (PK_FILE_PATH_BIT);
  @<Set initial values@>@/
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [5] Eliminate the |final_end| label
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ This module is deleted, because it is only useful for
a non-local goto, which we don't use in C.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [6] remove terminal_line_length, since there is no dialog.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Constants...@>=
@!name_length=80; {maximum length of a file name}
@!terminal_line_length=132; {maximum length of an input line}
@y
@d name_length==PATH_MAX

@<Constants...@>=
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [7] Have abort append <nl> to end of msg and eliminate non-local goto
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d abort(#)==begin print_ln(' ',#); jump_out; end

@p procedure jump_out;
begin goto final_end;
end;

@y
@d abort(#)==begin verbose:=true; print_ln(#); uexit(1);
    end

@z
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [30] remove an unused variable (de-linting)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
function pk_packed_num : integer ;
var i, j, k : integer ;
@y
function pk_packed_num : integer ;
var i, j : integer ;
@z
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [35] Use path-searching to open |pk_file|.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p procedure open_gf_file; {prepares to write packed bytes in a |gf_file|}
begin rewrite(gf_file,gf_name);
gf_loc := 0 ;
end;
@#
procedure open_pk_file; {prepares the input for reading}
begin reset(pk_file,pk_name);
pk_loc := 0 ;
end;

@y
In C, we use the external |test_read_access| procedure, which also does path
searching based on the user's environment or the default path.  In the course
of this routine we also check the command line for the \.{-v} flag, and make
other checks to see that it is worth running this program at all.

@p procedure open_pk_file; {prepares to read packed bytes in |pk_file|}
var j:integer;
begin
    verbose := false; gf_arg :=3;
    if argc < 2 then abort('Usage: pktogf [-v] <pk file> [gf file].');
    argv(1, pk_name);
    if pk_name[1]=xchr["-"] then begin
        if argc > 4 then abort('Usage: pktogf [-v] <pk file> [gf file].');
        if pk_name[2]=xchr["v"] then begin
            verbose := true; argv(2, pk_name); incr(gf_arg)
            end else abort('Usage: pktogf [-v] <pk file> [gf file].');
        end;
    print_ln(banner);@/
    if test_read_access(pk_name, PK_FILE_PATH) then begin
        reset(pk_file, pk_name)
        end
    else begin
        print_pascal_string (pk_name);
        abort(': PK file not found.');
    end;
    cur_loc:=0;
end;
@#
procedure open_gf_file; {prepares to write packed bytes in |gf_file|}
var dot_pos, slash_pos, last, gf_index, pk_index:integer;
begin
  if argc = gf_arg
  then argv (argc - 1, gf_name)
  else begin
    dot_pos := -1;
    slash_pos := -1;
    last := 1;
    
    {Find the end of |pk_name|.}
    while (pk_name[last] <> ' ') and (last <= PATH_MAX - 5)
    do begin
      if pk_name[last] = '.' then dot_pos := last;
      if pk_name[last] = '/' then slash_pos := last;
      incr (last);
    end;
    
    {If no \./ in |pk_name|, use it from the beginning.}
    if slash_pos = -1 then slash_pos := 0;
    
    {Filenames like \.{./foo} will have |dot_pos<slash_pos|.  In that
     case, we want to move |dot_pos| to the end of |pk_name|.  Similarly
     if |dot_pos| is still |-1|.}
    if dot_pos < slash_pos then dot_pos := last - 1;
    
    {Copy |pk_name| from |slash_pos+1| to |dot_pos| into |gf_name|.}
    gf_index := 1;
    for pk_index := slash_pos + 1 to dot_pos
    do begin
      gf_name[gf_index] := pk_name[pk_index];
      incr (gf_index);
    end;
    
    {Now we are ready to deal with the extension.  Copy everything to
     the first \.p.  Then add \.{gf}.  This loses on filenames like
     \.{foo.p300pk}, but no one uses such filenames, anyway.}
    pk_index := dot_pos + 1;
    while (pk_index < last) and (pk_name[pk_index] <> 'p')
    do begin
      gf_name[gf_index] := pk_name[pk_index];
      incr (pk_index);
      incr (gf_index);
    end;
    
    gf_name[gf_index] := 'g';
    gf_name[gf_index + 1] := 'f';
    gf_name[gf_index + 2] := ' ';
  end;

  {Report the filename mapping.}
  print (xchr[xord['[']]);

  pk_index := 1;
  while pk_name[pk_index] <> ' ' 
  do begin
    print (xchr[xord[pk_name[pk_index]]]);
    incr (pk_index);
  end;
  
  print (xchr[xord['-']]);
  print (xchr[xord['>']]);

  gf_index := 1;
  while gf_name[gf_index] <> ' ' 
  do begin
    print (xchr[xord[gf_name[gf_index]]]);
    incr (gf_index);
  end;
  
  print (xchr[xord[']']]);
  print_ln (xchr[xord[' ']]);

  rewrite(gf_file,gf_name);
  gf_loc:=0
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [36] Add some globals for file handling.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ We need a place to store the names of the input and output files, as well
as a byte counter for the output file.  

@<Glob...@>=
@!gf_name,@!pk_name:packed array[1..name_length] of char; {names of input
    and output files}
@!gf_loc, @!pk_loc:integer; {how many bytes have we sent?}
@y
@ We need a place to store the names of the input and output files, as well
as a byte counter for the output file. And a few other things besides.

@<Glob...@>=
@!gf_name,@!pk_name:packed array[1..name_length] of text_char; 
	{names of input and output files; pascal-style origin from one}
@!gf_loc, @!cur_loc:integer; {changed |pk_loc| to |cur_loc|}
@!gf_arg:integer; {where command line may supply |gf_name|}
@!verbose:boolean;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [37] define gf_byte (in place of pascal procedure)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ We need a procedure that will write a byte to the \.{GF} file.  If the
particular system
@^system dependencies@>
requires buffering, here is the place to do it.

@p procedure gf_byte (i : integer) ;
begin gf_file^ := i ;
put(gf_file) ;
incr(gf_loc) ;
end;
@y
@ Byte output is handled by a C definition.

@d gf_byte(#)==begin put_byte(#, gf_file); incr(gf_loc) end

@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38] use the |get_byte| routines from DVItype (renamed)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ We also need a function that will get a single byte from the \.{PK} file.
Again, buffering may be done in this procedure.

@p function pk_byte : eight_bits ;
var nybble, temp : eight_bits ;
begin
   temp := pk_file^ ;
   get(pk_file) ;
   pk_loc := pk_loc + 1 ;
   pk_byte := temp ;
end ;
@y
@ We shall use a set of simple functions to read the next byte or
bytes from |pk_file|. There are seven possibilities, each of which is
treated as a separate function in order to minimize the overhead for
subroutine calls.
@^system dependencies@>

@d pk_byte==get_byte
@d pk_loc==cur_loc 

@p function get_byte:integer; {returns the next byte, unsigned}
var b:eight_bits;
begin if eof(pk_file) then get_byte:=0
else  begin read(pk_file,b); incr(cur_loc); get_byte:=b;
  end;
end;
@#
function signed_byte:integer; {returns the next byte, signed}
var b:eight_bits;
begin read(pk_file,b); incr(cur_loc);
if b<128 then signed_byte:=b @+ else signed_byte:=b-256;
end;
@#
function get_two_bytes:integer; {returns the next two bytes, unsigned}
var a,@!b:eight_bits;
begin read(pk_file,a); read(pk_file,b);
cur_loc:=cur_loc+2;
get_two_bytes:=a*256+b;
end;
@#
function signed_pair:integer; {returns the next two bytes, signed}
var a,@!b:eight_bits;
begin read(pk_file,a); read(pk_file,b);
cur_loc:=cur_loc+2;
if a<128 then signed_pair:=a*256+b
else signed_pair:=(a-256)*256+b;
end;
@{
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [40] use definitions for adaptation to DVItype functions
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ As we are reading the packed file, we often need to fetch 16 and 32 bit
quantities.  Here we have two procedures to do this.

@p function signed_byte : integer ;
var a : integer ;
begin
   a := pk_byte ;
   if a > 127 then
      a := a - 256 ;
   signed_byte := a ;
end ;
@#
function get_16 : integer ;
var a : integer ;
begin
   a := pk_byte ;
   get_16 := a * 256 + pk_byte ; 
end ;
@#
function signed_16 : integer ;
var a : integer ;
begin
   a := signed_byte ;
   signed_16 := a * 256 + pk_byte ;
end ;
@#
function get_32 : integer ;
var a : integer ;
begin 
   a := get_16 ; 
   if a > 32767 then a := a - 65536 ;
   get_32 := a * 65536 + get_16 ; 
end ;
@y
@ We put definitions here to access the \.{DVItype} functions supplied
above.  (|signed_byte| is already taken care of).

@d get_16==get_two_bytes
@d signed_16==signed_pair
@d get_32==signed_quad
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] remove unused gf_sbyte 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p procedure gf_sbyte(i : integer) ;
begin
   if i < 0 then
      i := i + 256 ;
   gf_byte(i) ;
end ;
@#
procedure gf_16(i : integer) ;
@y
@p procedure gf_16(i : integer) ;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [49] preserve the METAFONT comment
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
j := pk_byte ;
for i := 1 to j do hppp := pk_byte ;
gf_byte(comm_length) ;
for i := 1 to comm_length do
   gf_byte(xord[comment[i]]) ;
@y
j := pk_byte ;
gf_byte(j) ;
print('{') ;
for i := 1 to j do begin
  hppp:=pk_byte;
  gf_byte(hppp) ;
  print(xchr[xord[hppp]]);
  end;
print_ln('}') ;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [51] since we preserve the METAFONT comment, this is unneeded
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
comment := preamble_comment ;
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [63] remove unused nybble
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!nybble : eight_bits ; {the current nybble}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [65] change jumpout to abort
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
         if rcp > max_counts then begin
            print_ln('A character had too many run counts') ;
            jump_out ;
         end ;
@y
         if rcp > max_counts then abort('A character had too many run counts');
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [71] There is no terminal communication.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
@y
@* Terminal communication.
Since this program runs entirely on command-line arguments, there
is no terminal communication.
@z


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [72] There is no dialog
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @p procedure dialog ;
var i : integer ; {index variable}
buffer : packed array [1..name_length] of char; {input buffer}
begin
   for i := 1 to name_length do begin
      gf_name[i] := ' ' ;
      pk_name[i] := ' ' ;
   end;
   print('Input file name:  ') ;
   flush_buffer ;
   get_line(pk_name) ;
   print('Output file name:  ') ;
   flush_buffer ;
   get_line(gf_name) ;
end ;
@y
@ The \.{pktogf.web} file has a |dialog| procedure here.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [73] There is no dialog and no |final_end| label
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
while not eof(pk_file) do i := pk_byte ;
@<Write \.{GF} postamble@> ;
print_ln(pk_loc:1,' bytes unpacked to ',gf_loc:1,' bytes.');
final_end :
end .
@y
@p begin
initialize ;
@<Open files@> ;
@<Read preamble@> ;
skip_specials ;
while flag_byte <> pk_post do begin
   @<Unpack and write character@> ;
   skip_specials ;
end ;
while not eof(pk_file) do i := pk_byte ;
@<Write \.{GF} postamble@> ;
print_ln(pk_loc:1,' bytes unpacked to ',gf_loc:1,' bytes.');
end .
@z
