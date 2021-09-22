% gftype.ch for C compilation with web2c.
%
% 09/27/88 Pierre A. MacKay	version 2.2.
% 11/10/88 (PAM) Bugs reported by Irwin Meisels.
%          Corrected floating-point printout, and restored options
%          info printout to make trap look better.
% 12/02/89 Karl Berry		version 3.
% (more recent changes in the ChangeLog)
% 
% The C version of GFtype uses command line switches to 
% request mnemonic ouput or pixel image output.
% The default is a restrained output which merely assures you
% that all characters are there and reports their position, tfm_width
% and escapement.  The -m switch turns on mnemonics, the -i switch
% turns on images.  There is no terminal input to this program.  
% Output is to stdout, and may, of course, be redirected.

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{GF$\,$\lowercase{type} changes for C}
@z

@x [3] No global labels.
@d print(#)==write(#)
@d print_ln(#)==write_ln(#)
@d print_nl==write_ln

@p program GF_type(@!gf_file,@!output);
label @<Labels in the outer block@>@/
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin print_ln(banner);@/
@y
@d print(#)==write(stdout, #)
@d print_ln(#)==write_ln(stdout, #)
@d print_nl==write_ln(stdout)

@p program GF_type(@!gf_file,@!output);
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
begin
  kpse_set_progname (argv[0]);
  kpse_init_prog ('GFTYPE', 0, nil, nil);
  parse_arguments;
  print(banner);print_ln(version_string);
@z

@x [4] Eliminate the |final_end| label.
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ This module is deleted, because it is only useful for
a non-local goto, which we can't use in C.
@z

@x [5] Remove |terminal_line_length|.
@<Constants...@>=
@!terminal_line_length=150; {maximum number of characters input in a single
  line of input from the terminal}
@y
@<Constants...@>=
@z

@x [7] Remove jump_out, and make `abort' end with a newline.
@d abort(#)==begin print(' ',#); jump_out;
    end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>

@p procedure jump_out;
begin goto final_end;
end;
@y
@d abort(#)==begin write_ln(stderr, #); uexit(1); end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>
@z

@x [22] Redo open_gf_file to do path searching.
@ To prepare this file for input, we |reset| it.

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin reset(gf_file);
cur_loc:=0;
end;
@y

@ In C, we do path searching based on the user's environment or the
default path.

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin
  gf_file := kpse_open_file (cmdline (optind), kpse_gf_format);
  cur_loc := 0;
  @<Print all the selected options@>;
end;
@z

@x [25] Since we set these from getopt, they have to be ints, more's the pity.
@!wants_mnemonics: boolean; {controls mnemonic output}
@!wants_pixels: boolean; {controls pixel output}
@y
@!wants_mnemonics: c_int_type; {controls mnemonic output}
@!wants_pixels: c_int_type; {controls pixel output}
@z

% [26] Initialize wants_pixels and wants_mnemonics to false implicitly
% by virtue of being global variables. Can't do it here, since this is
% after the option parsing.
@x
wants_mnemonics:=true; wants_pixels:=true;
@y
@z

@x [27] No input, and no dialog.
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
@ There is no terminal input.  The options for running this
program are offered through command line options.
@z

@x [29] `input_ln' is not needed.
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

@x [30--34] Eliminate `dialog' and its friends.
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

@x [45] Change chr to xchr (should be changed in the web source).
  print(chr(ord('0')+(s div unity))); s:=10*(s mod unity); delta:=delta*10;
@y
  print(xchr[ord('0')+(s div unity)]); s:=10*(s mod unity); delta:=delta*10;
@z

@x [48] Break up the first oversized case statement (or yacc breaks).
sixty_four_cases(new_row_0), sixty_four_cases(new_row_0+64),
  thirty_seven_cases(new_row_0+128): first_par:=o-new_row_0;
@y
sixty_four_cases(new_row_0): first_par:=o-new_row_0;
sixty_four_cases(new_row_0+64): first_par:=o-new_row_0;
thirty_seven_cases(new_row_0+128): first_par:=o-new_row_0;
@z

@x [64] Break up the second oversized case statement.
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

@x [65] No label and no dialog; finish last line written. 
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

@x System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{GFtype} work at a particular installation.
It is usually best to design your change file so that all changes to
previous sections preserve the section numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new sections, can be inserted here; then only the index
itself will get a new section number.
@^system dependencies@>
@y
Parse a Unix-style command line.

@d argument_is (#) == (strcmp (long_options[option_index].name, #) = 0)

@<Define |parse_arguments|@> =
procedure parse_arguments;
const n_options = 4; {Pascal won't count array lengths for us.}
var @!long_options: array[0..n_options] of getopt_struct;
    @!getopt_return_val: integer;
    @!option_index: c_int_type;
    @!current_option: 0..n_options;
begin
  @<Define the option table@>;
  repeat
    getopt_return_val := getopt_long_only (argc, argv, '', long_options,
                                           address_of (option_index));
    if getopt_return_val = -1 then begin
      {End of arguments; we exit the loop below.} ;
    
    end else if getopt_return_val = "?" then begin
      usage (1, 'gftype');

    end else if argument_is ('help') then begin
      usage (0, GFTYPE_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.R. Fuchs');

    end; {Else it was a flag.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one remaining argument.}
  if (optind + 1 <> argc) then begin
    write_ln (stderr, 'gftype: Need exactly one file argument.');
    usage (1, 'gftype');
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

@ Translate commands?
@<Define the option...@> =
long_options[current_option].name := 'mnemonics';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := address_of (wants_mnemonics);
long_options[current_option].val := 1;
incr (current_option);

@ Show pixels?
@<Define the option...@> =
long_options[current_option].name := 'images';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := address_of (wants_pixels);
long_options[current_option].val := 1;
incr (current_option);

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
@z
