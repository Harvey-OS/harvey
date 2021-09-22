% gftodvi.ch for C compilation with web2c.
%
% (More recent changes in the ChangeLog)
% 01/20/90 Karl		New gftodvi.web (same version number).
% 12/02/89 Karl Berry	To version 3.
% Revision 1.7.1.5  86/02/01  15:29:58  richards
% 	Released again for MF 1.0 package
% Revision 1.7.1.4  86/02/01  15:06:50  richards
% 	Added: <nl> at end of successful run
% Revision 1.7.1.3  86/01/27  16:39:48  richards
% 	Fixed: syntax error in previous edits
% Revision 1.7.1.2  86/01/27  15:55:58  richards
% 	Added: dvi_buf_type declaration and redefined dvi_buf[] in
% 	       terms of it, so we can use it as a parameter to b_write_buf()
% Revision 1.7.1.1  86/01/27  15:39:10  richards
% 	First edit to use new binary I/O routines
% Revision 1.7  85/10/21  21:55:50  richards
% 	Released for GFtoDVI 1.7
% Revision 1.3.7.1  85/10/18  22:59:01  richards
% 	Updated for GFtoDVI Version 1.7 (Distributed w/ MF84 Version 0.9999)
% Revision 1.3.5.1  85/10/09  17:02:35  richards
% 	First draft to run at 1.5 level
% Revision 1.3  85/05/27  21:15:30  richards
% 	Updated for GFtoDVI Version 1.3 (Distributed w/ MF84 Version 0.91)
% Revision 1.2  85/04/25  19:33:30  richards
% 	Updated to GFtoDVI Version 1.2 (Distributed w/ MF84 Version 0.81)
% Revision 1.1  85/03/03  21:47:17  richards
% 	Updated for GF utilities distributed with MF Version 0.77
% Revision 1.0  84/12/16  22:38:22  richards
% 	Updated for GFtoDVI Version 1.0 (New GF file format)
% Revision 0.6  84/12/05  13:32:01  richards
% 	Updated for GFtoDVI Version 0.6; merged in changes from sdcarl!rusty
% 	Note: still has BUGFIX in section 199 to keep GFtoDVI from trying
% 	to use non-existent characters in a gray font
% Revision 0.3  84/11/17  23:51:56  richards
% 	Base version for GFtoDVI Version 0.3

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{GF$\,$\lowercase{to}$\,$DVI changes for C}
@z

