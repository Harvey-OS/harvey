% tftopl.ch for C compilation with web2c.
% 
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
% (more recent changes in the ChangeLog)

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{TF\lowercase{to}PL changes for C}
@z

% [2] Fix files in program statement.  We need to tell web2c about one
% special variable.  Perhaps it would be better to allow @define's
% anywhere in a source file, but that seemed just as painful as this.
@x
@p program TFtoPL(@!tfm_file,@!pl_file,@!output);
@y
@p
{Tangle doesn't recognize @@ when it's right after the \.=.}
@\@= @@define var tfm;@>@\
program TFtoPL(@!tfm_file,@!pl_file,@!output);
@z

@x [still 2] Don't print banner until later (and unless verbose).
procedure initialize; {this procedure gets things started properly}
  begin print_ln(banner);@/
@y
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  begin
    kpse_set_progname (argv[0]);
    kpse_init_prog ('TFTOPL', 0, nil, nil);
    {We |xrealloc| when we know how big the file is.  The 1000 comes
     from the negative lower bound.}
    tfm_file_array := cast_to_byte_pointer (xmalloc (1003));
    parse_arguments;
@z

@x [5] Increase sizes to match vptovf.
@!tfm_size=30000; {maximum length of |tfm| data, in bytes}
@!lig_size=5000; {maximum length of |lig_kern| program, in words}
@!hash_size=5003; {preferably a prime number, a bit larger than the number
  of character pairs in lig/kern steps}
@y
@!tfm_size=40000; {maximum length of |tfm| data, in bytes}
@!lig_size=8000; {maximum length of |lig_kern| program, in words ($<2^{15}$)}
@!hash_size=8009; {preferably a prime number, a bit larger than the number
  of character pairs in lig/kern steps}
@z

@x [7] Open the TFM file.
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
tfm_file := kpse_open_file (tfm_name, kpse_tfm_format);
if verbose then begin
  print (banner);
  print_ln (version_string);
end;
@z

@x [17] Open the PL file.
@ @<Set init...@>=
rewrite(pl_file);
@y
@ If an explicit filename isn't given, we write to |stdout|.

@<Set init...@>=
if optind + 1 = argc then begin
  pl_file := stdout;
end else begin
  pl_name := extend_filename (cmdline (optind + 1), 'pl');
  rewrite (pl_file, pl_name);
end;
@z

@x [18,19] Make |tfm| be dynamically allocated, and rename `index'.
@<Types...@>=
@!byte=0..255; {unsigned eight-bit quantity}
@!index=0..tfm_size; {address of a byte in |tfm|}

@ @<Glob...@>=
@!tfm:array [-1000..tfm_size] of byte; {the input data all goes here}
@y
@d index == index_type

@<Types...@>=
@!byte=0..255; {unsigned eight-bit quantity}
@!index=integer; {address of a byte in |tfm|}

@ @<Glob...@>=
{Kludge here to define |tfm| as a macro which takes care of the negative
 lower bound.  We've defined |tfm| for the benefit of web2c above.}
@=#define tfm (tfmfilearray + 1001);@>@\
@!tfm_file_array: pointer_to_byte; {the input data all goes here}
@z

@x [20] Send error output to stderr.
@d abort(#)==begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a TFM?');
@y
@d abort(#)==begin write_ln(stderr, #);
  write_ln(stderr, 'Sorry, but I can''t go on; are you sure this is a TFM?');
@z

@x [20] Allow arbitrarily large input files.
if 4*lf-1>tfm_size then abort('The file is bigger than I can handle!');
@.The file is bigger...@>
@y
tfm_file_array
  := cast_to_byte_pointer (xrealloc (tfm_file_array, 4 * lf - 1 + 1002));
@z

% [27, 28] Change strings to C char pointers. The Pascal strings are
% indexed starting at 1, so we pad with a blank.
@x
@!ASCII_04,@!ASCII_10,@!ASCII_14: packed array [1..32] of char;
  {strings for output in the user's external character set}
@!MBL_string,@!RI_string,@!RCE_string:packed array [1..3] of char;
  {handy string constants for |face| codes}
@y
@!ASCII_04,@!ASCII_10,@!ASCII_14: c_string;
  {strings for output in the user's external character set}
@!ASCII_all: packed array[0..256] of char;
@!MBL_string,@!RI_string,@!RCE_string: c_string;
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
strcpy (ASCII_all, ASCII_04);
strcat (ASCII_all, '@@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_');
strcat (ASCII_all, '`abcdefghijklmnopqrstuvwxyz{|}~');@/
MBL_string:=' MBL'; RI_string:=' RI '; RCE_string:=' RCE';
@z

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

@x [40] Force 32-bit constant arithmetic for 16-bit machines.
f:=((tfm[k+1] mod 16)*@'400+tfm[k+2])*@'400+tfm[k+3];
@y
f:=((tfm[k+1] mod 16)*toint(@'400)+tfm[k+2])*@'400+tfm[k+3];
@z

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

% [89] Change the name of the variable `class', since AIX 3.1's <math.h>
% defines a function by that name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d pending=4 {$f(x,y)$ is being evaluated}
@y
@d pending=4 {$f(x,y)$ is being evaluated}

@d class == class_var 
@z

% [90] Change name of the function `f'.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
     r:=f(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@y
     r:=f_fn(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@z

% [94] web2c can't handle these mutually recursive procedures.
% But let's do a fake definition of f here, so that it gets into web2c's
% symbol table. We also have to change the name, because there is also a
% variable named `f', and some C compilers can't deal with that.
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

% [99] No final newline unless verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
do_characters; print_ln('.');@/
@y
do_characters; if verbose then print_ln('.');@/
@z

@x [100] System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{TFtoPL} work at a particular installation.
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
  @<Initialize the option variables@>;
  @<Define the option table@>;
  repeat
    getopt_return_val := getopt_long_only (argc, argv, '', long_options,
                                           address_of (option_index));
    if getopt_return_val = -1 then begin
      {End of arguments; we exit the loop below.} ;

    end else if getopt_return_val = "?" then begin
      usage (1, 'tftopl');

    end else if argument_is ('help') then begin
      usage (0, TFTOPL_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.E. Knuth');

    end else if argument_is ('charcode-format') then begin
      if strcmp (optarg, 'ascii') = 0 then
        charcode_format := charcode_ascii
      else if strcmp (optarg, 'octal') = 0 then
        charcode_format := charcode_octal
      else
        write_ln (stderr, 'Bad character code format', optarg, '.');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) then begin
    write_ln (stderr, 'tftopl: Need one or two file arguments.');
    usage (1, 'tftopl');
  end;

  tfm_name := cmdline (optind);
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
@.-verbose@>

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

@ This option changes how we output character codes.
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

@ Global filenames.

@<Global...@> =
@!tfm_name, @!pl_name:c_string;
@z
