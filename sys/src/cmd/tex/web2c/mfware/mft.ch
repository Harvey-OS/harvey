% mft.ch for C compilation with web2c.
% 
% 11/27/89 Karl Berry		version 2.0.
% 01/20/90 Karl			new mft.web (still 2.0, though).
% 
% From Pierre Mackay's version for pc, which was in turn based on Howard
% Trickey's and Pavel Curtis's change file for weave.
% 


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [0] MFT: print changes only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
\let\maybe=\iffalse
\def\title{MFT changes for Berkeley {\mc UNIX}}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [2] Change banner message
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d banner=='This is MFT, Version 2.0'
@y
@d banner=='This is MFT, C Version 2.0'
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] No need for the final label in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d end_of_MFT = 9999 {go here to wrap it up}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] main program header, remove external label.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
program MFT(@!mf_file,@!change_file,@!style_file,@!tex_file);
label end_of_MFT; {go here to finish}
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Error handling procedures@>@/
procedure initialize;
  var @<Local variables for initialization@>@/
  begin @<Set initial values@>@/
  end;
@y
program MFT;
const @<Constants in the outer block@>@/
type @<Types in the outer block@>@/
var @<Globals in the outer block@>@/
@<Error handling procedures@>@/
@<|scan_args| procedure@>@/
procedure initialize;
  var @<Local variables for initialization@>@/
  begin @<Set initial values@>@/
  end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [4] No compiler directives.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@{@&$C+,A+,D-@} {range check, catch arithmetic overflow, no debug overhead}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] The text_char type is used as an array index into xord.  The
% default type `char' produces signed integers, which are bad array
% indices in C.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] enable maximum character set
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
for i:=0 to @'37 do xchr[i]:=' ';
for i:=@'177 to @'377 do xchr[i]:=' ';
@y
for i:=1 to @'37 do xchr[i]:=chr(i);
for i:=@'177 to @'377 do xchr[i]:=chr(i);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] terminal output: use standard i/o
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d print(#)==write(term_out,#) {`|print|' means write on the terminal}
@y
@d term_out==stdout
@d print(#)==write(term_out,#) {`|print|' means write on the terminal}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Remove term_out.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
@x
@<Globals...@>=
@!term_out:text_file; {the terminal as an output file}
@y
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] init terminal
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ Different systems have different ways of specifying that the output on a
certain file will appear on the user's terminal. Here is one way to do this
on the \PASCAL\ system that was used in \.{WEAVE}'s initial development:
@^system dependencies@>

@<Set init...@>=
rewrite(term_out,'TTY:'); {send |term_out| output to the terminal}
@y
@ Different systems have different ways of specifying that the output on a
certain file will appear on the user's terminal.
@^system dependencies@>

@<Set init...@>=
{nothing need be done}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [22] flush terminal buffer
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@d update_terminal == break(term_out) {empty the terminal output buffer}
@y
@d update_terminal == fflush(term_out) {empty the terminal output buffer}
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] open input files
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The following code opens the input files.  Since these files were listed
in the program header, we assume that the \PASCAL\ runtime system has
already checked that suitable file names have been given; therefore no
additional error checking needs to be done.
@^system dependencies@>

@p procedure open_input; {prepare to read the inputs}
begin reset(mf_file); reset(change_file); reset(style_file);
end;
@y
@ The following code opens the input files.  This is called after
|scan_args| has set the file name variables appropriately.  We use this
opportunity to define some constants related to file opening and the use
of paths.  These constants are repeated in \.{mfwarext.c} and must match the
values given there.

The |test_access| routine expects a name in the global variable
|cur_name|, so we have to do a string copy to it.  |test_access| puts
the filename into |real_name_of_file|.  Likewise, the |reset|
macro expects a leading space before the filename, so we have to set
|style_file_name| to |real_name_of_file|, offset by one.  What a mess.

@^system dependencies@>

@d read_access_mode=4  {``read'' mode for |test_access|}
@d style_file_path=2  {path specifier for style files}

