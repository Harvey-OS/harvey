% pktogf.ch for C compilation with web2c.
%
% 09/19/88 Pierre A. MacKay	version 1.0.
% 12/02/89 Karl Berry		cosmetic changes.
% 02/04/90 Karl			new file-searching routines.
% (more recent changes in the ChangeLog)
%
% One major change in output format is incorporated by this change
% file.  The local pktogf preamble comment is ignored and the 
% dated METAFONT comment is passed through unaltered.  This
% provides a continuous check on the origin of fonts in both
% gf and pk formats.  PKtoGF runs silently unless it is given the
% -v switch in the command line.

@x [0] WEAVE: print changes only
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{PK$\,$\lowercase{to}$\,$GF changes for C}
@z

@x [3] No global labels.
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
@y 
@ Both the input and output come from binary files.  On line
interaction is handled through \PASCAL's standard |input| and |output|
files.  For C compilation terminal input and output is directed to
|stdin| and |stdout|.  In this program there is no terminal input.
Since the terminal output is really not very interesting, it is
produced only when the \.{-v} command line flag is presented.

@d print_ln(#)==if verbose then write_ln(output, #)
@d print(#)==if verbose then write(output, #)

@p program PKtoGF(input, output);
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
begin
  kpse_set_progname (argv[0]);
  kpse_init_prog ('PKTOGF', 0, nil, nil);
  parse_arguments;
  print_ln(banner);@/
@z

@x [5] Eliminate the |final_end| label
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ This module is deleted, because it is only useful for
a non-local goto, which we don't use in C.
@z

% [6] Remove terminal_line_length, since there is no dialog, and
% name_length, since there is no maximum size.
@x
@<Constants...@>=
@!name_length=80; {maximum length of a file name}
@!terminal_line_length=132; {maximum length of an input line}
@y
@<Constants...@>=
@z

@x [7] Just exit, instead of doing a nonlocal goto.
@d abort(#)==begin print_ln(' ',#); jump_out; end

@p procedure jump_out;
begin goto final_end;
end;
@y
@d abort(#)==begin verbose:=true; print_ln(#); uexit(1);
    end
@z

@x [30] Remove an unused variable (de-linting)
function pk_packed_num : integer ;
var i, j, k : integer ;
@y
function pk_packed_num : integer ;
var i, j : integer ;
@z

@x [35] Use path searching to open |pk_file|.
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
In C, we do path searching based on the user's environment or the
default path, via the Kpathsea library.

@p procedure open_pk_file; {prepares to read packed bytes in |pk_file|}
begin
  {Don't use |kpse_find_pk|; we want the exact file or nothing.}
  pk_name := cmdline (optind);
  pk_file := kpse_open_file (cmdline (optind), kpse_pk_format);
  if pk_file then begin
    cur_loc := 0;
  end;
end;
@#
procedure open_gf_file; {prepares to write packed bytes in |gf_file|}
begin
  {If an explicit output filename isn't given, we construct it from |pk_name|.}
  if optind + 1 = argc then begin
    gf_name := basename_change_suffix (pk_name, 'pk', 'gf');
  end else begin
    gf_name := cmdline (optind + 1);
  end;
  rewritebin (gf_file, gf_name);
  gf_loc := 0;
end;
@z

@x [36] No arbitrary limit on filename length.
@ We need a place to store the names of the input and output files, as well
as a byte counter for the output file.  

@<Glob...@>=
@!gf_name,@!pk_name:packed array[1..name_length] of char; {names of input
    and output files}
@y
@ No arbitrary limit on filename length.

@<Glob...@>=
@!gf_name,@!pk_name:c_string; {names of input and output files}
@z

@x [42] Define gf_byte (in place of Pascal procedure).
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

@x [43] Use the |get_byte| routines from dvitype (renamed).
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

@x [45] Adapt the DVItype functions.
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

@x [46] Remove unused function gf_sbyte.
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

@x [46] Fix unlikely but possible overflow in addition in gf_quad.
      i := (i + 1073741824) + 1073741824 ;
@y
      {|i<0| at this point, but a compiler is permitted to rearrange the
       order of the additions, which would cause wrong results
       in the unlikely event of a non-2's-complement representation.}
      i := i + 1073741824;
      i := i + 1073741824;
@z

@x [49] Preserve the METAFONT comment.
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

@x [51] Since we preserve the METAFONT comment, this is unneeded.
comment := preamble_comment ;
@y
@z

@x [63] Remove unused variable.
@!nybble : eight_bits ; {the current nybble}
@y
@z

@x [65] Change jumpout to abort.
         if rcp > max_counts then begin
            print_ln('A character had too many run counts') ;
            jump_out ;
         end ;
@y
         if rcp > max_counts then abort('A character had too many run counts');
@z

@x [71] There is no terminal communication.
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

@x [72] There is no dialog.
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
@ \.{pktogf.web} has a |dialog| procedure here.
@z

@x [73] There is no dialog and no |final_end| label.
dialog ;
@y
@z
@x
final_end :
@y
@z

@x System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{PKtoGF} work at a particular installation.
Any additional routines should be inserted here.
@^system dependencies@>
@y
Parse a Unix-style command line.

@d argument_is (#) == (strcmp (long_options[option_index].name, #) = 0)

@<Define |parse_arguments|@> =
procedure parse_arguments;
const n_options = 3; {Pascal won't count array lengths for us.}
var @!long_options: array[0..n_options] of getopt_struct;
    @!getopt_return_val: integer;
    @!option_index: c_int_type;
    @!current_option: 0..n_options;
begin
  @<Initialize the option variables@>;
  @<Define the option table@>;
  repeat
    getopt_return_val := getopt_long_only (argc, argv, '', long_options,
                                           address_of (option_index));
    if getopt_return_val = -1 then begin
      {End of arguments; we exit the loop below.} ;

    end else if getopt_return_val = "?" then begin
      usage (1, 'pktogf');

    end else if argument_is ('help') then begin
      usage (0, PKTOGF_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'Tomas Rokicki');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one or two remaining arguments.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) then begin
    write_ln (stderr, 'pktogf: Need one or two file arguments.');
    usage (1, 'pktogf');
  end;
end;

@ Here are the options we allow.  The first is one of the standard GNU options.
@.-help@>

@<Define the option...@> =
current_option := 0;
long_options[current_option].name := 'help';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ Another of the standard options.
@.-version@>

@<Define the option...@> =
long_options[current_option].name := 'version';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ Print progress information?

@<Define the option...@> =
long_options[current_option].name := 'verbose';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := address_of (verbose);
long_options[current_option].val := 1;
incr (current_option);

@ 
@<Glob...@> =
@!verbose: c_int_type;

@
@<Initialize the option...@> =
verbose := false;

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
@z
