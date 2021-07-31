@echo off 
@rem $Id: ps2ps.bat,v 1.1 2000/03/09 08:40:40 lpd Exp $
@rem "Distill" PostScript.

if "%1"=="" goto usage
if "%2"=="" goto usage
echo -dNODISPLAY -dNOPAUSE -dBATCH >_.at
:cp
if "%3"=="" goto doit
echo %1 >>_.at
shift
goto cp

:doit
rem Watcom C deletes = signs, so use # instead.
gs -q -sDEVICE#pswrite -sOutputFile#%2 @_.at %1
goto end

:usage
echo "Usage: ps2ps ...switches... input.ps output.ps"

:end
