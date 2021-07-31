@echo off 
@rem $Id: eps2eps.bat,v 1.1 2000/03/10 07:51:55 lpd Exp $
@rem "Distill" Encapsulated PostScript.

if "%1"=="" goto usage
if "%2"=="" goto usage
echo -dNODISPLAY -dNOPAUSE -dBATCH >_.at
rem Watcom C deletes = signs, so use # instead.
echo -dDEVICEWIDTH#250000 -dDEVICEHEIGHT#250000 >>_.at
:cp
if "%3"=="" goto doit
echo %1 >>_.at
shift
goto cp

:doit
rem Watcom C deletes = signs, so use # instead.
gs -q -sDEVICE#epswrite -sOutputFile#%2 @_.at %1
goto end

:usage
echo "Usage: eps2eps ...switches... input.eps output.eps"

:end