@p procedure open_input; {prepare to read inputs}
var i:integer;
begin reset(mf_file,mf_file_name);
      reset(change_file,change_file_name);
      for i:=1 to FILENAMESIZE do
         cur_name[i]:=style_file_name[i];
      if test_access(read_access_mode,style_file_path) then begin
         for i:=1 to FILENAMESIZE do
            style_file_name[i]:=real_name_of_file[i-1];
         reset(style_file,style_file_name);
      end
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Opening the .tex file.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The following code opens |tex_file|.
Since this file was listed in the program header, we assume that the
\PASCAL\ runtime system has checked that a suitable external file name has
been given.
@^system dependencies@>

@<Set init...@>=
rewrite(tex_file);
@y
@ The following code opens |tex_file|.
The |scan_args| procedure is used to set up |tex_file_name| as required.
@^system dependencies@>

@<Set init...@>=
scan_args;
rewrite(tex_file,tex_file_name);
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] faster input_ln
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@p function input_ln(var f:text_file):boolean;
  {inputs a line or returns |false|}
var final_limit:0..buf_size; {|limit| without trailing blanks}
begin limit:=0; final_limit:=0;
if eof(f) then input_ln:=false
else  begin while not eoln(f) do
    begin buffer[limit]:=xord[f^]; get(f);
    incr(limit);
    if buffer[limit-1]<>" " then final_limit:=limit;
    if limit=buf_size then
      begin while not eoln(f) do get(f);
      decr(limit); {keep |buffer[buf_size]| empty}
      if final_limit>limit then final_limit:=limit;
      print_nl('! Input line too long'); loc:=0; error;
@.Input line too long@>
      end;
    end;
  read_ln(f); limit:=final_limit; input_ln:=true;
  end;
end;
@y
With C, we call an external C procedure, |line_read|.  That routine
fills |buffer| from 0 onwards with the |xord|'ed values of the next
line, setting |limit| appropriately (ignoring trailing blanks).  It will
stop if |limit=buf_size|, and the following will cause an error message.
For bootstrapping purposes it is all right to use the original
form of |input_ln|; it will just run slower.

@p function input_ln(var f:text_file):boolean;
  {inputs a line or returns |false|}
begin limit:=0;
if test_eof(f) then input_ln:=false
else  begin line_read(f);
    if limit=buf_size then
      begin
      decr(limit); {keep |buffer[buf_size]| empty}
      {if final_limit>limit then final_limit:=limit;}
      print_nl('! Input line too long'); loc:=0; error;
@.Input line too long@>
      end;
   input_ln:=true;
  end;
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] Fix jump_out
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@ The |jump_out| procedure just cuts across all active procedure levels
and jumps out of the program. This is the only non-local \&{goto} statement
in \.{MFT}. It is used when no recovery from a particular error has
been provided.

Some \PASCAL\ compilers do not implement non-local |goto| statements.
@^system dependencies@>
In such cases the code that appears at label |end_of_MFT| should be
copied into the |jump_out| procedure, followed by a call to a system procedure
that terminates the program.

@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; jump_out;
  end

@<Error handling...@>=
procedure jump_out;
begin goto end_of_MFT;
end;
@y
@ The |jump_out| procedure cleans up, prints appropriate messages,
and exits back to the operating system.

