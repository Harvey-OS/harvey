#!/bin/sh
# $Id: ps2ps2.bat,v 1.2 2005/07/20 06:00:54 igor Exp $
@rem Converting Postscript 3 or PDF into PostScript 2.

if %1/==/ goto usage
if %2/==/ goto usage
call gssetgs.bat
echo -dNOPAUSE -dSAFER -dBATCH >_.at
:cp
if %3/==/ goto doit
echo %1 >>_.at
shift
goto cp

:doit
rem Watcom C deletes = signs, so use # instead.
%GSC% -q -sDEVICE#ps2write -sOutputFile#%2 @_.at %1
goto end

:usage
echo "Usage: ps2ps [options] input.ps output.ps"
echo "  e.g. ps2ps -sPAPERSIZE=a4 input.ps output.ps

:end
