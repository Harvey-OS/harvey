% patgen.ch for C compilation with web2c.
% 
% The original version of this file was created by Howard Trickey.
%
% 07/01/83 (HWT) Original version, made to work with patgen released with
%		 version 0.99 of TeX in July 1983.  It may not work
%		 properly---it is hard to test without more information.
% 03/23/88 (ETM) Brought up to date, converted for use with WEB to C.
% (more recent changes in ../ChangeLog.W2C)



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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Change files in program statement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p @<Compiler directives@>@/
program PATGEN(@!dictionary,@!patterns,@!output);
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
  begin @<Set up input/output translation tables@>@/
@y
  begin @<Open files@>@/
  @<Set up input/output translation tables@>@/
@z
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Add f_name declaration
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!dictionary, @!patterns, @!pattmp: file of text_char;
@y
@!dictionary, @!patterns, @!pattmp, @!outfile: file of text_char;
@!f_name: packed array [1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Terminal I/O
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d print(#)==write(tty,#)
@d print_ln(#)==writeln(tty,#)
@d input(#)==read(tty,#)
@d input_ln(#)==begin if eoln(tty) then readln(tty); read(tty,#) end @#

@d error(#)==begin print_ln(#); goto end_of_PATGEN; end;
@y
@d print(#)==write(std_output,#)
@d print_ln(#)==writeln(std_output,#)
@d input(#)==#:=input_int(std_input)
@d input_ln(#)==begin #:=getc(std_input); readln(std_input); end @#

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
% Change write(...) to write(outfile,...)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  if hval[0]>0 then write(hval[0]:1);
  for d:=1 to pat_len do
  begin  if pat[d]=edge_of_word then write('.')
    else write(xchr[pat[d]]);
    if hval[d]>0 then write(hval[d]:1);
  end;
  writeln;
@y
  if hval[0]>0 then write(outfile,hval[0]:1);
  for d:=1 to pat_len do
  begin  if pat[d]=edge_of_word then write(outfile,'.')
    else write(outfile,xchr[pat[d]]);
    if hval[d]>0 then write(outfile,hval[d]:1);
  end;
  writeln(outfile);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Change readln of string into loop
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p procedure read_word;
var c: ascii_code;
begin  readln(dictionary,buf);
@y
@d read_to_buf(#)== buf_ptr:=1;
    while not eoln(#) and (buf_ptr<80) do begin
	buf[buf_ptr]:=getc(#); incr(buf_ptr);
	end;
    buf[buf_ptr]:=' '; readln(#)

@p procedure read_word;
var c: ascii_code;
    bptr: integer;
begin  read_to_buf(dictionary);
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
% Remove file close; fix another floating point i/o problem
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  print_ln((100*good_count/(good_count+miss_count)):1:2,' %, ',
    (100*bad_count/(good_count+miss_count)):1:2,' %, ',
    (100*miss_count/(good_count+miss_count)):1:2,' %');
  if procesp then
  print_ln(pat_count:1,' patterns, ',triec_count:1,
    ' nodes in count trie, ','triec_max = ',triec_max:1);
  if hyphp then close(pattmp);
@y
  print_real((100*good_count/(good_count+miss_count)),1,2);
  print(' %, ');
  print_real((100*bad_count/(good_count+miss_count)),1,2);
  print(' %, ');
  print_real((100*miss_count/(good_count+miss_count)),1,2);
  print_ln(' %');
  if procesp then
  print_ln(pat_count:1,' patterns, ',triec_count:1,
    ' nodes in count trie, ','triec_max = ',triec_max:1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Another readln change
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  reset(patterns);
  while not eof(patterns) do
  begin  readln(patterns,buf);
@y
  argv(2, f_name);
  reset(patterns, f_name);
  while not eof(patterns) do
  begin read_to_buf(patterns);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix reading of multiple variables in the same line
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  input(good_wt,bad_wt,thresh);
@y
  input_3ints(good_wt,bad_wt,thresh);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Define <Open files>
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@* Index.
@y
@ Use the command line arguments to get the file names.  Open the
output file here; the others are reset above.

@<Open files@>=
if argc < 4 then begin
    error('Usage: patgen <dictionary file> <pattern file> <output file>')
end;
argv(3, f_name);
rewrite(outfile, f_name);

@* Index.
@z
