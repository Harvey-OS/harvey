% dvitomp.ch for C compilation with web2c.
%
% Copyright 1990 - 1995 by AT&T Bell Laboratories.
%
% Permission to use, copy, modify, and distribute this software
% and its documentation for any purpose and without fee is hereby
% granted, provided that the above copyright notice appear in all
% copies and that both that the copyright notice and this
% permission notice and warranty disclaimer appear in supporting
% documentation, and that the names of AT&T Bell Laboratories or
% any of its entities not be used in advertising or publicity
% pertaining to distribution of the software without specific,
% written prior permission.
%
%   Change file for the DVItype processor, for use with WEB to C
%   This file was created by John Hobby.  It is loosely based on the
%   change file for the WEB to C version of dvitype (due to Howard
%   Trickey and Pavel Curtis).
%
%   3/11/90  (JDH) Original version.
%   4/30/90  (JDH) Update to handle virtual fonts
%   4/16/93  (JDH) Make output go to standard output and require mpx file
%                  to be a command line argument.
%
%   1/18/95  (UV)  Update based on dvitype.ch for web2c-6.1
%   4/13/95  (UV)  Cosmetic changes for release of web2c-mp
%  10/08/95  (UV)  Bug fix: need to replace abs() with floating-point arg 
%                  by fabs() because of different definition in cpascal.h
%                  as reported by Dane Dwyer <dwyer@geisel.csl.uiuc.edu>.

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{DVI$\,$\lowercase{to}MP changes for C}
@z

@x [1] Duplicate banner line for use in |print_version_and_exit|.
@d banner=='% Written by DVItoMP, Version 0.64'
  {the first line of the output file}
@y
@d banner=='% Written by DVItoMP, Version 0.64'
  {the first line of the output file}
@d term_banner=='This is DVItoMP, Version 0.64'
  {the same in the usual format, as it would be shown on a terminal}
@z

@x [3] Set up kpathsea.
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin @<Set initial values@>@/
@y
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin
    kpse_set_progname (argv[0]); {initialize for the filename searches}
    parse_arguments;
    @<Set initial values@>@/
@z

