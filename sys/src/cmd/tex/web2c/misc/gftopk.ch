% gftopk.ch for C compilation with web2c.
%
% 09/19/88 Pierre A. MacKay	Version 1.4.
% 12/02/89 Karl Berry		2.1.
% 01/20/90 Karl			2.2.
% (more recent changes in the ChangeLog)
% 
% One major change in output format is made by this change file.  The
% local gftopk preamble comment is ignored and the dated METAFONT
% comment is passed through unaltered.  This provides a continuous check
% on the origin of fonts in both gf and pk formats.  The program runs
% silently unless it is given the -v switch in the command line.

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{GF$\,$\lowercase{to}$\,$PK changes C}
@z

@x [4] No global labels.
@ The binary input comes from |gf_file|, and the output font is written
on |pk_file|.  All text output is written on \PASCAL's standard |output|
file.  The term |print| is used instead of |write| when this program writes
on |output|, so that all such output could easily be redirected if desired.

@d print(#)==write(#)
@d print_ln(#)==write_ln(#)

@p program GFtoPK(@!gf_file,@!pk_file,@!output);
label @<Labels in the outer block@>@/
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin print_ln(banner);@/
@y
@ The binary input comes from |gf_file|, and the output font is written
on |pk_file|.  All text output is written on \PASCAL's standard |output|
file.  The term |print| is used instead of |write| when this program writes
on |output|, so that all such output could easily be redirected if desired.
Since the terminal output is really not very interesting, it is
produced only when the \.{-v} command line flag is presented.

@d print(#)==if verbose then write(stdout, #)
@d print_ln(#)==if verbose then write_ln(stdout, #)

@p program GFtoPK(@!gf_file,@!pk_file,@!output);
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
begin 
  kpse_set_progname (argv[0]);
  kpse_init_prog ('GFTOPK', 0, nil, nil);
  parse_arguments;
  print(banner); print_ln(version_string);
@z

@x [5] Eliminate the |final_end| label.
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ This module is deleted, because it is only useful for
a non-local goto, which we can't use in C.
@z

@x [7] Allow for bigger fonts.  Too bad it's not dynamically allocated.
@!max_row=16000; {largest index in the main |row| array}
@y
@!max_row=100000; {largest index in the main |row| array}
@z

@x [8] Make `abort' end with a newline, and remove the nonlocal goto.
@d abort(#)==begin print(' ',#); jump_out;
    end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>

@p procedure jump_out;
begin goto final_end;
end;
@y
@d abort(#)==begin write_ln(stderr, #); uexit (1);
    end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>
@z

@x [40] Use paths in open_gf_file.
@ To prepare the |gf_file| for input, we |reset| it.

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin reset(gf_file);
gf_loc := 0 ;
end;
@y
@ In C, we do path searching based on the user's environment or the
default paths.

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin
  gf_file := kpse_open_file (gf_name, kpse_gf_format);
  gf_loc := 0;
end;
@z

% [41] If the PK filename isn't given on the command line, we construct
% it from the GF filename.
@x
@p procedure open_pk_file; {prepares to write packed bytes in |pk_file|}
begin rewrite(pk_file);
pk_loc := 0 ; pk_open := true ;
end;
@y
@p procedure open_pk_file; {prepares to write packed bytes in |pk_file|}
begin
  rewritebin (pk_file, pk_name);
  pk_loc := 0;
  pk_open := true;
end;
@z

@x [46] Redefine pk_byte, pk_halfword, pk_three_bytes, and pk_word.
@p procedure pk_byte(a:integer) ;
begin
   if pk_open then begin
      if a < 0 then a := a + 256 ;
      write(pk_file, a) ;
      incr(pk_loc) ;
   end ;
end ;
@#
procedure pk_halfword(a:integer) ;
begin
   if a < 0 then a := a + 65536 ;
   write(pk_file, a div 256) ;
   write(pk_file, a mod 256) ;
   pk_loc := pk_loc + 2 ;
end ;
@#
procedure pk_three_bytes(a:integer);
begin
   write(pk_file, a div 65536 mod 256) ;
   write(pk_file, a div 256 mod 256) ;
   write(pk_file, a mod 256) ;
   pk_loc := pk_loc + 3 ;
end ;
@#
procedure pk_word(a:integer) ;
var b : integer ;
begin
   if pk_open then begin
      if a < 0 then begin
         a := a + @'10000000000 ;
         a := a + @'10000000000 ;
         b := 128 + a div 16777216 ;
      end else b := a div 16777216 ;
      write(pk_file, b) ;
      write(pk_file, a div 65536 mod 256) ;
      write(pk_file, a div 256 mod 256) ;
      write(pk_file, a mod 256) ;
      pk_loc := pk_loc + 4 ;
   end ;
end ;
@y
@ Output is handled through |putbyte| which is supplied by web2c.

@d pk_byte(#)==begin putbyte(#, pk_file); incr(pk_loc) end

@p procedure pk_halfword(a:integer) ;
begin
   if a < 0 then a := a + 65536 ;
   putbyte(a div 256, pk_file) ;
   putbyte(a mod 256, pk_file) ;
   pk_loc := pk_loc + 2 ;
end ;
@#
procedure pk_three_bytes(a:integer);
begin
   putbyte(a div 65536 mod 256, pk_file) ;
   putbyte(a div 256 mod 256, pk_file) ;
   putbyte(a mod 256, pk_file) ;
   pk_loc := pk_loc + 3 ;
end ;
@#
procedure pk_word(a:integer) ;
var b : integer ;
begin
   if a < 0 then begin
      a := a + @'10000000000 ;
      a := a + @'10000000000 ;
      b := 128 + a div 16777216 ;
   end else b := a div 16777216 ;
   putbyte(b, pk_file) ;
   putbyte(a div 65536 mod 256, pk_file) ;
   putbyte(a div 256 mod 256, pk_file) ;
   putbyte(a mod 256, pk_file) ;
   pk_loc := pk_loc + 4 ;
end ;
@z

@x [48] Redefine find_gf_length and move_to_byte.
@p procedure find_gf_length ;
begin
   set_pos(gf_file, -1) ; gf_len := cur_pos(gf_file) ;
end ;
@#
procedure move_to_byte(@!n : integer) ;
begin
   set_pos(gf_file, n); gf_loc := n ;
end ;
@y
@d find_gf_length==gf_len:=gf_length

@p function gf_length:integer;
begin
  xfseek (gf_file, 0, 2, 'gftopk');
  gf_length := xftell (gf_file, 'gftopk');
end;
@#
procedure move_to_byte (n:integer);
begin xfseek (gf_file, n, 0, 'gftopk');
end;
@z

% [53] Make sure that |gf_byte| gets past the comment when not
% |verbose|; add do_the_rows to break up huge run of cases.
@x
   repeat
     gf_com := gf_byte ;
     case gf_com of
@y
   repeat
     gf_com := gf_byte ;
     do_the_rows:=false;
     case gf_com of
@z

@x [54] Declare |thirty_seven_cases| to help avoid breaking yacc.
@d one_sixty_five_cases(#)==sixty_four_cases(#),sixty_four_cases(#+64),
         sixteen_cases(#+128),sixteen_cases(#+144),four_cases(#+160),#+164
@y
@d thirty_seven_cases(#)==sixteen_cases(#),sixteen_cases(#+16),
	 four_cases(#+32),#+36
@d new_row_64=new_row_0 + 64
@d new_row_128=new_row_64 + 64
@z

@x [59] Break up an oversized sequence of cases for yacc.
one_sixty_five_cases(new_row_0) : begin
  if on = state then put_in_rows(extra) ;
  put_in_rows(end_of_row) ;
  on := true ; extra := gf_com - new_row_0 ; state := false ;
end ;
@t\4@>@<Specials and |no_op| cases@> ;
eoc : begin
  if on = state then put_in_rows(extra) ;
  if ( row_ptr > 2 ) and ( row[row_ptr - 1] <> end_of_row) then
    put_in_rows(end_of_row) ;
  put_in_rows(end_of_char) ;
  if bad then abort('Ran out of internal memory for row counts!') ;
@.Ran out of memory@>
  pack_and_send_character ;
  status[gf_ch_mod_256] := sent ;
  if pk_loc <> pred_pk_loc then
    abort('Internal error while writing character!') ;
@.Internal error@>
end ;
othercases bad_gf('Unexpected ',gf_com:1,' command in character definition')
@.Unexpected command@>
    endcases ;
@y
sixty_four_cases(new_row_0) : do_the_rows:=true;
sixty_four_cases(new_row_64) : do_the_rows:=true;
thirty_seven_cases(new_row_128) : do_the_rows:=true;
@<Specials and |no_op| cases@> ;
eoc : begin
  if on = state then put_in_rows(extra) ;
  if ( row_ptr > 2 ) and ( row[row_ptr - 1] <> end_of_row) then
    put_in_rows(end_of_row) ;
  put_in_rows(end_of_char) ;
  if bad then abort('Ran out of internal memory for row counts!') ;
@.Ran out of memory@>
  pack_and_send_character ;
  status[gf_ch_mod_256] := sent ;
  if pk_loc <> pred_pk_loc then
    abort('Internal error while writing character!') ;
@.Internal error@>
end ;
othercases bad_gf('Unexpected ',gf_com:1,' character in character definition');
    endcases ;
if do_the_rows then begin
  do_the_rows:=false;
  if on = state then put_in_rows(extra) ;
  put_in_rows(end_of_row) ;
  on := true ; extra := gf_com - new_row_0 ; state := false ;
end ;
@z

@x [60] Add do_the_rows to break up huge run of cases.
@ A few more locals used above and below:

@<Locals to |convert_gf_file|@>=
@y
@ A few more locals used above and below:

@<Locals to |convert_gf_file|@>=
@!do_the_rows:boolean;
@z

@x [81] Don't add `GFtoPK 2.3 output from ' to the font comment.
@d comm_length = 23 {length of |preamble_comment|}
@d from_length = 6 {length of its |' from '| part}
@y
@d comm_length = 0 {length of |preamble_comment|}
@d from_length = 0 {length of its |' from '| part}
@z

@x [83] Don't do any assignments to |preamble_comment|.
@ @<Set init...@>=
comment := preamble_comment ;
@y
@z

@x [86] Remove the final_end label
final_end : end .
@y
end.
@z

@x System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{GFtoPK} work at a particular installation.
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
      usage (1, 'gftopk'); {|getopt| has already given an error message.}

    end else if argument_is ('help') then begin
      usage (0, GFTOPK_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'Tomas Rokicki');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one or two remaining arguments.}
  if (optind + 1 <> argc) and (optind + 2 <> argc) then begin
    write_ln (stderr, 'gftopk: Need one or two file arguments.');
    usage (1, 'gftopk');
  end;
  
  gf_name := cmdline (optind);

  {If an explicit output filename isn't given, construct it from |gf_name|.}
  if optind + 2 = argc then begin
    pk_name := cmdline (optind + 1);
  end else begin
    pk_name := basename_change_suffix (gf_name, 'gf', 'pk');
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
@!gf_name, @!pk_name, @!vpl_name:c_string;
@z
