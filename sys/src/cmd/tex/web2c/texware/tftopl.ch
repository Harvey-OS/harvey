% tftopl.ch for C compilation with web2c.
% 
% The original version of this file was created by Pavel Curtis.
%
% History:
% 04/04/83 (PC)  Original version, made to work with version 1.0 of TFtoPL,
%                released with version 0.96 of TeX in February, 1983.
% 04/16/83 (PC)  Brought up to version 1.0 released with version 0.97 of TeX
%                in April, 1983.
% 06/30/83 (HWT) Revised changefile format, for use with version 1.7 Tangle.
% 07/28/83 (HWT) Brought up to version 2
% 11/21/83 (HWT) Brought up to version 2.1
% 03/24/84 (HWT) Brought up to version 2.2
% 07/12/84 (HWT) Brought up to version 2.3
% 07/05/87 (ETM) Brought up to version 2.5
% 03/22/88 (ETM) Converted for use with WEB to C.
% 11/30/89 (KB)  Version 3.
% 01/16/90 (SR)  Version 3.1.
% (more recent changes in ../ChangeLog.W2C)


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{TF\lowercase{to}PL changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is TFtoPL, Version 3.1' {printed when the program starts}
@y
@d banner=='This is TFtoPL, C Version 3.1' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [2] Fix files in program statement.  We need to tell web2c about one
% special variable.  Perhaps it would be better to allow @define's
% anywhere in a source file, but that seemed equally ugly as this.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p program TFtoPL(@!tfm_file,@!pl_file,@!output);
@y
{Tangle doesn't recognize @@ when it's right after the \.=.}
@p
@= @@define var tfm;@>@\
program TFtoPL;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [still 2] Don't print banner unless verbose.  Also, we need to
% initialize various things, and tftopl doesn't have an `initialize'
% procedure, so we do it here.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  begin print_ln(banner);@/
@y
  var @<Local variables for initialization@>
  begin
    if (argc < 2) or (argc > n_options + arg_options + 3)
    then begin
      print ('Usage: tftopl ');
      print ('[-verbose] ');
      print ('[-charcode-format=<format>] ');
      print_ln ('<tfm file> [<property list file>].');
@.Usage: ...@>
      uexit (1);
    end;

    {We |xrealloc| when we know how big the file is.  The 1000 comes
     from the negative lower bound.}
    tfm_file_array := cast_to_byte_pointer (xmalloc (1002));
    @<Initialize the option variables@>;
    @<Parse arguments@>;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [6] Declare tfm_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!tfm_file:packed file of 0..255;
@y
@!tfm_file:packed file of 0..255;
@!tfm_name:packed array [1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [7] Open the TFM file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ On some systems you may have to do something special to read a
packed file of bytes. For example, the following code didn't work
when it was first tried at Stanford, because packed files have to be
opened with a special switch setting on the \PASCAL\ that was used.
@^system dependencies@>

@<Set init...@>=
reset(tfm_file);
@y
@ On some systems you may have to do something special to read a
packed file of bytes.  With C under Unix, we just open the file by name
and read characters from it.

@<Set init...@>=
set_paths (TFM_FILE_PATH_BIT);
argv (optind, tfm_name);
if test_read_access (tfm_name, TFM_FILE_PATH)
then begin
  reset (tfm_file, tfm_name);
end else begin
  print_pascal_string (tfm_name);
  print_ln (': TFM file not found.');
  uexit (1);
end;
if verbose then print_ln (banner);@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [16] Declare pl_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!pl_file:text;
@y
@!pl_file: text;
@!pl_name: array[1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [17] Open the PL file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
rewrite(pl_file);
@y
@ If an explicit filename isn't given, we write the output to |stdout|.

@<Set init...@>=
if optind + 1 = argc
then pl_file := stdout
else begin
  argv (optind + 1, pl_name);
  rewrite (pl_file, pl_name);
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [18,19] Make |tfm| be dynamically allocated.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!index=0..tfm_size; {address of a byte in |tfm|}

@ @<Glob...@>=
@!tfm:array [-1000..tfm_size] of byte; {the input data all goes here}
@y
@!index=integer; {address of a byte in |tfm|}

@ @<Glob...@>=
{Kludge here to define |tfm| as a macro which takes care of the negative
 lower bound.  We've defined |tfm| for the benefit of web2c above.}
@=#define tfm (tfmfilearray + 1001);@>@\
@!tfm_file_array: pointer_to_byte; {the input data all goes here}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [20] Allow arbitrarily large input files.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if 4*lf-1>tfm_size then abort('The file is bigger than I can handle!');
@.The file is bigger...@>
@y
tfm_file_array
  := cast_to_byte_pointer (xrealloc (tfm_file_array, 4 * lf - 1 + 1002));
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [27, 28] Change strings to C char pointers. The Pascal strings are
% indexed starting at 1, so we pad with a blank.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!ASCII_04,@!ASCII_10,@!ASCII_14: packed array [1..32] of char;
  {strings for output in the user's external character set}
@!MBL_string,@!RI_string,@!RCE_string:packed array [1..3] of char;
  {handy string constants for |face| codes}
@y
@!ASCII_04,@!ASCII_10,@!ASCII_14: ccharpointer;
  {strings for output in the user's external character set}
@!ASCII_all: packed array[0..256] of char;
@!MBL_string,@!RI_string,@!RCE_string: ccharpointer;
  {handy string constants for |face| codes}
@z

@x
ASCII_04:=' !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:='@@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:='`abcdefghijklmnopqrstuvwxyz{|}~ ';@/
MBL_string:='MBL'; RI_string:='RI '; RCE_string:='RCE';
@y
ASCII_04:='  !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:=' @@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:=' `abcdefghijklmnopqrstuvwxyz{|}~ ';@/
vstrcpy (ASCII_all, ASCII_04);
vstrcat (ASCII_all, '@@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_');
vstrcat (ASCII_all, '`abcdefghijklmnopqrstuvwxyz{|}~');@/
MBL_string:=' MBL'; RI_string:=' RI '; RCE_string:=' RCE';
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38] How we output the character code depends on |charcode_format|.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
begin if font_type>vanilla then
  begin tfm[0]:=c; out_octal(0,1)
  end
else if (c>="0")and(c<="9") then
  out(' C ',c-"0":1)
else if (c>="A")and(c<="Z") then
  out(' C ',ASCII_10[c-"A"+2])
else if (c>="a")and(c<="z") then
  out(' C ',ASCII_14[c-"a"+2])
else  begin tfm[0]:=c; out_octal(0,1);
@y
begin if (font_type > vanilla) or (charcode_format = charcode_octal) then
  begin tfm[0]:=c; out_octal(0,1)
  end
else if (charcode_format = charcode_ascii) and (c > " ") and (c <= "~")
        and (c <> "(") and (c <> ")") then
  out(' C ', ASCII_all[c - " " + 1])
{default case, use \.C only for letters and digits}
else if (c>="0")and(c<="9") then
  out(' C ',c-"0":1)
else if (c>="A")and(c<="Z") then
  out(' C ',ASCII_10[c-"A"+2])
else if (c>="a")and(c<="z") then
  out(' C ',ASCII_14[c-"a"+2])
else  begin tfm[0]:=c; out_octal(0,1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [39] Don't output the face code as an integer.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
  out(MBL_string[1+(b mod 3)]);
  out(RI_string[1+s]);
  out(RCE_string[1+(b div 3)]);
@y
  put_byte(MBL_string[1+(b mod 3)], pl_file);
  put_byte(RI_string[1+s], pl_file);
  put_byte(RCE_string[1+(b div 3)], pl_file);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [40] Force 32-bit constant arithmetic for 16-bit machines.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
f:=((tfm[k+1] mod 16)*@'400+tfm[k+2])*@'400+tfm[k+3];
@y
f:=((tfm[k+1] mod 16)*toint(@'400)+tfm[k+2])*@'400+tfm[k+3];
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [78] No progress reports unless verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
    incr(chars_on_line);
    end;
  print_octal(c); {progress report}
@y
    if verbose then incr(chars_on_line);
    end;
  if verbose then print_octal(c); {progress report}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [90] Change name of the function `f'.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
     r:=f(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@y
     r:=f_fn(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [94] web2c can't handle these mutually recursive procedures.
% But let's do a fake definition of f here, so that it gets into web2c's
% symbol table. We also have to change the name, because there is also a
% variable named `f', and some C compilers can't deal with that.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function f(@!h,@!x,@!y:index):index; forward;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
@y
@p 
ifdef('notdef') 
function f_fn(@!h,@!x,@!y:index):index; begin end;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
endif('notdef')
@z
@x
else eval:=f(h,x,y);
@y
else eval:=f_fn(h,x,y);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [95] The real definition of f.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p function f;
@y
@p function f_fn(@!h,@!x,@!y:index):index; 
@z
@x
f:=lig_z[h];
@y
f_fn:=lig_z[h];
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [99] No final newline unless verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
do_characters; print_ln('.');@/
@y
do_characters; if verbose then print_ln('.');@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [100] System-dependent changes.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@* System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{TFtoPL} work at a particular installation.
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
