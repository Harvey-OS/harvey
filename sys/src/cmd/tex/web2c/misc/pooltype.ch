% pooltype.ch for C compilation with web2c.
%
% 03/23/88 (ETM) Created for use with WEB to C.
% 11/29/89 (KB)  Version released with 8-bit TeX.
% (more recent changes in the ChangeLog)

@x [0] WEAVE: print changes only
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{POOL\lowercase{type} changes for C}
@z

@x [2] main program changes: no global labels, read command line. 
label 9999; {this labels the end of the program}
@y
@z
@x 
procedure initialize; {this procedure gets things started properly}
  var @<Local variables for initialization@>@;
  begin @<Set initial values of key variables@>@/
@y
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var @<Local variables for initialization@>@;
  begin
    kpse_set_progname (argv[0]);
    parse_arguments;
    @<Set initial values of key variables@>
@z

% [??] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x [12] Permissiveness
for i:=0 to @'37 do xchr[i]:=' ';
for i:=@'177 to @'377 do xchr[i]:=' ';
@y
for i:=0 to @'37 do xchr[i]:=chr(i);
for i:=@'177 to @'377 do xchr[i]:=chr(i);
@z

@x Write errors to stderr, avoid nonlocal label.
@d abort(#)==begin write_ln(#); goto 9999;
  end
@y
@d abort(#)==begin write_ln(stderr, #); uexit(1); end
@z

@x Remove unused label from end of program; add uexit(0) call
9999:end.
@y
uexit(0);
end.
@z

@x Add pool_name variable.
@!pool_file:packed file of text_char;
  {the string-pool file output by \.{TANGLE}}
@y
@!pool_file:packed file of text_char;
  {the string-pool file output by \.{TANGLE}}
@!pool_name:^char;
@z

% The name of the pool file is dynamically determined. We open it at the
% end of parse_arguments.
@x
reset(pool_file); xsum:=false;
@y
xsum:=false;
@z

@x Change single read into two reads
read(pool_file,m,n); {read two digits of string length}
@y
read(pool_file,m); read(pool_file,n); {read two digits of string length}
@z

@x System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{POOLtype} work at a particular installation.
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
      usage (1, 'pooltype');
    
    end else if argument_is ('help') then begin
      usage (0, POOLTYPE_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit ('This is POOLtype, Version 3.0', nil,
                              'D.E. Knuth');

    end; {Else it was just a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.}
  if (optind + 1 <> argc) then begin
    write_ln (stderr, 'pooltype: Need exactly one file argument.');
    usage (1, 'pooltype');
  end;

  pool_name := extend_filename (cmdline (optind), 'pool');
  {Try opening the file here, to avoid printing the first 256 strings if
   they give a bad filename.}
  reset (pool_file, pool_name); 
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
