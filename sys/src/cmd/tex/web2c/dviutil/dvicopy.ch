% dvicopy.ch for C compilation with web2c.
% The original version of this file was created by Monika Jayme and
% Klaus Guntermann at TH Darmstadt (THD), FR Germany.
%                                       (xitikgun@ddathd21.bitnet)
% Some parts are borrowed from the changes to dvitype, vftovp and vptovf.
%
% History:
%  July 90   THD  First versions for dvicopy 0.91 and 0.92
%  Aug 09 90 THD  Updated to dvicopy 1.0 and released
%  Mar 20 91 THD  Updated to dvicopy 1.2
%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string and preamble comment
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is DVIcopy, Version 1.2' {printed when the program starts}
@y
@d banner=='This is DVIcopy, C Version 1.2' {printed when the program starts}
@z

@x
@d preamble_comment=='DVIcopy 1.2 output from '
@y
@d preamble_comment==' DVIcopy 1.2 output from '
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3] Change filenames in program statement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
program DVI_copy(@!dvi_file,@!out_file,@!output);
@y
program DVI_copy;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [5] Make name_length match the system constant.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Constants...@>=
@y
@d name_length==PATH_MAX

@<Constants...@>=
@z

@x
@!name_length=50; {a file name shouldn't be longer than this}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [11] Use `stdout' instead of `output'.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d print(#)==write(output,#)
@y
@d output == stdout
@d print(#)==write(output,#)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [12] Use C macros for incr/decr
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d incr(#) == #:=#+1 {increase a variable by unity}
@d decr(#) == #:=#-1 {decrease a variable by unity}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [14] Permissive input.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!ASCII_code=" ".."~"; {a subrange of the integers}
@y
@!ASCII_code=0..255; {a subrange of the integers}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [15] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d text_char == char {the data type of characters in text files}
@d first_text_char=0 {ordinal number of the smallest element of |text_char|}
@d last_text_char=127 {ordinal number of the largest element of |text_char|}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@d first_text_char=0 {ordinal number of the smallest element of |text_char|}
@d last_text_char=255 {ordinal number of the largest element of |text_char|}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [23] Remove non-local goto
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If an input (\.{DVI}, \.{TFM}, \.{VF}, or other) file is badly malformed,
the whole process must be aborted; \.{\title} will give up, after issuing
an error message about what caused the error. These messages will, however,
in most cases just indicate which input file caused the error. One of the
programs \.{DVItype}, \.{TFtoPL} or \.{VFtoVP} should then be used to
diagnose the error in full detail.

Such errors might be discovered inside of subroutines inside of subroutines,
so a procedure called |jump_out| has been introduced. This procedure, which
transfers control to the label |final_end| at the end of the program,
contains the only non-local |@!goto| statement in \.{\title}.
@^system dependencies@>
Some \PASCAL\ compilers do not implement non-local |goto| statements. In
such cases the |goto final_end| in |jump_out| should simply be replaced
by a call on some system procedure that quietly terminates the program.
@^system dependencies@>

@d abort(#)==begin print_ln(' ',#,'.'); jump_out;
    end

@<Error handling...@>=
@<Basic printing procedures@>@;
procedure close_files_and_terminate; forward;
@#
procedure jump_out;
begin mark_fatal; close_files_and_terminate;
goto final_end;
end;
@y
@ If an input (\.{DVI}, \.{TFM}, \.{VF}, or other) file is badly malformed,
the whole process must be aborted; \.{\title} will give up, after issuing
an error message about what caused the error. These messages will, however,
in most cases just indicate which input file caused the error. One of the
programs \.{DVItype}, \.{TFtoPL} or \.{VFtoVP} should then be used to
diagnose the error in full detail.

Such errors might be discovered inside of subroutines inside of subroutines,
so a procedure called |jump_out| has been introduced. This procedure is
actually a macro which calls |uexit()| with a non-zero exit status.

@d abort(#)==begin print_ln(' ',#,'.'); jump_out;
    end

@<Error handling...@>=
@<Basic printing procedures@>@;
procedure close_files_and_terminate; forward;
@#
procedure jump_out;
begin mark_fatal; close_files_and_terminate;
uexit(1);
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [51] Fix casting problem in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d comp_spair(#) == if a<128 then #:=a*256+b @+ else #:=(a-256)*256+b
@d comp_upair(#) == #:=a*256+b
@y
@d comp_spair(#) == if a<128 then #:=a*toint(256)+b
                             @+ else #:=(a-toint(256))*toint(256)+b
@d comp_upair(#) == #:=a*toint(256)+b
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [52]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if a<128 then #:=(a*256+b)*256+c @+ else #:=((a-256)*256+b)*256+c
@d comp_utrio(#) == #:=(a*256+b)*256+c
@y
if a<128 then #:=(a*toint(256)+b)*toint(256)+c @+
else #:=((a-toint(256))*toint(256)+b)*toint(256)+c
@d comp_utrio(#) == #:=(a*toint(256)+b)*toint(256)+c
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [53]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if a<128 then #:=((a*256+b)*256+c)*256+d
else #:=(((a-256)*256+b)*256+c)*256+d
@y
if a<128 then #:=((a*toint(256)+b)*toint(256)+c)*toint(256)+d
else #:=(((a-toint(256))*toint(256)+b)*toint(256)+c)*toint(256)+d
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [63]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ For \.{TFM} and \.{VF} files we just append the apropriate extension
to the file name packet; in addition a system dependent area part
(usually different for \.{TFM} and \.{VF} files) is prepended if
the file name packet contains no area part.
@^system dependencies@>
 
@d append_to_name(#)==
  if cur_name_length<name_length then
    begin incr(cur_name_length); cur_name[cur_name_length]:=#;
    end
  else overflow(str_name_length,name_length)
@d make_font_name_end(#)==
  append_to_name(#[l]); make_name
@d make_font_name(#)==
  cur_name_length:=0; for l:=1 to # do make_font_name_end
@y
@ For \.{TFM} and \.{VF} files we just append the apropriate extension
to the file name packet.
Since files are actually searched through path definitions given in
the environment variables area definitions are ignored here.
To reduce the required changes we ignore most of the parameters given
to |make_font_name| and just keep the requested extension.
@^system dependencies@>
 
@d append_to_name(#)==
  if cur_name_length<name_length then
    begin incr(cur_name_length); cur_name[cur_name_length]:=#;
    end
  else overflow(str_name_length,name_length)
@d make_font_name_end(#)==
  make_name
@d make_font_name(#)==
  cur_name_length:=0; make_font_name_end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [67] No conversion of file names in lower case
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  if (b>="a")and(b<="z") then Decr(b)("a"-"A"); {convert to upper case}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [92] File names in lower case
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
id4(".")("T")("F")("M")(tfm_ext); {file name extension for \.{TFM} files}
@y
id4(".")("t")("f")("m")(tfm_ext); {file name extension for \.{TFM} files}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [93] Set default directory name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If no font directory has been specified, \.{\title} is supposed to use
the default \.{TFM} directory, which is a system-dependent place where
the \.{TFM} files for standard fonts are kept.
The string variable |TFM_default_area| contains the name of this area.
@^system dependencies@>
 
@d TFM_default_area_name=='TeXfonts:' {change this to the correct name}
@d TFM_default_area_name_length=9 {change this to the correct length}

@<Glob...@>=
@!TFM_default_area:packed array[1..TFM_default_area_name_length] of char;
@y
@ If no font directory has been specified, \.{\title} is supposed to use
the default \.{TFM} directory, which is a system-dependent place where
the \.{TFM} files for standard fonts are kept.
@^system dependencies@>
 
Actually, under UNIX the standard area is defined in an external
file \.{site.h}.  And the users have a path searched for fonts,
by setting the \.{TEXFONTS} environment variable.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [94] Remove initialization of now-defunct array
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
TFM_default_area:=TFM_default_area_name;
@y
@ (No initialization to be done.  Keep this module to preserve numbering.)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [96] Open TFM file
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<TFM: Open |tfm_file|@>=
make_font_name(TFM_default_area_name_length)(TFM_default_area)(tfm_ext);
reset(tfm_file,cur_name);
if eof(tfm_file) then
@^system dependencies@>
  abort('---not loaded, TFM file can''t be opened!')
@.TFM file can\'t be opened@>
@y
In C, we use the external |test_access| procedure, which also does path
searching based on the user's environment or the default path.
Note that |TFM_default_area_name_length| and |TFM_default_area| will not
be used by |make_font_name|.

@<TFM: Open |tfm_file|@>=
make_font_name(TFM_default_area_name_length)(TFM_default_area)(tfm_ext);
if test_read_access(cur_name,TFM_FILE_PATH) then begin
    reset(tfm_file,cur_name);
end else begin
    print_pascal_string(cur_name);
    abort('---not loaded, TFM file can''t be opened!');
@.TFM file can\'t be opened@>
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [103] Fix casting problem in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d tfm_b01(#)== {|tfm_b0..tfm_b1| as non-negative integer}
if tfm_b0>127 then bad_font
else #:=tfm_b0*256+tfm_b1
@d tfm_b23(#)== {|tfm_b2..tfm_b3| as non-negative integer}
if tfm_b2>127 then bad_font
else #:=tfm_b2*256+tfm_b3
@d tfm_squad(#)== {|tfm_b0..tfm_b3| as signed integer}
if tfm_b0<128 then #:=((tfm_b0*256+tfm_b1)*256+tfm_b2)*256+tfm_b3
else #:=(((tfm_b0-256)*256+tfm_b1)*256+tfm_b2)*256+tfm_b3
@d tfm_uquad== {|tfm_b0..tfm_b3| as unsigned integer}
(((tfm_b0*256+tfm_b1)*256+tfm_b2)*256+tfm_b3)
@y
@d tfm_b01(#)== {|tfm_b0..tfm_b1| as non-negative integer}
if tfm_b0>127 then bad_font
else #:=tfm_b0*toint(256)+tfm_b1
@d tfm_b23(#)== {|tfm_b2..tfm_b3| as non-negative integer}
if tfm_b2>127 then bad_font
else #:=tfm_b2*toint(256)+tfm_b3
@d tfm_squad(#)== {|tfm_b0..tfm_b3| as signed integer}
if tfm_b0<128
then #:=((tfm_b0*toint(256)+tfm_b1)*toint(256)+tfm_b2)*toint(256)+tfm_b3
else #:=(((tfm_b0-toint(256))*toint(256)+tfm_b1)
        *toint(256)+tfm_b2)*toint(256)+tfm_b3
@d tfm_uquad== {|tfm_b0..tfm_b3| as unsigned integer}
(((tfm_b0*toint(256)+tfm_b1)*toint(256)+tfm_b2)*toint(256)+tfm_b3)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [109] Declare real_name_of_file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
The program uses the binary file variable |dvi_file| for its main input
file; |dvi_loc| is the number of the byte about to be read next from
|dvi_file|.

@<Glob...@>=
@!dvi_file:byte_file; {the stuff we are \.{\title}ing}
@!dvi_loc:int_32; {where we are about to look, in |dvi_file|}
@y
The program uses the binary file variable |dvi_file| for its main input
file; |dvi_loc| is the number of the byte about to be read next from
|dvi_file|.
In C, we also have a |real_name_of_file| string, that gets
set by the external |test_access| procedure after path searching.

@<Glob...@>=
@!dvi_file:byte_file; {the stuff we are \.{\title}ing}
@!dvi_loc:int_32; {where we are about to look, in |dvi_file|}
@!real_name_of_file:packed array[0..name_length] of text_char;
i:integer; {loop control}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [111] Fix up opening the binary files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare |dvi_file| for input, we |reset| it.

@<Open input file(s)@>=
reset(dvi_file); {prepares to read packed bytes from |dvi_file|}
dvi_loc:=0;
@y
@ To prepare |dvi_file| for input, we |reset| it.

@<Open input file(s)@>=
argv(toint(1), cur_name);
reset(dvi_file, cur_name);
dvi_loc:=0;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [113] Make dvi_length() and dvi_move() work.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function dvi_length:int_32;
begin set_pos(dvi_file,-1); dvi_length:=cur_pos(dvi_file);
end;
@#
procedure dvi_move(@!n:int_32);
begin set_pos(dvi_file,n); dvi_loc:=n;
end;
@y
@p function dvi_length:int_32;
begin checked_fseek(dvi_file, 0, 2);
dvi_loc:=ftell(dvi_file);
dvi_length:=dvi_loc;
end;
@#
procedure dvi_move(n:int_32);
begin checked_fseek(dvi_file, n, 0);
dvi_loc:=n;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [135] File names in lower case
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
id3(".")("V")("F")(vf_ext); {file name extension for \.{VF} files}
@y
id3(".")("v")("f")(vf_ext); {file name extension for \.{VF} files}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [137/138] Set default directory name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If no font directory has been specified, \.{\title} is supposed to use
the default \.{VF} directory, which is a system-dependent place where
the \.{VF} files for standard fonts are kept.
The string variable |VF_default_area| contains the name of this area.
@^system dependencies@>
 
@d VF_default_area_name=='TeXvfonts:' {change this to the correct name}
@d VF_default_area_name_length=10 {change this to the correct length}

@<Glob...@>=
@!VF_default_area:packed array[1..VF_default_area_name_length] of char;

@ @<Set init...@>=
VF_default_area:=VF_default_area_name;
@y
@ If no font directory has been specified, \.{\title} is supposed to use
the default \.{VF} directory, which is a system-dependent place where
the \.{VF} files for standard fonts are kept.
 
Actually, under UNIX the standard area is defined in an external
file \.{site.h}.  And the users have a path searched for fonts,
by setting the \.{VFFONTS} environment variable.

@ (No initialization to be done.  Keep this module to preserve numbering.)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [139] Open VF file
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<VF: Open |vf_file| or |goto not_found|@>=
make_font_name(VF_default_area_name_length)(VF_default_area)(vf_ext);
reset(vf_file,cur_name);
if eof(vf_file) then
@^system dependencies@>
  goto not_found;
vf_loc:=0
@y
In C, we use the external |test_access| procedure, which also does path
searching based on the user's environment or the default path.
Note that |VF_default_area_name_length| and |VF_default_area| will not
be used by |make_font_name|.

@<VF: Open |vf_file| or |goto not_found|@>=
make_font_name(VF_default_area_name_length)(VF_default_area)(vf_ext);
if test_read_access(cur_name,VF_FILE_PATH) then begin
    reset(vf_file,cur_name);
end else goto not_found;
vf_loc:=0
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [163] copy elements of array piece by piece
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<VF: Start a new level@>=
append_one(push);
vf_move[vf_ptr]:=vf_move[vf_ptr-1];
@y
@ \.{web2c} does not like array assignments. So we need to do them
through a macro replacement.

@d do_vf_move(#) == vf_move[vf_ptr]# := vf_move[vf_ptr-1]#
@d vf_move_assign == begin do_vf_move([0][0]); do_vf_move([0][1]);
			   do_vf_move([1][0]); do_vf_move([1][1])
		     end

@<VF: Start a new level@>=
append_one(push);
vf_move_assign;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [170] and again...
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  vf_move[vf_ptr]:=vf_move[vf_ptr-1];
@y
  vf_move_assign;
@z


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [175]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d update_terminal == break(output) {empty the terminal output buffer}
@y
@d update_terminal == flush(stdout) {empty the terminal output buffer}
@d input == stdin {standard input}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [175]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
procedure input_ln; {inputs a line from the terminal}
var k:0..terminal_line_length;
begin print('Enter option: '); update_terminal; reset(input);
if eoln(input) then read_ln(input);
k:=0; pckt_room(terminal_line_length);
while (k<terminal_line_length)and not eoln(input) do
  begin append_byte(xord[input^]); incr(k); get(input);
  end;
end;
@y
procedure input_ln; {inputs a line from the terminal}
var k:0..terminal_line_length;
begin print('Enter option: '); update_terminal;
{|if eoln(input) then read_ln(input);|}
k:=0; pckt_room(terminal_line_length);
while (k<terminal_line_length)and not eoln(input) do
  begin append_byte(xord[getc(input)]); incr(k);
  end;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [179] update read pointer within loop
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  else  begin print_ln('Valid options are:'); @<Print valid options@>@;
    end;
@y
  else  begin print_ln('Valid options are:'); @<Print valid options@>@;
    end;
  if eoln(input) then read_ln(input); 
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [231] Check usage; remove unused label
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p begin initialize; {get all variables initialized}
@<Initialize predefined strings@>@;
@y
@p begin
set_paths (TFM_FILE_PATH_BIT + VF_FILE_PATH_BIT);
initialize; {get all variables initialized}
if argc <> 3 then
  begin write_ln('Usage: dvicopy <dvi file> <outfile>.'); 
  uexit(1);
  end;
@<Initialize predefined strings@>@;
@z

@x
final_end:end.
@y
end.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [234] Declare DVI filename
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!out_file:byte_file; {the \.{DVI} file we are writing}
@y
@!out_file:byte_file; {the \.{DVI} file we are writing}
@!outf_name:packed array[1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [236] Get DVI filename
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Open output file(s)@>=
rewrite(out_file); {prepares to write packed bytes to |out_file|}
@y
@<Open output file(s)@>=
argv(toint(2), outf_name);
rewrite(out_file, outf_name); {prepares to write packed bytes to |out_file|}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [238] Fix up output of bytes.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ Writing the |out_file| should be done as efficient as possible for a
particular system; on many systems this means that a large number of
bytes will be accumulated in a buffer and is then written from that
buffer to |out_file|. In order to simplify such system dependent changes
we use the \.{WEB} macro |out_byte| to write the next \.{DVI} byte. Here
we give a simple minded definition for this macro in terms of standard
\PASCAL.
@^system dependencies@>
@^optimization@>

@d out_byte(#) == write(out_file,#) {write next \.{DVI} byte}
@y
@ Writing the |out_file| should be done as efficient as possible;
on many systems this means that a large number of
bytes will be accumulated in a buffer and is then written from that
buffer to |out_file|.
Under UNIX we assume that using the buffered I/O will be sufficiently
efficient. Thus we map |out_byte| to |put_byte|, which is defined
externally.
@^optimization@>

@d out_byte(#) == put_byte(#,out_file) {write next \.{DVI} byte}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [250] Initialization of the out_pre_string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!comment:packed array[1..comm_length] of char; {preamble comment prefix}
@y
@!comment:c_char_pointer; {preamble comment prefix}
@z