@x [7] Remove non-local goto.
@d abort(#)==begin err_print_ln('DVItoMP abort: ',#);
    history:=fatal_error; jump_out;
    end
@d bad_dvi(#)==abort('Bad DVI file: ',#,'!')
@.Bad DVI file@>
@d warn(#)==begin err_print_ln('DVItoMP warning: ',#);
    history:=warning_given;
    end

@p procedure jump_out;
begin goto final_end;
end;
@y
@d jump_out==uexit(history)
@d abort(#)==begin err_print_ln('DVItoMP abort: ',#);
    history:=fatal_error; jump_out;
    end
@d bad_dvi(#)==abort('Bad DVI file: ',#,'!')
@.Bad DVI file@>
@d warn(#)==begin err_print_ln('DVItoMP warning: ',#);
    history:=warning_given;
    end
@z

@x [11] Permissive input.
@!ASCII_code=" ".."~"; {a subrange of the integers}
@y
@!ASCII_code=0..255; {a subrange of the integers}
@z

% [12] The text_char type is used as an array index into `xord'.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
@x
@d text_char == char {the data type of characters in text files}
@d first_text_char=0 {ordinal number of the smallest element of |text_char|}
@d last_text_char=127 {ordinal number of the largest element of |text_char|}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@d first_text_char=0 {ordinal number of the smallest element of |text_char|}
@d last_text_char=255 {ordinal number of the largest element of |text_char|}
@z

@x [14] Fix up opening the files.
@p procedure open_mpx_file; {prepares to write text on |mpx_file|}
begin rewrite(mpx_file);
end;
@y
@p procedure open_mpx_file; {prepares to write text on |mpx_file|}
begin
   cur_name := extend_filename (mpx_name, 'mpx');
   rewrite (mpx_file, cur_name);
end;
@z

@x [19] More file opening.
@p procedure open_dvi_file; {prepares to read packed bytes in |dvi_file|}
begin reset(dvi_file);
if eof(dvi_file) then abort('DVI file not found');
end;
@#
function open_tfm_file:boolean; {prepares to read packed bytes in |tfm_file|}
begin reset(tfm_file,cur_name);
open_tfm_file:=(not eof(tfm_file));
end;
@#
function open_vf_file:boolean; {prepares to read packed bytes in |vf_file|}
begin reset(vf_file,cur_name);
open_vf_file:=(not eof(vf_file));
end;
@y
@p procedure open_dvi_file; {prepares to read packed bytes in |dvi_file|}
begin 
    cur_name := extend_filename (dvi_name, 'dvi');
    resetbin(dvi_file, cur_name);
end;
@#
function open_tfm_file:boolean; {prepares to read packed bytes in |tfm_file|}
begin 
  tfm_file := kpse_open_file (cur_name, kpse_tfm_format);
  free (cur_name); {We |xmalloc|'d this before we got called.}
  open_tfm_file := true; {If we get here, we succeeded.}
end;
@#
function open_vf_file:boolean; {prepares to read packed bytes in |tfm_file|}
var @!full_name:^char;
begin
  {It's ok if the \.{VF} file doesn't exist.}
  full_name := kpse_find_vf (cur_name);
  if full_name then begin
    resetbin (vf_file, full_name);
    free (cur_name);
    free (full_name);
    open_vf_file := true;
  end else
    open_vf_file := false;
end;
@z

@x [24] No arbitrary limit on filename length.
@!cur_name:packed array[1..name_length] of char; {external name,
  with no lower case letters}
@y
@!cur_name:^char; {external name}
@z

@x [26] Make get_n_bytes routines work with 16-bit math.
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

@x [41] Fix abs() with floating-point arg.
      begin if abs(font_scaled_size[f]-font_scaled_size[ff])
@y
      begin if fabs(font_scaled_size[f]-font_scaled_size[ff])
@z

@x [43] Fix abs() with floating-point arg.
if abs(font_design_size[f]-font_design_size[ff]) > font_tolerance then
@y
if fabs(font_design_size[f]-font_design_size[ff]) > font_tolerance then
@z 

@x [46] Make 16-bit TFM calculations work.
read_tfm_word; lh:=b2*256+b3;
read_tfm_word; font_bc[f]:=b0*256+b1; font_ec[f]:=b2*256+b3;
@y
read_tfm_word; lh:=b2*toint(256)+b3;
read_tfm_word; font_bc[f]:=b0*toint(256)+b1; font_ec[f]:=b2*toint(256)+b3;
@z
@x
    if b0<128 then tfm_check_sum:=((b0*256+b1)*256+b2)*256+b3
    else tfm_check_sum:=(((b0-256)*256+b1)*256+b2)*256+b3;
@y
    if b0<128 then tfm_check_sum:=((b0*toint(256)+b1)*256+b2)*256+b3
    else tfm_check_sum:=(((b0-256)*toint(256)+b1)*256+b2)*256+b3;
@z

% [61] Don't set default_directory_name.
@x
@d default_directory_name=='TeXfonts:' {change this to the correct name}
@d default_directory_name_length=9 {change this to the correct length}

@<Glob...@>=
@!default_directory:packed array[1..default_directory_name_length] of char;
@y
There is no single |default_directory| with C.
@z

@x [62] Remove initialization of default_directory.
@ @<Set init...@>=
default_directory:=default_directory_name;
@y
@ (No initialization needs to be done.  Keep this module to preserve
numbering.)
@z

@x [63] Dynamically allocate cur_name, don't add .vf.
for k:=1 to name_length do cur_name[k]:=' ';
if area_length[f]=0 then
  begin for k:=1 to default_directory_name_length do
    cur_name[k]:=default_directory[k];
  l:=default_directory_name_length;
  end
else l:=0;
for k:=font_name[f] to font_name[f+1]-1 do
  begin incr(l);
  if l+3>name_length then
    abort('DVItoMP capacity exceeded (max font name length=',
      name_length:1,')!');
@.DVItoMP capacity exceeded...@>
  if (names[k]>="a")and(names[k]<="z") then
      cur_name[l]:=xchr[names[k]-@'40]
  else cur_name[l]:=xchr[names[k]];
  end;
cur_name[l+1]:='.'; cur_name[l+2]:='V'; cur_name[l+3]:='F'
@y
{This amounts to a string copy. }
cur_name := xmalloc ((font_name[f+1] - font_name[f]) + 1);
for k:=font_name[f] to font_name[f+1]-1 do begin
  cur_name[k - font_name[f]] := xchr[names[k]];
end;
cur_name[font_name[f+1] - font_name[f]] := 0;
@z

@x [64] Since we didn't add .vf, don't need to change it to .tfm.
l:=area_length[f];
if l=0 then l:=default_directory_name_length;
l:=l+font_name[f+1]-font_name[f];
if l+4>name_length then
  abort('DVItoMP capacity exceeded (max font name length=',
    name_length:1,')!');
@.DVItoMP capacity exceeded...@>
cur_name[l+2]:='T'; cur_name[l+3]:='F'; cur_name[l+4]:='M'
@y
do_nothing
@z

@x [78] Fix printing of real numbers.
  if (abs(x)>=4096.0)or(abs(y)>=4096.0)or(m>=4096.0)or(m<0) then
    begin warn('text scaled ',m:1:1,@|
        ' at (',x:1:1,',',y:1:1,') is out of range');
    end_char_string(60);
    end
  else end_char_string(40);
  print_ln(',_n',str_f:1,',',m:1:5,',',x:1:4,',',y:1:4,');');
@y
  if (fabs(x)>=4096.0)or(fabs(y)>=4096.0)or(m>=4096.0)or(m<0) then
    begin warn('text is out of range');
    end_char_string(60);
    end
  else end_char_string(40);
  print(',_n',str_f:1,',');
  fprint_real(mpx_file, m,1,5); print(',');
  fprint_real(mpx_file, x,1,4); print(',');
  fprint_real(mpx_file, y,1,4);
  print_ln(');');
@z

@x [79] Another fix for printing of real numbers.
  if (abs(xx1)>=4096.0)or(abs(yy1)>=4096.0)or@|
      (abs(xx2)>=4096.0)or(abs(yy2)>=4096.0)or(ww>=4096.0) then
    warn('hrule or vrule near (',xx1:1:1,',',yy1:1:1,') is out of range');
  print_ln('_r((',xx1:1:4,',',yy1:1:4,')..(',xx2:1:4,',',yy2:1:4,
      '), ',ww:1:4,');');
@y
  if (fabs(xx1)>=4096.0)or(fabs(yy1)>=4096.0)or@|
      (fabs(xx2)>=4096.0)or(fabs(yy2)>=4096.0)or(ww>=4096.0) then
    warn('hrule or vrule is out of range');
  print('_r((');
  fprint_real(mpx_file, xx1,1,4); print(',');
  fprint_real(mpx_file, yy1,1,4); print(')..(');
  fprint_real(mpx_file, xx2,1,4); print(',');
  fprint_real(mpx_file, yy2,1,4); print('), ');
  fprint_real(mpx_file, ww,1,4);
  print_ln(');');
@z

@x [80] Yet another fix for printing of real numbers.
print_ln('setbounds _p to (0,',dd:1:4,')--(',w:1:4,',',dd:1:4,')--');
print_ln(' (',w:1:4,',',h:1:4,')--(0,',h:1:4,')--cycle;')
@y
print('setbounds _p to (0,');
fprint_real(mpx_file, dd,1,4); print(')--(');
fprint_real(mpx_file, w,1,4);  print(',');
fprint_real(mpx_file, dd,1,4); print_ln(')--');@/
print(' (');
fprint_real(mpx_file, w,1,4);  print(',');
fprint_real(mpx_file, h,1,4);  print(')--(0,');
fprint_real(mpx_file, h,1,4);  print_ln(')--cycle;')
@z

@x [98] Main program.
print_ln(banner);
@y
print (banner); 
print_ln (version_string);
@z
@x Exit with appropriate status.
final_end:end.
@y
if history<=cksum_trouble then uexit(0)
else uexit(history);
end.
@z

@x [103] System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{DVItoMP} work at a particular installation.
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
const n_options = 2; {Pascal won't count array lengths for us.}
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
      usage (1, 'dvitomp');

    end else if argument_is ('help') then begin
      usage (0, DVITOMP_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (term_banner, 'AT&T Bell Laboraties', 'John Hobby');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one or two remaining arguments.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) then begin
    write_ln (stderr, 'dvitomp: Need one or two file arguments.');
    usage (1, 'dvitomp');
  end;

  dvi_name := cmdline (optind);
  
  if optind + 2 <= argc then begin
    mpx_name := cmdline (optind + 1); {The user specified the other name.}
  end else begin
    {User did not specify the other name; default it from the first.}
    mpx_name := basename_change_suffix (dvi_name, '.dvi', '.mpx');
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

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;

@ Global filenames.

@<Global...@> =
@!dvi_name, @!mpx_name:c_string;
@z