@d fatal_error(#)==begin new_line; print(#); error; mark_fatal; jump_out;
	end

@<Error handling...@>=
procedure jump_out;
begin
@t\4\4@>{here files should be closed if the operating system requires it}
  @<Print the job |history|@>;
  new_line;
  if (history <> spotless) and (history <> harmless_message) then
    uexit(1)
  else
    uexit(0);
end;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [??] print newline at end of run, exit based upon value of history,
% and remove the end_of_MFT label.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
Let's put it all together now: \.{MFT} starts and ends here.
@^system dependencies@>

@y
Let's put it all together now: \.{MFT} starts and ends here.
@^system dependencies@>

@d UNIXexit==e@&x@&i@&t
@z

@x
end_of_MFT:{here files should be closed if the operating system requires it}
@<Print the job |history|@>;
end.
@y
@<Print the job |history|@>;
new_line;
if (history <> spotless) and (history <> harmless_message) then
    UNIXexit(1)
else
    UNIXexit(0);
end.
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [112] system dependent changes--the scan_args procedure
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@x
@* System-dependent changes.
This module should be replaced, if necessary, by changes to the program
that are necessary to make \.{MFT} work at a particular installation.
It is usually best to design your change file so that all changes to
previous modules preserve the module numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new modules, can be inserted here; then only the index
itself will get a new module number.
@^system dependencies@>
@y
@* System-dependent changes.

The user calls \.{MFT} with arguments on the command line.  These are
either file names or flags (beginning with `\.-').  The following globals
are for communicating the user's desires to the rest of the program. The
various |file_name| variables contain strings with the full names of
those files, as UNIX knows them.

The flags that affect \.{MFT} are \.{-c} and \.{-s}, whose 
statuses is kept in |no_change| and |no_style|, respectively.

@<Globals...@>=
@!mf_file_name,@!change_file_name,@!style_file_name,@!tex_file_name,
        @!cur_name:packed array[1..FILENAMESIZE] of char;
@!real_name_of_file:packed array[0..FILENAMESIZE] of char;
@!no_change,no_style:boolean;

@ The |scan_args| procedure looks at the command line arguments and sets
the |file_name| variables accordingly.

At least one file name must be present as the first argument: the \.{mf}
file.  It may have an extension, or it may omit it to get |'.mf'| added.
If there is only one file name, the output file name is formed by
replacing the \.{mf} file name extension by |'.tex'|.  Thus, the command
line \.{mf foo} implies the use of the \METAFONT\ input file \.{foo.mf}
and the output file \.{foo.tex}.  If this style of command line, with
only one argument is used, the default style file, |plain.mft|, will be
used to provide minimal formatting.

An argument beginning with a minus sign is a flag.  Any letters
following the minus sign may cause global flag variables to be set.
Currently, a \.c means that there is a change file, and an \.s means
that there is a style file.  The auxiliary files must of course appear
in the same order as the flags.  For example, the flag \.{-sc} must be
followed by the name of the |style_file| first, and then the name of the
|change_file|.

@<|scan_args|...@>=
procedure scan_args;
var dot_pos,i,a: integer; {indices}
@!which_file_next: char;
@!fname: array[1..FILENAMESIZE] of char; {temporary argument holder}
@!found_mf,@!found_change,found_style: boolean; {|true| when those 
                                                 file names have been seen}
begin
set_paths; {Get style file path from the environment variable \.{TEXINPUTS}}
found_mf:=false;
@<Set up null change file@>; found_change:=true; {Default to no change file}
@<Set up plain style file@>; found_style:=true; {Default to plain style file}
for a:=1 to argc-1 do begin
  argv(a,fname); {put argument number |a| into |fname|}
  if fname[1]<>'-' then begin
    if not found_mf then
      @<Get |mf_file_name| from |fname|, and set up |tex_file_name@>
    else 
      if not found_change then begin
        if which_file_next <> 's' then begin
                @<Get |change_file_name| from |fname|@>;
                which_file_next := 's'
                end
        else @<Get |style_file_name| from |fname|@>
        end
      else if not found_style then begin
        if which_file_next = 's' then begin
                @<Get |style_file_name| from |fname|@>; 
                which_file_next := 'c'
                end;
        end
    else  @<Print usage error message and quit@>;
    end
  else @<Handle flag argument in |fname|@>;
  end;
if not found_mf then @<Print usage error message and quit@>;
end;

@ Use all of |fname| for the |mf_file_name| if there is a |'.'| in it,
otherwise add |'.mf'|.  The \TeX\ file name comes from adding things
after the dot.  The |argv| procedure will not put more than
|FILENAMESIZE-5| characters into |fname|, and this leaves enough room in
the |file_name| variables to add the extensions.  We declared |fname| to
be of size |FILENAMESIZE| instead of |FILENAMESIZE-5| because the latter
implies |FILENAMESIZE| must be a numeric constant---and we don't want to
repeat the value from \.{site.h} just to save five bytes of memory.

The end of a file name is marked with a |' '|, the convention assumed by 
the |reset| and |rewrite| procedures.

@<Get |mf_file_name|...@>=
begin
dot_pos:=-1;
i:=1;
while (fname[i]<>' ') and (i<=FILENAMESIZE-5) do begin
        mf_file_name[i]:=fname[i];
        if fname[i]='.' then dot_pos:=i;
        incr(i);
        end;
if dot_pos=-1 then begin
        dot_pos:=i;
        mf_file_name[dot_pos]:='.';
        mf_file_name[dot_pos+1]:='m';
        mf_file_name[dot_pos+2]:='f';
        mf_file_name[dot_pos+3]:=' ';
        end;
for i:=1 to dot_pos do begin
        tex_file_name[i]:=mf_file_name[i];
        end;
tex_file_name[dot_pos+1]:='t';
tex_file_name[dot_pos+2]:='e';
tex_file_name[dot_pos+3]:='x';
tex_file_name[dot_pos+4]:=' ';
which_file_next := 'z';
found_mf:=true;
end

@ Use all of |fname| for the |change_file_name| if there is a |'.'| in it,
otherwise add |'.ch'|.

@<Get |change_file_name|...@>=
begin
dot_pos:=-1;
i:=1;
while (fname[i]<>' ') and (i<=FILENAMESIZE-5)
do begin
        change_file_name[i]:=fname[i];
        if fname[i]='.' then dot_pos:=i;
        incr(i);
        end;
if dot_pos=-1 then begin
        dot_pos:=i;
        change_file_name[dot_pos]:='.';
        change_file_name[dot_pos+1]:='c';
        change_file_name[dot_pos+2]:='h';
        change_file_name[dot_pos+3]:=' ';
        end;
which_file_next := 'z';
found_change:=true;
end

@ Use all of |fname| for the |style_file_name| if there is a |'.'| in it,
otherwise add |'.mft'|.


@<Get |style_file_name|...@>=
begin
dot_pos:=-1;
i:=1;
while (fname[i]<>' ') and (i<=FILENAMESIZE-5)
do begin
        style_file_name[i]:=fname[i];
        if fname[i]='.' then dot_pos:=i;
        incr(i);
        end;
if dot_pos=-1 then begin
        dot_pos:=i;
        style_file_name[dot_pos]:='.';
        style_file_name[dot_pos+1]:='m';
        style_file_name[dot_pos+2]:='f';
        style_file_name[dot_pos+3]:='t';
        style_file_name[dot_pos+4]:=' ';
        end;
which_file_next := 'z';
found_style:=true;
end

@

@<Set up null change file@>=
begin
        change_file_name[1]:='/';
        change_file_name[2]:='d';
        change_file_name[3]:='e';
        change_file_name[4]:='v';
        change_file_name[5]:='/';
        change_file_name[6]:='n';
        change_file_name[7]:='u';
        change_file_name[8]:='l';
        change_file_name[9]:='l';
        change_file_name[10]:=' ';
end

@

@<Set up plain style file@>=
begin
        style_file_name[1]:='p';
        style_file_name[2]:='l';
        style_file_name[3]:='a';
        style_file_name[4]:='i';
        style_file_name[5]:='n';
        style_file_name[6]:='.';
        style_file_name[7]:='m';
        style_file_name[8]:='f';
        style_file_name[9]:='t';
        style_file_name[10]:=' ';
end

@

@<Handle flag...@>=
begin
i:=2;
while (fname[i]<>' ') and (i<=FILENAMESIZE-5) do begin
        if fname[i]='c' then begin found_change:=false;
                if which_file_next <> 's' then which_file_next := 'c'
                end
        else if fname[i]='s' then begin found_style:=false;
                  if which_file_next <> 'c' then which_file_next := 's'
                  end
	        else print_nl('Invalid flag ',xchr[xord[fname[i]]], '.');
        incr(i);
        end;
end

@

@<Print usage error message and quit@>=
begin
print_ln('Usage: mft file[.mf] [-cs] [change[.ch]] [style[.mft]].');
uexit(1);
end
@z
