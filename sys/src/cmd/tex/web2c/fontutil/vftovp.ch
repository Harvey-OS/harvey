% vftovp.ch for C compilation with web2c.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{VF$\,$\lowercase{to}$\,$VP changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is VFtoVP, Version 1.2' {printed when the program starts}
@y
@d banner=='This is VFtoVP, C Version 1.2' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [2] Remove files in program statement.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p program VFtoVP(@!vf_file,@!tfm_file,@!vpl_file,@!output);
@y
@p program VFtoVP;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% still [2] Set up for path reading.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  begin print_ln(banner);@/
@y
  @<Local variables for initialization@>
  begin
    if (argc < 3) or (argc > n_options + arg_options + 4)
    then begin
      print ('Usage: vftovp ');
      print ('[-verbose] ');
      print_ln ('[-charcode-format=<format>] ');
      print_ln ('  <vfm file> <tfm file> [<vpl file>].');
@.Usage: ...@>
      uexit (1);
    end;

    @<Initialize the option variables@>;
    @<Parse arguments@>;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] Set name_length to the system constant
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
% [7] Declare vf_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!vf_file:packed file of byte;
@y
@!vf_file:packed file of byte; {files that contain binary data}
@!vf_name:packed array[1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [10] Declare tfm_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!tfm_file:packed file of byte;
@y
@!tfm_file:packed file of byte;
@!tfm_name:packed array[1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [11] Open the files.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ On some systems you may have to do something special to read a
packed file of bytes. For example, the following code didn't work
when it was first tried at Stanford, because packed files have to be
opened with a special switch setting on the \PASCAL\ that was used.
@^system dependencies@>

@<Set init...@>=
reset(tfm_file); reset(vf_file);
@y
@ We don't have to do anything special to read a packed file of bytes,
but we do want to use environment variables to find the input files.
@^system dependencies@>

@<Set init...@>=
{Use path searching to find the input files.}
set_paths (TFM_FILE_PATH_BIT + VF_FILE_PATH_BIT);

argv (optind, vf_name);
if test_read_access (vf_name, VF_FILE_PATH)
then reset (vf_file, vf_name)
else begin
  print_pascal_string (vf_name);
  print_ln (': VF file not found.');
  uexit (1);
end;

argv (optind + 1, tfm_name);
if test_read_access (tfm_name, TFM_FILE_PATH)
then reset (tfm_file, tfm_name)
else begin
  print_pascal_string (tfm_name);
  print_ln (': TFM file not found.');
  uexit (1);
end;
if verbose then print_ln (banner);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [20] Declare vpl_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!vpl_file:text;
@y
@!vpl_file:text;
@!vpl_name:packed array[1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [21] Open VPL file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
rewrite(vpl_file);
@y
@ @<Set init...@>=
if optind + 2 = argc
then vpl_file := stdout
else begin
  argv (optind + 2, vpl_name);
  rewrite (vpl_file, vpl_name);
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [24] abort() should cause a bad exit code.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d abort(#)==begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a TFM?');
  goto final_end;
  end
@y
@d abort(#)==begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a TFM?');
  uexit(1);
  end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [31] Ditto for vf_abort.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d vf_abort(#)==
  begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a VF?');
  goto final_end;
  end
@y
@d vf_abort(#)==
  begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a VF?');
  uexit(1);
  end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [32] Be quiet if not -verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
for k:=0 to vf_ptr-1 do print(xchr[vf[k]]);
print_ln(' '); count:=0;
@y
if verbose
then begin
  for k:=0 to vf_ptr-1 do print(xchr[vf[k]]);
  print_ln(' ');
end;
count:=0;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [35] Be quiet if not -verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Print the name of the local font@>;
@y
if verbose then begin
  @<Print the name of the local font@>;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [36] Output of real numbers.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
print_ln(' at ',(((vf[k]*256+vf[k+1])*256+vf[k+2])/@'4000000)*real_dsize:2:2,
  'pt')
@y
print(' at ');
print_real((((vf[k]*256+vf[k+1])*256+vf[k+2])/@'4000000)*real_dsize, 2, 2);
print_ln('pt')
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [39] Open another TFM file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
reset(tfm_file,cur_name);
@^system dependencies@>
if eof(tfm_file) then
  print_ln('---not loaded, TFM file can''t be opened!')
@.TFM file can\'t be opened@>
else  begin font_bc:=0; font_ec:=256; {will cause error if not modified soon}
@y
if not test_read_access(cur_name, TFM_FILE_PATH) then
  print_ln('---not loaded, TFM file can''t be opened!')
@.TFM file can\'t be opened@>
else begin reset(tfm_file, cur_name);
  font_bc:=0; font_ec:=256; {will cause error if not modified soon}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [40] Be quiet if not -verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
    begin print_ln('Check sum in VF file being replaced by TFM check sum');
@y
    begin
      if verbose
      then print_ln('Check sum in VF file being replaced by TFM check sum');
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [42] Remove initialization of now-defunct array.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
default_directory:=default_directory_name;
@y
@ (No initialization to be done.  Keep this module to preserve numbering.)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [44] Use lowercase `.tfm' suffix.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The string |cur_name| is supposed to be set to the external name of the
\.{TFM} file for the current font. This usually means that we need to
prepend the name of the default directory, and
to append the suffix `\.{.TFM}'. Furthermore, we change lower case letters
to upper case, since |cur_name| is a \PASCAL\ string.
@y
@ The string |cur_name| is supposed to be set to the external name of the
\.{TFM} file for the current font. This usually means that we need to
append the suffix ``.tfm''. 
@z

@x
if a=0 then
  begin for k:=1 to default_directory_name_length do
    cur_name[k]:=default_directory[k];
  r:=default_directory_name_length;
  end
else r:=0;
@y
r:=0;
@z

@x
  if (vf[k]>="a")and(vf[k]<="z") then
      cur_name[r]:=xchr[vf[k]-@'40]
  else cur_name[r]:=xchr[vf[k]];
  end;
cur_name[r+1]:='.'; cur_name[r+2]:='T'; cur_name[r+3]:='F'; cur_name[r+4]:='M'
@y
  cur_name[r]:=xchr[vf[k]];
  end;
cur_name[r+1]:='.'; cur_name[r+2]:='t'; cur_name[r+3]:='f'; cur_name[r+4]:='m'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [49] Change strings to C char pointers, so we can initialize them.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!ASCII_04,@!ASCII_10,@!ASCII_14: packed array [1..32] of char;
  {strings for output in the user's external character set}
@!xchr:packed array [0..255] of char;
@!MBL_string,@!RI_string,@!RCE_string:packed array [1..3] of char;
  {handy string constants for |face| codes}
@y
@!ASCII_04,@!ASCII_10,@!ASCII_14: ccharpointer;
  {strings for output in the user's external character set}
@!xchr:packed array [0..255] of char;
@!MBL_string,@!RI_string,@!RCE_string: ccharpointer;
  {handy string constants for |face| codes}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [50] The Pascal strings are indexed starting at 1, so we pad with a blank.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
ASCII_04:=' !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:='@@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:='`abcdefghijklmnopqrstuvwxyz{|}~?';@/
@y
ASCII_04:='  !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:=' @@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:=' `abcdefghijklmnopqrstuvwxyz{|}~?';@/
@z

@x
MBL_string:='MBL'; RI_string:='RI '; RCE_string:='RCE';
@y
MBL_string:=' MBL'; RI_string:=' RI '; RCE_string:=' RCE';
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [60] How we output the character code depends on |charcode_format|.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
begin if font_type>vanilla then
  begin tfm[0]:=c; out_octal(0,1)
  end
else if ((c>="0")and(c<="9"))or@|
   ((c>="A")and(c<="Z"))or@|
   ((c>="a")and(c<="z")) then out(' C ',xchr[c])
else begin tfm[0]:=c; out_octal(0,1);
  end;
@y
begin if (font_type > vanilla) or (charcode_format = charcode_octal) then
  begin tfm[0]:=c; out_octal(0,1)
  end
else if (charcode_format = charcode_ascii) and (c > " ") and (c <= "~")
        and (c <> "(") and (c <> ")") then
  out(' C ', xchr[c - " " + 1])
{default case, use \.C only for letters and digits}
else if ((c>="0")and(c<="9"))or@|
   ((c>="A")and(c<="Z"))or@|
   ((c>="a")and(c<="z")) then out(' C ',xchr[c])
else begin tfm[0]:=c; out_octal(0,1);
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [61] Don't output the face code as an integer.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
  out(MBL_string[1+(b mod 3)]);
  out(RI_string[1+s]);
  out(RCE_string[1+(b div 3)]);
@y
  put_byte(MBL_string[1+(b mod 3)], vpl_file);
  put_byte(RI_string[1+s], vpl_file);
  put_byte(RCE_string[1+(b div 3)], vpl_file);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [62] Force 32-bit constant arithmetic for 16-bit machines.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
f:=((tfm[k+1] mod 16)*@'400+tfm[k+2])*@'400+tfm[k+3];
@y
f:=((tfm[k+1] mod 16)*toint(@'400)+tfm[k+2])*@'400+tfm[k+3];
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [100] No progress reports unless verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
    incr(chars_on_line);
    end;
  print_octal(c); {progress report}
@y
    if verbose then incr(chars_on_line); {keep |chars_on_line = 0|}
    end;
  if verbose then print_octal(c); {progress report}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [112] No nonlocal goto's.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
  begin print_ln('Sorry, I haven''t room for so many ligature/kern pairs!');
@.Sorry, I haven't room...@>
  goto final_end;
@y
  begin print_ln('Sorry, I haven''t room for so many ligature/kern pairs!');
@.Sorry, I haven't room...@>
  uexit(1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% still [112] We can't have a function named `f', because of the local
% variable in do_simple_things.  It would be better, but harder, to fix
% web2c.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
     r:=f(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@y
     r:=lig_f(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@z

@x
  out('(INFINITE LIGATURE LOOP MUST BE BROKEN!)'); goto final_end;
@y
  out('(INFINITE LIGATURE LOOP MUST BE BROKEN!)'); uexit(1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [116] web2c can't handle these mutually recursive procedures.
% But let's do a fake definition of f here, so that it gets into web2c's
% symbol table...
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p function f(@!h,@!x,@!y:index):index; forward;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
@y
@p 
ifdef('notdef') 
function lig_f(@!h,@!x,@!y:index):index; begin end;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
endif('notdef')
@z

@x
else eval:=f(h,x,y);
@y
else eval:=lig_f(h,x,y);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [117] ... and then really define it now.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p function f;
@y
@p function lig_f(@!h,@!x,@!y:index):index;
@z

@x
f:=lig_z[h];
@y
lig_f:=lig_z[h];
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [124] Some cc's can't handle 136 case labels in a row.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
    begin o:=vf[vf_ptr]; incr(vf_ptr);
    case o of
    @<Cases of \.{DVI} instructions that can appear in character packets@>@;
@y
    begin o:=vf[vf_ptr]; incr(vf_ptr);
    if ((o>=set_char_0)and(o<=set_char_0+127))or
       ((o>=set1)and(o<=set1+3))or((o>=put1)and(o<=put1+3)) then
begin if o>=set1 then
    if o>=put1 then c:=get_bytes(o-put1+1,false)
    else c:=get_bytes(o-set1+1,false)
  else c:=o;
  if f=font_ptr then
    bad_vf('Character ',c:1,' in undeclared font will be ignored')
@.Character...will be ignored@>
  else begin vf[font_start[f+1]-1]:=c; {store |c| in the ``hole'' we left}
    k:=font_chars[f];@+while vf[k]<>c do incr(k);
    if k=font_start[f+1]-1 then
      bad_vf('Character ',c:1,' in font ',f:1,' will be ignored')
    else begin if o>=put1 then out('(PUSH)');
      left; out('SETCHAR'); out_char(c);
      if o>=put1 then out(')(POP');
      right;
      end;
    end;
  end
    else case o of
    @<Cases of \.{DVI} instructions that can appear in character packets@>
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [125] `signed' is a keyword in ANSI C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function get_bytes(@!k:integer;@!signed:boolean):integer;
@y
@p function get_bytes(@!k:integer;@!is_signed:boolean):integer;
@z

@x
if (k=4) or signed then
@y
if (k=4) or is_signed then
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [126] No nonlocal goto's.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
    begin print_ln('Stack overflow!'); goto final_end;
@y
    begin print_ln('Stack overflow!'); uexit(1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [129] This code moved outside the case statement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ Before we typeset a character we make sure that it exists.

@<Cases...@>=
sixty_four_cases(set_char_0),sixty_four_cases(set_char_0+64),
 four_cases(set1),four_cases(put1):begin if o>=set1 then
    if o>=put1 then c:=get_bytes(o-put1+1,false)
    else c:=get_bytes(o-set1+1,false)
  else c:=o;
  if f=font_ptr then
    bad_vf('Character ',c:1,' in undeclared font will be ignored')
@.Character...will be ignored@>
  else begin vf[font_start[f+1]-1]:=c; {store |c| in the ``hole'' we left}
    k:=font_chars[f];@+while vf[k]<>c do incr(k);
    if k=font_start[f+1]-1 then
      bad_vf('Character ',c:1,' in font ',f:1,' will be ignored')
    else begin if o>=put1 then out('(PUSH)');
      left; out('SETCHAR'); out_char(c);
      if o>=put1 then out(')(POP');
      right;
      end;
    end;
  end;
@y
@ Before we typeset a character we make sure that it exists.
(These cases moved outside the case statement, section 124.)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [134] No final newline unless verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
print_ln('.');@/
@y
if verbose then print_ln('.');@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [135] System-dependent changes.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@* System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{VFtoVP} work at a particular installation.
It is usually best to design your change file so that all changes to
previous sections preserve the section numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new sections, can be inserted here; then only the index
itself will get a new section number.
@^system dependencies@>
@y
@* System-dependent changes.  We want to parse a Unix-style command line.

This macro tests if its argument is the current option, as represented
by the index variable |option_index|.

@d argument_is (#) == (strcmp (long_options[option_index].name, #) = 0)

@<Parse arguments@> =
begin
  @<Define the option table@>;
  repeat
    getopt_return_val := getopt_long_only (argc, gargv, '', long_options,
                                           address_of_int (option_index));
    if getopt_return_val <> -1
    then begin
      if getopt_return_val = "?"
      then uexit (1); {|getopt| has already given an error message.}

      if argument_is ('charcode-format')
      then begin
        if strcmp (optarg, 'ascii') = 0
        then charcode_format := charcode_ascii
        else if strcmp (optarg, 'octal') = 0
        then charcode_format := charcode_octal
        else print ('Bad character code format', optarg, '.');
      end
      
      else
        {It was just a flag; |getopt| has already done the assignment.}
        do_nothing;

    end;
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.}
end


@ The array of information we pass in.  The type |getopt_struct| is
defined in C, to avoid type clashes.  We also need to know the return
value from getopt, and the index of the current option.

@<Local var...@> =
@!long_options: array[0..n_options] of getopt_struct;
@!getopt_return_val: integer;
@!option_index: integer;
@!current_option: 0..n_options;

@ Here is the first of the options we allow.
@.-verbose@>

@<Define the option...@> =
current_option := 0;
long_options[0].name := 'verbose';
long_options[0].has_arg := 0;
long_options[0].flag := address_of_int (verbose);
long_options[0].val := 1;
incr (current_option);

@ The global variable |verbose| determines whether or not we print
progress information.

@<Glob...@> =
@!verbose: integer;

@ It starts off |false|.

@<Initialize the option...@> =
verbose := false;


@ Here is an option to change how we output character codes.
@.-charcode-format@>

@<Define the option...@> =
long_options[current_option].name := 'charcode-format';
long_options[current_option].has_arg := 1;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ We use an ``enumerated'' type to store the information.

@<Type...@> =
@!charcode_format_type = charcode_ascii..charcode_default;

@
@<Const...@> =
@!charcode_ascii = 0;
@!charcode_octal = 1;
@!charcode_default = 2;

@
@<Global...@> =
@!charcode_format: charcode_format_type;

@ It starts off as the default, that is, we output letters and digits as
ASCII characters, everything else in octal.

@<Initialize the option...@> =
charcode_format := charcode_default;


@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;


@ Pascal compilers won't count the number of elements in an array
constant for us.  This doesn't include the zero-element at the end,
because this array starts at index zero.

@<Constants...@> =
@!n_options = 2;
@!arg_options = 1;
@z
