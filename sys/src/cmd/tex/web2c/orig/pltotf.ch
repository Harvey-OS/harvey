% pltotf.ch for C compilation with web2c.
%
% 04/04/83 (PC)  Original version, made to work with version 1.2 of PLtoTF.
% 04/16/83 (PC)  Brought up to version 1.3 of PLtoTF.
% 06/30/83 (HWT) Revised changefile format for version 1.7 Tangle
% 07/28/83 (HWT) Brought up to version 2
% 12/19/86 (ETM) Brought up to version 2.1
% 07/05/87 (ETM) Brought up to version 2.3
% 03/22/88 (ETM) Converted for use with WEB to C
% 11/29/89 (KB)  Version 3.
% 01/16/90 (SR)  Version 3.2.
% (more recent changes in the ChangeLog)

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{PL$\,$\lowercase{to}$\,$TF changes for C}
@z

@x [still 2] No banner unless verbose.
procedure initialize; {this procedure gets things started properly}
  var @<Local variables for initialization@>@/
  begin print_ln(banner);@/
@y
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var @<Local variables for initialization@>@/
begin
  kpse_set_progname (argv[0]);
  parse_arguments;
@z

@x [3] Larger constants.
@!buf_size=60; {length of lines displayed in error messages}
@y
@!buf_size=3000; {max input line length, output error line length}
@z
@x
@!max_lig_steps=5000;
  {maximum length of ligature program, must be at most $32767-257=32510$}
@!max_kerns=500; {the maximum number of distinct kern values}
@!hash_size=5003; {preferably a prime number, a bit larger than the number
  of character pairs in lig/kern steps}
@y
@!max_lig_steps=32500;
  {maximum length of ligature program, must be at most $32767-257=32510$}
@!max_kerns=15000; {the maximum number of distinct kern values}
@!hash_size=15077; {preferably a prime number, a bit larger than the number
  of character pairs in lig/kern steps}
@z

@x [6] Open PL file.
reset(pl_file);
@y
reset (pl_file, pl_name);
if verbose then begin
  print (banner);
  print_ln (version_string);
end;
@z

@x [16] Open TFM file.
@ On some systems you may have to do something special to write a
packed file of bytes. For example, the following code didn't work
when it was first tried at Stanford, because packed files have to be
opened with a special switch setting on the \PASCAL\ that was used.
@^system dependencies@>

@<Set init...@>=
rewrite(tfm_file);
@y
@ On some systems you may have to do something special to write a
packed file of bytes.  It's no problem in C.
@^system dependencies@>

@<Set init...@>=
rewritebin (tfm_file, tfm_name);
@z

@x [79] `index' might be a library routine.
|k|th element of its list.
@y
|k|th element of its list.
@d index == index_var
@z

@x [103] No output (except errors) unless verbose.
@<Print |c| in octal notation@>;
@y
if verbose then @<Print |c| in octal notation@>;
@z

@x [115] Output of reals.
@ @d round_message(#)==if delta>0 then print_ln('I had to round some ',
@.I had to round...@>
  #,'s by ',(((delta+1) div 2)/@'4000000):1:7,' units.')
@y
@ @d round_message(#)==if delta>0 then begin print('I had to round some ',
@.I had to round...@>
  #,'s by '); print_real((((delta+1) div 2)/@'4000000),1,7);
  print_ln(' units.'); end
@z

% [118] Change the name of the variable `class', since AIX 3.1's <math.h>
% defines a function by that name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d pending=4 {$f(x,y)$ is being evaluated}
@y
@d pending=4 {$f(x,y)$ is being evaluated}
@d class == class_var  {Avoid problems with AIX \.{<math.h>}}
@z

% [123] web2c can't handle these mutually recursive procedures.
% But let's do a fake definition of f here, so that it gets into web2c's
% symbol table...
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p function f(@!h,@!x,@!y:indx):indx; forward;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
@y
@p 
ifdef('notdef') 
function f(@!h,@!x,@!y:indx):indx; begin end;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
endif('notdef')
@z

% [124] ... and then really define it now.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p function f;
@y
@p function f(@!h,@!x,@!y:indx):indx; 
@z

@x [127] Fix up output of bytes.
@d out(#)==write(tfm_file,#)
@y
@d out(#)==putbyte(#,tfm_file)
@z

@x [136] Fix output of reals.
@p procedure out_scaled(x:fix_word); {outputs a scaled |fix_word|}
var @!n:byte; {the first byte after the sign}
@!m:0..65535; {the two least significant bytes}
begin if abs(x/design_units)>=16.0 then
  begin print_ln('The relative dimension ',x/@'4000000:1:3,
    ' is too large.');
@.The relative dimension...@>
  print('  (Must be less than 16*designsize');
  if design_units<>unity then print(' =',design_units/@'200000:1:3,
      ' designunits');
@y
@p procedure out_scaled(x:fix_word); {outputs a scaled |fix_word|}
var @!n:byte; {the first byte after the sign}
@!m:0..65535; {the two least significant bytes}
begin if fabs(x/design_units)>=16.0 then
  begin print('The relative dimension ');
    print_real(x/@'4000000,1,3);
    print_ln(' is too large.');
@.The relative dimension...@>
  print('  (Must be less than 16*designsize');
  if design_units<>unity then begin print(' =');
	print_real(design_units/@'200000,1,3);
	print(' designunits');
  end;
@z

% [141] char_remainder[c] is unsigned, and label_table[sort_ptr].rr
% might be -1, and if -1 is coerced to being unsigned, it will be bigger
% than anything else.
@x
  while label_table[sort_ptr].rr>char_remainder[c] do
@y
  while label_table[sort_ptr].rr>toint(char_remainder[c]) do
@z

@x [147] Be quiet unless verbose. 
read_input; print_ln('.');@/
@y
read_input;
if verbose then print_ln('.');
@z

@x [148] System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{PLtoTF} work at a particular installation.
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
const n_options = 3; {Pascal won't count array lengths for us.}
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
      usage (1, 'pltotf');

    end else if argument_is ('help') then begin
      usage (0, PLTOTF_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.E. Knuth');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one or two remaining arguments.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) then begin
    write_ln (stderr, 'pltotf: Need one or two file arguments.');
    usage (1, 'pltotf');
  end;
  
  pl_name := extend_filename (cmdline (optind), 'pl');

  {If an explicit output filename isn't given, construct it from |pl_name|.}
  if optind + 2 = argc then begin
    tfm_name := extend_filename (cmdline (optind + 1), 'tfm');
  end else begin
    tfm_name := basename_change_suffix (pl_name, '.pl', '.tfm');
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

@ Print progress information?

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

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;

@ Global filenames.

@<Global...@> =
@!tfm_name,@!pl_name:c_string;
@z
