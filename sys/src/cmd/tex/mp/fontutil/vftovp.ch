% vftovp.ch for C compilation with web2c.
% 
% 01/20/90 Karl Berry		Version 1.
% 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{VF\lowercase{to}VP changes for C}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [1] Change banner string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is VFtoVP, Version 1' {printed when the program starts}
@y
@d banner=='This is VFtoVP, C Version 1' {printed when the program starts}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [2] Remove files in program statement.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p program VFtoVP(@!vf_file,@!tfm_file,@!vpl_file,@!output);
@y
@p program VFtoVP;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% still [2] Set up for path reading.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  begin print_ln(banner);@/
  @<Set initial values@>@/
@y
  begin setpaths;
  @<Set initial values@>@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] Make name_length match FILENAMESIZE.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@<Constants...@>=
@y
@d name_length==FILENAMESIZE
@<Constants...@>=
@z
@x
@!name_length=50; {a file name shouldn't be longer than this}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [7] Declare vf_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!vf_file:packed file of 0..255;
@y
@!vf_file:packed file of 0..255; {files that contain binary data}
@!vf_name:packed array[1..FILENAMESIZE] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [10] Declare tfm_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!tfm_file:packed file of 0..255;
@y
@!tfm_file:packed file of 0..255;
@!tfm_name:packed array[1..FILENAMESIZE] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [11] Open the files.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ On some systems you may have to do something special to read a
packed file of bytes. For example, the following code didn't work
when it was first tried at Stanford, because packed files have to be
opened with a special switch setting on the \PASCAL\ that was used.
@^system dependencies@>

@<Set init...@>=
reset(tfm_file); reset(vf_file);
@y
@ On some systems you may have to do something special to read a
packed file of bytes. 
@^system dependencies@>

@<Set init...@>=
if argc <> 4 then begin
  print_ln('Usage: <vfm file> <tfm file> <vpl file>.');
  uexit(1);
end;
argv(1, vf_name);
reset(vf_file, vf_name);
argv(2, tfm_name); {Use path searching to find the TFM file.}
if test_read_access(tfm_name, TFM_FILE_PATH)
then reset(tfm_file, tfm_name)
else begin
  print_pascal_string(tfm_name);
  print_ln(': TFM file not found.');
  uexit(1);
end;
print_ln(banner);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [20] Declare vpl_name.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!vpl_file:text;
@y
@!vpl_file:text;
@!vpl_name:packed array[1..FILENAMESIZE] of char;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [21] Open VPL file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
rewrite(vpl_file);
@y
@ @<Set init...@>=
argv(3, vpl_name);
rewrite(vpl_file, vpl_name);
@z


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [24] abort() should cause a bad exit code.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d abort(#)==begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a TFM?');
  goto final_end;
  end
@y
@d abort(#)==begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a TFM?');
  uexit(1);
  end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [31] Ditto for vf_abort.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d vf_abort(#)==
  begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a VF?');
  goto final_end;
  end
@y
@d vf_abort(#)==
  begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a VF?');
  uexit(1);
  end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [36] Output of real numbers.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
print_ln(' at ',(((vf[k]*256+vf[k+1])*256+vf[k+2])/@'4000000)*real_dsize:2:2,
  'pt')
@y
print(' at ');
print_real((((vf[k]*256+vf[k+1])*256+vf[k+2])/@'4000000)*real_dsize, 2, 2);
print_ln('pt')
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [39] Open another TFM file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
reset(tfm_file,cur_name);
@^system dependencies@>
if eof(tfm_file) then
  print_ln('---not loaded, TFM file can''t be opened!')
@.TFM file can\'t be opened@>
else  begin read_tfm_word; lh:=b2*256+b3;
@y
if not test_read_access(cur_name, TFM_FILE_PATH) then
  print_ln('---not loaded, TFM file can''t be opened!')
@.TFM file can\'t be opened@>
else begin reset(tfm_file, cur_name);
read_tfm_word; lh:=b2*256+b3;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [42] Remove initialization of now-defunct array.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
default_directory:=default_directory_name;
@y
@ (No initialization to be done.  Keep this module to preserve numbering.)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [44] Use lowercase `.tfm' suffix.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The string |cur_name| is supposed to be set to the external name of the
\.{TFM} file for the current font. This usually means that we need to
prepend the name of the default directory, and
to append the suffix `\.{.TFM}'. Furthermore, we change lower case letters
to upper case, since |cur_name| is a \PASCAL\ string.
@y
@ The string |cur_name| is supposed to be set to the external name of the
\.{TFM} file for the current font. This usually means that we need to,
at most sites, append the
suffix ``.tfm''. 
@z

@x
if a=0 then
  begin for k:=1 to default_directory_name_length do
    cur_name[k]:=default_directory[k];
  r:=default_directory_name_length;
  end
else r:=0;
@y
r:=0;
@z

@x
  if (vf[k]>="a")and(vf[k]<="z") then
      cur_name[r]:=xchr[vf[k]-@'40]
  else cur_name[r]:=xchr[vf[k]];
  end;
cur_name[r+1]:='.'; cur_name[r+2]:='T'; cur_name[r+3]:='F'; cur_name[r+4]:='M'
@y
  cur_name[r]:=xchr[vf[k]];
  end;
cur_name[r+1]:='.'; cur_name[r+2]:='t'; cur_name[r+3]:='f'; cur_name[r+4]:='m'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [49] Change strings to C char pointers, so we can initialize them.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!ASCII_04,@!ASCII_10,@!ASCII_14: packed array [1..32] of char;
  {strings for output in the user's external character set}
@!xchr:packed array [0..255] of char;
@!MBL_string,@!RI_string,@!RCE_string:packed array [1..3] of char;
  {handy string constants for |face| codes}
@y
@!ASCII_04,@!ASCII_10,@!ASCII_14: ccharpointer;
  {strings for output in the user's external character set}
@!xchr:packed array [0..255] of char;
@!MBL_string,@!RI_string,@!RCE_string: ccharpointer;
  {handy string constants for |face| codes}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [50] The Pascal strings are indexed starting at 1, so we pad with a blank.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
ASCII_04:=' !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:='@@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:='`abcdefghijklmnopqrstuvwxyz{|}~?';@/
@y
ASCII_04:='  !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:=' @@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:=' `abcdefghijklmnopqrstuvwxyz{|}~?';@/
@z

@x
MBL_string:='MBL'; RI_string:='RI '; RCE_string:='RCE';
@y
MBL_string:=' MBL'; RI_string:=' RI '; RCE_string:=' RCE';
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [39] Don't output the face code as an integer.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
  out(MBL_string[1+(b mod 3)]);
  out(RI_string[1+s]);
  out(RCE_string[1+(b div 3)]);
@y
  put_byte(MBL_string[1+(b mod 3)], vpl_file);
  put_byte(RI_string[1+s], vpl_file);
  put_byte(RCE_string[1+(b div 3)], vpl_file);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [62] Force 32-bit constant arithmetic for 16-bit machines.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
f:=((tfm[k+1] mod 16)*@'400+tfm[k+2])*@'400+tfm[k+3];
@y
f:=((tfm[k+1] mod 16)*toint(@'400)+tfm[k+2])*@'400+tfm[k+3];
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [112] No nonlocal goto's.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
  begin print_ln('Sorry, I haven''t room for so many ligature/kern pairs!');
@.Sorry, I haven't room...@>
  goto final_end;
@y
  begin print_ln('Sorry, I haven''t room for so many ligature/kern pairs!');
@.Sorry, I haven't room...@>
  uexit(1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% still [112] We can't have a function named `f', because of the local
% variable in do_simple_things.  It would be better, but harder, to fix
% web2c.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
     r:=f(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@y
     r:=lig_f(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);
@z

@x
  out('(INFINITE LIGATURE LOOP MUST BE BROKEN!)'); goto final_end;
@y
  out('(INFINITE LIGATURE LOOP MUST BE BROKEN!)'); uexit(1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [116] web2c can't handle these mutually recursive procedures.
% But let's do a fake definition of f here, so that it gets into web2c's
% symbol table...
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p function f(@!h,@!x,@!y:index):index; forward;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
@y
@p 
ifdef('notdef') 
function lig_f(@!h,@!x,@!y:index):index; begin end;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
endif('notdef')
@z

@x
else eval:=f(h,x,y);
@y
else eval:=lig_f(h,x,y);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [117] ... and then really define it now.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@p function f;
@y
@p function lig_f(@!h,@!x,@!y:index):index;
@z

@x
f:=lig_z[h];
@y
lig_f:=lig_z[h];
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [118] Make some labels numeric, and get rid of a return.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function string_balance(@!k,@!l:integer):boolean;
@y
@d not_found=99
@d exit=999
@p function string_balance(@!k,@!l:integer):boolean;
@z

@x
string_balance:=true; return;
@y
string_balance:=true; goto exit;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [125] `signed' is a keyword in ANSI C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function get_bytes(@!k:integer;@!signed:boolean):integer;
@y
@p function get_bytes(@!k:integer;@!is_signed:boolean):integer;
@z

@x
if (k=4) or signed then
@y
if (k=4) or is_signed then
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [126] No nonlocal goto's.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
    begin print_ln('Stack overflow!'); goto final_end;
@y
    begin print_ln('Stack overflow!'); uexit(1);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% [134] Print a newline at the end.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
final_end:end.
@y
final_end:print_ln(' '); out_ln; end.
@z
