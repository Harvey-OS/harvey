% gftopk.ch for C compilation with web2c.
%
% 09/19/88 Pierre A. MacKay	Version 1.4.
% 12/02/89 Karl Berry		2.1.
% 01/20/90 Karl			2.2.
% 
% One major change in output format is incorporated by this change
% file.  The local gftopk preamble comment is ignored and the 
% dated METAFONT comment is passed through unaltered.  This
% provides a continuous check on the origin of fonts in both
% gf and pk formats.  The program runs silently unless it is given the
% -v switch in the command line.
%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{GF$\,$\lowercase{to}$\,$PK changes C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is GFtoPK, Version 2.2' {printed when the program starts}
@y
@d banner=='This is GFtoPK, C Version 2.2'
                                         {printed when the program starts}
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
  setpaths;@/
  @<Set initial values@>;@/
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [5] Eliminate the |final_end| label
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ This module is deleted, because it is only useful for
a non-local goto, which we don't use in C.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [6]  remove terminal_line_length since there is no dialog
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!line_length=79; {bracketed lines of output will be at most this long}
@!terminal_line_length=150; {maximum number of characters input in a single
  line of input from the terminal}
@y
@!line_length=79; {bracketed lines of output will be at most this long}
@z


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [8] have abort() add <nl> to end of msg and eliminate non-local goto
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
@d abort(#)==begin verbose := true; print_ln(' ',#); uexit(1);
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
@!UNIX_file_name=packed array [1..FILENAMESIZE] of text_char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [39] add gf_name, pk_name, cur_name and real_name_of_file
% 	global vars; also a boolean, gf_file_exists
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!gf_file:byte_file; {the stuff we are \.{GFtoPK}ing}
@!pk_file:byte_file; {the stuff we have \.{GFtoPK}ed}
@y
@!gf_file:byte_file; {the stuff we are \.{GFtoPK}ing}
@!pk_file:byte_file; {the stuff we have \.{GFtoPK}ed}
@!gf_name,@!pk_name,@!cur_name: UNIX_file_name;
	{names of input and output files; pascal-style origin from one}
@!real_name_of_file:packed array[0..FILENAMESIZE] of text_char;
	{C style origin from zero}
gf_file_exists, verbose:boolean;
pk_arg:integer; {where we may be looking for the name of the |pk_file|}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [40] redo open_gf_file
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare the |gf_file| for input, we |reset| it.

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin reset(gf_file);
gf_loc := 0 ;
end;
@y
@ In C, we use the external |test_access| procedure, which also does path
searching based on the user's environment or the default path.  In the course
of this routine we also check the command line for the \.{-v} flag, and make
other checks to see that it is worth running this program at all.

@d read_access_mode=4  {``read'' mode for |test_access|}
@d write_access_mode=2 {``write'' mode for |test_access|}

@d no_file_path=0    {no path searching should be done}
@d tex_font_file_path=3  {path specifier for \.{TFM} files}
@d generic_font_file_path=4  {path specifier for \.{GF} files}
@d packed_font_file_path=5  {path specifier for \.{PK} files}

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
var j:integer;
begin
    verbose := false; pk_arg :=3;
    if argc < 2 then abort('Usage: gftopk [-v] <gf file> [pk_file].');
    argv(1, cur_name);
    if cur_name[1]=xchr["-"] then begin
        if argc > 4 then abort('Usage: gftopk [-v] <gf file> [pk_file].');
        if cur_name[2]=xchr["v"] then begin
            verbose := true; argv(2, cur_name); incr(pk_arg)
            end else abort('Usage: gftopk [-v] <gf file> [pk_file].');
        end;
    print_ln(banner);@/
    gf_file_exists := test_access(read_access_mode,generic_font_file_path);
    if gf_file_exists then begin
        for j:=1 to FILENAMESIZE do gf_name[j]:=real_name_of_file[j-1];
        reset(gf_file, gf_name)
        end
    else abort('GF file not found.');
    gf_loc:=0;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [41] and open_pk_file...
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare the |pk_file| for output, we |rewrite| it.

@p procedure open_pk_file; {prepares to write packed bytes in |pk_file|}
begin rewrite(pk_file);
pk_loc := 0 ; pk_open := true ;
end;
@y
procedure open_pk_file; {prepares to write packed bytes in |pk_file|}
var j,k:integer;
begin
    if argc = pk_arg then argv(argc-1, pk_name)
    else
    begin
	j := FILENAMESIZE; k := 1;@/
	while (j > 1) and (gf_name[j] <> xchr["/"]) do@/
	    decr(j);
	if (gf_name[j]=xchr["/"]) then incr(j); { to avoid picking up the / }
        print(xchr["["]); print(xchr[" "]);
	while (j < FILENAMESIZE)
	    and (not (gf_name[j] = xchr["."]) or 
                     (gf_name[j] = xchr[" "])) do begin @/
	    pk_name[k] := gf_name[j];
            print(xchr[xord[gf_name[j]]]);
	    incr(j); incr(k)
	end;
	while (j < FILENAMESIZE)
	and not (gf_name[j] = xchr["g"]) do begin @/
 	    if gf_name[j] = xchr[" "] then abort(' No gf in suffix!');
	    pk_name[k] := gf_name[j];
            print(xchr[xord[gf_name[j]]]);
	    incr(k); incr(j)
	end;
        print(xchr[xord[gf_name[j]]]); incr(j); print(xchr[xord[gf_name[j]]]);
        print(xchr[" "]);print(xchr["-"]);print(xchr[">"]); print(xchr[" "]);
	pk_name[k] := xchr["p"]; incr(k);
	pk_name[k] := xchr["k"]; incr(k);
	pk_name[k] := xchr[" "];
        for j:=1 to k do print(xchr[xord[pk_name[j]]]);
        print_ln(xchr["]"])
    end;
    rewrite(pk_file,pk_name);
    pk_loc:=0; pk_open:=true
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [46] redefine pk_byte, pk_halfword, pk_three_bytes, and pk_word
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
% [48] redefine find_gf_length and move_to_byte
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
begin zfseek(gf_file, 0, 2); gf_length:=ftell(gf_file);
end;
@#
procedure move_to_byte(n:integer);
begin zfseek(gf_file, n, 0);
end;
@z


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [53] make sure that |gf_byte| gets past the comment when not |verbose| 
% and add do_the_rows to break up huge run of cases
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
% [54] declare |thirty_seven_cases| to avoid breaking yacc
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
% [56] break up the first oversized sequence of cases (or yacc breaks)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
sixty_four_cases(paint_0), eoc, skip0, one_sixty_five_cases(new_row_0) : ;
@y
sixty_four_cases(paint_0), eoc, skip0 : ; 
sixty_four_cases(new_row_0) : ;
sixty_four_cases(new_row_64) : ;
thirty_seven_cases(new_row_128) : ;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [59] Break up an oversized sequence of cases (or yacc breaks)
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
% [60] add do_the_rows to break up huge run of cases
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
% [84] Don't need comment
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
comment := preamble_comment ;
@y
@ @<Set init...@>=
vstrcpy(comment + 1, preamble_comment) ;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [87] Remove the final_end label
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
final_end : end .
@y
end.
@z
