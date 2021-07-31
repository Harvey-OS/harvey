% patgen2.ch for C compilation with web2c.
% 
% The original version of this file was created by Howard Trickey.
%
% 07/01/83 (HWT) Original version, made to work with patgen released with
%		 version 0.99 of TeX in July 1983.  It may not work
%		 properly---it is hard to test without more information.
% 03/23/88 (ETM) Brought up to date, converted for use with WEB to C.
% (more recent changes in ../ChangeLog.W2C and ./ChangeLog)



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{PATGEN changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Change banner string.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d banner=='This is PATGEN, Version 2.0' {printed when the program starts}
@y
@d banner=='This is PATGEN, C Version 2.0' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Terminal I/O
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d print(#)==write(output,#)
@d print_ln(#)==write_ln(output,#)
@d get_input(#)==read(input,#)
@d get_input_ln(#)==
  begin if eoln(input) then read_ln(input);
  read(input,#);
  end
@#
@y
@d print(#)==write(std_output,#)
@d print_ln(#)==writeln(std_output,#)
@d get_input(#)==#:=input_int(std_input)
@d get_input_ln(#)==begin #:=getc(std_input); readln(std_input); end @#
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Change files in program statement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p @<Compiler directives@>@/
program PATGEN(@!dictionary,@!patterns,@!translate,@!patout);
@y
@d std_input==stdin
@d std_output==stdout

@p @<Compiler directives@>@/
program PATGEN;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Add file opening to initialization
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  begin print_ln(banner);@/
@y
  begin @<Open files@>@/
  print_ln(banner);@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Error handling
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d error(#)==begin print_ln(#); jump_out; end
@y
@d error(#)==begin print_ln(#); uexit(1); end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Compiler directives
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@{@&$C-,A+,D-@} {no range check, catch arithmetic overflow, no debug overhead}
@y
{none needed for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix signed char problem
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!text_char=char; {the data type of characters in text files}
@!ASCII_code=0..last_ASCII_code; {internal representation of input characters}
@y
@!ASCII_code=0..last_ASCII_code; {internal representation of input characters}
@!text_char=ASCII_code; {the data type of characters in text files}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Remove file close
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d close_out(#)==close(#) {close an output file}
@d close_in(#)==do_nothing {close an input file}
@y
@d close_out(#)== {close an output file}
@d close_in(#)== {close an input file}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Add f_name declaration
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!dictionary, @!patterns, @!translate, @!patout, @!pattmp: text_file;
@y
@!dictionary, @!patterns, @!translate, @!patout, @!pattmp: text_file;
@!f_name: packed array [1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Use args to get translate file name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
reset(translate);
@y
argv(4, f_name);
reset(translate, f_name);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Floating point output
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  print_ln(', efficiency = ',
    good_count/(good_pat_count+bad_count/bad_eff):1:2)
@y
  begin print(', efficiency = ');
    print_real(good_count/(good_pat_count+bad_count/bad_eff),1,2);
    writeln(std_output);
  end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Use args to get dictionary file name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  reset(dictionary);@/
@y
argv(1, f_name);
reset(dictionary, f_name);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix file name initialization and printing
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
    begin filnam:='pattmp. ';
@y
    begin  filnam[1]:='p';
      filnam[2]:='a';
      filnam[3]:='t';
      filnam[4]:='t';
      filnam[5]:='m';
      filnam[6]:='p';
      filnam[7]:='.';
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix another floating point i/o problem
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  if (good_count+miss_count)>0 then
    print_ln((100*good_count/(good_count+miss_count)):1:2,' %, ',
      (100*bad_count/(good_count+miss_count)):1:2,' %, ',
      (100*miss_count/(good_count+miss_count)):1:2,' %');
@y
  if (good_count+miss_count)>0 then
  begin print_real((100*good_count/(good_count+miss_count)),1,2);
    print(' %, ');
    print_real((100*bad_count/(good_count+miss_count)),1,2);
    print(' %, ');
    print_real((100*miss_count/(good_count+miss_count)),1,2);
    print_ln(' %');
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Use the command line argument to get the file name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
reset(patterns);
@y
argv(2, f_name);
reset(patterns, f_name);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix reading of multiple variables in the same line
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
repeat print('hyph_start, hyph_finish: '); get_input(n1,n2);@/
@y
repeat print('hyph_start, hyph_finish: '); input_2ints(n1,n2);@/
@z
@x
  repeat print('pat_start, pat_finish: '); get_input(n1,n2);@/
@y
  repeat print('pat_start, pat_finish: '); input_2ints(n1,n2);@/
@z
@x
  get_input(good_wt,bad_wt,thresh);@/
@y
  input_3ints(good_wt,bad_wt,thresh);@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Use args to get patout file name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
rewrite(patout);
@y
argv(3, f_name);
rewrite(patout, f_name);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Define <Open files>
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@* System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{PATGEN} work at a particular installation.
It is usually best to design your change file so that all changes to
previous sections preserve the section numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new sections, can be inserted here; then only the index
itself will get a new section number.
@^system dependencies@>
@y
@* System-dependent changes.
Use the command line arguments to get the file names.  Open the
output file here; the others are reset above.

@<Open files@>=
if argc < 4 then begin
    error('Usage: patgen <dictionary file> <pattern file> <output file>',
          ' <translate file>')
end;
@z
