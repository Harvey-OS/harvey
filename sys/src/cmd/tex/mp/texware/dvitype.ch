% dvitype.ch for C compilation with web2c.
% 
% The original version of this file was created by Howard Trickey, and
% modified by Pavel Curtis.
%
% History:
%  04/04/83 (PC)  Merged with Pavel's change file and made to work with the
%                 version 1.0 of DVItype released with version 0.95 of TeX in
%                 February, 1983.
%  04/18/83 (PC)  Added changes to module 47 so that it would work the same
%                 when input was a file (or pipe) as with a terminal.
%  06/29/83 (HWT) Brought up to version 1.1 as released with version 0.99 of
%	          TeX, with new change file format
%  07/28/83 (HWT) Brought up to version 2 as released with version 0.999.
%		  Only the banner changes.
%  11/21/83 (HWT) Brought up to version 2.2 as released with version 1.0.
%  02/19/84 (HWT) Made it use TEXFONTS environment.
%  03/23/84 (HWT) Brought up to version 2.3.
%  07/11/84 (HWT) Brought up to version 2.6 as released with version 1.1.
%  11/07/84 (ETM) Brought up to version 2.7 as released with version 1.2.
%  03/09/88 (ETM) Brought up to version 2.9
%  03/16/88 (ETM) Converted for use with WEB to C.
%  11/30/89 (KB)  To version 3.
%  01/16/90 (SR)  To version 3.2.
% 


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{DVI$\,$\lowercase{type} changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is DVItype, Version 3.2' {printed when the program starts}
@y
@d banner=='This is DVItype, C Version 3.2' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3] Change filenames in program statement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d print(#)==write(#)
@d print_ln(#)==write_ln(#)

@p program DVI_type(@!dvi_file,@!output);
@y
@d print(#)==write(dvityout,#)
@d print_ln(#)==write_ln(dvityout,#)

@p program DVI_type;
@z

@x
  begin print_ln(banner);@/
@y
  begin
  setpaths; {read environment, to find \.{TEXFONTS}, if there}
  rewrite(dvityout,' dvitype.out'); {prepare typescript for output}
  print_ln(banner);@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [5] Make name_length match FILENAMESIZE in site.h.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Constants...@>=
@y
@d name_length==FILENAMESIZE
@<Constants...@>=
@z
@x
@!name_length=50; {a file name shouldn't be longer than this}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Remove non-local goto
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If the \.{DVI} file is badly malformed, the whole process must be aborted;
\.{DVItype} will give up, after issuing an error message about the symptoms
that were noticed.

Such errors might be discovered inside of subroutines inside of subroutines,
so a procedure called |jump_out| has been introduced. This procedure, which
simply transfers control to the label |final_end| at the end of the program,
contains the only non-local |goto| statement in \.{DVItype}.
@^system dependencies@>

@d abort(#)==begin print(' ',#); jump_out;
    end
@d bad_dvi(#)==abort('Bad DVI file: ',#,'!')
@.Bad DVI file@>

@p procedure jump_out;
begin goto final_end;
end;
@y
@ If the \.{DVI} file is badly malformed, the whole process must be aborted;
\.{DVItype} will give up, after issuing an error message about the symptoms
that were noticed.

Such errors might be discovered inside of subroutines inside of subroutines,
so a procedure called |jump_out| has been introduced. This procedure is
actually a macro which calls exit() with a non-zero exit status.

@d jump_out==uexit(1)
@d abort(#)==begin write_ln(term_out,#); jump_out;
    end
@d bad_dvi(#)==abort('Bad DVI file: ',#,'!')
@.Bad DVI file@>
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Permissive input.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@!ASCII_code=" ".."~"; {a subrange of the integers}
@y
@!ASCII_code=0..255; {a subrange of the integers}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] The text_char type is used as an array index into xord.  The
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
% [23] Fix up opening the binary files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p procedure open_dvi_file; {prepares to read packed bytes in |dvi_file|}
begin reset(dvi_file);
cur_loc:=0;
end;
@#
procedure open_tfm_file; {prepares to read packed bytes in |tfm_file|}
begin reset(tfm_file,cur_name);
end;
@y
In C, we use the external |test_access| procedure, which also does path
searching based on the user's environment or the default path.

@d read_access_mode=4  {``read'' mode for |test_access|}
@d write_access_mode=2 {``write'' mode for |test_access|}

@d no_file_path=0    {no path searching should be done}
@d font_file_path=3  {path specifier for \.{TFM} files}

@p procedure open_dvi_file; {prepares to read packed bytes in |dvi_file|}
var i:integer;
begin
    argv(1, cur_name);
    if test_access(read_access_mode,no_file_path) then begin
        for i:=1 to FILENAMESIZE do
            cur_name[i]:=real_name_of_file[i-1];
        reset(dvi_file, cur_name);
    end else begin
        i := 1;
        while (i < name_length) and (cur_name[i] <> ' ') do
           incr(i);
	print_string(cur_name, i-1);
        abort(': DVI file not found.');
    end;
    cur_loc:=0;
end;
@#
procedure open_tfm_file; {prepares to read packed bytes in |tfm_file|}
var i:integer;
begin
if test_access(read_access_mode,font_file_path) then begin
    for i:=1 to FILENAMESIZE do
        cur_name[i]:=real_name_of_file[i-1];
    reset(tfm_file,cur_name);
end else begin
    i := 1;
    while (i < name_length) and (cur_name[i] <> ' ') do
        incr(i);
    print_string(cur_name, i-1);
    abort(': TFM file not found.');
end;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [25] Declare real_name_of_file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
|dvi_file|, and |cur_name| is a string variable that will be set to the
current font metric file name before |open_tfm_file| is called.

@<Glob...@>=
@!cur_loc:integer; {where we are about to look, in |dvi_file|}
@!cur_name:packed array[1..name_length] of char; {external name,
  with no lower case letters}
@y
|dvi_file|, and |cur_name| is a string variable that will be set to the
current font metric file name before |open_tfm_file| is called.
In C, we also have a |real_name_of_file| string, that gets
set by the external |test_access| procedure after path searching.

@<Glob...@>=
@!cur_loc:integer; {where we are about to look, in |dvi_file|}
@!cur_name:packed array[1..name_length] of char;
@!real_name_of_file:packed array[0..name_length] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [27] Fix bugs in get_n_bytes routines so they work with 16-bit math.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
get_two_bytes:=a*256+b;
@y
get_two_bytes:=a*toint(256)+b;
@z
@x
get_three_bytes:=(a*256+b)*256+c;
@y
get_three_bytes:=(a*toint(256)+b)*256+c;
@z
@x
if a<128 then signed_trio:=(a*256+b)*256+c
else signed_trio:=((a-256)*256+b)*256+c;
@y
if a<128 then signed_trio:=(a*toint(256)+b)*256+c
else signed_trio:=((a-toint(256))*256+b)*256+c;
@z
@x
if a<128 then signed_quad:=((a*256+b)*256+c)*256+d
else signed_quad:=(((a-256)*256+b)*256+c)*256+d;
@y
if a<128 then signed_quad:=((a*toint(256)+b)*256+c)*256+d
else signed_quad:=(((a-256)*toint(256)+b)*256+c)*256+d;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [28] Make dvi_length() and move_to_byte() work.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function dvi_length:integer;
begin set_pos(dvi_file,-1); dvi_length:=cur_pos(dvi_file);
end;
@#
procedure move_to_byte(n:integer);
begin set_pos(dvi_file,n); cur_loc:=n;
end;
@y
@p function dvi_length:integer;
begin zfseek(dvi_file, 0, 2);
cur_loc:=ftell(dvi_file);
dvi_length:=cur_loc;
end;
@#
procedure move_to_byte(n:integer);
begin zfseek(dvi_file, n, 0); cur_loc:=n;
cur_loc:=n;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [34] Fix 16-bit bugs in TFM calculations.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
read_tfm_word; lh:=b2*256+b3;
read_tfm_word; font_bc[nf]:=b0*256+b1; font_ec[nf]:=b2*256+b3;
@y
read_tfm_word; lh:=b2*toint(256)+b3;
read_tfm_word; font_bc[nf]:=b0*toint(256)+b1; font_ec[nf]:=b2*toint(256)+b3;
@z
@x
    if b0<128 then tfm_check_sum:=((b0*256+b1)*256+b2)*256+b3
    else tfm_check_sum:=(((b0-256)*256+b1)*256+b2)*256+b3
@y
    if b0<128 then tfm_check_sum:=((b0*toint(256)+b1)*256+b2)*256+b3
    else tfm_check_sum:=(((b0-256)*toint(256)+b1)*256+b2)*256+b3
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [45] Define term_in and term_out and declare dvityout
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
and |term_out| for terminal output.
@^system dependencies@>

@<Glob...@>=
@!buffer:array[0..terminal_line_length] of ASCII_code;
@!term_in:text_file; {the terminal, considered as an input file}
@!term_out:text_file; {the terminal, considered as an output file}
@y
and |term_out| for terminal output.
@^system dependencies@>

@d term_in==stdin
@d term_out==stdout

@<Glob...@>=
@!buffer:array[0..terminal_line_length] of ASCII_code;
@!dvityout:text;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [46] Define update_terminal
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d update_terminal == break(term_out) {empty the terminal output buffer}
@y
@d update_terminal == flush(term_out) {empty the terminal output buffer}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [47] Fix input_ln.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p procedure input_ln; {inputs a line from the terminal}
var k:0..terminal_line_length;
begin update_terminal; reset(term_in);
if eoln(term_in) then read_ln(term_in);
k:=0;
while (k<terminal_line_length)and not eoln(term_in) do
  begin buffer[k]:=xord[term_in^]; incr(k); get(term_in);
  end;
buffer[k]:=" ";
end;
@y
@p procedure input_ln; {inputs a line from the terminal}
var k:0..terminal_line_length;
begin update_terminal;
if eoln(term_in) then read_ln(term_in);
k:=0;
while (k<terminal_line_length)and not eoln(term_in) do
  begin buffer[k]:=xord[getc(term_in)]; incr(k);
  end;
buffer[k]:=" ";
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [50] Remove call to rewrite(term_out)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
begin rewrite(term_out); {prepare the terminal for output}
@y
begin
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Fix printing of floating point number
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
print_ln('  Resolution = ',resolution:12:8,' pixels per inch');
if new_mag>0 then print_ln('  New magnification factor = ',new_mag/1000:8:3)
@y
print('  Resolution = ');
dvi_print_real(resolution,12,8);
print_ln(' pixels per inch');
if new_mag>0 then begin
  print('  New magnification factor = ');
  dvi_print_real(new_mag/1000.0,8,3);
  print_ln('')
end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [64] Set default_directory_name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d default_directory_name=='TeXfonts:' {change this to the correct name}
@d default_directory_name_length=9 {change this to the correct length}

@<Glob...@>=
@!default_directory:packed array[1..default_directory_name_length] of char;
@y
Actually, under UNIX the standard area is defined in an external
file \.{site.h}.  And the users have a path searched for fonts,
by setting the \.{TEXFONTS} environment variable.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [65] Remove initialization of now-defunct array
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
default_directory:=default_directory_name;
@y
@ (No initialization to be done.  Keep this module to preserve numbering.)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [66] Fix addition of ".tfm" suffix for portability and keep lowercase
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The string |cur_name| is supposed to be set to the external name of the
\.{TFM} file for the current font. This usually means that we need to
prepend the name of the default directory, and
to append the suffix `\.{.TFM}'. Furthermore, we change lower case letters
to upper case, since |cur_name| is a \PASCAL\ string.
@y
@ The string |cur_name| is supposed to be set to the external name of the
\.{TFM} file for the current font. This usually means that we need to,
at most sites, append the
suffix ``.tfm''. 
@z

@x
if p=0 then
  begin for k:=1 to default_directory_name_length do
    cur_name[k]:=default_directory[k];
  r:=default_directory_name_length;
  end
else r:=0;
@y
r:=0;
@z

@x
  if (names[k]>="a")and(names[k]<="z") then
      cur_name[r]:=xchr[names[k]-@'40]
  else cur_name[r]:=xchr[names[k]];
  end;
cur_name[r+1]:='.'; cur_name[r+2]:='T'; cur_name[r+3]:='F'; cur_name[r+4]:='M'
@y
  cur_name[r]:=xchr[names[k]];
  end;
cur_name[r+1]:='.'; cur_name[r+2]:='t'; cur_name[r+3]:='f'; cur_name[r+4]:='m'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [106] Check usage; print newline at end of program; remove unused label
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
dialog; {set up all the options}
@y
if argc <> 2 then
  begin write_ln('Usage: dvitype <DVI file>.'); jump_out;
  end;
dialog; {set up all the options}
@z

@x
final_end:end.
@y
print_ln(' '); end.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix another floating point print
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
print_ln('magnification=',mag:1,'; ',conv:16:8,' pixels per DVI unit')
@y
print('magnification=',mag:1,'; ');
dvi_print_real(conv,16,8);
print_ln(' pixels per DVI unit')
@z
