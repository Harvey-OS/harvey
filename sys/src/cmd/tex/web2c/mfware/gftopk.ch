% gftopk.ch for C compilation with web2c.
%
% 09/19/88 Pierre A. MacKay	Version 1.4.
% 12/02/89 Karl Berry		2.1.
% 01/20/90 Karl			2.2.
% (more recent changes in ./ChangeLog)
% 
% One major change in output format is made by this change file.  The
% local gftopk preamble comment is ignored and the dated METAFONT
% comment is passed through unaltered.  This provides a continuous check
% on the origin of fonts in both gf and pk formats.  The program runs
% silently unless it is given the -v switch in the command line.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{GF$\,$\lowercase{to}$\,$PK changes C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is GFtoPK, Version 2.3' {printed when the program starts}
@y
@d banner=='This is GFtoPK, C Version 2.3' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] Redefine program header.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
  @<Set initial values@>@/
  end;
@y
@ The binary input comes from |gf_file|, and the output font is written
on |pk_file|.  All text output is written on \PASCAL's standard |output|
file.  The term |print| is used instead of |write| when this program writes
on |output|, so that all such output could easily be redirected if desired.
Since the terminal output is really not very interesting, it is
produced only when the \.{-v} command line flag is presented.

@d term_out==stdout {standard output}
@d print(#)==if verbose then write(term_out, #)
@d print_ln(#)==if verbose then write_ln(term_out, #)

@p program GFtoPK;
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var i:integer; {loop index for initializations}
  begin 
  set_paths (GF_FILE_PATH_BIT);@/
  @<Set initial values@>;@/
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [5] Eliminate the |final_end| label.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ This module is deleted, because it is only useful for
a non-local goto, which we can't use in C.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [8] Make `abort' end with a newline, and remove the nonlocal goto.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d abort(#)==begin print(' ',#); jump_out;
    end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>

@p procedure jump_out;
begin goto final_end;
end;
@y
@d abort(#)==begin verbose := true; print_ln(#); uexit (1);
    end
@d bad_gf(#)==abort('Bad GF file: ',#,'!')
@.Bad GF file@>
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [38] Add UNIX_file_name type.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!eight_bits=0..255; {unsigned one-byte quantity}
@!byte_file=packed file of eight_bits; {files that contain binary data}
@y
@!eight_bits=0..255; {unsigned one-byte quantity}
@!byte_file=packed file of eight_bits; {files that contain binary data}
@!UNIX_file_name=packed array [1..PATH_MAX] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [39] Add globals.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!gf_file:byte_file; {the stuff we are \.{GFtoPK}ing}
@!pk_file:byte_file; {the stuff we have \.{GFtoPK}ed}
@y
@!gf_file:byte_file; {the stuff we are \.{GFtoPK}ing}
@!pk_file:byte_file; {the stuff we have \.{GFtoPK}ed}
@!verbose:boolean; {chatter about the conversion?}
@!pk_arg:integer; {where we may be looking for the name of the |pk_file|}
@!gf_name: UNIX_file_name;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [40] Use paths in open_gf_file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare the |gf_file| for input, we |reset| it.

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin reset(gf_file);
gf_loc := 0 ;
end;
@y
@ In C, we use the external |test_read_access| procedure, which also
does path searching based on the user's environment or the default path.
In the course of this routine we also check the command line for the
\.{-v} flag, and make other checks to see that it is worth running this
program at all.

@d usage==abort ('Usage: gftopk [-v] <gf file> [pk file].')

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
var j: integer;
begin
  verbose := false;
  pk_arg := 3;
  if (argc < 2) or (argc > 4)
  then usage;

  argv (1, gf_name);
  if gf_name[1] = xchr["-"]
  then begin
    if gf_name[2]=xchr["v"]
    then begin
      verbose := true;
      argv (2, gf_name);
      incr (pk_arg)
    end else
      usage;
  end;

  print_ln(banner);@/
  if test_read_access (gf_name, GFFILEPATH)
  then begin
    reset (gf_file, gf_name)
  end else begin
    print_pascal_string (gf_name);
    abort(': GF file not found.');
  end;

  gf_loc:=0;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [41] If the PK filename isn't given on the command line, we construct
% it from the GF filename.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare the |pk_file| for output, we |rewrite| it.

@p procedure open_pk_file; {prepares to write packed bytes in |pk_file|}
begin rewrite(pk_file);
pk_loc := 0 ; pk_open := true ;
end;
@y
procedure open_pk_file; {prepares to write packed bytes in |pk_file|}
var dot_pos, slash_pos, last, gf_index, pk_index:integer;
  @!pk_name: UNIX_file_name;
begin
  if argc = pk_arg
  then argv (argc - 1, pk_name)
  else begin
    dot_pos := -1;
    slash_pos := -1;
    last := 1;
    
    {Find the end of |gf_name|.}
    while (gf_name[last] <> ' ') and (last <= PATH_MAX - 5)
    do begin
      if gf_name[last] = '.' then dot_pos := last;
      if gf_name[last] = '/' then slash_pos := last;
      incr (last);
    end;
    
    {If no \./ in |gf_name|, use it from the beginning.}
    if slash_pos = -1 then slash_pos := 0;
    
    {Filenames like \.{./foo} will have |dot_pos<slash_pos|.  In that
     case, we want to move |dot_pos| to the end of |gf_name|.  Similarly
     if |dot_pos| is still |-1|.}
    if dot_pos < slash_pos then dot_pos := last - 1;
    
    {Copy |gf_name| from |slash_pos+1| to |dot_pos| into |pk_name|.}
    pk_index := 1;
    for gf_index := slash_pos + 1 to dot_pos
    do begin
      pk_name[pk_index] := gf_name[gf_index];
      incr (pk_index);
    end;
    
    {Now we are ready to deal with the extension.  Copy everything to
     the first \.g.  Then add \.{pk}.  This loses on filenames like
     \.{foo.g300gf}, but no one uses such filenames, anyway.}
    gf_index := dot_pos + 1;
    while (gf_index < last) and (gf_name[gf_index] <> 'g')
    do begin
      pk_name[pk_index] := gf_name[gf_index];
      incr (gf_index);
      incr (pk_index);
    end;
    
    pk_name[pk_index] := 'p';
    pk_name[pk_index + 1] := 'k';
    pk_name[pk_index + 2] := ' ';
  end;

  {Report the filename mapping.}
  print (xchr[xord['[']]);

  gf_index := 1;
  while gf_name[gf_index] <> ' ' 
  do begin
    print (xchr[xord[gf_name[gf_index]]]);
    incr (gf_index);
  end;
  
  print (xchr[xord['-']]);
  print (xchr[xord['>']]);

  pk_index := 1;
  while pk_name[pk_index] <> ' ' 
  do begin
    print (xchr[xord[pk_name[pk_index]]]);
    incr (pk_index);
  end;
  
  print (xchr[xord[']']]);
  print_ln (xchr[xord[' ']]);
  
  rewrite (pk_file, pk_name);
  pk_loc := 0;
  pk_open := true
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [46] Redefine pk_byte, pk_halfword, pk_three_bytes, and pk_word.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [48] Redefine find_gf_length and move_to_byte.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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
  checked_fseek (gf_file, 0, 2);
  gf_length := ftell (gf_file);
end;
@#
procedure move_to_byte (n:integer);
begin checked_fseek (gf_file, n, 0);
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [53] Make sure that |gf_byte| gets past the comment when not
% |verbose|; add do_the_rows to break up huge run of cases.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [54] Declare |thirty_seven_cases| to help avoid breaking yacc.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d one_sixty_five_cases(#)==sixty_four_cases(#),sixty_four_cases(#+64),
         sixteen_cases(#+128),sixteen_cases(#+144),four_cases(#+160),#+164
@y
@d thirty_seven_cases(#)==sixteen_cases(#),sixteen_cases(#+16),
	 four_cases(#+32),#+36
@d new_row_64=new_row_0 + 64
@d new_row_128=new_row_64 + 64
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [59] Break up an oversized sequence of cases for yacc.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [60] Add do_the_rows to break up huge run of cases.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ A few more locals used above and below:

@<Locals to |convert_gf_file|@>=
@y
@ A few more locals used above and below:

@<Locals to |convert_gf_file|@>=
@!do_the_rows:boolean;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [81] Don't add `GFtoPK 2.3 output from ' to the font comment.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d comm_length = 23 {length of |preamble_comment|}
@d from_length = 6 {length of its |' from '| part}
@y
@d comm_length = 0 {length of |preamble_comment|}
@d from_length = 0 {length of its |' from '| part}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [83] Don't do any assignments to |preamble_comment|.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
comment := preamble_comment ;
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [86] Remove the final_end label
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
final_end : end .
@y
end.
@z
