% gftype.ch for C compilation with web2c.
%
% 09/27/88 Pierre A. MacKay	version 2.2.
% 11/10/88 (PAM) Bugs reported by Irwin Meisels.
%          Corrected floating-point printout, and restored options
%          info printout to make trap look better.
% 12/02/89 Karl Berry		version 3.
% 
% The C version of GFtype uses command line switches to 
% request mnemonic ouput or pixel image output.
% The default is a restrained output which merely assures you
% that all characters are there and reports their position, tfm_width
% and escapement.  The -m switch turns on mnemonics, the -i switch
% turns on images.  There is no terminal input to this program.  
% Output is to stdout, and may, of course, be redirected.
% 


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{GF$\,$\lowercase{type} changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is GFtype, Version 3' {printed when the program starts}
@y
@d banner=='This is GFtype, C Version 3'
                                         {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3] Redirect GFtype output to term_out
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d print(#)==write(#)
@d print_ln(#)==write_ln(#)
@d print_nl==write_ln
@y
@d term_out==stdout
@d print(#)==write(term_out, #)
@d print_ln(#)==write_ln(term_out, #)
@d print_nl==write_ln(term_out)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [3] Remove IO from program header, eliminate labels, and
%	and add setpaths.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p program GF_type(@!gf_file,@!output);
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
@p program GF_type;
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin
  setpaths;@/
  print_ln(banner);@/
  @<Set initial values@>@/
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] Eliminate the |final_end| label
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ this module is deleted, because it is only useful for
a non-local goto, which we don't use in C.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [5] remove |terminal_line_length|, and define |max_int| as a large number.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Constants...@>=
@!terminal_line_length=150; {maximum number of characters input in a single
  line of input from the terminal}
@!line_length=79; {\\{xxx} strings will not produce lines longer than this}
@!max_row=79; {vertical extent of pixel image array}
@!max_col=79; {horizontal extent of pixel image array}
@y
@d max_int==536870911

@<Constants...@>=
@!line_length=79; {\\{xxx} strings will not produce lines longer than this}
@!max_row=79; {vertical extent of pixel image array}
@!max_col=79; {horizontal extent of pixel image array}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [7] Have abort append <nl> to end of msg
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d abort(#)==begin print(' ',#); jump_out;
    end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>

@p procedure jump_out;
begin goto final_end;
end;
@y
@d abort(#)==begin print_ln(' ',#); uexit(1);
    end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [20] Add UNIX_file_name type.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!eight_bits=0..255; {unsigned one-byte quantity}
@!byte_file=packed file of eight_bits; {files that contain binary data}
@y
@!eight_bits=0..255; {unsigned one-byte quantity}
@!byte_file=packed file of eight_bits; {files that contain binary data}
@!UNIX_file_name=packed array[1..FILENAMESIZE] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [21] Add gf_name, cur_name, and real_name_of_file 
%      to global vars; also, a boolean, gf_file_exists.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The program deals with one binary file variable: |gf_file| is the main
input file that we are translating into symbolic form.

@<Glob...@>=
@!gf_file:byte_file; {the stuff we are \.{GF}typing}
@y
@ The program deals with one binary file variable: |gf_file| is the main
input file that we are translating into symbolic form.
We also declare array |gf_name|, used to hold the name of |gf_file|
and |cur_name| and |real_name_of_file|, to interact with |test_access|.
The boolean |gf_file_exists| tells us just what you think it would.

@<Glob...@>=
@!gf_file:byte_file; {the stuff we are \.{GF}typing}
@!gf_name,@!cur_name: UNIX_file_name;
	{name of input file; pascal-style origin from one}
@!real_name_of_file:packed array[0..FILENAMESIZE] of text_char;
	{C style origin from zero}
gf_file_exists:boolean;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [22] redo open_gf_file
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare this file for input, we |reset| it.

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin reset(gf_file);
cur_loc:=0;
end;
@y
@ In C, we use the external |test_access| procedure, which also does path
searching based on the user's environment or the default path.  In the course
of this routine we also check the command line for the \.{-m} 
(|wants_mnemonics|) flag and the [-i] wants_pixels flag. and make
other checks to see that it is worth running this program at all.

@d read_access_mode=4  {``read'' mode for |test_access|}
@d write_access_mode=2 {``write'' mode for |test_access|}

@d no_file_path=0    {no path searching should be done}
@d tex_font_file_path=3  {path specifier for \.{TFM} files}
@d generic_font_file_path=4  {path specifier for \.{GF} files}
@d packed_font_file_path=5  {path specifier for \.{PK} files}

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
var j,k:integer;
begin
    k:=1;
    if argc < 2 then abort('Usage: gftype [-m] [-i] <gf file>.');
    argv(k, cur_name);
    while cur_name[1]=xchr["-"] do begin
	incr(k);
        wants_mnemonics:=
         wants_mnemonics or (cur_name[2]=xchr["m"]) or (cur_name[3]=xchr["m"]);
        wants_pixels:=
         wants_pixels or (cur_name[2]=xchr["i"]) or (cur_name[3]=xchr["i"]);
	if wants_pixels or wants_mnemonics then argv(k, cur_name)
            else abort('Usage: gftype [-m] [-i] <gf file>.');
        end;
    if argc > 4 then abort('Usage: gftype [-m] [-i] <gf file>.');
    gf_file_exists := test_access(read_access_mode,generic_font_file_path);
    if gf_file_exists then begin
        for j:=1 to FILENAMESIZE do gf_name[j]:=real_name_of_file[j-1];
        reset(gf_file, gf_name);
        cur_loc:=0;
        end
    else abort('GF file not found');
@<Print all the selected options@>;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [26] Initialize wants_pixels and wants_mnemonics to false
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
wants_mnemonics:=true; wants_pixels:=true;
@y
wants_mnemonics:=false; wants_pixels:=false;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [27] No input, and no dialog.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The |input_ln| routine waits for the user to type a line at his or her
terminal; then it puts ASCII-code equivalents for the characters on that line
into the |buffer| array. The |term_in| file is used for terminal input,
and |term_out| for terminal output.
@^system dependencies@>

@<Glob...@>=
@!buffer:array[0..terminal_line_length] of ASCII_code;
@!term_in:text_file; {the terminal, considered as an input file}
@!term_out:text_file; {the terminal, considered as an output file}
@y
@ There is no terminal input.  The basic choices for running this
program are offered through switches on the command line.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [28] update_terminal is not needed
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d update_terminal == break(term_out) {empty the terminal output buffer}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [29] eliminate input_ln
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
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [30--34] eliminate dialog and its friends.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ This is humdrum.

@p function lower_casify(@!c:ASCII_code):ASCII_code;
begin
if (c>="A") and (c<="Z") then lower_casify:=c+"a"-"A"
else lower_casify:=c;
end;

@ The selected options are put into global variables by the |dialog|
procedure, which is called just as \.{GFtype} begins.
@^system dependencies@>

@p procedure dialog;
label 1,2;
begin rewrite(term_out); {prepare the terminal for output}
write_ln(term_out,banner);@/
@<Determine whether the user |wants_mnemonics|@>;
@<Determine whether the user |wants_pixels|@>;
@<Print all the selected options@>;
end;

@ @<Determine whether the user |wants_mnemonics|@>=
1: write(term_out,'Mnemonic output? (default=no, ? for help): ');
@.Mnemonic output?@>
input_ln;
buffer[0]:=lower_casify(buffer[0]);
if buffer[0]<>"?" then
  wants_mnemonics:=(buffer[0]="y")or(buffer[0]="1")or(buffer[0]="t")
else  begin write(term_out,'Type Y for complete listing,');
  write_ln(term_out,' N for errors/images only.');
  goto 1;
  end

@ @<Determine whether the user |wants_pixels|@>=
2: write(term_out,'Pixel output? (default=yes, ? for help): ');
@.Pixel output?@>
input_ln;
buffer[0]:=lower_casify(buffer[0]);
if buffer[0]<>"?" then
  wants_pixels:=(buffer[0]="y")or(buffer[0]="1")or(buffer[0]="t")
    or(buffer[0]=" ")
else  begin write(term_out,'Type Y to list characters pictorially');
  write_ln(term_out,' with *''s, N to omit this option.');
  goto 2;
  end

@ After the dialog is over, we print the options so that the user
can see what \.{GFtype} thought was specified.

@y
@ This was so humdrum that we got rid of it. (module 30)

@ The dialog procedure module is eliminated. (module 31)

@ So is its first part. (module 32)

@ So is its second part. (module 33)

@ After the command-line switches have been processed, 
we print the options so that the user
can see what \.{GFtype} thought was specified.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [45] change chr to xchr (ought really to be changed in the web)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  print(chr(ord('0')+(s div unity))); s:=10*(s mod unity); delta:=delta*10;
@y
  print(xchr[ord('0')+(s div unity)]); s:=10*(s mod unity); delta:=delta*10;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [48] break up the first oversized case statement (or yacc breaks)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
sixty_four_cases(new_row_0), sixty_four_cases(new_row_0+64),
  thirty_seven_cases(new_row_0+128): first_par:=o-new_row_0;
@y
sixty_four_cases(new_row_0): first_par:=o-new_row_0;
sixty_four_cases(new_row_0+64): first_par:=o-new_row_0;
thirty_seven_cases(new_row_0+128): first_par:=o-new_row_0;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [64] break up the second oversized case statement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
sixty_four_cases(new_row_0), sixty_four_cases(new_row_0+64),
 thirty_seven_cases(new_row_0+128):
  @<Translate a |new_row| command@>;
@t\4@>@<Cases for commands |no_op|, |pre|, |post|, |post_post|, |boc|,
  and |eoc|@>@;
four_cases(xxx1): @<Translate an |xxx| command@>;
yyy: @<Translate a |yyy| command@>;
othercases error('undefined command ',o:1,'!')
@.undefined command@>
endcases
@y
sixty_four_cases(new_row_0): @<Translate a |new_row| command@>;
sixty_four_cases(new_row_0+64): @<Translate a |new_row| command@>;
thirty_seven_cases(new_row_0+128): @<Translate a |new_row| command@>;
@t\4@>@<Cases for commands |no_op|, |pre|, |post|, |post_post|, |boc|,
  and |eoc|@>@;
four_cases(xxx1): @<Translate an |xxx| command@>;
yyy: @<Translate a |yyy| command@>;
othercases error('undefined command ',o:1,'!')
@.undefined command@>
endcases
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [65] no label and no dialog; finish last line written. 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p begin initialize; {get all variables initialized}
dialog; {set up all the options}
@<Process the preamble@>;
@<Translate all the characters@>;
print_nl;
read_postamble;
print('The file had ',total_chars:1,' character');
if total_chars<>1 then print('s');
print(' altogether.');
@.The file had n characters...@>
final_end:end.
@y
@p begin initialize; {get all variables initialized}
@<Process the preamble@>;
@<Translate all the characters@>;
print_nl;
read_postamble;
print('The file had ',total_chars:1,' character');
if total_chars<>1 then print('s');
print_ln(' altogether.');
@.The file had n characters...@>
end.
@z