@x [3] Redirect output to term_out.
@d print(#)==write(#)
@d print_ln(#)==write_ln(#)
@d print_nl(#)==@+begin write_ln; write(#);@+end
@y
@d print(#)==write(stdout, #)
@d print_ln(#)==write_ln(stdout, #)
@d print_nl(#)==@+begin write_ln(stdout); write(stdout, #);@+end
@z

@x [still 3] Toss labels.
label @<Labels in the outer block@>@/
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
procedure initialize; {this procedure gets things started properly}
  var @!i,@!j,@!m,@!n:integer; {loop indices for initializations}
  begin print_ln(banner);@/
@y
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var @!i,@!j,@!m,@!n:integer; {loop indices for initializations}
  begin
    kpse_set_progname (argv[0]);
    kpse_init_prog ('GFTODVI', 0, nil, nil);
    parse_arguments;
    if verbose then begin
      print (banner);
      print_ln (version_string);
    end;
@z

% [4] Remove the final_end label.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@ If the program has to stop prematurely, it goes to the
`|final_end|'.

@d final_end=9999 {label for the end of it all}

@<Labels...@>=final_end;
@y
@ This module deleted, since it only defined the label |final_end|.
@z

@x [8] Add newline to end of abort() message, and exit abnormally.
@d abort(#)==@+begin print(' ',#); jump_out;@+end
@y
@d abort(#)==@+begin write_ln (stderr, #); uexit (1);@+end
@z

% [8] Remove nonlocal goto.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p procedure jump_out;
begin goto final_end;
end;
@y
@p procedure jump_out;
begin uexit(0);
end;
@z

% [11] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x [14] Allow any input character.
for i:=0 to @'37 do xchr[i]:='?';
for i:=@'177 to @'377 do xchr[i]:='?';
@y
for i:=1 to @'37 do xchr[i]:=chr(i);
for i:=@'177 to @'377 do xchr[i]:=chr(i);
@z


@x [15] Change `update_terminal' to `flush', `term_in' is stdin.
Since the terminal is being used for both input and output, some systems
need a special routine to make sure that the user can see a prompt message
before waiting for input based on that message. (Otherwise the message
may just be sitting in a hidden buffer somewhere, and the user will have
no idea what the program is waiting for.) We shall call a system-dependent
subroutine |update_terminal| in order to avoid this problem.

@d update_terminal == break(output) {empty the terminal output buffer}

@<Glob...@>=
@!buffer:array[0..terminal_line_length] of 0..255;
@!term_in:text_file; {the terminal, considered as an input file}
@y
Since the terminal is being used for both input and output, some systems
need a special routine to make sure that the user can see a prompt message
before waiting for input based on that message. (Otherwise the message
may just be sitting in a hidden buffer somewhere, and the user will have
no idea what the program is waiting for.) We shall call a system-dependent
subroutine |update_terminal| in order to avoid this problem.
@^system dependencies@>

@d update_terminal == fflush (stdout) {empty the terminal output buffer}
@d term_in == stdin {standard input}

@<Glob...@>=
@!buffer:array[0..terminal_line_length] of 0..255;
@z

@x [17] Change term_in^, etc.
@p procedure input_ln; {inputs a line from the terminal}
begin update_terminal; reset(term_in);
if eoln(term_in) then read_ln(term_in);
line_length:=0;
while (line_length<terminal_line_length)and not eoln(term_in) do
  begin buffer[line_length]:=xord[term_in^]; incr(line_length); get(term_in);
  end;
end;
@y
@p procedure input_ln; {inputs a line from the terminal}
begin update_terminal;
if eoln(term_in) then read_ln(term_in);
line_length:=0;
while (line_length<terminal_line_length)and not eoln(term_in) do
  begin buffer[line_length]:=xord[getc(term_in)]; incr(line_length);
  end;
end;
@z

@x [47] Open files based on paths.
@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin reset(gf_file,name_of_file);
cur_loc:=0;
end;
@#
procedure open_tfm_file; {prepares to read packed bytes in |tfm_file|}
begin reset(tfm_file,name_of_file);
end;
@#
procedure open_dvi_file; {prepares to write packed bytes in |dvi_file|}
begin rewrite(dvi_file,name_of_file);
end;
@y
In C, we do path searching based on the user's environment or the
default path.  We also read the command line and print the banner here
(since we don't want to print the banner if the command line is
unreasonable).

@p procedure open_gf_file; {prepares to read packed bytes in |gf_file|}
begin
  gf_file := kpse_open_file (name_of_file, kpse_gf_format);
  cur_loc := 0;
end;
@#
procedure open_tfm_file; {prepares to read packed bytes in |tfm_file|}
begin
   tfm_file := kpse_open_file (name_of_file, kpse_tfm_format);
end;
@#
procedure open_dvi_file; {prepares to write packed bytes in |dvi_file|}
begin rewritebin(dvi_file,name_of_file);
end;
@z

% [48] Don't force a maximum length for name_of_file.  See comments in
% tex.ch for why we change the element type to text_char.
@x
@!name_of_file:packed array[1..file_name_size] of char; {external file name}
@y
@!name_of_file:^text_char;
@z

@x [51] Make get_n_bytes routines work with 16-bit math.
get_two_bytes:=a*256+b;
@y
get_two_bytes:=a*toint(256)+b;
@z
@x
get_three_bytes:=(a*256+b)*256+c;
@y
get_three_bytes:=(a*toint(256)+b)*256+c;
@z
@x
if a<128 then signed_quad:=((a*256+b)*256+c)*256+d
else signed_quad:=(((a-256)*256+b)*256+c)*256+d;
@y
if a<128 then signed_quad:=((a*toint(256)+b)*256+c)*256+d
else signed_quad:=(((a-256)*toint(256)+b)*256+c)*256+d;
@z

% [52] The memory_word structure is too hard to translate via web2c, so
% we use a hand-coded include file.  Also, b0 (et al.) is used both as a
% field and as a regular variable.  web2c puts field names in the global
% symbol table, so this loses.  Rather than fix web2c (hard), we change
% the name of the field (ugly, but easy).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@!four_quarters = packed record@;@/
  @!b0:quarterword;
  @!b1:quarterword;
  @!b2:quarterword;
  @!b3:quarterword;
  end;
@!memory_word = record@;@/
  case boolean of
  true: (@!sc:scaled);
  false: (@!qqqq:four_quarters);
  end;
@y
@!four_quarters = packed record@;@/
  @!B0:quarterword;
  @!B1:quarterword;
  @!B2:quarterword;
  @!B3:quarterword;
  end;
@\@/@=#include "gftodmem.h";@>@\ {note the |;| so |web2c| will translate
                                  types that come after this}
@z

% [55] fix references to .b0
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d char_width_end(#)==#.b0].sc
@d char_width(#)==font_info[width_base[#]+char_width_end
@d char_exists(#)==(#.b0>min_quarterword)
@d char_italic_end(#)==(qo(#.b2)) div 4].sc
@d char_italic(#)==font_info[italic_base[#]+char_italic_end
@d height_depth(#)==qo(#.b1)
@d char_height_end(#)==(#) div 16].sc
@d char_height(#)==font_info[height_base[#]+char_height_end
@d char_depth_end(#)==# mod 16].sc
@d char_depth(#)==font_info[depth_base[#]+char_depth_end
@d char_tag(#)==((qo(#.b2)) mod 4)
@d skip_byte(#)==qo(#.b0)
@d next_char(#)==#.b1
@d op_byte(#)==qo(#.b2)
@d rem_byte(#)==#.b3
@y
@d char_width_end(#)==#.B0].sc
@d char_width(#)==font_info[width_base[#]+char_width_end
@d char_exists(#)==(#.B0>min_quarterword)
@d char_italic_end(#)==(qo(#.B2)) div 4].sc
@d char_italic(#)==font_info[italic_base[#]+char_italic_end
@d height_depth(#)==qo(#.B1)
@d char_height_end(#)==(#) div 16].sc
@d char_height(#)==font_info[height_base[#]+char_height_end
@d char_depth_end(#)==# mod 16].sc
@d char_depth(#)==font_info[depth_base[#]+char_depth_end
@d char_tag(#)==((qo(#.B2)) mod 4)
@d skip_byte(#)==qo(#.B0)
@d next_char(#)==#.B1
@d op_byte(#)==qo(#.B2)
@d rem_byte(#)==#.B3
@z

% [60] Fix 16-bit arithmetic bugs in TFM calculations.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@ @d read_two_halves_end(#)==#:=b2*256+b3
@d read_two_halves(#)==read_tfm_word; #:=b0*256+b1; read_two_halves_end
@y
@ @d read_two_halves_end(#)==#:=b2*toint(256)+b3
@d read_two_halves(#)==read_tfm_word; #:=b0*toint(256)+b1; read_two_halves_end
@z

% [62] More .b?'s.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
  qw.b0:=qi(b0); qw.b1:=qi(b1); qw.b2:=qi(b2); qw.b3:=qi(b3);
@y
  qw.B0:=qi(b0); qw.B1:=qi(b1); qw.B2:=qi(b2); qw.B3:=qi(b3);
@z

% [62] More arithmetic fixes.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
z:=((b0*256+b1)*256+b2)*16+(b3 div 16);
@y
z:=((b0*toint(256)+b1)*toint(256)+b2)*16+(b3 div 16);
@z
@x
      else if 256*(b2-128)+b3>=nk then abend;
@y
      else if toint(256)*(b2-128)+b3>=nk then abend;
@z

@x [78] Change default extension to `.2602gf'.
l:=3; init_str3(".")("g")("f")(gf_ext);@/
@y
l:=7; init_str7(".")("2")("6")("0")("2")("g")("f")(gf_ext);@/
@z

@x [88] Change home_font_area to null_string.
l:=9; init_str9("T")("e")("X")("f")("o")("n")("t")("s")(":")(home_font_area);@/
@y
l:=0; init_str0(home_font_area);@/
@z

@x [90] Change more_name to understand Unix file name syntax.
else  begin if (c=">")or(c=":") then
    begin area_delimiter:=pool_ptr; ext_delimiter:=0;
    end
  else if (c=".")and(ext_delimiter=0) then ext_delimiter:=pool_ptr;
@y
else  begin if (c="/") then
    begin area_delimiter:=pool_ptr; ext_delimiter:=0;
    end
  else if c="." then ext_delimiter:=pool_ptr;
@z

@x [92] pack_file_name must pack for C conventions.
@d append_to_name(#)==begin c:=#; incr(k);
  if k<=file_name_size then name_of_file[k]:=xchr[c];
@y
@d append_to_name(#)==begin c:=#; incr(k);
  name_of_file[k]:=xchr[c];
@z

@x
@!name_length:0..file_name_size; {number of characters packed}
begin k:=0;
for j:=str_start[a] to str_start[a+1]-1 do append_to_name(str_pool[j]);
for j:=str_start[n] to str_start[n+1]-1 do append_to_name(str_pool[j]);
for j:=str_start[e] to str_start[e+1]-1 do append_to_name(str_pool[j]);
if k<=file_name_size then name_length:=k@+else name_length:=file_name_size;
for k:=name_length+1 to file_name_size do name_of_file[k]:=' ';
@y
@!name_length: integer;
begin
  name_length := length (a) + length (n) + length (e);
  name_of_file := xmalloc (name_length + 1);
  k := -1; {C strings start at position zero.}
for j:=str_start[a] to str_start[a+1]-1 do append_to_name(str_pool[j]);
for j:=str_start[n] to str_start[n+1]-1 do append_to_name(str_pool[j]);
for j:=str_start[e] to str_start[e+1]-1 do append_to_name(str_pool[j]);
  name_of_file[name_length] := 0;
@z

@x [94] Change start_gf to get file name from the command line.
@ The |start_gf| procedure prompts the user for the name of the generic
font file to be input. It opens the file, making sure that some input is
present; then it opens the output file.

Although this routine is system-independent, it should probably be
modified to take the file name from the command line (without an initial
prompt), on systems that permit such things.

@p procedure start_gf;
label found,done;
begin loop@+begin print_nl('GF file name: '); input_ln;
@.GF file name@>
  buf_ptr:=0; buffer[line_length]:="?";
  while buffer[buf_ptr]=" " do incr(buf_ptr);
  if buf_ptr<line_length then
    begin @<Scan the file name in the buffer@>;
    if cur_ext=null_string then cur_ext:=gf_ext;
    pack_file_name(cur_name,cur_area,cur_ext); open_gf_file;
    if not eof(gf_file) then goto found;
    print_nl('Oops... I can''t find file '); print(name_of_file);
@.Oops...@>
@.I can't find...@>
    end;
  end;
found:job_name:=cur_name; pack_file_name(job_name,null_string,dvi_ext);
open_dvi_file;
end;
@y
@ The |start_gf| procedure obtains the name of the generic font file to
be input from the command line.  It opens the file, making sure that
some input is present; then it opens the output file.

@p procedure start_gf;
label done;
var arg_buffer: ^char;
    arg_buf_ptr: integer;
begin
  arg_buffer := cmdline (optind);
  arg_buf_ptr := 0;
  while (line_length < terminal_line_length)
        and (arg_buffer[arg_buf_ptr] <> 0) do begin
    buffer[line_length] := xord[arg_buffer[arg_buf_ptr]];
    incr(line_length);
    incr(arg_buf_ptr);
  end;

  buf_ptr:=0; buffer[line_length]:="?";
  while buffer[buf_ptr]=" " do incr(buf_ptr);
  if buf_ptr < line_length
  then begin
    @<Scan the file name in the buffer@>;
    if cur_ext = null_string then cur_ext:=gf_ext;
    pack_file_name (cur_name, cur_area, cur_ext);
    open_gf_file;
  end;
  job_name := cur_name;
  pack_file_name(job_name, null_string, dvi_ext);
  open_dvi_file;
end;
@z

@x [107] `write_dvi' is now an external C routine.
@p procedure write_dvi(@!a,@!b:dvi_index);
var k:dvi_index;
begin for k:=a to b do write(dvi_file,dvi_buf[k]);
end;
@y
In C, we can write out the entire array with one call.
@p procedure write_dvi(@!a,@!b:dvi_index);
begin 
  write_chunk (dvi_file, dvi_buf, a, b);
end;
@z

% [111] More .b?'s.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
dvi_out(qo(font_check[f].b0));
dvi_out(qo(font_check[f].b1));
dvi_out(qo(font_check[f].b2));
dvi_out(qo(font_check[f].b3));@/
@y
dvi_out(qo(font_check[f].B0));
dvi_out(qo(font_check[f].B1));
dvi_out(qo(font_check[f].B2));
dvi_out(qo(font_check[f].B3));@/
@z

% [115] Don't go to final_end, just exit; this is the normal exit from
% the program, so we want to end with a newline if we are being verbose.
@x
goto final_end;
@y
if verbose then print_ln (' ');
uexit (0);
@z

% [118] And still more .b?'s.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
dummy_info.b0:=qi(0); dummy_info.b1:=qi(0); dummy_info.b2:=qi(0);
dummy_info.b3:=qi(0);
@y
dummy_info.B0:=qi(0); dummy_info.B1:=qi(0); dummy_info.B2:=qi(0);
dummy_info.B3:=qi(0);
@z

% [138] write_ln formatting.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
begin if abs(r-slant_reported)>0.001 then
  begin print_nl('Sorry, I can''t make diagonal rules of slant ',r:10:5,'!');
@y
begin if fabs(r-slant_reported)>0.001 then
  begin print_nl('Sorry, I can''t make diagonal rules of slant ');
        print_real(r,10,5); print('!');
@z

% [164] No progress report unless verbose.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
print('[',total_pages:1); update_terminal; {print a progress report}
@y
if verbose
then begin
  print('[',total_pages:1);
  update_terminal; {print a progress report}
end;
@z

@x
print(']'); update_terminal;
@y
if verbose
then begin
  print(']');
  if total_pages mod 13 = 0
  then print_ln (' ')
  else print (' ');
  update_terminal;
end;
@z

% [170] Change offset for overflow labels.  The defaults adds about 2.1
% inches to the right edge of the diagram, which puts it off the paper
% for even moderately large fonts.  Instead, we make it a command-line
% option.
@x
over_col:=over_col+delta_x+10000000;
@y
over_col := over_col + delta_x + overflow_label_offset;
@z

% [215] Some broken compilers cannot handle 165 labels for the same
% branch of a switch.
@x
@<Read and process...@>=
loop  @+begin continue: case cur_gf of
  sixty_four_cases(0): k:=cur_gf;
  paint1:k:=get_byte;
  paint2:k:=get_two_bytes;
  paint3:k:=get_three_bytes;
  eoc:goto done1;
  skip0:end_with(blank_rows:=0; do_skip);
  skip1:end_with(blank_rows:=get_byte; do_skip);
  skip2:end_with(blank_rows:=get_two_bytes; do_skip);
  skip3:end_with(blank_rows:=get_three_bytes; do_skip);
  sixty_four_cases(new_row_0),sixty_four_cases(new_row_0+64),
   thirty_two_cases(new_row_0+128),five_cases(new_row_0+160):
    end_with(z:=cur_gf-new_row_0;paint_black:=true);
  xxx1,xxx2,xxx3,xxx4,yyy,no_op:begin skip_nop; goto continue;
    end;
  othercases bad_gf('Improper opcode')
  endcases;@/
@y
@<Read and process...@>=
loop  @+begin continue:
 if (cur_gf>=new_row_0)and(cur_gf<=new_row_0+164) then
    end_with(z:=cur_gf-new_row_0;paint_black:=true)
 else case cur_gf of
  sixty_four_cases(0): k:=cur_gf;
  paint1:k:=get_byte;
  paint2:k:=get_two_bytes;
  paint3:k:=get_three_bytes;
  eoc:goto done1;
  skip0:end_with(blank_rows:=0; do_skip);
  skip1:end_with(blank_rows:=get_byte; do_skip);
  skip2:end_with(blank_rows:=get_two_bytes; do_skip);
  skip3:end_with(blank_rows:=get_three_bytes; do_skip);
  xxx1,xxx2,xxx3,xxx4,yyy,no_op:begin skip_nop; goto continue;
    end;
  othercases bad_gf('Improper opcode')
  endcases;@/
@z

@x [still 219] If verbose, output a newline at the end.
final_end:end.
@y
  if verbose and (total_pages mod 13 <> 0) then print_ln (' ');
end.
@z

@x [222] System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{GFtoDVI} work at a particular installation.
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
      usage (1, 'gftodvi');
    
    end else if argument_is ('help') then begin
      usage (0, GFTODVI_HELP);

    end else if argument_is ('version') then begin
      print_version_and_exit (banner, nil, 'D.E. Knuth');

    end else if argument_is ('overflow-label-offset') then begin
      offset_in_points := atof (optarg);
      overflow_label_offset := round (offset_in_points * 65536);

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one remaining argument.}
  if (optind + 1 <> argc) then begin
    write_ln (stderr, 'gftodvi: Need exactly one file argument.');
    usage (1, 'gftodvi');
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

@ Change how far from the right edge of the character boxes we print
overflow labels.
@.-overflow-label-offset@>

@<Define the option...@> =
long_options[current_option].name := 'overflow-label-offset';
long_options[current_option].has_arg := 1;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ It's easier on the user to specify the value in \TeX\ points, but we
want to store it in scaled points.

@<Glob...@> =
@!overflow_label_offset: integer; {in scaled points}
@!offset_in_points: real;

@ The default offset is ten million scaled points---a little more than
two inches.

@<Initialize the option...@> =
overflow_label_offset := 10000000;

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
@z
