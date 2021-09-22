% vptovf.ch for C compilation with web2c.

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{VP$\,$\lowercase{to}$\,$VF changes for C}
@z

@x [2] Print the banner later.
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

@x [3] Increase constants.
@!buf_size=60; {length of lines displayed in error messages}
@y
@!buf_size=3000; {max input line length, output error line length}
@z
@x
@!vf_size=10000; {maximum length of |vf| data, in bytes}
@!max_stack=100; {maximum depth of simulated \.{DVI} stack}
@!max_param_words=30; {the maximum number of \.{fontdimen} parameters allowed}
@!max_lig_steps=5000;
  {maximum length of ligature program, must be at most $32767-257=32510$}
@!max_kerns=500; {the maximum number of distinct kern values}
@!hash_size=5003; {preferably a prime number, a bit larger than the number
  of character pairs in lig/kern steps}
@y
@!vf_size=50000; {maximum length of |vf| data, in bytes}
@!max_stack=100; {maximum depth of simulated \.{DVI} stack}
@!max_param_words=30; {the maximum number of \.{fontdimen} parameters allowed}
@!max_lig_steps=32000;
  {maximum length of ligature program, must be at most $32767-257=32510$}
@!max_kerns=15000; {the maximum number of distinct kern values}
@!hash_size=15077; {preferably a prime number, a bit larger than the number
  of character pairs in lig/kern steps}
@z

@x [6] Open VPL file.
reset(vpl_file);
@y
reset (vpl_file, vpl_name);
if verbose then begin
  print (banner);
  print_ln (version_string);
end;
@z

@x [22] Open output files.
@ On some systems you may have to do something special to write a
packed file of bytes. For example, the following code didn't work
when it was first tried at Stanford, because packed files have to be
opened with a special switch setting on the \PASCAL\ that was used.
@^system dependencies@>

@<Set init...@>=
rewrite(vf_file); rewrite(tfm_file);
@y
@ On some systems you may have to do something special to write a
packed file of bytes.
@^system dependencies@>

@<Set init...@>=
rewritebin (vf_file, vf_name);
rewritebin (tfm_file, tfm_name);
@z

% [89] `index' is not a good choice for an identifier on Unix systems.
% Neither is `class', on AIX.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
|k|th element of its list.
@y
|k|th element of its list.

@d index == index_var
@d class == class_var
@z

@x [118] No output unless verbose.
@<Print |c| in octal notation@>;
@y
if verbose then @<Print |c| in octal notation@>;
@z

@x [144] Output of real numbers.
@ @d round_message(#)==if delta>0 then print_ln('I had to round some ',
@.I had to round...@>
  #,'s by ',(((delta+1) div 2)/@'4000000):1:7,' units.')
@y
@ @d round_message(#)==if delta>0 then begin print('I had to round some ',
@.I had to round...@>
  #,'s by '); print_real((((delta+1) div 2)/@'4000000),1,7);
  print_ln(' units.'); end
@z

@x [152] Fix up the mutually recursive procedures a la pltotf.
@p function f(@!h,@!x,@!y:indx):indx; forward;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
@y
@p 
ifdef('notdef') 
function f(@!h,@!x,@!y:indx):indx; begin end;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
endif('notdef')
@z

@x [153] Finish fixing up f.
@p function f;
@y
@p function f(@!h,@!x,@!y:indx):indx; 
@z

@x [156] Change TFM-byte output to fix ranges.
@d out(#)==write(tfm_file,#)
@y
@d out(#)==putbyte(#,tfm_file)
@z

@x [165] Fix output of reals.
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
  while label_table[sort_ptr].rr>char_remainder[c] do
@y
  while label_table[sort_ptr].rr>toint(char_remainder[c]) do
@z

@x [175] Change VF-byte output to fix ranges.
@d vout(#)==write(vf_file,#)
@y
@d vout(#)==putbyte(#,vf_file)
@z

@x [181] Be quiet unless verbose. 
read_input; print_ln('.');@/
@y
read_input;
if verbose then print_ln('.');
@z

@x [182] System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{VPtoVF} work at a particular installation.
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
      usage (1, 'vptovf'); {|getopt| has already given an error message.}

    end else if argument_is ('help') then begin
      usage (0, VPTOVF_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.E. Knuth');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one to three remaining arguments.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) 
     and (optind + 3 <> argc) then begin
    write_ln (stderr, 'vptovf: Need one to three file arguments.');
    usage (1, 'vptovf');
  end;

  vpl_name := extend_filename (cmdline (optind), 'vpl');

  if optind + 2 <= argc then begin
    {Specified one or both of the output files.}
    vf_name := extend_filename (cmdline (optind + 1), 'vf');
    if optind + 3 <= argc then begin {Both.}
      tfm_name := extend_filename (cmdline (optind + 2), 'tfm');
    end else begin {Just one.}
      tfm_name := extend_filename (cmdline (optind + 1), 'tfm');
    end;
  end else begin {Neither.}
    vf_name := basename_change_suffix (vpl_name, '.vpl', '.vf');
    tfm_name := basename_change_suffix (vpl_name, '.vpl', '.tfm');
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
@.-verbose@>

@<Define the option...@> =
long_options[current_option].name := 'verbose';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := address_of (verbose);
long_options[current_option].val := 1;
incr (current_option);

@ The global variable |verbose| determines whether or not we print
progress information.

@<Glob...@> =
@!verbose: c_int_type;

@ It starts off |false|.

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
@!vpl_name, @!tfm_name, @!vf_name:c_string;
@z
