%   Change file for the DVItype processor, for use with WEB to C
%   This file was created by John Hobby.  It is loosely based on the
%   change file for the WEB to C version of dvitype (due to Howard
%   Trickey and Pavel Curtis).

% History:
%   3/11/90  (JDH) Original version.
%   4/30/90  (JDH) Update to handle virtual fonts

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% WEAVE: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{DVI$\,$\lowercase{type} changes for C}
@z
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Change banner string
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='% Written by DVItoMP, Version 0.1'
@y
@d banner=='% Written by DVItoMP, C Version 0.1'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Change filenames in program statement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d err_print(#)==write(#)
@d err_print_ln(#)==write_ln(#)

@p program DVI_to_MP(@!dvi_file,@!mpx_file,@!output);
@y
@d err_print(#)==write(stderr,#)
@d err_print_ln(#)==write_ln(stderr,#)

@p program DVI_to_MP;
@z

@x
  begin @<Set initial values@>@/
@y
  begin setpaths; {read environment, to find TEXFONTS, if there}
  @<Set initial values@>@/
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Increase name_length
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@!name_length=50; {a file name shouldn't be longer than this}
@y
@!name_length=FILENAMESIZE; {a file name shouldn't be longer than this}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Remove non-local goto
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
Such errors might be discovered inside of subroutines inside of subroutines,
so a procedure called |jump_out| has been introduced. This procedure, which
simply transfers control to the label |final_end| at the end of the program,
contains the only non-local |goto| statement in \.{DVItoMP}.
@^system dependencies@>

@d abort(#)==begin err_print_ln('DVItoMP abort: ',#); 
    history:=fatal_error; jump_out;
    end
@d bad_dvi(#)==abort('Bad DVI file: ',#,'!')
@.Bad DVI file@>
@d warn(#)==begin err_print_ln('DVItoMP warning: ',#);
    history:=warning_given;
    end

@p procedure jump_out;
begin goto final_end;
end;
@y
Such errors might be discovered inside of subroutines inside of subroutines,
so a procedure called |jump_out| has been introduced. This procedure is
actually just a macro that calls |exit()| with non-zero exit status.
@^system dependencies@>

@d jump_out==uexit(history)
@d abort(#)==begin err_print_ln('DVItoMP abort: ',#); 
    history:=fatal_error; jump_out;
    end
@d bad_dvi(#)==abort('Bad DVI file: ',#,'!')
@.Bad DVI file@>
@d warn(#)==begin err_print_ln('DVItoMP warning: ',#);
    history:=warning_given;
    end
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix up opening of files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ To prepare the |mpx_file| for output, we |rewrite| it.
@^system dependencies@>

@p procedure open_mpx_file; {prepares to write text on |mpx_file|}
begin rewrite(mpx_file);
end;
@y
@ To prepare the |mpx_file| for output, we |rewrite| it.
In C we retrieve the file name from the command line and use an external
|test_access| procedure to detect any problems so that we can issue
appropriate error messages.  This procedure takes the file name from
the global variable |cur_name|.  It has two other arguments that we
shall use later on to control path searching for input files.
@^system dependencies@>

@d write_access_mode=2 {``write'' mode for |test_access|}
@d no_file_path=0    {no path searching should be done}

@p procedure test_usage;
begin if (argc<2)or(argc>3) then
  begin err_print_ln('Usage: dvitomp <dvi-file> [<mpx-file>]');
  jump_out;
  end;
end;
@#
procedure open_mpx_file; {prepares to write text on |mpx_file|}
begin test_usage;
if argc<=2 then mpx_file:=stdout
else begin argv(2,cur_name);
  if test_access(write_access_mode,no_file_path) then
    rewrite(mpx_file, real_name_of_file)
  else abort('Cannot write on the mpx file');
  end;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix up opening the binary files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
specifies the file name. If |eof(f)| is true immediately after |reset(f,s)|
has acted, these routines assume that no file named |s| is accessible.
@^system dependencies@>

@p procedure open_dvi_file; {prepares to read packed bytes in |dvi_file|}
begin reset(dvi_file);
if eof(dvi_file) then abort('DVI file not found');
end;
@#
function open_tfm_file:boolean; {prepares to read packed bytes in |tfm_file|}
begin reset(tfm_file,cur_name);
return not eof(tfm_file);
end;
@#
function open_vf_file:boolean; {prepares to read packed bytes in |vf_file|}
begin reset(vf_file,cur_name);
return not eof(vf_file);
end;
@y
specifies the file name.  In C, we test the success of this operation by
first using the external |test_access| function, which also does path
searching based on the user's environment or the default path.
@^system dependencies@>

@d read_access_mode=4  {``read'' mode for |test_access|}
@d font_file_path=3  {path specifier for \.{TFM} files}
@d vf_file_path=12 {path specifier for \.{VF} files}

@p procedure open_dvi_file; {prepares to read packed bytes in |dvi_file|}
begin test_usage;
    argv(1, cur_name);
    if test_access(read_access_mode,no_file_path) then
	reset(dvi_file, real_name_of_file)
    else abort('DVI file not found');
end;
@#
function open_tfm_file:boolean; {prepares to read packed bytes in |tfm_file|}
var @!ok:boolean;
begin ok:=test_access(read_access_mode,font_file_path);
if ok then reset(tfm_file,real_name_of_file);
open_tfm_file:=ok;
end;
@#
function open_vf_file:boolean; {prepares to read packed bytes in |tfm_file|}
var @!ok:boolean;
begin ok:=test_access(read_access_mode,vf_file_path);
if ok then reset(vf_file,real_name_of_file);
open_vf_file:=ok;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Declare real_name_of_file
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
variable: |cur_name| is a string variable that will be set to the
current font metric file name before |open_tfm_file| is called.

@<Glob...@>=
@!cur_name:packed array[1..name_length] of char; {external name,
  with no lower case letters}
@y
variable: |cur_name| is a string variable that will be set to the
current font metric file name before |open_tfm_file| is called.
In C, we also have a |real_name_of_file| string, that gets
set by the external |test_access| procedure after path searching.

@<Glob...@>=
@!cur_loc:integer; {where we are about to look, in |dvi_file|}
@!cur_name,@!real_name_of_file:packed array[1..name_length] of char;
	 {external name}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set default_directory_name
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d default_directory_name=='TeXfonts:' {change this to the correct name}
@d default_directory_name_length=9 {change this to the correct length}

@<Glob...@>=
@!default_directory:packed array[1..default_directory_name_length] of char;
@y
Actually, under UNIX the standard area is defined in an external
``site.h'' file.  And the user have a path serached for fonts,
by setting the TEXFONTS environment variable.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Remove initialization of now-defunct array
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ @<Set init...@>=
default_directory:=default_directory_name;
@y
@ (No initialization to be done.  Keep this module to preserve numbering.)
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix addition of ".vf" suffix for portability and keep lowercase
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The string |cur_name| is supposed to be set to the external name of the
\.{VF} file for the current font. This usually means that we need to
prepend the name of the default directory, and
to append the suffix `\.{.VF}'. Furthermore, we change lower case letters
to upper case, since |cur_name| is a \PASCAL\ string.
@y
@ The string |cur_name| is supposed to be set to the external name of the
\.{VF} file for the current font. This usually means that we need to,
at most sites, append the
suffix ``.vf''. 
@z

@x
if area_length[f]=0 then
  begin for k:=1 to default_directory_name_length do
    cur_name[k]:=default_directory[k];
  l:=default_directory_name_length;
  end
else l:=0;
@y
l:=0;
@z

@x
  if (names[k]>="a")and(names[k]<="z") then
      cur_name[l]:=xchr[names[k]-@'40]
  else cur_name[l]:=xchr[names[k]];
  end;
cur_name[l+1]:='.'; cur_name[l+2]:='V'; cur_name[l+3]:='F'
@y
  cur_name[l]:=xchr[names[k]];
  end;
cur_name[l+1]:='.'; cur_name[l+2]:='v'; cur_name[l+3]:='f'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix the changing of ".vf" to ".tfm" analogously
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
if l=0 then l:=default_directory_name_length;
@y
@z

@x
cur_name[l+2]:='T'; cur_name[l+3]:='F'; cur_name[l+4]:='M'
@y
cur_name[l+2]:='t'; cur_name[l+3]:='f'; cur_name[l+4]:='m'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Fix printing of real numbers
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  if (abs(x)>=4096.0)or(abs(y)>=4096.0)or(m>=4096.0)or(m<0) then
    begin warn('text scaled ',m:1:1,@|
        ' at (',x:1:1,',',y:1:1,') is out of range');
    end_char_string(60);
    end
  else end_char_string(40);
  print_ln(',n',str_f:1,',',m:1:5,',',x:1:4,',',y:1:4,');');
@y
  if (abs(x)>=4096.0)or(abs(y)>=4096.0)or(m>=4096.0)or(m<0) then
    begin warn('text is out of range');
    end_char_string(60);
    end
  else end_char_string(40);
  print(',n',str_f:1,',');
  print_real(m,1,5); print(',');
  print_real(x,1,4); print(',');
  print_real(y,1,4);
  print_ln(');');
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Another fix for printing of real numbers
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
  if (abs(xx1)>=4096.0)or(abs(yy1)>=4096.0)or@|
      (abs(xx2)>=4096.0)or(abs(yy2)>=4096.0) then
    warn('hrule or vrule at (',xx1:1:1,',',yy1:1:1,') is out of range');
  print_ln('r(',xx1:1:4,',',yy1:1:4,',',xx2:1:4,',',yy2:1:4,');');
@y
  if (abs(xx1)>=4096.0)or(abs(yy1)>=4096.0)or@|
      (abs(xx2)>=4096.0)or(abs(yy2)>=4096.0) then
    warn('hrule or vrule is out of range');
  print('r(');
  print_real(xx1,1,4); print(',');
  print_real(yy1,1,4); print(',');
  print_real(xx2,1,4); print(',');
  print_real(yy2,1,4);
  print_ln(');');
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Remove unused label at end of program and exit() based on history
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
final_end:end.
@y
if history<=cksum_trouble then uexit(0)
else uexit(history);
end.
@z
