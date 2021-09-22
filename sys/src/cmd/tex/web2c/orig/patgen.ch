% patgen.ch for C compilation with web2c.
%
% 07/01/83 (HWT) Original version, made to work with patgen released with
%		 version 0.99 of TeX in July 1983.  It may not work
%		 properly---it is hard to test without more information.
% 03/23/88 (ETM) Brought up to date, converted for use with WEB to C.

@x WEAVE: print changes only
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{PATGEN changes for C}
@z

@x Terminal I/O
@d get_input(#)==read(input,#)
@d get_input_ln(#)==
  begin if eoln(input) then read_ln(input);
  read(input,#);
  end
@#
@y
@d get_input(#)==#:=input_int(std_input)
@d get_input_ln(#)==begin #:=getc(std_input); read_ln(std_input); end @#
@z

@x Need standard input.
@p @<Compiler directives@>@/
@y
@d std_input==stdin
@p @<Compiler directives@>@/
@z

@x Add file opening to initialization
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
  print (banner);
  print_ln (version_string);
@z

@x Error handling
@d error(#)==begin print_ln(#); jump_out; end
@y
@d error(#)==begin write_ln(stderr, #); uexit(1); end;
@z

@x Fix signed char problem
@!text_char=char; {the data type of characters in text files}
@!ASCII_code=0..last_ASCII_code; {internal representation of input characters}
@y
@!ASCII_code=0..last_ASCII_code; {internal representation of input characters}
@!text_char=ASCII_code; {the data type of characters in text files}
@z

@x Increase constants.
@!max_buf_len=80; {maximum length of input lines, must be at least |max_len|}
@y
@!max_buf_len=3000; {maximum length of input lines, must be at least |max_len|}
@z

@x Remove file close
@d close_out(#)==close(#) {close an output file}
@d close_in(#)==do_nothing {close an input file}
@y
@d close_out(#)== {close an output file}
@d close_in(#)== {close an input file}
@z

@x Add f_name declaration, and temporaries for efficiency printing.
@!dictionary, @!patterns, @!translate, @!patout, @!pattmp: text_file;
@y
@!dictionary, @!patterns, @!translate, @!patout, @!pattmp: text_file;
@!f_name: ^char;
@!bad_frac, @!denom, @!eff: real;
@z

@x Get translate filename from command line.
reset(translate);
@y
f_name := cmdline (4);
reset (translate, f_name);
@z

@x Input kludge.
  repeat print('left_hyphen_min, right_hyphen_min: '); get_input(n1,n2);@/
@y
  repeat print('left_hyphen_min, right_hyphen_min: '); input_2ints(n1,n2);@/
@z

@x Floating point output kludge for Web2c.
  print_ln(', efficiency = ',
    good_count/(good_pat_count+bad_count/bad_eff):1:2)
@y
begin
  print(', efficiency = ');
  print_real(good_count/(good_pat_count+bad_count/bad_eff),1,2);
  write_ln(output);
end
@z

@x Get dictionary filename from command line.
  reset(dictionary);@/
@y
f_name := cmdline(1);
reset (dictionary, f_name);
@z

% Fix file name initialization, since can't assign a constant string
% that we're going to write into.
@x
    begin filnam:='pattmp. ';
@y
    begin strcpy (filnam, 'pattmp');
@z

@x Work around floating point I/O deficiency.
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

@x Get pattern filename from command line.
reset(patterns);
@y
f_name := cmdline (2);
reset (patterns, f_name);
@z

@x Fix reading of multiple variables in the same line
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
    get_input(n1,n2,n3);@/
@y
    input_3ints(n1,n2,n3);@/
@z

@x Get output file name from command line.
rewrite(patout);
@y
f_name := cmdline (3);
rewrite (patout, f_name);
@z

@x System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{PATGEN} work at a particular installation.
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
      do_nothing;
    
    end else if getopt_return_val = '?' then begin
      usage (1, 'patgen');
    
    end else if argument_is ('help') then begin
      usage (0, PATGEN_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil,
                              'Frank M. Liang and Peter Breitenlohner');

    end; {Else it was just a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.}
  if (optind + 4 <> argc) then begin
    write_ln (stderr, 'patgen: Need exactly four arguments.');
    usage (1, 'patgen');
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
@z
