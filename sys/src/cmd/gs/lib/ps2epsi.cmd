/* $Id: ps2epsi.cmd,v 1.3 2001/06/22 16:09:22 lpd Exp $ */
/*
 * This file is maintained by a user: if you have any questions about it,
 * please contact Mark Hale (mark.hale@physics.org).
 */

@echo off
if %1/==/ goto usage
if %2/==/ goto usage

set infile=%1
set outfile=%2

rem Ghostscript uses %outfile% to define the output file
gsos2 -q -dNOPAUSE -dSAFER -dDELAYSAFER -sDEVICE=bit -sOutputFile=NUL ps2epsi.ps < %infile%

rem We bracket the actual file with a few commands to help encapsulation
echo /InitDictCount countdictstack def gsave save mark newpath >> %outfile%

rem Append the original onto the preview header
copy %outfile% + %infile%

echo countdictstack InitDictCount sub { end } repeat >> %outfile%
echo cleartomark restore grestore >> %outfile%

goto end

:usage
echo "Usage: ps2epsi <infile.ps> <outfile.epi>"

:end
