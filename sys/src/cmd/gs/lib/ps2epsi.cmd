/* $Id: ps2epsi.cmd,v 1.1 2000/07/05 16:21:13 lpd Exp $ */
/*
 * This file is maintained by a user: if you have any questions about it,
 * please contact Mark Hale (mark.hale@physics.org).
 */

@echo off
if "%1"=="" goto usage
if "%2"=="" goto usage

set infile=%1
set outfile=%2

rem Ghostscript uses %outfile% to define the output file
gsos2 -q -dNOPAUSE -sDEVICE=bit -sOutputFile=NUL ps2epsi.ps < %infile%

rem We bracket the actual file with a few commands to help encapsulation
echo /InitDictCount countdictstack def gsave save mark newpath > %outfile%

rem Append the original onto the preview header
copy %outfile% + %infile%

echo countdictstack InitDictCount sub { end } repeat >> %outfile%
echo cleartomark restore grestore >> %outfile%

goto end

:usage
echo "Usage: ps2epsi <infile.ps> <outfile.epi>"

:end
